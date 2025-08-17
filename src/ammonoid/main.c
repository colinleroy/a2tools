#include <fcntl.h>
#include <dirent.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <device.h>
#include <dio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include "clrzone.h"
#include "scrollwindow.h"
#include "malloc0.h"
#include "dget_text.h"
#include "surl.h"
#include "vsdrive.h"

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
#define set_select(entry, on) do { (entry)->d_blocks = on; } while(0)
#define select(entry, on) do { (entry)->d_blocks = 1; } while(0)
#define deselect(entry) do { (entry)->d_blocks = 0; } while(0)
#define toggle_select(entry) do { (entry)->d_blocks = !(entry)->d_blocks; } while(0)
#define is_selected(entry) ((entry)->d_blocks)

/* UI */
static unsigned char pane_left[2] = {0, 20};
#define pane_top 2
#define pane_btm 20
#define pane_height (pane_btm-pane_top)
#define pane_width 19
#define total_height 24
#define total_width 40

#define SEL ('D'|0x80)

#if 0
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

static void info_message(const char *msg, unsigned char valid);


static struct dirent *allocate_entries(unsigned char pane, unsigned int n_entries) {
  unsigned long alloc_size = (unsigned long)n_entries * sizeof(struct dirent);
  void *buffer = NULL;

  free(pane_entries[pane]);

  if (!(alloc_size >> 16)) {
    buffer = malloc(alloc_size);
  }
  pane_entries[pane] = buffer;
  if (IS_NULL(buffer)) {
    info_message("Not enough memory.", 1);
  }
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
      rewinddir(d);
      entries = allocate_entries(pane, dir_entry_count(d));
      if (entries == NULL) {
        n = 0;
        goto done;
      }
      DEBUG("allocated %d entries %p\n", dir_entry_count(d), entries);
      while ((ent = readdir(d))) {
        DEBUG("copying entry %d (%s)\n", n, ent->d_name);
        deselect(ent);
        memcpy(&entries[n], ent, sizeof(struct dirent));
        n++;
      }
done:
      DEBUG("closing directory\n");
      closedir(d);
    }
    pane_num_files[pane] = n;
  }
  return;
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

  gotoxy(pane_width - 1, 0);
  cputc('C'|0x80);

  /* Load pane if needed */
  if (pane_entries[pane] == NULL) {
    load_directory(pane);
  }

  /* Clear whole pane */
  set_scrollwindow(0, pane_btm);
  set_hscrollwindow(pane_left[pane]+1, pane_width);

  clrscr();

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
  clrzone(inactive_left, 0, inactive_left, pane_btm-1);
  /* Clear inactive pane's cursor and select indicators */
  inactive_left++;
  clrzone(inactive_left, 2, inactive_left + 1, pane_btm-1);

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

static char *build_full_path(unsigned char pane, const struct dirent *entry) {
  char *filename = malloc0(FILENAME_MAX+1);
  strcpy(filename, pane_directory[pane]);
  if (filename[0] && entry->d_name[0] != '/') {
    strcat(filename, "/");
  }
  strcat(filename, entry->d_name);

  return filename;
}

/* Return entry at index idx in active pane */
static struct dirent *get_entry_at(unsigned char idx) {
  /* Check there is at least one file displayed */
  if (pane_num_files[active_pane] == 0) {
    return NULL;
  }
  return &pane_entries[active_pane][idx];
}

/* Return the entry at cursor in active pane */
static struct dirent *get_current_entry(void) {
  return get_entry_at(pane_file_cursor[active_pane]);
}

/* Open directory selected in active pane, in target pane */
static void open_directory(unsigned char target_pane) {
  struct dirent *entry;
  char *new_dir;

  entry = get_current_entry();

  if (entry == NULL || !_DE_ISDIR(entry->d_type)) {
    return;
  }
  
  new_dir = build_full_path(active_pane, entry);
  strcpy(pane_directory[target_pane], new_dir);
  free(new_dir);

  cleanup_pane(target_pane);
  active_pane = target_pane;
}

/* Go up in the hierarchy */
static void close_directory(unsigned char pane) {
  char *last_slash = strrchr(pane_directory[pane], '/');
  if (!last_slash) {
    return;
  }
  *last_slash = '\0';
  cleanup_pane(pane);
}

/* Toggle an element's selection */
static void select_current(unsigned char pane) {
  struct dirent *entry;
  if (pane_num_files[active_pane] == 0) {
    return;
  }
  entry = &pane_entries[pane][pane_file_cursor[pane]];
  toggle_select(entry);
}

/* De-select all */
static void select_all(unsigned char pane) {
  struct dirent *entries, *entry;
  unsigned char on;
  int n;

  if (pane_num_files[pane] == 0) {
    return;
  }
  entries = pane_entries[pane];
  on = !is_selected(&entries[0]);
  for (n = 0; n < pane_num_files[pane]; n++) {
    entry = &entries[n];
    set_select(entry, on);
  }
}

