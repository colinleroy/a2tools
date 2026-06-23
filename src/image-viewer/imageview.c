#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "extended_conio.h"
#include "platform.h"
#include "file_select.h"
#include "hgr.h"
#include "hgr_addrs.h"
#include "path_helper.h"
#include "progress_bar.h"
#include "scrollwindow.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "a2_features.h"
#include "extrazp.h"
#include "iw_print_ui.h"

uint8 scrw, scrh;
#ifndef __CC65__
char HGR_PAGE[HGR_LEN];
#endif

uint8 is_dhgr = 0;

static void get_next_image(char *imgname) {
#ifdef __CC65__
  char *filename = strrchr(imgname, '/');
  DIR *d;
  struct dirent *ent;
  uint8 found = 0, loop = 0;

  if (filename) {
    *filename = '\0';
  } else {
    return;
  }

  iw_print_open_port(ser_params.data_slot);

  d = opendir(imgname);
  if (!d) {
    *filename = '/';
    return;
  }

start_from_first:
  while ((ent = readdir(d))) {
    if (found == 0 && !strcmp(ent->d_name, filename+1)) {
      found = 1;
      continue;
    }
    if (found == 1) {
      if (((ent->d_type == PRODOS_T_BIN && (ent->d_auxtype == 0x2000 || ent->d_auxtype == 0x0))
            || (ent->d_type == PRODOS_T_FOT && ent->d_auxtype < 0x4000))
        && ((ent->d_size <= 8192UL && ent->d_size >= 8184UL)
            || (ent->d_size <= 2*8192UL && ent->d_size >= 2*8184UL))) {
         /* this is, quite probably, an image */
         sprintf(filename, "/%s", ent->d_name);
         found = 2;
         break;
       }
    }
  }

  if (found < 2) {
    found = 1;
    rewinddir(d);
    loop++;
    if (loop == 1) {
      goto start_from_first;
    } else {
      /* Bail */
      *filename = '/';
    }
  }
  closedir(d);
#endif
}

static void print_help(void) {
  gotoxy(0, 20);
  if (has_128k)
    cputs("P: print to Imagewriter - C: toggle color - Space: next\r\n"
          "Esc: exit - Any other key: toggle help.");
  else
    cputs("P: print - Space: next - Esc: exit\r\n"
          "Any other key: show help.");
}

int main(int argc, char *argv[]) {
  int fd, ramfd;
  static char imgname[FILENAME_MAX];
  uint16 len;
  uint8 i, tries = 0, do_print = 0;
  static char cmdline[127];
  #define BLOCK_SIZE 512
  const char *filename = NULL;
  static char rambuf[BLOCK_SIZE];
  uint8 monochrome = 0;

  register_start_device();

  reserve_auxhgr_file();

  try_videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);
#ifndef __CC65__
  scrw = 80; scrh = 24;
#endif

  if (argc > 2 && !strcmp(argv[3], "-p")) {
    do_print = 1;
  }

choose_again:
  init_text();
  set_scrollwindow(0, scrh);
  clrscr();

  if (argc > 1) {
    if (strcmp(argv[1], "___SEL___"))
      filename = argv[1];
  }

  cputs("Image view\r\n\r\n");
  if (!filename) {
    char *tmp;
    cputs("Image (HGR): ");

    iw_print_open_port(ser_params.data_slot);
    tmp = file_select(0, "Select an HGR file");
    if (tmp == NULL)
      goto out;
    strcpy(imgname, tmp);
    free(tmp);
  } else {
    strcpy(imgname, filename);
  }

next_image:
  iw_print_open_port(ser_params.data_slot);

  fd = open(imgname, O_RDONLY);
  if (fd <= 0) {
    init_text();
    cprintf("Can not open image %s (%s).\r\n", imgname, strerror(errno));
    cgetc();
    goto out;
  }

  memset((char *)HGR_PAGE, 0x0, HGR_LEN);
  init_text();
  gotoxy(0, 17);
  cprintf("Loading image %s...               ", imgname);
  print_help();
  progress_bar(0, 18, scrw, 0, HGR_LEN*2);

  len = 0;

  if (has_128k && can_dhgr) {
    
    is_dhgr = (lseek(fd, 0, SEEK_END) == 2*HGR_LEN);
    lseek(fd, 0, SEEK_SET);
    if (is_dhgr && (ramfd = open(hgr_auxfile, O_WRONLY|O_CREAT)) > 0) {
      while (len < HGR_LEN) {
        read(fd, rambuf, BLOCK_SIZE);
        write(ramfd, rambuf, BLOCK_SIZE);
        if (len == 0) {
          monochrome = ((rambuf[0x78] % 2) == 0);
        }
        len += BLOCK_SIZE;
        progress_bar(-1, -1, scrw, len, HGR_LEN*2);
      }
      close(ramfd);
    }
  }

  len = 0;
  while (len < HGR_LEN) {
    read(fd, (char *)(HGR_PAGE + len), BLOCK_SIZE);
    if (len == 0 && !is_dhgr) {
      monochrome = (((char *)HGR_PAGE)[0x78] % 2 == 0);
    }
    len += BLOCK_SIZE;
    progress_bar(-1, -1, scrw, len+HGR_LEN, HGR_LEN*2);
  }
  close(fd);

redisplay:
  #ifdef __CC65__
  init_graphics(monochrome, is_dhgr);
#endif
again:
  if (do_print) {
    i = 'p';
    do_print = 0;
  } else {
    i = tolower(cgetc());
  }
  switch(i) {
    case 'c':
      monochrome = !monochrome;
      goto redisplay;
    case 'p':
      if (is_dhgr)
        dhgr_print();
      else
        hgr_print();
      goto again;
    case CH_ESC:
      if (argc > 1 && strcmp(argv[1], "___SEL___"))
        goto out;
      else
        goto choose_again;
    case ' ':
      get_next_image(imgname);
      goto next_image;
    default:
      if (hgr_mix_is_on) {
        hgr_mixoff();
      } else{
        hgr_mixon();
        clrscr();
        print_help();
      }
      goto again;
  }

out:
  init_text();

  /* Pass back extra arguments received at startup */
  if (argc > 2) {
    cmdline[0] = '\0';
    for (i = 3; i < argc; i++) {
      if (argv[i][0] != '-') {
        strcat(cmdline, argv[i]);
        strcat(cmdline, " ");
      }
    }
    exec(argv[2], cmdline);
  }
  return 0;
}
