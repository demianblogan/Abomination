find_package(GTest CONFIG REQUIRED)
find_package(Stb REQUIRED)  # ImageTests writes test PNG files with stb_image_write

# --- Unit tests: one executable for all tests, mirrors SourceCode/ ------------
set(TEST_SOURCES
    Audio/AudioEngineTests.cpp
    Audio/SoundStoreTests.cpp
    Audio/VoiceSelectionTests.cpp
    Core/VersionTests.cpp
    Core/Assets/AssetCacheTests.cpp
    Core/Files/FileSystemTests.cpp
    Core/Files/ImageTests.cpp
    Core/Logging/LogHistoryTests.cpp
    Core/Logging/LogTests.cpp
    Core/Math/BoundingBoxTests.cpp
    Core/Math/ConvexPolygonTests.cpp
    Core/Math/PlaneTests.cpp
    Core/Math/RandomTests.cpp
    Core/Math/SpringTests.cpp
    Core/Math/UnitsTests.cpp
    Core/Scene/TransformInterpolationTests.cpp
    Core/Scene/TransformTests.cpp
    Core/Time/FixedTimestepTests.cpp
    Core/Time/FrameLimiterTests.cpp
    Core/Time/FrameStatisticsTests.cpp
    Core/Time/FrameTimerTests.cpp

    Gameplay/SpinTests.cpp
    Gameplay/Animation/AnimatorTests.cpp
    Gameplay/Camera/FreeFlyCameraControllerTests.cpp
    Gameplay/Characters/ArmorTests.cpp
    Gameplay/Characters/HealthTests.cpp
    Gameplay/Enemies/DogMindTests.cpp
    Gameplay/Enemies/GroundFitTests.cpp
    Gameplay/Effects/EffectsTests.cpp
    Gameplay/Effects/ParticlesTests.cpp
    Gameplay/Player/DamageReactionTests.cpp
    Gameplay/Player/LandingDipTests.cpp
    Gameplay/Player/PlayerSystemTests.cpp
    Gameplay/Player/PlayerTests.cpp
    Gameplay/Weapons/AmmoTests.cpp
    Gameplay/Weapons/CrosshairTests.cpp
    Gameplay/Weapons/PumpActionTests.cpp
    Gameplay/Weapons/WeaponTests.cpp
    Gameplay/Weapons/WeaponViewModelMotionTests.cpp
    Gameplay/Weapons/WeaponViewModelTests.cpp

    Input/ActionStatesTests.cpp
    Input/InputBindingsTests.cpp
    Input/KeyboardTests.cpp
    Input/MouseTests.cpp

    Physics/CharacterMovementMapTests.cpp
    Physics/CharacterMovementTests.cpp

    Platform/WindowSizingTests.cpp

    Renderer/Animation/AnimationSamplingTests.cpp
    Renderer/Animation/SkeletonPoseTests.cpp
    Renderer/Assets/GLTFLoaderTests.cpp
    Renderer/Assets/MeshPrimitivesTests.cpp
    Renderer/Assets/ModelPartSplitTests.cpp
    Renderer/Camera/ViewTests.cpp
    Renderer/Debug/DebugLinesTests.cpp
    Renderer/OpenGL/GLTextureTests.cpp

    ThirdParty/EnTTTests.cpp

    UI/GameFontsTests.cpp

    World/BrushGeometryTests.cpp
    World/CollisionBrushTests.cpp
    World/CollisionDebugTests.cpp
    World/CollisionTraceTests.cpp
    World/LevelMeshTests.cpp
    World/MapCoordinatesTests.cpp
    World/MapParserTests.cpp
    World/PlayerStartTests.cpp
    World/TextureCoordinatesTests.cpp
)

add_executable(AbominationTests
    ${TEST_SOURCES}
)
target_link_libraries(AbominationTests PRIVATE
    AbominationCore
    GTest::gtest
    GTest::gtest_main   # Provides main() that runs all tests
)
target_include_directories(AbominationTests SYSTEM PRIVATE ${Stb_INCLUDE_DIR})
# The font tests read the fonts of the game from the source tree.
target_compile_definitions(AbominationTests PRIVATE ABOMINATION_TEST_ASSETS_DIRECTORY="${PROJECT_SOURCE_DIR}/Assets")
abomination_set_compiler_options(AbominationTests)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_SOURCES})

# --- CTest integration -------------------------------------------------------
# Registers every TEST(...) as a separate CTest test. PRE_TEST lists the tests
# when CTest runs instead of after every build, so building never runs test code.
include(GoogleTest)
gtest_discover_tests(AbominationTests DISCOVERY_MODE PRE_TEST)
