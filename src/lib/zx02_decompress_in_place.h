#ifndef QT_DECOMPRESS
#define QT_DECOMPRESS

#include "platform.h"

uint8 __fastcall__ zx02_decompress_in_place(const char *filename, char *destination_start, char *destination_end);

#endif
