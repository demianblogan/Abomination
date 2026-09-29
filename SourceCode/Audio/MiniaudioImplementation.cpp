// miniaudio is a "single-header" library, like stb_image: the header contains both the declarations and the
// implementation. The implementation is compiled only where MINIAUDIO_IMPLEMENTATION is defined, and that must be exactly
// one .cpp file in the whole program. This is that file; it contains nothing else, because the implementation brings in
// <windows.h> with its macros (min, max, ...), which must not reach our code.
//
// miniaudio decodes WAV, FLAC and MP3 by itself, but not OGG Vorbis, the format of our sounds: for that it uses
// stb_vorbis (from the stb libraries we already have for images), if its declarations are included before miniaudio.
// The order is the one miniaudio's documentation asks for:
//   1. the declarations of stb_vorbis (STB_VORBIS_HEADER_ONLY), so miniaudio compiles its Vorbis decoder;
//   2. the implementation of miniaudio;
//   3. the implementation of stb_vorbis, the same file again without STB_VORBIS_HEADER_ONLY.
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

// The compiler checks some things (like "a variable may be used before it gets a value") while it generates the code of
// a function, and such warnings are reported even for external headers, which are otherwise silent (see
// CMake/CompilerOptions.cmake). stb_vorbis triggers them where it is correct, so they are switched off for it alone:
//   C4701, C4703 - a local variable (a pointer for C4703) may be used before it gets a value.
#undef STB_VORBIS_HEADER_ONLY
#pragma warning(push)
#pragma warning(disable : 4701 4703)
#include <stb_vorbis.c>
#pragma warning(pop)
