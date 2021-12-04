#pragma once

#if defined(_WIN32)
	#define SOKOL_D3D11
#elif defined(__EMPSCRIPTEN__)
	#define SOKOL_GLES3
#else
	#define SOKOL_GLCORE33
#endif
