#include "sokol_config.h"

void log_printf(const char *fmt, ...);
#define SOKOL_LOG(s) log_printf("%s\n", (s));
#define SOKOL_LOGF(fmt, ...) log_printf(fmt, __VA_ARGS__)

#define SOKOL_IMPL

#include "sokol_gfx.h"
#include "sokol_gl.h"
