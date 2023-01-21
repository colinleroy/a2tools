#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "api.h"

#define BUF_SIZE 255

#define LEFT_COL_WIDTH 20

static unsigned char scrw, scrh;

static char *my_public_name = NULL;
static char *my_handle = NULL;

static void print_header(void) {
  //print_logo();

  if (my_public_name == NULL) {
    api_get_profile(&my_public_name, &my_handle);
  }
  if (strlen(my_public_name) > LEFT_COL_WIDTH)
    my_public_name[LEFT_COL_WIDTH] = '\0';

  if (strlen(my_handle) > LEFT_COL_WIDTH)
    my_handle[LEFT_COL_WIDTH] = '\0';

  cputsxy(0, 0, my_public_name);
  gotoxy(0, 1);
  cprintf("@%s\r\n", my_handle);
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
}

static void print_timeline(char *tlid) {
  status **posts;
  int n_posts, i;
  n_posts = api_get_timeline_posts(tlid, &posts);

  for (i = 0; i < n_posts; i++) {
    status *disp = posts[i];
    if (posts[i]->reblog) {
      cprintf("%s shared\r\n", posts[i]->account->display_name);
      disp = posts[i]->reblog;
    }
    cprintf("%s - %s\r\n", disp->account->display_name, posts[i]->created_at);
    cprintf("%s\r\n", posts[i]->account->username);
    cprintf("\r\n%s\r\n", posts[i]->content);
    
    cprintf("\r\n");
  }
}

void cli(void) {
  screensize(&scrw, &scrh);

  clrscr();

  print_header();
  print_timeline(HOME_TIMELINE);
  cgetc();
}
