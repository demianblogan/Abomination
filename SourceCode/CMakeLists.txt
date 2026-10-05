# --- Dependencies ------------------------------------------------------------
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(recastnavigation CONFIG REQUIRED)
find_package(RmlUi CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)  # Header-only: provides only Stb_INCLUDE_DIR, no target
find_path(CGLTF_INCLUDE_DIR cgltf.h REQUIRED)  # Header-only, and vcpkg installs no CMake package for it
find_path(MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED)  # The same

# --- Generated sources -------------------------------------------------------
# The version lives only in project(... VERSION ...) of the root CMakeLists.txt.
# It is written into a generated .cpp (not a header), so bumping the version
# recompiles one file instead of everything that includes Version.h.
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/Generated)
configure_file(Core/Version.cpp.in ${GENERATED_DIR}/Core/Version.cpp @ONLY)

# --- Game library: all game code except the entry point ----------------------
set(LIBRARY_SOURCES
    Application/Application.h
    Application/Application.cpp

    Audio/AudioEngine.h
    Audio/AudioEngine.cpp
    Audio/MiniaudioImplementation.cpp
    Audio/SoundClip.h
    Audio/SoundClip.cpp
    Audio/SoundEvent.h
    Audio/SoundGroup.h
    Audio/SoundStore.h
    Audio/SoundStore.cpp
    Audio/VoiceSelection.h
    Audio/VoiceSelection.cpp

    Core/BuildConfiguration.h
    Core/Version.h
    Core/Version.cpp.in
    Core/Assets/AssetCache.h
    Core/Assets/AssetHandle.h
    Core/Assets/AssetLifetime.h
    Core/Files/FileSystem.h
    Core/Files/FileSystem.cpp
    Core/Files/Image.h
    Core/Files/Image.cpp
    Core/Logging/Log.h
    Core/Logging/Log.cpp
    Core/Logging/LogHistory.h
    Core/Logging/LogHistory.cpp
    Core/Math/BoundingBox.h
    Core/Math/BoundingBox.cpp
    Core/Math/ConvexPolygon.h
    Core/Math/ConvexPolygon.cpp
    Core/Math/Plane.h
    Core/Math/Plane.cpp
    Core/Math/Random.h
    Core/Math/Random.cpp
    Core/Math/Spring.h
    Core/Math/Spring.cpp
    Core/Math/Units.h
    Core/Scene/Name.h
    Core/Scene/Transform.h
    Core/Scene/Transform.cpp
    Core/Scene/TransformInterpolation.h
    Core/Scene/TransformInterpolation.cpp
    Core/Time/Clock.h
    Core/Time/FixedTimestep.h
    Core/Time/FixedTimestep.cpp
    Core/Time/FrameLimiter.h
    Core/Time/FrameLimiter.cpp
    Core/Time/FrameStatistics.h
    Core/Time/FrameStatistics.cpp
    Core/Time/FrameTimer.h
    Core/Time/FrameTimer.cpp

    Gameplay/GameplayState.h
    Gameplay/GameplayState.cpp
    Gameplay/Spin.h
    Gameplay/Spin.cpp
    Gameplay/Animation/Animator.h
    Gameplay/Animation/Animator.cpp
    Gameplay/Camera/FreeFlyCameraController.h
    Gameplay/Camera/FreeFlyCameraController.cpp
    Gameplay/Camera/FreeFlyCameraSystem.h
    Gameplay/Camera/FreeFlyCameraSystem.cpp
    Gameplay/Camera/MouseLook.h
    Gameplay/Camera/MouseLook.cpp
    Gameplay/Camera/ViewSystem.h
    Gameplay/Camera/ViewSystem.cpp
    Gameplay/Characters/Armor.h
    Gameplay/Characters/CharacterCollision.h
    Gameplay/Characters/CharacterCollision.cpp
    Gameplay/Characters/DamageKind.h
    Gameplay/Characters/Health.h
    Gameplay/Effects/Effects.h
    Gameplay/Effects/Effects.cpp
    Gameplay/Effects/Gibs.h
    Gameplay/Effects/Gibs.cpp
    Gameplay/Effects/Particles.h
    Gameplay/Effects/Particles.cpp
    Gameplay/Effects/Tumbling.h
    Gameplay/Effects/Tumbling.cpp
    Gameplay/Enemies/Corpses.h
    Gameplay/Enemies/Corpses.cpp
    Gameplay/Enemies/DogMind.h
    Gameplay/Enemies/DogMind.cpp
    Gameplay/Enemies/GroundFit.h
    Gameplay/Enemies/GroundFit.cpp
    Gameplay/Enemies/MonsterDebug.h
    Gameplay/Enemies/MonsterDebug.cpp
    Gameplay/Enemies/Monsters.h
    Gameplay/Enemies/Monsters.cpp
    Gameplay/Player/DamageReaction.h
    Gameplay/Player/DamageReaction.cpp
    Gameplay/Player/LandingDip.h
    Gameplay/Player/LandingDip.cpp
    Gameplay/Player/Player.h
    Gameplay/Player/Player.cpp
    Gameplay/Player/PlayerController.h
    Gameplay/Player/PlayerController.cpp
    Gameplay/Player/PlayerDeath.h
    Gameplay/Player/PlayerDeath.cpp
    Gameplay/Player/PlayerSystem.h
    Gameplay/Player/PlayerSystem.cpp
    Gameplay/Weapons/Ammo.h
    Gameplay/Weapons/Crosshair.h
    Gameplay/Weapons/Crosshair.cpp
    Gameplay/Weapons/PumpAction.h
    Gameplay/Weapons/PumpAction.cpp
    Gameplay/Weapons/Shells.h
    Gameplay/Weapons/Shells.cpp
    Gameplay/Weapons/ViewRecoil.h
    Gameplay/Weapons/ViewRecoil.cpp
    Gameplay/Weapons/Weapon.h
    Gameplay/Weapons/Weapon.cpp
    Gameplay/Weapons/WeaponSystem.h
    Gameplay/Weapons/WeaponSystem.cpp
    Gameplay/Weapons/WeaponHands.h
    Gameplay/Weapons/WeaponHands.cpp
    Gameplay/Weapons/WeaponViewModel.h
    Gameplay/Weapons/WeaponViewModel.cpp
    Gameplay/Weapons/WeaponViewModelMotion.h
    Gameplay/Weapons/WeaponViewModelMotion.cpp

    Input/Action.h
    Input/ActionStates.h
    Input/ActionStates.cpp
    Input/InputBindings.h
    Input/InputBindings.cpp
    Input/InputDevices.h
    Input/Keyboard.h
    Input/Keyboard.cpp
    Input/Mouse.h
    Input/Mouse.cpp

    Navigation/NavMesh.h
    Navigation/NavMesh.cpp
    Physics/CharacterBody.h
    Physics/CharacterMovement.h
    Physics/CharacterMovement.cpp
    Platform/ImGuiPlatformBackend.h
    Platform/ImGuiPlatformBackend.cpp
    Platform/RmlUiPlatformBackend.h
    Platform/RmlUiPlatformBackend.cpp
    Platform/ScreenMode.h
    Platform/SDLLibrary.h
    Platform/SDLLibrary.cpp
    Platform/SystemServices.h
    Platform/SystemServices.cpp
    Platform/Window.h
    Platform/Window.cpp
    Platform/WindowSizing.h
    Platform/WindowSizing.cpp

    Renderer/ImGuiRendererBackend.h
    Renderer/ImGuiRendererBackend.cpp
    Renderer/DrawOffset.h
    Renderer/MeshRenderer.h
    Renderer/ModelPose.h
    Renderer/ModelRenderer.h
    Renderer/RenderSettings.h
    Renderer/RenderSystem.h
    Renderer/RenderSystem.cpp
    Renderer/RmlUiRendererBackend.h
    Renderer/RmlUiRendererBackend.cpp
    Renderer/SkinningBuffer.h
    Renderer/SkinningBuffer.cpp
    Renderer/Sprites/SpriteBatch.h
    Renderer/Sprites/SpriteBatch.cpp
    Renderer/Sprites/SpriteRenderer.h
    Renderer/Sprites/SpriteRenderer.cpp
    Renderer/Animation/AnimationSampling.h
    Renderer/Animation/AnimationSampling.cpp
    Renderer/Animation/SkeletonPose.h
    Renderer/Animation/SkeletonPose.cpp
    Renderer/Assets/GLTFLoader.h
    Renderer/Assets/GLTFLoader.cpp
    Renderer/Assets/Mesh.h
    Renderer/Assets/Mesh.cpp
    Renderer/Assets/MeshData.h
    Renderer/Assets/MeshPrimitives.h
    Renderer/Assets/MeshPrimitives.cpp
    Renderer/Assets/MeshStore.h
    Renderer/Assets/MeshStore.cpp
    Renderer/Assets/ModelData.h
    Renderer/Assets/ModelPartSplit.h
    Renderer/Assets/ModelPartSplit.cpp
    Renderer/Assets/ModelStore.h
    Renderer/Assets/ModelStore.cpp
    Renderer/Assets/RenderAssets.h
    Renderer/Assets/ShaderStore.h
    Renderer/Assets/ShaderStore.cpp
    Renderer/Assets/TextureStore.h
    Renderer/Assets/TextureStore.cpp
    Renderer/Camera/CameraLens.h
    Renderer/Camera/View.h
    Renderer/Camera/View.cpp
    Renderer/Debug/DebugLineRenderer.h
    Renderer/Debug/DebugLineRenderer.cpp
    Renderer/Debug/DebugLines.h
    Renderer/Debug/DebugLines.cpp
    Renderer/OpenGL/DebugOutput.h
    Renderer/OpenGL/DebugOutput.cpp
    Renderer/OpenGL/GLBuffer.h
    Renderer/OpenGL/GLBuffer.cpp
    Renderer/OpenGL/GLShaderProgram.h
    Renderer/OpenGL/GLShaderProgram.cpp
    Renderer/OpenGL/GLTexture.h
    Renderer/OpenGL/GLTexture.cpp
    Renderer/OpenGL/GLVertexArray.h
    Renderer/OpenGL/GLVertexArray.cpp
    Renderer/OpenGL/OpenGLLoader.h
    Renderer/OpenGL/OpenGLLoader.cpp
    Renderer/OpenGL/RenderCommands.h
    Renderer/OpenGL/RenderCommands.cpp
    Renderer/OpenGL/ShaderInterface.h

    UI/DeathScreen.h
    UI/DeathScreen.cpp
    UI/DebugOverlay.h
    UI/DebugOverlay.cpp
    UI/GameUI.h
    UI/GameUI.cpp
    UI/HUD.h
    UI/HUD.cpp
    UI/ImGuiLibrary.h
    UI/ImGuiLibrary.cpp
    UI/StyleValues.h
    UI/StyleValues.cpp
    UI/UIScale.h
    UI/UIScale.cpp
    UI/Widgets.h
    UI/Widgets.cpp
    UI/Windows/AnimationWindow.h
    UI/Windows/AnimationWindow.cpp
    UI/Windows/AssetsWindow.h
    UI/Windows/AssetsWindow.cpp
    UI/Windows/AudioWindow.h
    UI/Windows/AudioWindow.cpp
    UI/Windows/CollisionWindow.h
    UI/Windows/CollisionWindow.cpp
    UI/Windows/ConsoleWindow.h
    UI/Windows/ConsoleWindow.cpp
    UI/Windows/EffectsWindow.h
    UI/Windows/EffectsWindow.cpp
    UI/Windows/EnemiesWindow.h
    UI/Windows/EnemiesWindow.cpp
    UI/Windows/EntitiesWindow.h
    UI/Windows/EntitiesWindow.cpp
    UI/Windows/MovementWindow.h
    UI/Windows/MovementWindow.cpp
    UI/Windows/PerformanceWindow.h
    UI/Windows/PerformanceWindow.cpp
    UI/Windows/PlayerWindow.h
    UI/Windows/PlayerWindow.cpp
    UI/Windows/RendererWindow.h
    UI/Windows/RendererWindow.cpp
    UI/Windows/WeaponWindow.h
    UI/Windows/WeaponWindow.cpp

    World/BrushGeometry.h
    World/BrushGeometry.cpp
    World/CollisionBrush.h
    World/CollisionBrush.cpp
    World/CollisionDebug.h
    World/CollisionDebug.cpp
    World/CollisionTrace.h
    World/CollisionTrace.cpp
    World/Level.h
    World/Level.cpp
    World/LevelMesh.h
    World/LevelMesh.cpp
    World/MapCoordinates.h
    World/MapCoordinates.cpp
    World/MapData.h
    World/MapData.cpp
    World/MapParser.h
    World/MapParser.cpp
    World/MonsterStart.h
    World/NavMeshGeometry.h
    World/NavMeshGeometry.cpp
    World/PlayerStart.h
    World/TextureCoordinates.h
    World/TextureCoordinates.cpp
)

