/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#ifdef __CC65__
#include <device.h>
#include <dio.h>
#endif
#include "stp_list.h"
#include "stp_save.h"
#include "get_buf_size.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "math.h"
#include "file_select.h"
#include "get_dev_from_path.h"

#define APPLESINGLE_HEADER_LEN 58

#pragma code-name(push, "LC")

char *stp_confirm_save_all(void) {
  char *out_dir;

  clrzone(0, 2, NUMCOLS - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 4);
  cprintf("Save all files in current directory\r\n\r\n");

  cprintf("Save to: ");
  out_dir = file_select(1, "Select destination directory");
  return out_dir;
}

#pragma code-name(pop)

char *cleanup_filename(char *in) {
  int len = strlen(in), i;
  if (len > 15) {
    in[16] = '\0';
    len = 15;
  }
#ifdef __CC65__
  in = strupper(in);
#endif
  for (i = 0; i < len; i++) {
    if (in[i] >='A' && in[i] <= 'Z')
      continue;
    if (in[i] >= '0' && in[i] <= '9')
      continue;
    if (in[i] == '.')
      continue;

    in[i] = '.';
  }
  return in;
}

int stp_save_dialog(char *url, char *out_dir) {
  char *filename = strdup(strrchr(url, '/') + 1);
  int r;
  char free_out_dir = (out_dir == NULL);
  extern surl_response resp;

  clrzone(0, 2, NUMCOLS - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 4);
  cprintf("%s\r\n"
          "%s, %lu bytes\r\n"
          "\r\n"
          "Save to: ", filename, resp.content_type ? resp.content_type : "", resp.size);

  if (!out_dir) {
    out_dir = file_select(1, "Select destination directory");

    if (!out_dir) {
      free(filename);
      return 1;
    }
  } else {
    cprintf("%s", out_dir);
  }

  r = stp_save(filename, out_dir);

  free(filename);
  if (free_out_dir)
    free(out_dir);
  return r;
}

static char cancel_transfer(void) {
  if (kbhit()) {
    if (cgetc() == CH_ESC) {
      gotoxy(0, 14);
      cprintf("Cancel transfer? (y/N)                  ");
      if (tolower(cgetc()) == 'y') {
        return 1;
      }
      clrzone(0, 14, NUMCOLS - 1, 14);
    }
  }
  return 0;
}

#define SECTOR_SIZE 256U
#define PRODOS_BLOCK_SIZE 512U
#define TRACK_SIZE (PRODOS_BLOCK_SIZE * 8)
#define SECTORS_PER_TRACK (TRACK_SIZE/SECTOR_SIZE)
#define BLOCKS_PER_TRACK (TRACK_SIZE/PRODOS_BLOCK_SIZE)

