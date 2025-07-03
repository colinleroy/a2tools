#ifndef __cli_h
#define __cli_h

#include "a2_features.h"
#include "account.h"

#define NUMLINES 24
#define TIME_COLUMN 41
#define N_STATUS_TO_LOAD 10

extern unsigned char NUMCOLS;
extern unsigned char LEFT_COL_WIDTH;
extern unsigned char RIGHT_COL_START;
extern char STATE_FILE[];

#ifndef __APPLE2ENH__
#define CH_CURS_UP 0x0B
#define CH_CURS_DOWN 0x0A
#endif

/* actions mapped to keys */
extern unsigned char SHOW_HELP;
#define SHOW_FULL_STATUS     CH_ENTER
#define BACK                 CH_ESC
#define COMPOSE              'c'
#define CONFIGURE            'o'
#define SHOW_NOTIFICATIONS   'n'
#define SEARCH               '/'
#define SHOW_PROFILE         'p'
#define IMAGES               'i'
#define REPLY                'r'
#define EDIT                 'e'
#define VOTING               'v'
#define QUIT                 'q'
#define SHOW_QUOTE           't'

/* special cases (extra step or mapped arrays )*/
#define SHOW_HOME_TIMELINE   0
#define SHOW_LOCAL_TIMELINE  1
#define SHOW_GLOBAL_TIMELINE 2
#define SHOW_BOOKMARKS       3
#define SHOW_SEARCH_RES      'R'
#define NAVIGATE             'N'
#define ACCOUNT_TOGGLE_RSHIP 'F'

#endif
