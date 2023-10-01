#ifndef __cli_h
#define __cli_h

#include "account.h"

#ifdef __APPLE2ENH__
#define LEFT_COL_WIDTH 19
#define RIGHT_COL_START 20
#define TIME_COLUMN 41
#define NUMCOLS 80

#else
#define LEFT_COL_WIDTH -1
#define RIGHT_COL_START 0
#define NUMCOLS 40

#endif

extern unsigned char scrw, scrh;

#endif
