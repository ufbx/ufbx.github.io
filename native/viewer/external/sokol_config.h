#pragma once

#if defined(_WIN32)
	// #define SOKOL_D3D11
	#define SOKOL_GLCORE33
#elif defined(__EMSCRIPTEN__)
	#define SOKOL_GLES3
#else
	#define SOKOL_GLCORE33
#endif