static int stp_write_disk(char *out_dir, char prodos_order) {
#ifdef __CC65__
  char dev;
  dhandle_t dev_handle;
  size_t r = 0;
  uint16 cur_block = 0, verif_cur_block = 0;
  uint8 cur_sector, i;
  char *data = NULL, *cur_data, *verif_data = NULL;
  extern surl_response resp;
  uint16 num_blocks = (resp.size / PRODOS_BLOCK_SIZE);
  char dos_sector_map[16] = {0x0, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8,
                             0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0xF};

  /* Get device */
  dev = get_dev_from_path(out_dir);
  if (dev == INVALID_DEVICE) {
    goto err_out_no_free_data;
  }

  /* We need to receive full tracks to reorder sectors in case the image
   * is DOS-ordered. */
  data = malloc(TRACK_SIZE);
  if (!data) {
    goto err_out_no_free_data;
  }
  verif_data = malloc(TRACK_SIZE);

  /* Open device */
  dev_handle = dio_open(dev);
  if (dev_handle == NULL) {
    goto err_out_no_close;
  }

  gotoxy(0, 12);
  if (dio_query_sectcount(dev_handle) != num_blocks) {
    cprintf("Wrong volume size (%d blocks)", dio_query_sectcount(dev_handle));
    goto err_out;
  }
  cprintf("Writing disk...              ");

  progress_bar(0, 15, NUMCOLS - 1, 0, num_blocks);

  do {
    gotoxy(0, 14);
    cprintf("Receiving block %d/%d...   ", cur_block, num_blocks);
    if (prodos_order) {
      /* ProDOS-ordered images are easy, 1:1 mapping */
      /* Get one track from network */
      r = surl_receive_bindata(data, TRACK_SIZE, 1);

      /* Are we done? */
      if (r == 0) {
        goto out;
      }

      /* This should not happen. */
      if (r % PRODOS_BLOCK_SIZE) {
        goto err_out;
      }
    } else {
      /* Reorder sectors for DOS order to ProDOS order */
      for (cur_sector = 0; cur_sector < SECTORS_PER_TRACK; cur_sector ++) {
        char *cur_sector_ptr = data + (dos_sector_map[cur_sector] << 8);
        /* Build track from network */
        r = surl_receive_bindata(cur_sector_ptr, SECTOR_SIZE, 1);
        /* Are we done? */
        if (r == 0) {
          goto out;
        }
        if (r != SECTOR_SIZE) {
          goto err_out;
        }
      }
    }

    /* Write track */
    gotoxy(0, 14);
    cprintf("Writing block %d/%d...     ", cur_block, num_blocks);
    for (i = BLOCKS_PER_TRACK, cur_data = data; i ; i--, cur_data += PRODOS_BLOCK_SIZE) {
      if (dio_write(dev_handle, cur_block, cur_data) != 0) {
        goto err_out;
      }
      cur_block++;
    }
    if (IS_NOT_NULL(verif_data)) {
      gotoxy(0, 14);
      cprintf("Verifying block %d/%d...    ", verif_cur_block, num_blocks);
      for (i = BLOCKS_PER_TRACK, cur_data = verif_data; i ; i--, cur_data += PRODOS_BLOCK_SIZE) {
        if (dio_read(dev_handle, verif_cur_block, cur_data) != 0) {
          goto err_out;
        }
        verif_cur_block++;
      }
      if (memcmp(data, verif_data, PRODOS_BLOCK_SIZE*BLOCKS_PER_TRACK)) {
        cprintf("Data does not match!");
        goto err_out;
      }
    }

    progress_bar(0, 15, NUMCOLS - 1, cur_block, num_blocks);

    /* Check for user cancel */
    if (cancel_transfer()) {
      goto out;
    }
  } while (1);

out:
  /* All done */
  dio_close(dev_handle);
  free(data);
  if (IS_NOT_NULL(verif_data)) {
    free(verif_data);
  }
  return 0;

  /* Error path */
err_out:
  if (IS_NOT_NULL(verif_data)) {
    free(verif_data);
  }
  free(data);
err_out_no_free_data:
  dio_close(dev_handle);
err_out_no_close:
  gotoxy(0, 14);
  cprintf("Error opening disk.");
  cgetc();
  return -1;
#else
  return -1;
#endif
}

