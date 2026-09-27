# --- Dependencies ------------------------------------------------------------
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)  # Header-only: provides only Stb_INCLUDE_DIR, no target

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

    Core/AssetCache.h
    Core/AssetHandle.h
    Core/AssetLifetime.h
    Core/BoundingBox.h
    Core/BoundingBox.cpp
    Core/BuildConfiguration.h
    Core/Clock.h
    Core/ConvexPolygon.h
    Core/ConvexPolygon.cpp
    Core/FileSystem.h
    Core/FileSystem.cpp
    Core/FixedTimestep.h
    Core/FixedTimestep.cpp
    Core/FrameLimiter.h
    Core/FrameLimiter.cpp
    Core/FrameStatistics.h
    Core/FrameStatistics.cpp
    Core/FrameTimer.h
    Core/FrameTimer.cpp
    Core/Image.h
    Core/Image.cpp
    Core/Log.h
    Core/Log.cpp
    Core/LogHistory.h
    Core/LogHistory.cpp
    Core/Name.h
    Core/Plane.h
    Core/Plane.cpp
    Core/Transform.h
    Core/Transform.cpp
    Core/TransformInterpolation.h
    Core/TransformInterpolation.cpp
    Core/Version.h
    Core/Version.cpp.in

    Gameplay/FreeFlyCamera.h
    Gameplay/FreeFlyCameraController.h
    Gameplay/FreeFlyCameraController.cpp
    Gameplay/Spin.h
    Gameplay/Spin.cpp

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

    Renderer/CameraLens.h
    Renderer/DebugLineRenderer.h
    Renderer/DebugLineRenderer.cpp
    Renderer/DebugLines.h
    Renderer/DebugLines.cpp
    Renderer/DebugOutput.h
    Renderer/DebugOutput.cpp
    Renderer/GLBuffer.h
    Renderer/GLBuffer.cpp
    Renderer/GLShaderProgram.h
    Renderer/GLShaderProgram.cpp
    Renderer/GLTexture.h
    Renderer/GLTexture.cpp
    Renderer/GLVertexArray.h
    Renderer/GLVertexArray.cpp
    Renderer/ImGuiRendererBackend.h
    Renderer/ImGuiRendererBackend.cpp
    Renderer/Mesh.h
    Renderer/Mesh.cpp
    Renderer/MeshData.h
    Renderer/MeshPrimitives.h
    Renderer/MeshPrimitives.cpp
    Renderer/MeshRenderer.h
    Renderer/MeshStore.h
    Renderer/MeshStore.cpp
    Renderer/OpenGLLoader.h
    Renderer/OpenGLLoader.cpp
    Renderer/RenderAssets.h
    Renderer/RenderCommands.h
    Renderer/RenderCommands.cpp
    Renderer/RenderSettings.h
    Renderer/RenderSystem.h
    Renderer/RenderSystem.cpp
    Renderer/ShaderStore.h
    Renderer/ShaderStore.cpp
    Renderer/TextureStore.h
    Renderer/TextureStore.cpp
    Renderer/View.h
    Renderer/View.cpp

    UI/DebugOverlay.h
    UI/DebugOverlay.cpp
    UI/EntitiesWindow.h
    UI/EntitiesWindow.cpp
    UI/ImGuiLibrary.h
    UI/ImGuiLibrary.cpp
    UI/UIScale.h
    UI/UIScale.cpp

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
    World/TextureCoordinates.h
    World/TextureCoordinates.cpp
)

add_library(AbominationCore STATIC
    ${LIBRARY_SOURCES}
    ${GENERATED_DIR}/Core/Version.cpp
)
target_include_directories(AbominationCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(AbominationCore SYSTEM PRIVATE ${Stb_INCLUDE_DIR})  # Only Core/Image.cpp includes stb
target_link_libraries(AbominationCore
    PUBLIC
        EnTT                  # Header-only; game headers will hold entt::registry and entities, so it is public
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
