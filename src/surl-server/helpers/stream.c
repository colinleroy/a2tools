#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>

#include "../surl_protocol.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "hgr-convert.h"

#define MAX_OFFSET 126
#define MIN_REPS   3
#define MAX_REPS   10
#define FPS        25
#define FPS_STR   "25"

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

static long lateness = 0;
/* Returns 1 if should skip frame */
static int sync_fps(struct timeval *start, struct timeval *end) {
  unsigned long secs      = (end->tv_sec - start->tv_sec)*1000000;
  unsigned long microsecs = end->tv_usec - start->tv_usec;
  unsigned long elapsed   = secs + microsecs;
  long wait      = 1000000/FPS;

  wait = wait-elapsed;
  if (wait > 0) {
    DEBUG("frame took %lu microsecs, late %ld\n", elapsed, lateness);
    if (lateness > 0) {
      if (wait < lateness) {
        lateness -= wait;
        wait = 0;
      } else {
        wait -= lateness;
        lateness = 0;
      }
    }
    if (wait == 0) {
      DEBUG("catching up, now late %ld\n", lateness);
    } else {
      DEBUG("sleeping %ld\n", wait);
      usleep(wait);
    }
  } else {
    lateness += -wait;
    DEBUG("frame took %lu microsecs, late %ld, not sleeping\n", elapsed, lateness);
  }
  if (lateness > (1000000/FPS)) {
    return 1;
  } else {
    return 0;
  }
}

char frames_directory[64];

static int generate_frames(char *uri) {
  char *ffmpeg_args[7];
  char frame_format[FILENAME_MAX];
  pid_t pid;
  
  strcpy(frames_directory, "/tmp/frames-XXXXXX");

  if (mkdtemp(frames_directory) == NULL) {
    printf("Could not make temporary directory.\n");
    exit(1);
  }

  sprintf(frame_format, "%s/out-%%06d.jpg", frames_directory);
  ffmpeg_args[0] = "ffmpeg";
  ffmpeg_args[1] = "-i";
  ffmpeg_args[2] = uri;
  ffmpeg_args[3] = "-vf";
  ffmpeg_args[4] = "fps="FPS_STR",scale=128:-1";
  ffmpeg_args[5] = frame_format;
  ffmpeg_args[6] = NULL;
  
  printf("Dumping frames (%s %s %s %s %s %s)...\n",
         ffmpeg_args[0], ffmpeg_args[1],
         ffmpeg_args[2], ffmpeg_args[3],
         ffmpeg_args[4], ffmpeg_args[5]);
  
  pid = fork();
  if (pid == -1) {
    printf("Could not fork\n");
    return -1;
  }
  if (pid == 0) {
    /* Child process */
    int r = execvp(ffmpeg_args[0], ffmpeg_args);
    exit(r);
  } else {
    int status;
    wait(&status);
    if (status != 0) {
      printf("ffmpeg failed: %d\n", status);
      return -1;
    }
  }
  return 0;
}

static void cleanup(void) {
  DIR *d = opendir(frames_directory);
  struct dirent *file;
  char full_name[FILENAME_MAX-5];
  if (!d) {
    printf("Could not open directory %s\n", frames_directory);
    return;
  }
  while ((file = readdir(d)) != NULL) {
    if (!strstr(file->d_name, ".hgr")) {
      continue;
    }
    sprintf(full_name, "%s/%s", frames_directory, file->d_name);
    unlink(full_name);
  }
  rmdir(frames_directory);
}
static int generate_hgr_files(void) {
  DIR *d = opendir(frames_directory);
  struct dirent *file;
  char full_name[FILENAME_MAX-5];
  if (!d) {
    printf("Could not open directory %s\n", frames_directory);
    return -1;
  }
  printf("Converting frames...\n");
  while ((file = readdir(d)) != NULL) {
    if (strstr(file->d_name, ".jpg")) {
      size_t len;
      unsigned char *hgr_buf;
      char hgr_filename[FILENAME_MAX];
      FILE *fp;
      sprintf(full_name, "%s/%s", frames_directory, file->d_name);
      if ((hgr_buf = sdl_to_hgr(full_name, 1, 0, &len, 1, 1)) == NULL) {
        printf("Could not convert %s\n", full_name);
        return -1;
      }
      strcpy(hgr_filename, full_name);
      strcpy(strstr(hgr_filename, ".jpg"), ".hgr");

      if ((fp = fopen(hgr_filename, "wb")) == NULL) {
        printf("Could not open %s\n", hgr_filename);
        return -1;
      }
      if (fwrite(hgr_buf, 1, len, fp) != len) {
        printf("Could not write to %s\n", hgr_filename);
        fclose(fp);
        return -1;
      }
      fclose(fp);
      unlink(full_name);
    }
  }
  printf("done.\n");
  closedir(d);
  return 0;
}

int surl_stream_url(char *url) {
  int i, j, diffs;
  int last_diff;
  int last_val, ident_vals;
  int total = 0, min = 0xFFFF, max = 0;
  FILE *fp1 = NULL, *fp2 = NULL;
  char filename1[FILENAME_MAX], filename2[FILENAME_MAX];
  unsigned char buf1[8192], buf2[8192];
  struct timeval frame_start, frame_end;
  int command;

  if (generate_frames(url) != 0) {
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    return -1;
  }
  if (generate_hgr_files() != 0) {
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    return -1;
  }

  simple_serial_putc(SURL_ANSWER_STREAM_START);

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

  gettimeofday(&frame_start, 0);

next_file:
  i++;
  sprintf(filename2, "%s/out-%06d.hgr", frames_directory, i);
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
  DEBUG("sync point\n");
  command = simple_serial_getc_with_timeout();
  switch (command) {
    case CH_ESC:
    case EOF:
      goto close_last;
    case ' ':
      printf("Pause.\n");
      command = simple_serial_getc();
      if (command == SURL_METHOD_ABORT)
        goto close_last;
      printf("Play.\n");
  }
  if (command == EOF) {
    goto close_last;
  } else {
    printf("got ack %x\n", command);
  }
  gettimeofday(&frame_end, 0);
  if (sync_fps(&frame_start, &frame_end)) {
    gettimeofday(&frame_start, 0);
    DEBUG("skipping frame\n");
    fclose(fp2);
    goto next_file;
  }
  gettimeofday(&frame_start, 0);

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
  if (fp1)
    fclose(fp1);

  cleanup();
  printf("Max: %d, Min: %d, Average: %d\n", max, min, total / i);
  printf("Sent %lu bytes for %d frames: %lu avg\n", bytes_sent, i, bytes_sent/i);
  return 0;
}