int stp_save(char *full_filename, char *out_dir) {
  extern surl_response resp;
  FILE *fp = NULL;
  char *data = NULL;
  char *filename;
#ifdef __APPLE2__
  char *filetype;
#endif
  size_t r = 0;
  uint32 total = 0;
  unsigned int buf_size;
  char keep_bin_header = 0;
  char had_error = 0;
  char *full_path = NULL;
  char start_y = 10;
#ifdef __APPLE2__
  char is_prodos_disk, is_dos_disk;
#endif
  filename = strdup(full_filename);

  clrzone(0, start_y, NUMCOLS - 1, start_y);
  gotoxy(0, start_y);

#ifdef __APPLE2__
  if (strchr(filename, '.') != NULL) {
    filetype = strrchr(filename, '.') + 1;
  } else {
    filetype = "TXT";
  }

  is_prodos_disk = !strcasecmp(filetype, "PO");
  is_dos_disk = !strcasecmp(filetype, "DSK");
  if (is_prodos_disk || is_dos_disk) {
    char prodos = is_prodos_disk || resp.size != ((uint32)PRODOS_BLOCK_SIZE * 280U);
    free(filename);
    gotoxy(0, 14);
    cprintf("Detected disk image with %s sector order.\r\n", prodos ? "ProDOS":"DOS3.3");
    cprintf("Overwrite disk %s? (y/N)", out_dir);
    if (tolower(cgetc()) != 'y') {
      return -1;
    }
    clrzone(0, 14, NUMCOLS - 1, 14);
    return stp_write_disk(out_dir, prodos);
  } else if (!strcasecmp(filetype, "TXT")) {
    _filetype = PRODOS_T_TXT;
    _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  } else if (!strcasecmp(filetype,"HGR")) {
    _filetype = PRODOS_T_BIN;
  } else if (!strncasecmp(filetype,"SYS", 3)) {
    _filetype = PRODOS_T_SYS;
    _auxtype = 0x2000;
    *(strrchr(filename, '.')) = '\0';
  } else if (!strcasecmp(filetype,"BIN")) {
    _filetype = PRODOS_T_BIN;
    *(strrchr(filename, '.')) = '\0';

    /* Look into the header, and skip it by the way */
    data = malloc(APPLESINGLE_HEADER_LEN);
    r = surl_receive_bindata(data, APPLESINGLE_HEADER_LEN, 1);

    if (r == APPLESINGLE_HEADER_LEN
     && data[0] == 0x00 && data[1] == 0x05
     && data[2] == 0x16 && data[3] == 0x00) {
      cprintf("AppleSingle: $%04x\r\n", (data[56]<<8 | data[57]));
      _auxtype = (data[56]<<8 | data[57]);
      free(data);
      data = NULL;
    } else {
      keep_bin_header = 1;
    }
    total = r;

  } else {
    cprintf("Filetype unknown, using BIN.");
    _filetype = PRODOS_T_BIN;
  }

  gotoxy(0, 12);
  cprintf("Saving file...              ");

  filename = cleanup_filename(filename);
#endif

  /* Unlink before writing (a change in AppleSingle header
   * would not be reflected, as it's written only at CREATE)
   */
  full_path = malloc(FILENAME_MAX);
  snprintf(full_path, FILENAME_MAX, "%s/%s", out_dir, filename);
  unlink(full_path);
  fp = fopen(full_path, "w");
  if (fp == NULL) {
    gotoxy(0, 15);
    cprintf("%s: %s", full_path, strerror(errno));
    cgetc();
    had_error = 1;
    goto err_out;
  }

  /* coverity[dead_error_condition] */
  if (keep_bin_header) {
    /* Write what we read */
    if (fwrite(data, sizeof(char), r, fp) < r) {
      gotoxy(0, 15);
      cprintf("%s.", strerror(errno));
      cgetc();
      had_error = 1;
      goto err_out;
    }
    free(data);
  }

  buf_size = get_buf_size();
  data = malloc(buf_size);

  if (data == NULL) {
    gotoxy(0, 15);
    cprintf("Cannot allocate buffer.");
    cgetc();
    had_error = 1;
    goto err_out;
  }

  progress_bar(0, 15, NUMCOLS - 1, 0, resp.size);
  do {
    size_t bytes_to_read = buf_size;
    if (resp.size - total < (uint32)buf_size)
      bytes_to_read = (uint16)(resp.size - total);

    clrzone(0,14, NUMCOLS - 1, 14);
    gotoxy(0, 14);
    cprintf("Reading %zu bytes...", bytes_to_read);

    r = surl_receive_bindata(data, bytes_to_read, 1);

    progress_bar(0, 15, NUMCOLS - 1, total + (bytes_to_read / 2), resp.size);
    clrzone(0,14, NUMCOLS - 1, 14);
    gotoxy(0, 14);
    total += r;
    cprintf("Saving %lu/%lu bytes...", total, resp.size);

    if (cancel_transfer()) {
      had_error = 1;
      goto err_out;
    }

    if (fwrite(data, sizeof(char), r, fp) < r) {
      gotoxy(0, 14);
      cprintf("%s.", strerror(errno));
      cgetc();
      had_error = 1;
      goto err_out;
    }

    progress_bar(0, 15, NUMCOLS - 1, total, resp.size);
  } while (r > 0);

err_out:
  if (fp) {
    fclose(fp);
  }
  if (had_error) {
    unlink(full_path);
  }
  free(full_path);
  free(filename);
  if (data)
    free(data);
  return had_error;
}