add_library(AbominationCore STATIC
    ${LIBRARY_SOURCES}
    ${GENERATED_DIR}/Core/Version.cpp
)
target_include_directories(AbominationCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(AbominationCore SYSTEM PRIVATE ${Stb_INCLUDE_DIR})  # Only Core/Files/Image.cpp includes stb
target_include_directories(AbominationCore SYSTEM PRIVATE ${MINIAUDIO_INCLUDE_DIR})  # Only Audio/*.cpp include miniaudio
target_include_directories(AbominationCore SYSTEM PRIVATE ${CGLTF_INCLUDE_DIR})  # Only Renderer/Assets/GLTFLoader.cpp includes cgltf
target_link_libraries(AbominationCore
    PUBLIC
        EnTT                  # Header-only; game headers hold entt::registry and entities, so it is public
        glm::glm-header-only  # Renderer headers use glm types, so everyone who uses the library needs glm too
    PRIVATE
        GLAD                  # Only Renderer/*.cpp include GLAD, no header does
        imgui::imgui          # Only .cpp files include ImGui, no header does
        RecastNavigation::Recast  # Only Navigation/*.cpp include Recast (it builds the navmesh) and Detour (it
        RecastNavigation::Detour  # finds paths on it); no header does
        RmlUi::RmlUi          # Only the RmlUi backends (Platform, Renderer) and the game interface (UI) include RmlUi
        SDL3::SDL3            # Only Platform/*.cpp include SDL, no header does
        spdlog::spdlog        # Only Log.cpp includes spdlog, Log.h does not
)
abomination_set_compiler_options(AbominationCore)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LIBRARY_SOURCES})
source_group(Generated FILES ${GENERATED_DIR}/Core/Version.cpp)

