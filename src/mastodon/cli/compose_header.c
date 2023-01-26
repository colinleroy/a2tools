#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef __APPLE2__
#include <apple2enh.h>
#endif
#include "surl.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "dputs.h"
#include "dputc.h"
#include "scroll.h"
#include "cli.h"
#include "api.h"
#include "header.h"
#include "list.h"
#include "math.h"
#include "dgets.h"


account *my_account = NULL;

void compose_print_header(status *root_status) {
  if (my_account == NULL) {
    my_account = api_get_profile(NULL);
  }
  if (my_account != NULL) {
    if (strlen(my_account->display_name) > LEFT_COL_WIDTH)
      my_account->display_name[LEFT_COL_WIDTH] = '\0';

    if (strlen(my_account->username) > LEFT_COL_WIDTH)
      my_account->username[LEFT_COL_WIDTH] = '\0';

    cputsxy(0, 0, my_account->display_name);
    gotoxy(0, 1);
    cprintf("%c%s\r\n", arobase, my_account->username);
  }

  #define BTM 10
  clrzone(0, BTM, LEFT_COL_WIDTH, 23);
  gotoxy(0,BTM);

  cputs("Set Audience:\r\n");
  cputs(" Open-Apple +...\r\n");
  cputs(" Send     : S\r\n");
  cputs(" Cancel   : Escape\r\n");

  cputs("\r\n");
  cputs("Set Audience:\r\n");
  cputs(" Open-Apple +...\r\n");
  cputs(" Public   : P\r\n");
  cputs(" Unlisted : U\r\n");
  cputs(" Private  : R\r\n");
  cputs(" Mention  : M\r\n");
  cputs("\r\n");


#ifdef __CC65__
  gotoxy(0, 23);
  cprintf("%zuB free     ", _heapmemavail());
#endif
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
}
