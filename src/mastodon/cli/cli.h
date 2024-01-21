#ifndef __cli_h
#define __cli_h

#include "account.h"

#ifdef __APPLE2ENH__
#define LEFT_COL_WIDTH 19
#define RIGHT_COL_START 20
#define TIME_COLUMN 41
#define NUMCOLS 80
#define KEY_COMB "Open-Apple"
#define N_STATUS_TO_LOAD 10

#else
#define LEFT_COL_WIDTH 39
#define RIGHT_COL_START 0
#define NUMCOLS 40
#define KEY_COMB "Ctrl"
#define N_STATUS_TO_LOAD 8
#define HELP_KEY ('Y'-'A'+1)
#endif

/* actions mapped to keys */
#if NUMCOLS == 40
#define SHOW_HELP            HELP_KEY
#endif
#define SHOW_FULL_STATUS     CH_ENTER
#define BACK                 CH_ESC
#define COMPOSE              'c'
#define CONFIGURE            'o'
#define SHOW_NOTIFICATIONS   'n'
#define SEARCH               's'
#define SHOW_PROFILE         'p'
#define IMAGES               'i'
#define REPLY                'r'
#define EDIT                 'e'
#define VOTING               'v'

/* special cases (extra step or mapped arrays )*/
#define SHOW_HOME_TIMELINE   0
#define SHOW_LOCAL_TIMELINE  1
#define SHOW_GLOBAL_TIMELINE 2
#define SHOW_BOOKMARKS       3
#define SHOW_SEARCH_RES      'R'
#define NAVIGATE             'N'
#define ACCOUNT_TOGGLE_RSHIP 'F'

extern unsigned char scrw, scrh;

#endif
