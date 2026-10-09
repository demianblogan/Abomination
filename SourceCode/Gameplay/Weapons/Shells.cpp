#include "Gameplay/Weapons/Shells.h"

#include "Audio/AudioEngine.h"
#include "Core/Scene/Name.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "Physics/CharacterBody.h"
#include "Renderer/Assets/MaterialFiles.h"
#include "Renderer/Assets/MeshPrimitives.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/MeshRenderer.h"
#include "World/CollisionTrace.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <numbers>

namespace Abomination::Gameplay
{
    namespace
    {
        // The shell is round enough with 8 sides: it is small and seen for a moment.
        constexpr int ShellSideCount = 8;

        // A random direction with every component from -1 to 1 (not of length 1: only to shake a direction a little).
        glm::vec3 RandomShake(Core::Random& random)
        {
            return {random.GetFloat(-1.0f, 1.0f), random.GetFloat(-1.0f, 1.0f), random.GetFloat(-1.0f, 1.0f)};
        }

        // The rotation of a shell lying on a floor: its axis (its local Y) horizontal, along the horizontal part of where
        // it pointed, and turned around the axis by a random angle.
        glm::quat CalculateLyingRotation(const glm::quat& rotation, Core::Random& random)
        {
            glm::vec3 axis = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
            axis.y = 0.0f;
            axis = glm::dot(axis, axis) > 0.0001f ? glm::normalize(axis) : glm::vec3(1.0f, 0.0f, 0.0f);

            // The columns of a rotation matrix are where it puts the local X, Y and Z. Y goes along the axis, Z straight
            // up, and X = Y x Z, so that X x Y = Z (right-handed, like every rotation).
            const glm::vec3 up = Core::WorldUp;
            const glm::mat3 lying(glm::cross(axis, up), axis, up);
            return glm::angleAxis(random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>), axis) * glm::quat_cast(lying);
        }

        // A puff of grey smoke at position, drifting with velocity: rises slowly, grows and fades.
        void EmitSmoke(Effects& effects, const glm::vec3& position, const glm::vec3& velocity, float lifetime, float halfSize)
        {
            Core::Random& random = effects.random;
            effects.particles.Emit(Particle{
                .position = position,
                .velocity = velocity,
                .lifetime = lifetime * random.GetFloat(0.7f, 1.3f),
                .startHalfSize = halfSize * 0.4f,
                .endHalfSize = halfSize,
                .rotation = random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
                .rotationSpeed = random.GetFloat(-1.0f, 1.0f),
                .startColor = {0.8f, 0.8f, 0.8f, 0.35f},
                .endColor = {0.8f, 0.8f, 0.8f, 0.0f},
                .gravityScale = -0.04f,
                .drag = 3.0f,
                .texture = effects.textures.smoke,
            });
        }

