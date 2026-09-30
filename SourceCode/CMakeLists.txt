# --- Dependencies ------------------------------------------------------------
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
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

    Gameplay/CharacterCollision.h
    Gameplay/CharacterCollision.cpp
    Gameplay/Crosshair.h
    Gameplay/Crosshair.cpp
    Gameplay/FreeFlyCameraController.h
    Gameplay/FreeFlyCameraController.cpp
    Gameplay/FreeFlyCameraSystem.h
    Gameplay/FreeFlyCameraSystem.cpp
    Gameplay/GameplayState.h
    Gameplay/GameplayState.cpp
    Gameplay/Health.h
    Gameplay/LandingDip.h
    Gameplay/LandingDip.cpp
    Gameplay/MouseLook.h
    Gameplay/MouseLook.cpp
    Gameplay/Player.h
    Gameplay/Player.cpp
    Gameplay/PlayerController.h
    Gameplay/PlayerController.cpp
    Gameplay/PlayerSystem.h
    Gameplay/PlayerSystem.cpp
    Gameplay/Spin.h
    Gameplay/Spin.cpp
    Gameplay/TargetDummy.h
    Gameplay/TargetDummy.cpp
    Gameplay/ViewModel.h
    Gameplay/ViewModel.cpp
    Gameplay/ViewModelMotion.h
    Gameplay/ViewModelMotion.cpp
    Gameplay/ViewRecoil.h
    Gameplay/ViewRecoil.cpp
    Gameplay/ViewSystem.h
    Gameplay/ViewSystem.cpp
    Gameplay/Weapon.h
    Gameplay/Weapon.cpp
    Gameplay/WeaponSystem.h
    Gameplay/WeaponSystem.cpp

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

    Physics/CharacterBody.h
    Physics/CharacterMovement.h
    Physics/CharacterMovement.cpp
    Platform/ImGuiPlatformBackend.h
    Platform/ImGuiPlatformBackend.cpp
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
    Renderer/ModelRenderer.h
    Renderer/RenderSettings.h
    Renderer/RenderSystem.h
    Renderer/RenderSystem.cpp
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

    UI/AssetsWindow.h
    UI/AssetsWindow.cpp
    UI/AudioWindow.h
    UI/AudioWindow.cpp
    UI/CollisionWindow.h
    UI/CollisionWindow.cpp
    UI/ConsoleWindow.h
    UI/ConsoleWindow.cpp
    UI/CrosshairOverlay.h
    UI/CrosshairOverlay.cpp
    UI/DebugOverlay.h
    UI/DebugOverlay.cpp
    UI/EntitiesWindow.h
    UI/EntitiesWindow.cpp
    UI/ImGuiLibrary.h
    UI/ImGuiLibrary.cpp
    UI/MovementWindow.h
    UI/MovementWindow.cpp
    UI/PerformanceWindow.h
    UI/PerformanceWindow.cpp
    UI/RendererWindow.h
    UI/RendererWindow.cpp
    UI/UIScale.h
    UI/UIScale.cpp
    UI/ViewModelWindow.h
    UI/ViewModelWindow.cpp
    UI/WeaponWindow.h
    UI/WeaponWindow.cpp

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
