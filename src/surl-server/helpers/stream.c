#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>

#include "../surl_protocol.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "ffmpeg.h"
#include "array_sort.h"
#include "hgr-convert.h"
#include "char-convert.h"

#define MAX_OFFSET 126
#define NUM_BASES  (HGR_LEN/MAX_OFFSET)+1

#define MIN_REPS   3
#define MAX_REPS   10

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

unsigned long bytes_sent = 0, data_bytes = 0, offset_bytes = 0, base_bytes = 0;

char changes_buffer[HGR_LEN*2];
int changes_num = 0;

int cur_base, offset;

extern int serial_delay;

char output_file[64];

#define FRAME_FILE_FMT "%s/frame-%06d.hgr"

char tmp_filename[FILENAME_MAX];

int video_len;

static void *generate_frames(void *th_data) {
  decode_data *data = (decode_data *)th_data;
  unsigned char *buf;
  int frameno = 0;
  int vhgr_file;

  printf("Generating frames for %s...\n", data->url);

  vhgr_file = -1;
  sprintf(tmp_filename, "/tmp/vhgr-XXXXXX");
  if ((vhgr_file = mkstemp(tmp_filename)) < 0) {
    printf("Could not open output file %s (%s).\n", tmp_filename, strerror(errno));
    pthread_mutex_lock(&data->mutex);
    data->decoding_end = 1;
    data->decoding_ret = -1;
    pthread_mutex_unlock(&data->mutex);
  }

  pthread_mutex_lock(&data->mutex);
  if (ffmpeg_to_hgr_init(data->url, &video_len) != 0) {
    printf("Could not init ffmpeg.\n");
    data->decoding_end = 1;
    data->decoding_ret = -1;
  }
  pthread_mutex_unlock(&data->mutex);

  while ((buf = ffmpeg_convert_frame()) != NULL) {
    if (write(vhgr_file, buf, HGR_LEN) != HGR_LEN) {
      printf("Could not write data\n");
      close(vhgr_file);
      vhgr_file = -1;
      pthread_mutex_lock(&data->mutex);
      data->decoding_end = 1;
      data->decoding_ret = -1;
      pthread_mutex_unlock(&data->mutex);
      break;
    }
    frameno++;
    if (frameno % 100 == 0) {
      printf("%d frames decoded...\n", frameno);
    }
    pthread_mutex_lock(&data->mutex);
    if (frameno == 24 * 10) {
      data->data_ready = 1;
    }
    if (data->stop) {
      pthread_mutex_unlock(&data->mutex);
      break;
    } else {
      pthread_mutex_unlock(&data->mutex);
    }
  }

  pthread_mutex_lock(&data->mutex);
  data->decoding_end = 1;
  data->decoding_ret = 0;
  pthread_mutex_unlock(&data->mutex);

  close(vhgr_file);

  ffmpeg_to_hgr_deinit();
  if (frameno == 0) {
    return NULL;
  }
  return NULL;
}

static void enqueue_byte(unsigned char b) {
  bytes_sent++;
  changes_buffer[changes_num++] = b;
}

static void flush_changes(void) {
  simple_serial_write_fast(changes_buffer, changes_num);
  changes_num = 0;
}

static int last_sent_base = -1;
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
  last_sent_base = b;
  base_bytes++;
}

int last_sent_offset = -1;
static void send_offset(unsigned char o) {
  DEBUG("offset %d (should be written at %x)\n", o, 0x2000+(cur_base*MAX_OFFSET)+offset);
  if ((o| 0x80) == 0xFF) {
    printf("Error! Should not!\n");
    exit(1);
  }
  enqueue_byte(o|0x80);

  last_sent_offset = o;
  offset_bytes++;
}
static void send_num_reps(unsigned char b) {
  DEBUG("  => %d * ", b);
  enqueue_byte(0xFF);
  if ((b & 0x80) != 0) {
    printf("Error! Should not!\n");
    exit(1);
  }
  enqueue_byte(b);

  data_bytes += 2;
}
static void send_byte(unsigned char b) {
  DEBUG("  => %d\n", b);
  if ((b & 0x80) != 0) {
    printf("Error! Should not!\n");
    exit(1);
  }
  enqueue_byte(b);

  data_bytes++;
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
static int sample_rate = 115200 / (1+8+1);

#define BUFFER_LEN    (60*10)
#define SAMPLE_OFFSET 0x40
#define MAX_LEVEL       32
#define END_OF_STREAM   (MAX_LEVEL+1)

extern FILE *ttyfp;

#define send_sample(i) fputc((i) + SAMPLE_OFFSET, ttyfp)

static void send_end_of_stream(void) {
  send_sample(END_OF_STREAM);
  fflush(ttyfp);
}

void *ffmpeg_decode(void *arg) {
  decode_data *th_data = (decode_data *)arg;

  ffmpeg_to_raw_snd(th_data);
  return (void *)0;
}

#define IO_BARRIER(msg) do {                            \
    int r;                                              \
    printf("IO Barrier (%s)\n", msg);                   \
    do {                                                \
      r = simple_serial_getc();                         \
    } while (r != SURL_CLIENT_READY                     \
          && r != SURL_METHOD_ABORT);                   \
} while (0)