        // The shell a new throw uses: a new one while the ring is not full, otherwise the oldest. A ring made smaller in
        // the debug overlay loses its last shells first.
        Shell& TakeShell(Shells& shells, entt::registry& registry)
        {
            const auto maximumCount = static_cast<std::size_t>(std::max(shells.settings.maximumCount, 1));
            while (shells.shells.size() > maximumCount)
            {
                registry.destroy(shells.shells.back().entity);
                shells.shells.pop_back();
            }

            if (shells.shells.size() < maximumCount)
            {
                const entt::entity entity = registry.create();
                registry.emplace<Core::Name>(entity, "Shell");
                registry.emplace<Core::Transform>(entity);
                registry.emplace<Renderer::MeshRenderer>(entity, shells.mesh, shells.material, shells.shaderProgram);
                return shells.shells.emplace_back(Shell{.entity = entity});
            }

            shells.nextReplaced %= shells.shells.size();
            Shell& shell = shells.shells[shells.nextReplaced];
            shells.nextReplaced = (shells.nextReplaced + 1) % shells.shells.size();
            return shell;
        }
    }

    Shells LoadShells(Renderer::RenderAssets& renderAssets, Audio::AudioEngine& audio)
    {
        Shells shells;
        shells.mesh = renderAssets.meshes.Add("Primitives/Shell",
                                              Renderer::CreateCylinderMeshData(ShellSideCount, ShellRadius, ShellLength),
                                              Core::AssetLifetime::Global);
        shells.material = Renderer::LoadMaterialByFileNames(renderAssets.textures, "Textures/Weapons/ShotgunShell.png",
                                                            Core::AssetLifetime::Global);
        shells.shaderProgram = renderAssets.shaders.Load("Shaders/TexturedShaded");

        // Shells falling one after another may ring together; more than 4 at once is only noise.
        shells.dropSound = LoadGameSound(audio, "Sounds/Weapons/Shotgun/ShellDrop", 3, Audio::SoundGroup::Effects, 4);
        return shells;
    }

    void EjectShell(GameplayState& state, entt::registry& registry, const Core::Transform& eyes,
                    const glm::vec3& playerVelocity, std::span<const World::CollisionBrush> brushes)
    {
        const WeaponViewModel* weaponViewModel = registry.try_get<WeaponViewModel>(state.player);
        if (weaponViewModel == nullptr)
            return;

        Shells& shells = state.shells;
        const ShellSettings& settings = shells.settings;
        Core::Random& random = shells.random;

        // The weapon is placed relative to the eyes (see CalculateWeaponViewModelMatrix), the eyes in the world: together
        // they take the window and the directions of the weapon into the world. The rotation part of the weapon's matrix
        // (its upper-left 3 x 3) turns directions; the whole matrix also moves points.
        const glm::mat4 weaponMatrix = CalculateWeaponViewModelMatrix(*weaponViewModel);
        const glm::mat3 weaponToWorld = glm::mat3_cast(eyes.rotation) * glm::mat3(weaponMatrix);
        const glm::vec3 right = weaponToWorld * glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 up = weaponToWorld * glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 back = weaponToWorld * glm::vec3(0.0f, 0.0f, 1.0f);
        const glm::vec3 windowInEyes(weaponMatrix * glm::vec4(weaponViewModel->shellWindow + settings.windowOffset, 1.0f));
        const glm::vec3 window = eyes.position + eyes.rotation * windowInEyes;

        // The weapon is drawn over the world, so it may reach into a wall the player stands at: then the shell starts
        // where the way from the eyes to the window meets the wall, not inside it.
        const World::TraceResult way = World::TraceBox(brushes, glm::dvec3(eyes.position), glm::dvec3(window),
                                                       glm::dvec3(ShellRadius));

        Shell& shell = TakeShell(shells, registry);
        Core::Transform& transform = registry.get<Core::Transform>(shell.entity);
        transform.position = glm::vec3(way.endPosition);

        // It lies in the weapon along the barrel, the brass end (its local -Y) to the stock (back): its local Y goes to the
        // front (-back), and X stays the right of the weapon, so Z = X x Y is the up of the weapon.
        transform.rotation = glm::quat_cast(glm::mat3(right, -back, up));

        const float speedScale = 1.0f + random.GetFloat(-settings.speedVariation, settings.speedVariation);
        const glm::vec3 throwVelocity = right * settings.sideSpeed + up * settings.upSpeed + back * settings.backSpeed;
        shell.motion.velocity = throwVelocity * speedScale + RandomShake(random) * 0.2f + playerVelocity;

        // End over end: around the up of the weapon, shaken a little so no two shells turn the same way.
        shell.motion.spinAxis = glm::normalize(up + RandomShake(random) * 0.3f);
        shell.motion.spinSpeed = settings.spinSpeed * random.GetFloat(0.7f, 1.3f);
        shell.age = 0.0f;
        shell.nextTrailTime = 0.0f;
        shell.isResting = false;

        // The smoke of the burnt powder out of the window: slowly up and a little to the side the shell flies.
        for (int puff = 0; puff < settings.windowSmokeCount; ++puff)
        {
            const glm::vec3 drift = up * 0.25f + right * 0.1f + RandomShake(state.effects.random) * 0.08f + playerVelocity;
            EmitSmoke(state.effects, transform.position, drift, settings.windowSmokeLifetime, settings.windowSmokeHalfSize);
        }
    }

    void UpdateShells(GameplayState& state, entt::registry& registry, const Core::Transform& eyes,
                      std::span<const World::CollisionBrush> brushes, Audio::AudioEngine& audio, float deltaTime)
    {
        // The pump has just reached the back (see UpdateWeaponViewModel): the spent shell flies out. Only the player has
        // hands; in the free-fly camera the request is dropped.
        if (WeaponViewModel* weaponViewModel = registry.try_get<WeaponViewModel>(state.player); weaponViewModel != nullptr)
        {
            if (weaponViewModel->pump.isShellEjectRequested && state.controlMode == ControlMode::Player)
            {
                const glm::vec3 playerVelocity = registry.get<Physics::CharacterBody>(state.player).velocity;
                EjectShell(state, registry, eyes, playerVelocity, brushes);
            }
            weaponViewModel->pump.isShellEjectRequested = false;
        }

        Shells& shells = state.shells;
        const ShellSettings& settings = shells.settings;
        for (Shell& shell : shells.shells)
        {
            if (shell.isResting)
                continue;

            Core::Transform& transform = registry.get<Core::Transform>(shell.entity);
            shell.age += deltaTime;

            // The thin smoke behind it while it is young: one puff every trailInterval, left where it is now.
            while (shell.age >= shell.nextTrailTime && shell.nextTrailTime < settings.trailDuration)
            {
                EmitSmoke(state.effects, transform.position, glm::vec3(0.0f), settings.trailLifetime, settings.trailHalfSize);
                shell.nextTrailTime += std::max(settings.trailInterval, 0.005f);
            }

            // It falls, tumbles and bounces (see Tumble).
            const BounceSettings bounce{.bounce = settings.bounce, .slide = settings.slide, .spinKept = 0.5f,
                                        .restSpeed = settings.restSpeed};
            const TumbleStep step = Tumble(shell.motion, transform, brushes, state.physicsSettings.gravity, ShellRadius,
                                           bounce, deltaTime);
            if (!step.hasHit)
            {
                shell.isResting = step.hasStopped; // stuck in a brush (thrown into a moving door, later)
                continue;
            }

            // The clink: louder the harder it hits; the soft touches of a shell settling down are not heard. The ringing of
            // the bounce before is faded out quickly, so the bounces of one shell do not pile up.
            if (step.speedIntoSurface > settings.soundSpeed)
            {
                const float volume = std::clamp(step.speedIntoSurface / settings.fullVolumeSpeed, 0.15f, 1.0f);
                audio.FadeOut(shell.voice, 0.05f);
                shell.voice = audio.Play(shells.dropSound, transform.position, false, volume);
            }

            // Slow enough on a floor: it lies down, and its sound dies away with it.
            if (step.hasStopped)
            {
                shell.isResting = true;
                transform.rotation = CalculateLyingRotation(transform.rotation, shells.random);
                audio.FadeOut(shell.voice, settings.soundFadeOut);
            }
        }
    }

    void ClearShells(Shells& shells, entt::registry& registry)
    {
        for (const Shell& shell : shells.shells)
            registry.destroy(shell.entity);
        shells.shells.clear();
        shells.nextReplaced = 0;
    }
}