void set_logwindow(void) {
  set_scrollwindow(pane_btm, total_height);
  set_hscrollwindow(0, total_width);
  clrscr();
  chline(total_width);
}

static void info_message(const char *msg, unsigned char valid) {
  set_logwindow();
  cputs(msg);
  if (valid) {
    cputs("\r\nPress a key to continue.");
    cgetc();
    set_logwindow();
    cputs("Press H for help.");
  }
}

static char *prompt(const char *verb, const char *dir, const char *file, int len) {
  char *buf = malloc(len);

  set_logwindow();

  strcpy(buf, file);
  cputs(verb);
  cputs(dir);
  cputc('/');
  dget_text_single(buf, len, NULL);

  return buf;
}

static void refresh_other_pane(void) {
  if (!strcmp(pane_directory[active_pane], pane_directory[!active_pane])) {
    cleanup_pane(!active_pane);
    display_pane(!active_pane);
  }
}

static void file_info(void) {
  struct dirent *entry = get_current_entry();
  if (!entry) {
    return;
  }
  set_logwindow();
  printf("%s: %lub, type $%02x, access $%02x",
         entry->d_name, entry->d_size, entry->d_type, entry->d_access);
  cputs("\r\nPress a key to continue.");
  cgetc();
  clrscr();
}

/* Rename current file in pane */
static void rename_file(void) {
  struct dirent *entry = get_current_entry();
  char *new_name;

  if (entry == NULL) {
    return;
  }

  if (entry->d_name[0] == '/') {
    return;
  }

  if (chdir(pane_directory[active_pane]) != 0) {
    return;
  }

  new_name = prompt("New name: ", pane_directory[active_pane], entry->d_name, 16);
  if (new_name[0] == '\0' || !strcmp(new_name, entry->d_name)) {
    goto out;
  }

  if (rename(entry->d_name, new_name) == 0) {
    strcpy(entry->d_name, new_name);
  }
out:
  free(new_name);
  clrscr();
  refresh_other_pane();
}

/* Create directory in pane */
static void make_directory(void) {
  char *new_name = malloc0(16);

  if (pane_directory[active_pane][0] == '\0'
   || chdir(pane_directory[active_pane]) != 0) {
    return;
  }

  new_name = prompt("Directory name: ", pane_directory[active_pane], new_name, 16);
  if (new_name[0] == '\0') {
    goto out;
  }

  mkdir(new_name, O_RDWR);
out:
  free(new_name);
  clrscr();
  refresh_other_pane();
  load_directory(active_pane);
}

static void pane_chdir(unsigned char pane, const char *dir) {
  cleanup_pane(pane);
  strcpy(pane_directory[pane], dir);
  load_directory(pane);
}

static char *copy_buf = NULL;
#define COPY_BUF_SIZE 4096

typedef struct _file_list {
  char filename[FILENAME_MAX+1];
} file_list;

static char *selection = NULL;

static void backup_selection(void) {
  int i;
  if (selection) {
    __asm__("brk");
  }
  selection = malloc0(pane_num_files[active_pane]);
  for (i = 0; i < pane_num_files[active_pane]; i++) {
    struct dirent *entry = get_entry_at(i);
    selection[i] = is_selected(entry);
  }
}

static void restore_selection(void) {
  int i;
  for (i = 0; i < pane_num_files[active_pane]; i++) {
    struct dirent *e = get_entry_at(i);
    set_select(e, selection[i]);
  }
  free(selection);
  selection = NULL;
}