# --- Game executable: only the entry point -----------------------------------
add_executable(Abomination
    Main.cpp
)
target_link_libraries(Abomination PRIVATE AbominationCore)
abomination_set_compiler_options(Abomination)

# A Windows application, not a console application: Windows opens no console window for the game. The log is read
# in the in-game console (the ` key) and in Abomination.log next to the executable.
# A Windows application normally starts in WinMain(); /ENTRY:mainCRTStartup makes it start in the ordinary main()
# instead, so Main.cpp stays the same (and SDL, which could provide a WinMain, stays inside the Platform module).
set_target_properties(Abomination PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_options(Abomination PRIVATE /ENTRY:mainCRTStartup)

# --- Assets: the Assets folder is copied next to the executable ------------------
# The game finds its files relative to the executable (Platform::GetExecutableDirectory() / "Assets"),
# the same way it will after installation. The target runs on every build (ALL), but copies only new or
# changed files, so it takes almost no time. A file deleted from Assets/ stays in the output folder until
# the Build folder is cleaned.
add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${PROJECT_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:Abomination>/Assets
    COMMENT "Copying assets next to the executable"
    VERBATIM
)
add_dependencies(Abomination CopyAssets)

# A Debug build reads the Assets folder of the repository directly (see Main.cpp), so edited assets need no build.
target_compile_definitions(Abomination PRIVATE
    "$<$<CONFIG:Debug>:ABOMINATION_SOURCE_ASSETS_DIRECTORY=\"${PROJECT_SOURCE_DIR}/Assets\">"
)
