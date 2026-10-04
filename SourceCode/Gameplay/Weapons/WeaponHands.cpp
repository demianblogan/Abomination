#include "Gameplay/Weapons/WeaponHands.h"

#include "Gameplay/Weapons/WeaponViewModel.h"
#include "Renderer/Animation/AnimationSampling.h"
#include "Renderer/Animation/SkeletonPose.h"

#include <glm/matrix.hpp>

#include <algorithm>
#include <string_view>
#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        const Renderer::AnimationClipData* FindClip(const Renderer::Model& model, std::string_view name)
        {
            const auto clip = std::ranges::find(model.animations, name, &Renderer::AnimationClipData::name);
            return clip != model.animations.end() ? &*clip : nullptr;
        }
    }

    glm::mat4 CalculateWeaponHandsMatrix(const WeaponViewModel& weaponViewModel, const Renderer::Model& handsModel)
    {
        // The skinned parts of a model are drawn at its centering (see ModelStore); undone, the coordinates of the hands
        // file are those of the weapon.
        return CalculateWeaponViewModelMatrix(weaponViewModel) * glm::inverse(handsModel.skeletonTransform);
    }

    void CalculateWeaponHandsPose(WeaponViewModel& weaponViewModel, const Renderer::Model& handsModel)
    {
        Renderer::ModelPose& pose = weaponViewModel.hands.pose;
        const Renderer::AnimationClipData* hold = FindClip(handsModel, "Hold");
        const Renderer::AnimationClipData* pumpBack = FindClip(handsModel, "HoldPumpBack");
        if (!handsModel.skeleton.has_value() || hold == nullptr || pumpBack == nullptr)
        {
            pose.jointMatrices.clear();
            return;
        }

        // Each pose is a clip of one frame: sampled at its start. They are mixed as far as the pump is back.
        std::vector<Renderer::JointPose> holdPose = Renderer::CreateRestPose(*handsModel.skeleton);
        std::vector<Renderer::JointPose> pumpBackPose = holdPose;
        Renderer::SampleAnimationClip(*hold, 0.0f, holdPose);
        Renderer::SampleAnimationClip(*pumpBack, 0.0f, pumpBackPose);
        Renderer::BlendPoses(holdPose, pumpBackPose, weaponViewModel.pump.progress, holdPose);
        Renderer::CalculateJointMatrices(*handsModel.skeleton, holdPose, pose.jointMatrices);
    }
}