#pragma static-locals (push,off) /* need reentrancy */
static int do_iterate_files(unsigned char all, unsigned char copy, unsigned char remove) {
  int global_err = 0;
  int n;
  file_list *to_delete = NULL;
  int n_to_delete = 0;

  if (copy_buf == NULL) {
    copy_buf = malloc(COPY_BUF_SIZE);
  }

  if (remove) {
    to_delete = malloc0(pane_num_files[active_pane]*sizeof(file_list));
  }

  for (n = 0; n < pane_num_files[active_pane]; n++) {
    struct dirent *entry = get_entry_at(n);
    if (all || is_selected(entry)) {
      char *src = build_full_path(active_pane, entry);
      char *dest = build_full_path(!active_pane, entry);
      if (_DE_ISDIR(entry->d_type)) {
        int dir_err = 0;

        if (copy) {
          if (!strncmp(dest, src, strlen(src))
            && (dest[strlen(src)] == '\0' || dest[strlen(src)] == '/')) {
            printf("Can not copy %s to %s\n", src, dest);
            global_err = 1;
            goto next;
          }
          printf("mkdir %s", dest);
          dir_err = (mkdir(dest, O_RDWR) != 0);
          if (!dir_err) {
            printf(": OK\n");
          }
        }
        if (!dir_err) {
          if (!all) {
            backup_selection();
          }

          /* Recurse */
          pane_chdir(active_pane, src);
          if (copy) {
            pane_chdir(!active_pane, dest);
          }

          dir_err = do_iterate_files(1, copy, remove);
          global_err |= dir_err;
          if (remove && !dir_err) {
            strcpy(to_delete[n_to_delete].filename, src);
            n_to_delete++;
          }

          *strrchr(src, '/') = '\0';
          *strrchr(dest, '/') = '\0';

          pane_chdir(active_pane, src);
          if (copy) {
            pane_chdir(!active_pane, dest);
          }

          if (!all) {
            restore_selection();
          }
        } else {
          global_err = 1;
          printf(": %s\n", strerror(errno));
        }
      } else {
        FILE *in;
        FILE *out;
        int err = 0;
        if (copy) {
          printf("%s", src);
          in = fopen(src, "r");
          if (in) {
            out = fopen(dest, "w");
            if (out) {
              size_t r;
              while ((r = fread(copy_buf, 1, COPY_BUF_SIZE, in)) > 0) {
                if (fwrite(copy_buf, 1, r, out) < r) {
                  printf(": %s\n", strerror(errno));
                  err = 1;
                  break;
                }
              }
              fclose(out);
            } else {
              printf(": %s\n", strerror(errno));
              err = 1;
            }
            fclose(in);
          } else {
            printf(": %s\n", strerror(errno));
            err = 1;
          }
        }
        if (!err) {
          if (copy) {
            printf(": OK\n");
          }
          if (remove) {
            strcpy(to_delete[n_to_delete].filename, src);
            n_to_delete++;
          }
        } else {
          global_err = 1;
        }
      }
next:
      free(src);
      free(dest);
    }
  }

  if (remove) {
    for (n = 0; n < n_to_delete; n++) {
      char *path;
      path = to_delete[n].filename;

      if (!copy) {
        printf("%s", path);
      }
      if (unlink(path) == 0) {
        if (!copy) {
          printf(": OK\n");
        }
      } else if (!copy) {
        printf(": %s\n", strerror(errno));
      }
    }
    free(to_delete);
  }
  return global_err;
}

#pragma static-locals (pop)

static void iterate_files(unsigned char all, unsigned char copy, unsigned char remove) {
  char *pane_orig_directory[2] = {NULL, NULL};

  if (copy && !strcmp(pane_directory[0], pane_directory[1])) {
    return;
  }

  set_logwindow();

  pane_orig_directory[0] = strdup(pane_directory[0]);
  pane_orig_directory[1] = strdup(pane_directory[1]);

  if (do_iterate_files(all, copy, remove) != 0) {
    printf("There have been errors.\n");
    cgetc();
  }

  clrscr();

  strcpy(pane_directory[0], pane_orig_directory[0]);
  strcpy(pane_directory[1], pane_orig_directory[1]);
  free(pane_orig_directory[0]);
  free(pane_orig_directory[1]);
  cleanup_pane(active_pane);
  if (copy) {
    cleanup_pane(!active_pane);
    display_pane(!active_pane);
  } else {
    refresh_other_pane();
  }
}

void help(void) {
  set_scrollwindow(0, 24);
  set_hscrollwindow(0, total_width);
  clrscr();

  cputs("Left/right: switch active pane\r\n"
        "Up/Down: scroll in active pane\r\n"
        "Enter: open in active pane\r\n"
        "Esc: close directory in active pane\r\n"
        "Space: select in active pane\r\n"
        "A: select all in active pane\r\n"
        "O: open in other pane\r\n"
        "R: Rename file\r\n"
        "D: Delete selected\r\n"
        "N: Create directory in active pane\r\n"
        "C: Copy selected to other pane\r\n"
        "M: Move selected to other pane\r\n"
        "I: File information\r\n"
        "\r\n\r\n"
        "(c) Colin Leroy-Mira, 2025\r\n"
        "https://colino.net/\r\n"
        "\r\n\r\n"
        "Press a key to go back."
      );

  cgetc();
  clrscr();
  display_pane(0);
  display_pane(1);
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
      select_current(active_pane);
      return;
    case 'a':
      select_all(active_pane);
      return;
    case 'r':
      rename_file();
      return;
    case 'n':
      make_directory();
      return;
    case 'c':
      iterate_files(0, 1, 0);
      return;
    case 'm':
      iterate_files(0, 1, 1);
      return;
    case 'd':
      iterate_files(0, 0, 1);
      return;
    case 'i':
      file_info();
      return;
    case 'h':
      help();
      return;
  }
}

void main(void) {
  clrscr();
  getcwd(pane_directory[1], FILENAME_MAX);

  if (simple_serial_open() == 0) {
    vsdrive_install();
  }

  display_pane(0);
  display_pane(1);
  while (1) {
    info_message("Press H for help.", 0);
    display_active_pane();
    handle_input();
  }
}
