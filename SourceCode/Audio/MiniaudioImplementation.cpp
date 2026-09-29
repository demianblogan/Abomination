// miniaudio is a "single-header" library, like stb_image: the header contains both the declarations and the
// implementation. The implementation is compiled only where MINIAUDIO_IMPLEMENTATION is defined, and that must be exactly
// one .cpp file in the whole program. This is that file; it contains nothing else, because the implementation brings in
// <windows.h> with its macros (min, max, ...), which must not reach our code.
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
