#include <dirent.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <device.h>
#include <dio.h>
#include <unistd.h>
#include <ctype.h>

#include "clrzone.h"
#include "scrollwindow.h"
#include "prodos_dir_file_count.h"
#include "malloc0.h"

#define PRODOS_MAX_VOLUMES 37

/* State */
static unsigned char active_pane = 0;
static struct dirent *pane_entries[2] = {NULL, NULL};
static char pane_directory[2][FILENAME_MAX+1] = {"", ""};
static unsigned int pane_num_files[2] = {0, 0};
static unsigned int pane_first_file[2] = {0, 0};
static unsigned int pane_file_cursor[2] = {0, 0};

/* Selection status, stored in dirent's d_block as we don't really
 * care about that field. */
#define select(entry) do { entry->d_blocks = 1; } while(0)
#define deselect(entry) do { entry->d_blocks = 0; } while(0)
#define toggle_select(entry) do { entry->d_blocks = !entry->d_blocks; } while(0)
#define is_selected(entry) (entry->d_blocks)

/* UI */
static unsigned char pane_left[2] = {0, 20};
#define pane_top 2
#define pane_btm 22
#define pane_height (pane_btm-pane_top)
#define pane_width 19
#define total_width 40

#define SEL ('D'|0x80)

#if 0
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

static struct dirent *allocate_entries(unsigned char pane, unsigned int n_entries) {
  free(pane_entries[pane]);
  pane_entries[pane] = malloc0(sizeof(struct dirent)*n_entries);
  return pane_entries[pane];
}

static void load_devices(unsigned char pane) {
  char dev;
  struct dirent *entries = NULL;
  struct dirent *entry;
  unsigned int n = 0;

  DEBUG("listing devices at %d\n", pane);

  entries = allocate_entries(pane, PRODOS_MAX_VOLUMES);
  entry = &entries[0];

  dev = getfirstdevice();
  do {
    /* We're getting device name into d_name[16] but the
     * result can be 17 bytes including NULL terminator,
     * as cc65's runtime adds a / in front of the 15-char
     * long device name. This will overwrite d_ino with
     * the terminator, but we don't care.
     */
    if (getdevicedir(dev, entry->d_name, 17) == NULL) {
#ifdef FILESEL_ALLOW_NONPRODOS_VOLUMES
      int blocks;
      dhandle_t dev_handle = dio_open(dev);
      if (!dev_handle) {
        continue;
      }
      blocks = dio_query_sectcount(dev_handle);
      dio_close(dev_handle);
      if (blocks != 280U) {
        continue;
      }
      /* Dev: 0000DSSS (as ProDOS, but shifted >>4)*/
      sprintf(entry->d_name, "S%dD%d",
        (dev & 0x07), (dev & 0x08) ? 2:1);
#else
      continue;
#endif
    }
    DEBUG("got device %d %s\n", n, entry->d_name);
    entry->d_type = 0x0F;
    deselect(entry);
    n++;
    entry = &entries[n];
  } while ((dev = getnextdevice(dev)) != INVALID_DEVICE);
  pane_num_files[pane] = n;
}

static void load_directory(unsigned char pane) {
  DEBUG("load dir %d\n", pane);
  if (pane_directory[pane][0] == '\0') {
    load_devices(pane);
  } else {
    DIR *d = opendir(pane_directory[pane]);
    struct dirent *ent;
    struct dirent *entries = NULL;
    unsigned int n = 0;
    DEBUG("opened %s (%p)\n", pane_directory[pane], d);
    if (d) {
      entries = allocate_entries(pane, prodos_dir_file_count(d));
      DEBUG("allocated %d entries %p\n", prodos_dir_file_count(d), entries);
      while ((ent = readdir(d))) {
        DEBUG("copying entry %d (%s)\n", n, ent->d_name);
        deselect(ent);
        memcpy(&entries[n], ent, sizeof(struct dirent));
        n++;
      }
      DEBUG("closing directory\n");
      closedir(d);
    }
    pane_num_files[pane] = n;
  }
}

/* Display pane's title */
static void display_title(const char *title) {
  int len = strlen(title);
  if (len < pane_width) {
    cputs(title);
  } else {
    cputs("...");
    cputs(title + len + 3 - pane_width);
  }
  gotoxy(0, 1);
  chline(pane_width);
}

