#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "simple_serial.h"
#include "extended_conio.h"

#define FRAMES_DIR "frames/"

#define MAX_OFFSET 126
#define MIN_REPS 3
#define MAX_REPS 10

#if 0
  #define DEBUG printf
#else
 #define DEBUG if (0) printf
#endif

unsigned long bytes_sent = 0;
int do_send = 1;

int cur_base, offset;

extern int serial_delay;

static void send(unsigned char b) {
  bytes_sent++;
  if (do_send)
    simple_serial_putc(b);
}

static void send_base(unsigned char b) {
  DEBUG(" new base %d\n", b);
  DEBUG(" base %d offset %d (should be written at %x)\n", b, offset, 0x2000+(cur_base*MAX_OFFSET)+offset);
  /* Shift the base to help the Apple2 */
  if ((b| 0x80) == 0xFF) {
    printf("Error! Should not!\n");
    exit(1);
  }
  send(b|0x80);
  // cgetc();
}
static void send_offset(unsigned char o) {
  DEBUG("offset %d (should be written at %x)\n", o, 0x2000+(cur_base*MAX_OFFSET)+offset);
  if ((o| 0x80) == 0xFF) {
    printf("Error! Should not!\n");
    exit(1);
  }
  send(o|0x80);
  // cgetc();
}
static void send_num_reps(unsigned char b) {
  DEBUG("  => %d * ", b);
  send(0xFF);
  if ((b & 0x80) != 0) {
    printf("Error! Should not!\n");
    exit(1);
  }
  send(b);
}
static void send_byte(unsigned char b) {
  DEBUG("  => %d\n", b);
  if ((b & 0x80) != 0) {
    printf("Error! Should not!\n");
    exit(1);
  }
  send(b);
}

static void flush_ident(int ident_vals, int last_val) {
  if (ident_vals > MIN_REPS) {
    send_num_reps(ident_vals);
    send_byte(last_val);
  } else {
    while (ident_vals > 0) {
      send_byte(last_val);
      ident_vals--;
    }
  }
}

int main(int argc, char *argv[]) {
  int i, j, diffs;
  int last_diff;
  int last_val, ident_vals;
  int total = 0, min = 0xFFFF, max = 0;
  FILE *fp1 = NULL, *fp2 = NULL;
  char filename1[FILENAME_MAX], filename2[FILENAME_MAX];
  unsigned char buf1[8192], buf2[8192];
  // time_t last_time;
  // time_t now;
  // int fps = 0;

  simple_serial_open();

  printf("hit to start\n");
  cgetc();

  /* Force clear */
  offset = 0;
  send_offset(offset);
  for(cur_base = 0; cur_base < 8192/MAX_OFFSET; cur_base++) {
    send_base(cur_base);
    while (1) {
      flush_ident(MAX_REPS, 0);
      offset += MAX_REPS;
      if (offset >= MAX_OFFSET) {
        offset -= MAX_OFFSET;
        send_offset(offset);
        break;
      } else {
        send_offset(offset);
      }
    }
  }
  /* clear finished */
  send_base(cur_base);

  i = 12;
  memset(buf1, 0, 8192);

next_file:
  i++;
  sprintf(filename2, FRAMES_DIR"out-%05d.jpg.hgr", i);
  fp2 = fopen(filename2, "rb");
  if (!fp2) {
    printf("Can't open %s: %s\n", filename2, strerror(errno));
    goto close_last;
  }

  if (fp1 != NULL) {
    fread(buf1, 1, 8192, fp1);
  }
  fread(buf2, 1, 8192, fp2);

  /* count diffs */
  diffs = 0;
  last_diff = 0;
  cur_base = 0;
  ident_vals = 0;

  /* Sync point - force a switch to base 0 */
  send_offset(0);
  send_base(0);
  printf("sync point\n");
  simple_serial_getc();

  last_val = -1;
  for (j = 0; j < 8192; j++) {
    if (buf1[j] != buf2[j]) {
      offset = j - (cur_base*MAX_OFFSET);
      if (offset >= MAX_OFFSET) {
        /* must flush ident */
        flush_ident(ident_vals, last_val);
        ident_vals = 0;

        /* we have to update base */
        cur_base = j / MAX_OFFSET;
        offset = j - (cur_base*MAX_OFFSET);

        send_offset(offset);
        send_base(cur_base);
      } else if (j != last_diff+1) {
        /* must flush ident */
        flush_ident(ident_vals, last_val);
        ident_vals = 0;
        /* We have to send offset */
        send_offset(offset);
      }
      if (last_val == -1 || 
         (ident_vals < MAX_REPS && buf2[j] == last_val && j == last_diff+1)) {
        ident_vals++;
      } else {
        flush_ident(ident_vals, last_val);
        ident_vals = 1;
      }
      last_val = buf2[j];

      last_diff = j;
      diffs++;
    }
  }
  flush_ident(ident_vals, last_val);
  // cgetc();
  ident_vals = 0;
  total += diffs;
  if (diffs < min) {
    min = diffs;
  }
  if (diffs > max) {
    max = diffs;
  }
  DEBUG("%s => %s : %d differences\n", filename1, filename2, diffs);

  // cgetc();
  /* First image has fp1 NULL */
  if (fp1 != NULL) {
    fclose(fp1);
  }
  fp1 = fp2;
  rewind(fp1);
  memcpy(buf1, buf2, 8192);
  strcpy (filename1, filename2);
  goto next_file;

close_last:
    fclose(fp1);

    printf("Max: %d, Min: %d, Average: %d\n", max, min, total / i);
    printf("Sent %lu bytes for %d frames: %lu avg\n", bytes_sent, i, bytes_sent/i);
    return 0;
}
