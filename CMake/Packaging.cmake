# What a player receives: the install rules describe the finished game folder.
#
#   cmake --build --preset release
#   cmake --install Build/windows-msvc --config Release
#
# The second command copies everything below into the preset's installDir (Build/Package/),
# which is the folder that is zipped and attached to a GitHub Release:
#
#   Abomination.exe
#   Assets/          - the same files the build copies next to the executable
#   *.dll            - Microsoft C++ runtime, so the game starts without the Visual C++ Redistributable
#   LICENSE.md       - the license of the game
#   Licenses/        - the licenses of the libraries compiled into the executable
#
# Destinations are relative to the install prefix; "." is the root of the game folder.

# --- Game --------------------------------------------------------------------
install(TARGETS Abomination RUNTIME DESTINATION .)

# The trailing slash copies the contents of Assets/ into Assets/, not into Assets/Assets/.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/Assets/ DESTINATION Assets)

install(FILES ${PROJECT_SOURCE_DIR}/LICENSE.md DESTINATION .)

# --- Microsoft C++ runtime ---------------------------------------------------
# The vcpkg triplet x64-windows-static-md links the libraries statically, but the C++ runtime dynamically:
# the executable needs vcruntime140.dll, msvcp140.dll and a few others. Microsoft allows shipping them next to
# the executable ("app-local" deployment). InstallRequiredSystemLibraries finds them in the Redist folder of
# the Visual Studio that compiles the game and puts their paths into CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS.
# It would also add its own install rule for all configurations, which is skipped: Debug executables need the
# debug runtime, which may not be redistributed, so the release runtime is installed only with Release.
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
include(InstallRequiredSystemLibraries)
install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION . CONFIGURATIONS Release)

# --- Third-party licenses ----------------------------------------------------
# MIT, zlib and Apache licenses require their text to be shipped together with the compiled code.
# vcpkg keeps the license of every port in <installed>/<triplet>/share/<port>/copyright;
# each one is installed as Licenses/<Library>.txt. GoogleTest is not listed: it is used only by the tests.
set(VCPKG_SHARE_DIR ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share)

function(abomination_install_vcpkg_license port libraryName)
    install(FILES ${VCPKG_SHARE_DIR}/${port}/copyright DESTINATION Licenses RENAME ${libraryName}.txt)
endfunction()

abomination_install_vcpkg_license(cgltf cgltf)
abomination_install_vcpkg_license(glm glm)
abomination_install_vcpkg_license(imgui DearImGui)
abomination_install_vcpkg_license(miniaudio miniaudio)
abomination_install_vcpkg_license(sdl3 SDL3)
abomination_install_vcpkg_license(spdlog spdlog)
abomination_install_vcpkg_license(stb stb)

# EnTT and GLAD do not come from vcpkg; their license files are kept next to their code in ThirdParty/.
install(FILES ${PROJECT_SOURCE_DIR}/ThirdParty/EnTT/LICENSE DESTINATION Licenses RENAME EnTT.txt)
install(FILES ${PROJECT_SOURCE_DIR}/ThirdParty/GLAD/LICENSE DESTINATION Licenses RENAME GLAD.txt)

# The font license stays in Assets/Fonts/ next to the font itself, as the font license asks.