/* Display a pane */
static void display_pane(unsigned char pane) {
  struct dirent *entries = NULL;
  struct dirent *entry = NULL;
  unsigned int n, start, stop, cur;

  /* Clear whole pane */
  set_scrollwindow(0, pane_btm);
  set_hscrollwindow(pane_left[pane]+1, pane_width);
  clrscr();

  /* Load pane if needed */
  if (pane_entries[pane] == NULL) {
    load_directory(pane);
  }

  /* Print pane title */
  if (pane_directory[pane][0] == '\0') {
    display_title("Devices");
  } else {
    display_title(pane_directory[pane]);
  }

  set_scrollwindow(pane_top, pane_btm);
  gotoxy(0, 0);

  /* Print contents */
  entries = pane_entries[pane];
  start = pane_first_file[pane];
  if (pane == active_pane) {
    cur = pane_file_cursor[pane];
  } else {
    cur = 0xFFFF;
  }

  stop = pane_num_files[pane];
  if (stop - start > pane_height) {
    stop = start + pane_height;
  }

  for (n = start; n < stop; n++) {
    entry = &entries[n];
    cputc(n == cur           ? '>' : ' '); /* Current cursor */
    cputc(is_selected(entry) ? SEL : ' '); /* Selected */
    cputs(entry->d_name);
    if (_DE_ISDIR(entry->d_type)) {
      cputc('/');
    }
    clreol();
    cputs("\r\n");
  }
}

/* Display line on active pane, clear cursor/selection on inactive */
static void display_active_pane(void) {
  unsigned char inactive_left = pane_left[!active_pane];
  set_scrollwindow(0, pane_btm);
  set_hscrollwindow(0, total_width);
  gotoxy(pane_left[active_pane], 0);
  cvline(pane_btm);
  /* Clear inactive pane's line */
  clrzone(inactive_left, 0, inactive_left, pane_btm);
  /* Clear inactive pane's cursor and select indicators */
  inactive_left++;
  clrzone(inactive_left, 2, inactive_left + 1, pane_btm);

  display_pane(active_pane);
}

/* Make sure the cursor is in the displayed window */
static void fixup_start(unsigned char pane) {
  if (pane_first_file[pane] + pane_height <= pane_file_cursor[pane]) {
    pane_first_file[pane] = pane_file_cursor[pane] - pane_height + 1;
  }
  if (pane_file_cursor[pane] < pane_first_file[pane]) {
    pane_first_file[pane] = pane_file_cursor[pane];
  }
}

/* Deallocate and reset pane variables */
static void cleanup_pane(unsigned char pane) {
  free(pane_entries[pane]);
  pane_entries[pane] = NULL;
  pane_num_files[pane] = 0;
  pane_first_file[pane] = 0;
  pane_file_cursor[pane] = 0;
}

/* Open directory selected in active pane, in target pane */
static void open_directory(unsigned char target_pane) {
  struct dirent *entry;
  unsigned char cur = pane_file_cursor[active_pane];

  /* Check there is at least one file displayed */
  if (pane_num_files[active_pane] == 0) {
    return;
  }

  /* Get current entry and check if it's a directory */
  entry = &pane_entries[active_pane][cur];
  if (!_DE_ISDIR(entry->d_type)) {
    return;
  }
  /* Build directory path */
  if (target_pane != active_pane) {
    strcpy(pane_directory[target_pane], pane_directory[active_pane]);
  }
  if (pane_directory[target_pane][0] != '\0') {
    /* Add a slash if not at Devices */
    strcat(pane_directory[target_pane], "/");
  }
  strcat(pane_directory[target_pane], entry->d_name);

  cleanup_pane(target_pane);
  active_pane = target_pane;
}

static void close_directory(unsigned char pane) {
  char *last_slash = strrchr(pane_directory[pane], '/');
  if (!last_slash) {
    return;
  }
  *last_slash = '\0';
  cleanup_pane(pane);
}

static void do_select(unsigned char pane) {
  struct dirent *entry;
  if (pane_num_files[active_pane] == 0) {
    return;
  }
  entry = &pane_entries[active_pane][pane_file_cursor[active_pane]];
  toggle_select(entry);
}

static void handle_input(void) {
  unsigned char cmd = tolower(cgetc());
  switch(cmd) {
    /* Switch to right pane */
    case CH_CURS_LEFT:
      active_pane = 0;
      return;
    /* Switch to left pane */
    case CH_CURS_RIGHT:
      active_pane = 1;
      return;
    /* Scrollup in current pane */
    case CH_CURS_UP:
      if (pane_file_cursor[active_pane] > 0)
        pane_file_cursor[active_pane]--;
      else 
        pane_file_cursor[active_pane] = pane_num_files[active_pane] - 1;
      fixup_start(active_pane);
      return;
    /* Scrolldown in current pane */
    case CH_CURS_DOWN:
      if (pane_file_cursor[active_pane] < pane_num_files[active_pane] - 1)
        pane_file_cursor[active_pane]++;
      else 
        pane_file_cursor[active_pane] = 0;
      fixup_start(active_pane);
      return;
    /* Open directory on current pane */
    case CH_ENTER:
      open_directory(active_pane);
      return;
    /* Open directory on other pane */
    case 'o':
      open_directory(!active_pane);
      return;
    case CH_ESC:
      close_directory(active_pane);
      return;
    case ' ':
      do_select(active_pane);
  }
}

void main(void) {
  clrscr();
  getcwd(pane_directory[1], FILENAME_MAX);
  // strcpy(pane_directory[1], "/HD");

  display_pane(0);
  display_pane(1);
  while (1) {
    display_active_pane();
    handle_input();
  }
}
