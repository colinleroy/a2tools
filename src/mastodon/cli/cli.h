#ifndef __cli_h
#define __cli_h

#include "account.h"

#ifdef __APPLE2ENH__
#define LEFT_COL_WIDTH 19
#define RIGHT_COL_START 20
#define TIME_COLUMN 41
#define NUMCOLS 80
#define KEY_COMB "Open-Apple"

#else
#define LEFT_COL_WIDTH -1
#define RIGHT_COL_START 0
#define NUMCOLS 40
#define KEY_COMB "Ctrl"
#define HELP_KEY ('Y'-'A'+1)
#endif

extern unsigned char scrw, scrh;

#endif