static void send_metadata(char *key, char *value, char *translit) {
  char *buf, *translit_buf;
  size_t len, l;
  if (value == NULL) {
    return;
  }
  translit_buf = do_charset_convert(value, OUTGOING, translit, 0, &l);
  len = strlen(key) + strlen("\n") + l;
  buf = malloc(len + 1);
  sprintf(buf, "%s\n%s", key, translit_buf);

  simple_serial_putc(SURL_ANSWER_STREAM_METADATA);
  IO_BARRIER("metadata len");
  len = htons(len);
  simple_serial_write_fast((char *)&len, 2);
  IO_BARRIER("metadata");
  simple_serial_puts(buf);
  IO_BARRIER("metadata done");

  free(translit_buf);
  free(buf);
}

int surl_stream_audio(char *url, char *translit, char monochrome, enum HeightScale scale) {
  int num = 0;
  unsigned char c;
  int max = 0;
  size_t cur = 0;
  unsigned char *data = NULL;
  unsigned char *img_data = NULL;
  unsigned char *hgr_buf = NULL;
  size_t size = 0;
  size_t img_size = 0;
  int ret = 0;

  pthread_t decode_thread;
  decode_data *th_data = malloc(sizeof(decode_data));
  int ready = 0;
  int stop = 0;

  memset(th_data, 0, sizeof(decode_data));
  th_data->url = url;
  th_data->sample_rate = sample_rate;
  pthread_mutex_init(&th_data->mutex, NULL);

  printf("Starting decode thread (charset %s, monochrome %d, scale %d)\n", translit, monochrome, scale);
  pthread_create(&decode_thread, NULL, *ffmpeg_decode, (void *)th_data);

  while(!ready && !stop) {
    pthread_mutex_lock(&th_data->mutex);
    ready = th_data->data_ready;
    stop = th_data->decoding_end;
    pthread_mutex_unlock(&th_data->mutex);
    usleep(100);
  }

  printf("Decode thread state: %s\n", ready ? "ready" : stop ? "failure" : "unknown");

  if (!ready && stop) {
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    return -1;
  } else {
    pthread_mutex_lock(&th_data->mutex);
    img_data = th_data->img_data;
    img_size = th_data->img_size;
    pthread_mutex_unlock(&th_data->mutex);

    if (img_data && img_size) {
      FILE *fp = fopen("/tmp/imgdata", "w+b");
      if (fp) {
        if (fwrite(img_data, 1, img_size, fp) == img_size) {
          fclose(fp);
          hgr_buf = sdl_to_hgr("/tmp/imgdata", monochrome, 0, &img_size, 0, scale);
          if (img_size != HGR_LEN) {
            hgr_buf = NULL;
          }
        } else {
          fclose(fp);
        }
      }
    }

    pthread_mutex_lock(&th_data->mutex);
    send_metadata("artist", th_data->artist, translit);
    send_metadata("album", th_data->album, translit);
    send_metadata("title", th_data->title, translit);
    send_metadata("track", th_data->track, translit);
    pthread_mutex_unlock(&th_data->mutex);

    printf("Client ready\n");
    if (hgr_buf) {
      simple_serial_putc(SURL_ANSWER_STREAM_ART);
      if (simple_serial_getc() == SURL_CLIENT_READY) {
        printf("Sending image\n");
        simple_serial_write_fast((char *)hgr_buf, img_size);
        if (simple_serial_getc() != SURL_CLIENT_READY) {
          return -1;
        }
      } else {
        printf("Skip image sending\n");
      }
    }

    simple_serial_putc(SURL_ANSWER_STREAM_START);
    if (simple_serial_getc() != SURL_CLIENT_READY) {
      pthread_mutex_lock(&th_data->mutex);
      th_data->stop = 1;
      pthread_mutex_unlock(&th_data->mutex);
      ret = -1;
      goto cleanup_thread;
    }
  }

  max = 256;
  sleep(1); /* Let ffmpeg have a bit of time to push data so we don't starve */

  while (1) {
    pthread_mutex_lock(&th_data->mutex);
    if (cur > sample_rate*(2*BUFFER_LEN)) {
      /* Avoid ever-expanding buffer */
      memmove(th_data->data, th_data->data + sample_rate*BUFFER_LEN, sample_rate*BUFFER_LEN);
      th_data->data = realloc(th_data->data, th_data->size - sample_rate*BUFFER_LEN);
      th_data->size -= sample_rate*BUFFER_LEN;
      cur -= sample_rate*BUFFER_LEN;
    }
    data = th_data->data;
    size = th_data->size;
    stop = th_data->decoding_end;
    pthread_mutex_unlock(&th_data->mutex);

    if (cur == size) {
      if (stop) {
        printf("Stopping at %zu/%zu\n", cur, size);
        break;
      } else {
        /* We're starved but not done :-( */
        continue;
      }
    }
    send_sample(data[cur] * MAX_LEVEL/max);
    cur++;

    /* Kbd input polled directly for no wait at all */
    {
      struct pollfd fds[1];
      fds[0].fd = fileno(ttyfp);
      fds[0].events = POLLIN;
      if (poll(fds, 1, 0) > 0 && (fds[0].revents & POLLIN)) {
        c = simple_serial_getc();
        switch (c) {
          case ' ':
            printf("Pause\n");
            send_sample(MAX_LEVEL/2);
            simple_serial_getc();
            break;
          case APPLE_CH_CURS_LEFT:
            printf("Rewind\n");
            if (cur < sample_rate * 10) {
              cur = 0;
            } else {
              cur -= sample_rate * 10;
            }
            break;
          case APPLE_CH_CURS_RIGHT:
            if (cur + sample_rate * 10 < size) {
              cur += sample_rate * 10;
            } else {
              cur = size;
            }
            printf("Forward\n");
            break;
          case CH_ESC:
            printf("Stop\n");
            goto done;
          case SURL_METHOD_ABORT:
            printf("Connection reset\n");
            goto cleanup_thread;
        }
      }
    }

    num++;
  }

done:
  send_sample(MAX_LEVEL/2);
  send_end_of_stream();

  do {
    c = simple_serial_getc();
    printf("ignoring %02X\n", c);
  } while (c != SURL_CLIENT_READY
        && c != SURL_METHOD_ABORT);

cleanup_thread:
  printf("Cleaning up decoder thread\n");
  pthread_mutex_lock(&th_data->mutex);
  th_data->stop = 1;
  pthread_mutex_unlock(&th_data->mutex);
  pthread_join(decode_thread, NULL);
  free(th_data->img_data);
  free(th_data->data);
  free(th_data->artist);
  free(th_data->album);
  free(th_data->title);
  free(th_data->track);
  free(th_data);
  printf("Done\n");

  return ret;
}

