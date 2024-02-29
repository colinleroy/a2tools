#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pthread.h>

#include "../surl_protocol.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "ffmpeg_to_hgr.h"
#include "array_sort.h"

#define MAX_OFFSET 126
#define NUM_BASES  (8192/MAX_OFFSET)+1

#define MIN_REPS   3
#define MAX_REPS   9

/* Set very high because it looks nicer to drop frames than to
 * artifact all the way
 */
#define MAX_DIFFS_PER_FRAME 25000 

#define DOUBLE_BUFFER
#ifdef DOUBLE_BUFFER
#define NUM_PAGES 2
#else
#define NUM_PAGES 1
#endif

#if 0
  #define DEBUG printf
#else
 #define DEBUG if (0) printf
#endif

unsigned long bytes_sent = 0;

char changes_buffer[8192*2];
int changes_num = 0;

int cur_base, offset;

extern int serial_delay;

char output_file[64];

#define FRAME_FILE_FMT "%s/frame-%06d.hgr"

int vhgr_file;
char tmp_filename[FILENAME_MAX];

int video_len;

static int generate_frames(char *uri) {
  unsigned char *buf;
  int ret = 0;
  int frameno = 0;
  printf("Generating frames for %s...\n", uri);

  vhgr_file = -1;
  sprintf(tmp_filename, "/tmp/vhgr-XXXXXX");
  if ((vhgr_file = mkstemp(tmp_filename)) < 0) {
    printf("Could not open output file %s (%s).\n", tmp_filename, strerror(errno));
    return -1;
  }

  /* Make sure it won't linger around */
  unlink(tmp_filename);
  
  if (ffmpeg_to_hgr_init(uri, &video_len) != 0) {
    printf("Could not init ffmpeg.\n");
    return -1;
  }

  while ((buf = ffmpeg_convert_frame()) != NULL) {
    if (write(vhgr_file, buf, HGR_LEN) != HGR_LEN) {
      printf("Could not write data\n");
      close(vhgr_file);
      vhgr_file = -1;
      ret = -1;
      break;
    }
    frameno++;

    /* Send ping every 10% (and only once for every 25 frames matching the modulo)*/
    if ((frameno/FPS) % (video_len / 10) == 0
      && ((frameno/FPS)/(video_len / 10))*((video_len / 10)*FPS) == frameno) {
      int r;
      printf("Frame %d (%ds/%ds), %d\n", frameno, frameno/FPS, video_len,  (video_len / 10));
      simple_serial_putc(SURL_ANSWER_STREAM_LOAD);
      r = simple_serial_getc_with_timeout();
      if (r == SURL_METHOD_ABORT) {
        printf("Client abort\n");
        ret = -1;
        break;
      }
    }
  }

  if (lseek(vhgr_file, 0, SEEK_SET) != 0) {
    printf("lseek: %s\n", strerror(errno));
  }

  ffmpeg_to_hgr_deinit();

  if (ret == 0) {
    simple_serial_putc(SURL_ANSWER_STREAM_READY);
    if (simple_serial_getc() != SURL_CLIENT_READY) {
      printf("Client not ready\n");
      ret = -1;
    }
  }
  return ret;
}

static void cleanup(void) {
  if (vhgr_file > 0) {
    close(vhgr_file);
    vhgr_file = -1;
  }
}

static void enqueue_byte(unsigned char b) {
  bytes_sent++;
  changes_buffer[changes_num++] = b;
}

void flush_changes(void) {
  simple_serial_write(changes_buffer, changes_num);
  changes_num = 0;
}

