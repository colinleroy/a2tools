#ifndef PC_DEBUG
#define PC_DEBUG

#include "platform.h"

#ifdef __CC65__

#define PC_DEBUG_BUFFER(op, str, len)
#define PC_DEBUG_PRINTF(...)

#else
extern uint8 do_debug;
#define PC_DEBUG_PRINTF(...) do { if (do_debug) printf(__VA_ARGS__); } while (0)
void PC_DEBUG_BUFFER(char *op, const char *str, int len);

#endif

#endif