int surl_stream_video(char *url) {
  int i, j;
  int last_diff;
  int last_val, ident_vals;
  int total = 0, min = 0xFFFF, max = 0;
  size_t r;
  unsigned char buf_prev[2][HGR_LEN], buf[2][HGR_LEN];
  struct timeval frame_start, frame_end;
  int skipped = 0, skip_next = 0, duration;
  int command, page = 0;
  int num_diffs = 0;
  int vhgr_file;

  pthread_t decode_thread;
  decode_data *th_data = malloc(sizeof(decode_data));
  int ready = 0;
  int stop = 0;
  int err = 0;
  memset(th_data, 0, sizeof(decode_data));
  th_data->url = url;
  pthread_mutex_init(&th_data->mutex, NULL);

  printf("Starting video decode thread\n");
  pthread_create(&decode_thread, NULL, *generate_frames, (void *)th_data);

  simple_serial_putc(SURL_ANSWER_STREAM_LOAD);

  while(!ready && !stop) {
    pthread_mutex_lock(&th_data->mutex);
    ready = th_data->data_ready;
    stop = th_data->decoding_end;
    err = th_data->decoding_ret;
    pthread_mutex_unlock(&th_data->mutex);
    usleep(100);
  }

  if (stop && err) {
    printf("Error generating frames\n");
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);

    pthread_mutex_lock(&th_data->mutex);
    th_data->stop = 1;
    pthread_mutex_unlock(&th_data->mutex);
    pthread_join(decode_thread, NULL);

    return -1;
  }

  if (diffs == NULL) {
    diffs = malloc(HGR_LEN * sizeof(byte_diff *));
    for (j = 0; j < HGR_LEN; j++) {
      diffs[j] = malloc(sizeof(byte_diff));
    }
  }

  vhgr_file = open(tmp_filename, O_RDONLY);

  unlink(tmp_filename);
  if (vhgr_file == -1) {
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);

    pthread_mutex_lock(&th_data->mutex);
    th_data->stop = 1;
    pthread_mutex_unlock(&th_data->mutex);
    pthread_join(decode_thread, NULL);

    return -1;
  }

  simple_serial_putc(SURL_ANSWER_STREAM_READY);
  if (simple_serial_getc() != SURL_CLIENT_READY) {
    printf("Client not ready\n");

    pthread_mutex_lock(&th_data->mutex);
    th_data->stop = 1;
    pthread_mutex_unlock(&th_data->mutex);
    pthread_join(decode_thread, NULL);

    return -1;
  }

  printf("Starting stream\n");
  simple_serial_putc(SURL_ANSWER_STREAM_START);

  memset(changes_buffer, 0, sizeof changes_buffer);

  offset = cur_base = 0;
  send_offset(0);
  send_base(0);

  i = 1;
  memset(buf_prev[0], 0, HGR_LEN);
  memset(buf_prev[1], 0, HGR_LEN);

  gettimeofday(&frame_start, 0);

  page = 1;