static void send_base(unsigned char b) {
  DEBUG(" new base %d\n", b);
  DEBUG(" base %d offset %d (should be written at %x)\n", b, offset, 0x2000+(cur_base*MAX_OFFSET)+offset);

  if ((b| 0x80) == 0xFF) {
    printf("Error! Should not!\n");
    exit(1);
  }
  enqueue_byte(b|0x80);
  if (b == 0) {
    flush_changes();
  }
  // cgetc();
}
static void send_offset(unsigned char o) {
  DEBUG("offset %d (should be written at %x)\n", o, 0x2000+(cur_base*MAX_OFFSET)+offset);
  if ((o| 0x80) == 0xFF) {
    printf("Error! Should not!\n");
    exit(1);
  }
  enqueue_byte(o|0x80);
  // cgetc();
}
static void send_num_reps(unsigned char b) {
  DEBUG("  => %d * ", b);
  enqueue_byte(0xFF);
  if ((b & 0x80) != 0) {
    printf("Error! Should not!\n");
    exit(1);
  }
  enqueue_byte(b);
}
static void send_byte(unsigned char b) {
  DEBUG("  => %d\n", b);
  if ((b & 0x80) != 0) {
    printf("Error! Should not!\n");
    exit(1);
  }
  enqueue_byte(b);
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

/* Returns 1 if client is late and we should skip frame */
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
  /* More than two frame late? */
  if (lateness > 2*(1000000/FPS)) {
    return 1;
  } else {
    return 0;
  }
}

typedef struct _byte_diff {
  int offset;
  int changed;
} byte_diff;

static int diff_score(unsigned char a, unsigned char b) {
  int score = 0;
  score += (a & 0x80) != (b & 0x80);
  score += (a & 0x40) != (b & 0x40);
  score += (a & 0x20) != (b & 0x20);
  score += (a & 0x10) != (b & 0x10);
  score += (a & 0x8) != (b & 0x8);
  score += (a & 0x4) != (b & 0x4);
  score += (a & 0x2) != (b & 0x2);
  score += (a & 0x1) != (b & 0x1);
  if ((a > 0x8) && (b < 0x10))
    score += 10;
  if ((b > 0x8) && (a < 0x10))
    score += 10;

  if (b && !a)
    score += 20;
  if (a && !b)
    score += 20;

  return score;
}

#if 0
static int sort_by_score(byte_diff *a, byte_diff *b) {
  return a->changed < b->changed;
}

static int sort_by_offset(byte_diff *a, byte_diff *b) {
  return a->offset > b->offset;
}
#endif

static byte_diff **diffs = NULL;