next_file:
  i++;
  page = !page;

  if ((r = read(vhgr_file, buf[page], HGR_LEN)) != HGR_LEN) {
    goto close_last;
  }

  /* count diffs */
  last_diff = 0;
  cur_base = 0;
  ident_vals = 0;

  /* Sync point - force a switch to base 0 */
  offset = cur_base = 0;
  send_offset(0);
  send_base(0);

  if (i > FPS && (i % (15*FPS)) == 0) {
    duration = i/FPS;
    printf("%d seconds, %d frames skipped / %d: %d fps\n", duration,
         skipped, i, (i-skipped)/duration);

  }

  DEBUG("sync point %d\n", i);
  command = simple_serial_getc_immediate();
  switch (command) {
    case CH_ESC:
    case SURL_METHOD_ABORT:
      printf("Abort, exiting.\n");
      goto close_last;
    case ' ':
      printf("Pause.\n");
      command = simple_serial_getc();
      if (command == SURL_METHOD_ABORT)
        goto close_last;
      printf("Play.\n");
      lateness = 0;
      skip_next = 0;
      break;
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
#endif
    goto next_file;
  }

  gettimeofday(&frame_start, 0);

  for (num_diffs = 0, j = 0; j < HGR_LEN; j++) {
    if (buf_prev[page][j] != buf[page][j]) {
      diffs[num_diffs]->offset = j;
      diffs[num_diffs]->changed = diff_score(buf_prev[page][j], buf[page][j]);
      num_diffs++;
    }
  }

  last_val = -1;
  for (j = 0; j < num_diffs && j < MAX_DIFFS_PER_FRAME; j++) {
    int pixel = diffs[j]->offset;

    offset = pixel - (cur_base*MAX_OFFSET);
    /* If there's no hole in updated bytes, we can let offset
     * increment up to 255 */
    if ((offset >= MAX_OFFSET && pixel != last_diff+1)
      || offset > 255) {
      /* must flush ident */
      flush_ident(ident_vals, last_val);
      ident_vals = 0;

      /* we have to update base */
      cur_base = pixel / MAX_OFFSET;
      offset = pixel - (cur_base*MAX_OFFSET);

      if (offset < last_sent_offset && cur_base == last_sent_base + 1) {
        DEBUG("skip sending base (offset %d => %d, base %d => %d)\n",
                last_sent_offset, offset, last_sent_base, cur_base);
        send_offset(offset);
        last_sent_base = cur_base;
      } else {
        DEBUG("must send base (offset %d => %d, base %d => %d)\n",
                last_sent_offset, offset, last_sent_base, cur_base);
        send_offset(offset);
        send_base(cur_base);
      }
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

  goto next_file;

close_last:
  send_offset(0);
  send_base(NUM_BASES+2); /* Done */
  flush_changes();

  /* Get rid of possible last ack */
  simple_serial_flush();

  close(vhgr_file);

  pthread_mutex_lock(&th_data->mutex);
  th_data->stop = 1;
  pthread_mutex_unlock(&th_data->mutex);
  pthread_join(decode_thread, NULL);

  if (i - skipped > 0) {
    printf("Max: %d, Min: %d, Average: %d\n", max, min, total / (i-skipped));
    printf("Sent %lu bytes for %d non-skipped frames: %lu avg (%lu data, %lu offset, %lu base)\n",
            bytes_sent, (i-skipped), bytes_sent/(i-skipped),
            data_bytes/(i-skipped), offset_bytes/(i-skipped), base_bytes/(i-skipped));
  }
  if (i - skipped > FPS) {
    duration = i/FPS;
    printf("%d seconds, %d frames skipped / %d: %d fps\n", duration,
          skipped, i, (i-skipped)/duration);
  }
  return 0;
}