int surl_stream_url(char *url) {
  int i, j;
  int last_diff;
  int last_val, ident_vals;
  int total = 0, min = 0xFFFF, max = 0;
  size_t r;
  unsigned char buf_prev[2][8192], buf[2][8192];
  struct timeval frame_start, frame_end;
  int skipped = 0, skip_next = 0, duration;
  int command, page = 0;
  int num_diffs = 0;

  if (diffs == NULL) {
    diffs = malloc(8192 * sizeof(byte_diff *));
    for (j = 0; j < 8192; j++) {
      diffs[j] = malloc(sizeof(byte_diff));
    }
  }

  if (generate_frames(url) != 0) {
    printf("Error generating frames\n");
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    cleanup();
    return -1;
  }

  printf("Starting stream\n");
  simple_serial_putc(SURL_ANSWER_STREAM_START);

  memset(changes_buffer, 0, sizeof changes_buffer);

  /* Force clear */
  for (page = 0; page < NUM_PAGES; page++) {
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
  }
  i = 1;
  memset(buf_prev[0], 0, 8192);
  memset(buf_prev[1], 0, 8192);

  gettimeofday(&frame_start, 0);

  page = 0;

next_file:
  i++;

  if ((r = read(vhgr_file, buf[page], HGR_LEN)) != HGR_LEN) {
    goto close_last;
  }

  /* count diffs */
  last_diff = 0;
  cur_base = 0;
  ident_vals = 0;

  /* Sync point - force a switch to base 0 */
  send_offset(0);
  send_base(0);

  if (i > FPS && (i % (15*FPS)) == 0) {
    duration = i/FPS;
    printf("%d seconds, %d frames skipped / %d: %d fps\n", duration,
         skipped, i, (i-skipped)/duration);

  }

  DEBUG("sync point\n");
  command = simple_serial_getc_with_timeout();
  switch (command) {
    case CH_ESC:
    case EOF:
      printf("Timeout, exiting.\n");
      goto close_last;
    case ' ':
      printf("Pause.\n");
      command = simple_serial_getc();
      if (command == SURL_METHOD_ABORT)
        goto close_last;
      printf("Play.\n");
  }
  gettimeofday(&frame_end, 0);
  if (sync_fps(&frame_start, &frame_end) || skip_next) {
    gettimeofday(&frame_start, 0);
    DEBUG("skipping frame\n");
    skipped++;
#ifdef DOUBLE_BUFFER
    if (!skip_next) {
      skip_next = 1;
    } else {
      skip_next = 0;
    }
    page = !page;
#endif
    goto next_file;
  }

  gettimeofday(&frame_start, 0);

  for (num_diffs = 0, j = 0; j < 8192; j++) {
    if (buf_prev[page][j] != buf[page][j]) {
      diffs[num_diffs]->offset = j;
      diffs[num_diffs]->changed = diff_score(buf_prev[page][j], buf[page][j]);
      num_diffs++;
    }
  }

  /* Naive attempt at priorizing changes and flushing them over multiple
   * frames. The result sucks and it looks much better to framedrop.
   */
#if 0
  /* Sort by diff */
  bubble_sort_array((void **)diffs, num_diffs, (sort_func)sort_by_score);

  /* Sort the first ones by offset */
  bubble_sort_array((void **)diffs,
          num_diffs < MAX_DIFFS_PER_FRAME ? num_diffs : MAX_DIFFS_PER_FRAME,
          (sort_func)sort_by_offset);
#endif
  last_val = -1;
  for (j = 0; j < num_diffs && j < MAX_DIFFS_PER_FRAME; j++) {
    int pixel = diffs[j]->offset;

    offset = pixel - (cur_base*MAX_OFFSET);
    if (offset >= MAX_OFFSET) {
      /* must flush ident */
      flush_ident(ident_vals, last_val);
      ident_vals = 0;

      /* we have to update base */
      cur_base = pixel / MAX_OFFSET;
      offset = pixel - (cur_base*MAX_OFFSET);

      send_offset(offset);
      send_base(cur_base);
    } else if (pixel != last_diff+1) {
      /* must flush ident */
      flush_ident(ident_vals, last_val);
      ident_vals = 0;
      /* We have to send offset */
      send_offset(offset);
    }
    if (last_val == -1 || 
       (ident_vals < MAX_REPS && buf[page][pixel] == last_val && pixel == last_diff+1)) {
      ident_vals++;
    } else {
      flush_ident(ident_vals, last_val);
      ident_vals = 1;
    }
    last_val = buf[page][pixel];

    last_diff = pixel;

    /* Note diff done */
    buf_prev[page][pixel] = buf[page][pixel];
  }
  flush_ident(ident_vals, last_val);

  ident_vals = 0;
  total += num_diffs;
  if (num_diffs < min) {
    min = num_diffs;
  }
  if (num_diffs > max) {
    max = num_diffs;
  }

#ifdef DOUBLE_BUFFER
  page = !page;
#endif
  goto next_file;

close_last:
  send_offset(0);
  send_base(NUM_BASES+1); /* Done */
  flush_changes();
  /* Get rid of possible last ack */
  simple_serial_flush();

  cleanup();
  printf("Max: %d, Min: %d, Average: %d\n", max, min, total / i);
  printf("Sent %lu bytes for %d frames: %lu avg\n", bytes_sent, i, bytes_sent/i);
  if (i > FPS) {
    duration = i/FPS;
    printf("%d seconds, %d frames skipped / %d: %d fps\n", duration,
          skipped, i, (i-skipped)/duration);
  }
  return 0;
}