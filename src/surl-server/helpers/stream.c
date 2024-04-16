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
#include "extended_conio.h"

#define MAX_OFFSET 126
#define NUM_BASES  (HGR_LEN/MAX_OFFSET)+1

#define MIN_REPS   3
#define MAX_REPS   10

/* Set very high because it looks nicer to drop frames than to
 * artifact all the way
 */
#define MAX_BYTES_PER_FRAME 900
#define ACCEPT_ARTEFACT 0

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

#define PREDECODE_SECS 10

static void *generate_frames(void *th_data) {
  decode_data *data = (decode_data *)th_data;
  unsigned char *buf;
  int frameno = 0;
  int vhgr_file;
  struct timeval decode_start, cur_time;
  unsigned long secs;
  unsigned long microsecs;
  unsigned long elapsed;
  int decode_slow = 0;

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

  if (ffmpeg_to_hgr_init(data->url, &video_len, data->subtitles) != 0) {
    printf("Could not init ffmpeg.\n");
    pthread_mutex_lock(&data->mutex);
    data->decoding_end = 1;
    data->decoding_ret = -1;
    pthread_mutex_unlock(&data->mutex);
    return NULL;
  }

  pthread_mutex_lock(&data->mutex);

  if (data->decoding_end == 1) {
    pthread_mutex_unlock(&data->mutex);
    goto out;
  }
  pthread_mutex_unlock(&data->mutex);

  gettimeofday(&decode_start, 0);
  while ((buf = ffmpeg_convert_frame(video_len*FPS, frameno)) != NULL) {
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
    if (frameno == FPS * PREDECODE_SECS) {
      gettimeofday(&cur_time, 0);
      secs      = (cur_time.tv_sec - decode_start.tv_sec)*1000000;
      microsecs = cur_time.tv_usec - decode_start.tv_usec;
      elapsed   = secs + microsecs;
      printf("Decoded %d seconds in %luµs\n", PREDECODE_SECS, elapsed);
      if (elapsed / 1000000 > PREDECODE_SECS / 3) {
        printf("decoding too slow, not starting early\n");
        decode_slow = 1;
      } else {
        data->data_ready = 1;
      }
    } else if (decode_slow) {
      unsigned long usecs_per_frame, remaining_frames, done_len, remaining_len;
      /* We can start streaming as soon as what is remaining should
       * take less than the time we already spent decoding */
      gettimeofday(&cur_time, 0);
      secs      = (cur_time.tv_sec - decode_start.tv_sec)*1000000;
      microsecs = cur_time.tv_usec - decode_start.tv_usec;
      elapsed   = secs + microsecs;

      usecs_per_frame = elapsed / frameno;
      remaining_frames = video_len*FPS - frameno;
      remaining_len = remaining_frames * usecs_per_frame;
      done_len = frameno / FPS;

      if (frameno % 100 == 0) {
        printf("decoded %d frames (%lus) in %lus, remaining %lu frames, should take %lus\n",
              frameno, done_len, elapsed/1000000, remaining_frames, remaining_len/1000000);
      }
      if ((remaining_len/1000000)/2 < done_len) {
        data->data_ready = 1;
        decode_slow = 0;
      } else {
        /* Send ping every 10% (and only once for every 25 frames matching the modulo)*/
        if ((frameno/FPS) % (video_len / 10) == 0
          && ((frameno/FPS)/(video_len / 10))*((video_len / 10)*FPS) == frameno) {
          int r;
          printf("Frame %d (%ds/%ds), %d\n", frameno, frameno/FPS, video_len,  (video_len / 10));
          simple_serial_putc(SURL_ANSWER_STREAM_LOAD);
          r = simple_serial_getc_with_timeout();
          if (r != SURL_CLIENT_READY) {
            printf("Client abort\n");
            data->decoding_end = 1;
            data->decoding_ret = -1;
            goto out;
          }
        }
      }
    }

    if (data->stop) {
      pthread_mutex_unlock(&data->mutex);
      break;
    } else {
      pthread_mutex_unlock(&data->mutex);
    }
  }

  pthread_mutex_lock(&data->mutex);
  data->data_ready = 1;
  data->decoding_end = 1;
  data->decoding_ret = 0;
out:
  pthread_mutex_unlock(&data->mutex);

  close(vhgr_file);

  ffmpeg_to_hgr_deinit();
  return NULL;
}

extern FILE *ttyfp;
extern char *aux_tty_path;
FILE *ttyfp2 = NULL;

static void flush_changes(FILE *fp) {
  if (changes_num == 0) {
    return;
  }
  simple_serial_write_fast_fp(fp, changes_buffer, changes_num);
  changes_num = 0;
}

static void enqueue_byte(unsigned char b, FILE *fp) {
  if (!fp) {
    return;
  }
  bytes_sent++;
  /* Video only ? */
  if (1) {
    changes_buffer[changes_num++] = b;
    return;
  }
  /* Otherwise don't buffer */
  fputc(b, fp);
  //fflush(fp);
}

static int last_sent_base = -1;

static void send_base(unsigned char b, FILE *fp) {
  DEBUG(" new base %d\n", b);
  DEBUG(" base %d offset %d (should be written at %x)\n", b, offset, 0x2000+(cur_base*MAX_OFFSET)+offset);
  if ((b| 0x80) == 0xFF) {
    printf("Base error! Should not!\n");
    exit(1);
  }
  enqueue_byte(b|0x80, fp);
  last_sent_base = b;
  base_bytes++;
}

int last_sent_offset = -1;
static void send_offset(unsigned char o, FILE *fp) {
  DEBUG("offset %d (should be written at %x)\n", o, 0x2000+(cur_base*MAX_OFFSET)+offset);
  if ((o| 0x80) == 0xFF) {
    printf("Offset error! Should not!\n");
    exit(1);
  }
  enqueue_byte(o|0x80, fp);

  last_sent_offset = o;
  offset_bytes++;
}
static void send_num_reps(unsigned char b, FILE *fp) {
  DEBUG("  => %d * ", b);
  enqueue_byte(0xFF, fp);
  if ((b & 0x80) != 0) {
    printf("Reps error! Should not!\n");
    exit(1);
  }
  enqueue_byte(b, fp);

  data_bytes += 2;
}
static void send_byte(unsigned char b, FILE *fp) {
  DEBUG("  => %d\n", b);
  if ((b & 0x80) != 0) {
    printf("Byte error! Should not!\n");
    exit(1);
  }
  enqueue_byte(b, fp);

  data_bytes++;
}

static void flush_ident(int min_reps, int ident_vals, int last_val, FILE *fp) {
  if (ident_vals > min_reps) {
    send_num_reps(ident_vals, fp);
    send_byte(last_val, fp);
  } else {
    while (ident_vals > 0) {
      send_byte(last_val, fp);
      ident_vals--;
    }
  }
}

static long lateness = 0;

/* Returns 1 if client is late and we should skip frame */
static int sync_duration = 0;

#define UPDATE_SYNC_DURATION() do {                           \
  gettimeofday(&sync_end, 0);                                 \
  secs      = (sync_end.tv_sec - sync_start.tv_sec)*1000000;  \
  microsecs = sync_end.tv_usec - sync_start.tv_usec;          \
  elapsed   = secs + microsecs;                               \
  if (wait > 0) {                                             \
    elapsed -= wait;                                          \
  }                                                           \
  sync_duration = elapsed;                                    \
} while (0)

static inline int sync_fps(struct timeval *start) {
  struct timeval frame_end, sync_start, sync_end;
  gettimeofday(&sync_start, 0);
  
  gettimeofday(&frame_end, 0);

  unsigned long secs      = (frame_end.tv_sec - start->tv_sec)*1000000;
  unsigned long microsecs = frame_end.tv_usec - start->tv_usec;
  unsigned long elapsed   = secs + microsecs;
  long wait      = 1000000/FPS;

  wait = wait-elapsed-sync_duration;
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
    UPDATE_SYNC_DURATION();
    return 1;
  } else {
    UPDATE_SYNC_DURATION();
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

  return score;
}

#if ACCEPT_ARTEFACT
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

#define AV_SAMPLE_OFFSET 0x60
#define AV_MAX_LEVEL       31
#define AV_END_OF_STREAM   (AV_MAX_LEVEL+1)

#define send_sample(i) fputc((i) + SAMPLE_OFFSET, ttyfp)
#define send_av_sample(i) fputc((i) + AV_SAMPLE_OFFSET, ttyfp)

static void send_end_of_stream(void) {
  send_sample(END_OF_STREAM);
  fflush(ttyfp);
}

static void send_end_of_av_stream(void) {
  send_av_sample(AV_END_OF_STREAM);
  fflush(ttyfp);
}

void *ffmpeg_decode_snd(void *arg) {
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
  printf("=> %s %s\n", key, value);
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
  pthread_create(&decode_thread, NULL, *ffmpeg_decode_snd, (void *)th_data);

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
    send_metadata("has_video", th_data->has_video ? "1":"0", translit);
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
      printf("stop\n");
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
    printf("got %02X\n", c);
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
  signed int last_val;
  int ident_vals;
  int total = 0, min = 0xFFFF, max = 0;
  size_t r;
  unsigned char buf_prev[2][HGR_LEN], buf[2][HGR_LEN];
  struct timeval frame_start;
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

    goto cleanup;
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
    printf("Error opening %s\n", tmp_filename);
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    goto cleanup;
  }

  simple_serial_putc(SURL_ANSWER_STREAM_READY);

  if (simple_serial_getc() != SURL_CLIENT_READY) {
    printf("Client not ready\n");
    goto cleanup;
  }

  printf("Starting stream\n");
  simple_serial_putc(SURL_ANSWER_STREAM_START);
  printf("sent\n");

  memset(changes_buffer, 0, sizeof changes_buffer);

  offset = cur_base = 0;
  send_offset(0, ttyfp);
  send_base(0, ttyfp);
  flush_changes(ttyfp);
  i = 1;
  memset(buf_prev[0], 0, HGR_LEN);
  memset(buf_prev[1], 0, HGR_LEN);

  page = 1;

  gettimeofday(&frame_start, 0);
next_file:
  i++;
  page = !page;

  if (sync_fps(&frame_start) || skip_next) {
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

  if ((r = read(vhgr_file, buf[page], HGR_LEN)) != HGR_LEN) {
    goto close_last;
  }

  /* count diffs */
  last_diff = 0;
  cur_base = 0;
  ident_vals = 0;

  /* Sync point - force a switch to base 0 */
  offset = cur_base = 0;
  send_offset(0, ttyfp);
  send_base(0, ttyfp);
  flush_changes(ttyfp);

  if (i > FPS && (i % (15*FPS)) == 0) {
    duration = i/FPS;
    printf("%d seconds, %d frames skipped / %d: %.2f fps\n", duration,
         skipped, i, (float)(i-skipped)/duration);

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

  for (num_diffs = 0, j = 0; j < HGR_LEN; j++) {
    if (buf_prev[page][j] != buf[page][j]) {
      diffs[num_diffs]->offset = j;
      diffs[num_diffs]->changed = diff_score(buf_prev[page][j], buf[page][j]);
      num_diffs++;
    }
  }

  last_val = -1;
  for (j = 0; j < num_diffs; j++) {
    int pixel = diffs[j]->offset;

    offset = pixel - (cur_base*MAX_OFFSET);
    /* If there's no hole in updated bytes, we can let offset
     * increment up to 255 */
    if ((offset >= MAX_OFFSET && pixel != last_diff+1)
      || offset > 255) {
      /* must flush ident */
      flush_ident(MIN_REPS, ident_vals, last_val, ttyfp);
      ident_vals = 0;

      /* we have to update base */
      cur_base = pixel / MAX_OFFSET;
      offset = pixel - (cur_base*MAX_OFFSET);

      if (offset < last_sent_offset && cur_base == last_sent_base + 1) {
        DEBUG("skip sending base (offset %d => %d, base %d => %d)\n",
                last_sent_offset, offset, last_sent_base, cur_base);
        send_offset(offset, ttyfp);
        last_sent_base = cur_base;
      } else {
        DEBUG("must send base (offset %d => %d, base %d => %d)\n",
                last_sent_offset, offset, last_sent_base, cur_base);
        send_offset(offset, ttyfp);
        send_base(cur_base, ttyfp);
      }
    } else if (pixel != last_diff+1) {
      /* must flush ident */
      flush_ident(MIN_REPS, ident_vals, last_val, ttyfp);
      ident_vals = 0;
      /* We have to send offset */
      send_offset(offset, ttyfp);
    }
    if (last_val == -1 ||
       (ident_vals < MAX_REPS && buf[page][pixel] == last_val && pixel == last_diff+1)) {
      ident_vals++;
    } else {
      flush_ident(MIN_REPS, ident_vals, last_val, ttyfp);
      ident_vals = 1;
    }
    last_val = buf[page][pixel];

    last_diff = pixel;

    /* Note diff done */
    buf_prev[page][pixel] = buf[page][pixel];
  }
  flush_ident(MIN_REPS, ident_vals, last_val, ttyfp);
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
  send_offset(0, ttyfp);
  send_base(NUM_BASES+2, ttyfp); /* Done */
  flush_changes(ttyfp);

  /* Get rid of possible last ack */
  simple_serial_flush();


  if (i - skipped > 0) {
    printf("Max: %d, Min: %d, Average: %d\n", max, min, total / (i-skipped));
    printf("Sent %lu bytes for %d non-skipped frames: %lu avg (%lu data, %lu offset, %lu base)\n",
            bytes_sent, (i-skipped), bytes_sent/(i-skipped),
            data_bytes/(i-skipped), offset_bytes/(i-skipped), base_bytes/(i-skipped));
  }
  if (i - skipped > FPS) {
    duration = i/FPS;
    printf("%d seconds, %d frames skipped / %d: %.2f fps\n", duration,
          skipped, i, (float)(i-skipped)/duration);
  }

cleanup:
  if (vhgr_file != -1) {
    close(vhgr_file);
  }
  if (th_data) {
    pthread_mutex_lock(&th_data->mutex);
    th_data->stop = 1;
    printf("cleanup\n");
    pthread_mutex_unlock(&th_data->mutex);
    pthread_join(decode_thread, NULL);
    free(th_data);
  }
  return 0;
}

decode_data *audio_th_data;
decode_data *video_th_data;
int ready;
int stop;
int err;

unsigned char *audio_data = NULL;
unsigned char *img_data = NULL;
unsigned char *hgr_buf = NULL;
int audio_max = 0;
size_t audio_size = 0;
size_t img_size = 0;
int audio_ready = 0;

static void *audio_push(void *unused) {
  /* Audio vars */
  int num = 0;
  unsigned char c;
  size_t cur = 0;
  int stop;

  while (1) {
    pthread_mutex_lock(&audio_th_data->mutex);
    if (cur > sample_rate*(2*BUFFER_LEN)) {
      /* Avoid ever-expanding buffer */
      memmove(audio_th_data->data, audio_th_data->data + sample_rate*BUFFER_LEN, sample_rate*BUFFER_LEN);
      audio_th_data->data = realloc(audio_th_data->data, audio_th_data->size - sample_rate*BUFFER_LEN);
      audio_th_data->size -= sample_rate*BUFFER_LEN;
      cur -= sample_rate*BUFFER_LEN;
    }
    audio_data = audio_th_data->data;
    audio_size = audio_th_data->size;
    stop = audio_th_data->decoding_end;
    pthread_mutex_unlock(&audio_th_data->mutex);

    if (cur == audio_size) {
      if (stop) {
        printf("Stopping at %zu/%zu\n", cur, audio_size);
        return NULL; /* Don't set stop, so video can finish */
      } else {
        /* We're starved but not done :-( */
        continue;
      }
    }

    /* FIXME I should be able to sem_post */
    send_av_sample(audio_data[cur] * AV_MAX_LEVEL/audio_max);
    fflush(ttyfp);
    cur++;

    /* Kbd input polled directly for no wait at all */
    {
      struct pollfd fds[1];
      fds[0].fd = fileno(ttyfp);
      fds[0].events = POLLIN;
      if (poll(fds, 1, 0) > 0 && (fds[0].revents & POLLIN)) {
        c = simple_serial_getc();
        switch (c) {
          case CH_ESC:
            printf("Stop\n");
            goto abort;
          case SURL_METHOD_ABORT:
            printf("Connection reset\n");
            goto abort;
        }
      }
    }
    num++;
  }
  return NULL;
abort:
  pthread_mutex_lock(&audio_th_data->mutex);
  audio_th_data->stop = 1;
  printf("abort\n");
  pthread_mutex_unlock(&audio_th_data->mutex);
  return NULL;
}

int vhgr_file;

void *video_push(void *unused) {
  int i, j;
  int last_diff;
  int total = 0, min = 0xFFFF, max = 0;
  size_t r;
  unsigned char buf_prev[2][HGR_LEN], buf[2][HGR_LEN];
  struct timeval frame_start;
  int skipped = 0, skip_next = 0, duration;
  int page = 0;
  int num_diffs = 0;

  i = 0;
  page = 1;
  memset(buf_prev[0], 0x00, HGR_LEN);
  memset(buf_prev[1], 0x00, HGR_LEN);
  /* Send 30 full-black bytes first to make sure everything
  * started client-side (but don't change the first one) */
  memset(buf_prev[1] + 1, 0x7F, 30);

  memset(buf[page], 0x00, HGR_LEN);
  goto send;

next_file:
  i++;
  page = !page;

  if ((r = read(vhgr_file, buf[page], HGR_LEN)) != HGR_LEN) {
    printf("Starved!\n");
    goto close_last;
  }

  /* count diffs */
  last_diff = 0;
  cur_base = 0;

  /* Sync point - force a switch to base 0 */
  offset = cur_base = 0;
  send_offset(0, ttyfp2);
  send_base(0, ttyfp2);
  enqueue_byte(0xFF, ttyfp2); /* Switch page */
  flush_changes(ttyfp2);

  /* FIXME I should be able to sync with sem_wait() if it weren't
   * for unpredicable buffering */
  if (sync_fps(&frame_start) || skip_next) {
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

  if (i > FPS && (i % (15*FPS)) == 0) {
    duration = i/FPS;
    printf("%d seconds, %d frames skipped / %d: %.2f fps\n", duration,
         skipped, i, (float)(i-skipped)/duration);

  }

send:
  gettimeofday(&frame_start, 0);

  for (num_diffs = 0, j = 0; j < HGR_LEN; j++) {
    if (buf_prev[page][j] != buf[page][j]) {
      diffs[num_diffs]->offset = j;
      diffs[num_diffs]->changed = diff_score(buf_prev[page][j], buf[page][j]);
      num_diffs++;
    }
  }

#if ACCEPT_ARTEFACT
  /* Sort by diff */
  bubble_sort_array((void **)diffs, num_diffs, (sort_func)sort_by_score);

  /* Keep every diff with 4 or more pixels changed */
  for (j = 0; j < num_diffs; j++) {
    if (diffs[j]->changed < 4 && j > MAX_BYTES_PER_FRAME)
      break;
  }
  if (j < num_diffs)
    num_diffs = j;

  /* Sort the first ones by offset */
  bubble_sort_array((void **)diffs, num_diffs,
          (sort_func)sort_by_offset);
#endif

  for (j = 0; j < num_diffs; j++) {
    int pixel = diffs[j]->offset;
    int vidstop;

    pthread_mutex_lock(&video_th_data->mutex);
    vidstop = video_th_data->stop;
    pthread_mutex_unlock(&video_th_data->mutex);
    if (vidstop) {
      printf("Video thread stopping\n");
      goto close_last;
    }

    offset = pixel - (cur_base*MAX_OFFSET);
    /* If there's no hole in updated bytes, we can let offset
     * increment up to 255 */
    if ((offset >= MAX_OFFSET && pixel != last_diff+1)
      || offset > 255) {
      /* we have to update base */
      cur_base = pixel / MAX_OFFSET;
      offset = pixel - (cur_base*MAX_OFFSET);

      DEBUG("send base (offset %d => %d, base %d => %d)\n",
              last_sent_offset, offset, last_sent_base, cur_base);
      send_offset(offset, ttyfp2);
      send_base(cur_base, ttyfp2);
    } else if (pixel != last_diff+1) {
      /* We have to send offset */
      send_offset(offset, ttyfp2);
    }
    send_byte(buf[page][pixel], ttyfp2);

    last_diff = pixel;

    /* Note diff done */
    buf_prev[page][pixel] = buf[page][pixel];
  }

  total += num_diffs;
  if (num_diffs < min) {
    min = num_diffs;
  }
  if (num_diffs > max) {
    max = num_diffs;
  }

  goto next_file;

close_last:
  flush_changes(ttyfp2);
  if (i - skipped > 0) {
    printf("Max: %d, Min: %d, Average: %d\n", max, min, total / (i-skipped));
    printf("Sent %lu bytes for %d non-skipped frames: %lu avg (%lu data, %lu offset, %lu base)\n",
            bytes_sent, (i-skipped), bytes_sent/(i-skipped),
            data_bytes/(i-skipped), offset_bytes/(i-skipped), base_bytes/(i-skipped));
  }
  if (i - skipped > FPS) {
    duration = i/FPS;
    printf("%d seconds, %d frames skipped / %d: %.2f fps\n", duration,
          skipped, i, (float)(i-skipped)/duration);
  }
  printf("avg sync duration %d\n", sync_duration);

  pthread_mutex_lock(&video_th_data->mutex);
  video_th_data->stop = 1;
  printf("vstop\n");
  pthread_mutex_unlock(&video_th_data->mutex);

  return NULL;
}

int surl_stream_audio_video(char *url, char *translit, char monochrome, enum HeightScale scale, char subtitles) {
  int j;
  int cancelled = 0, playback_stop = 0;
  /* Control vars */
  unsigned char c;
  int ret = 0;
  pthread_t audio_decode_thread, video_decode_thread;
  pthread_t audio_push_thread, video_push_thread;
  audio_th_data = malloc(sizeof(decode_data));
  video_th_data = malloc(sizeof(decode_data));
  ready = 0;
  stop = 0;
  err = 0;

  if (aux_tty_path)
    ttyfp2 = simple_serial_open_file(aux_tty_path);

  if (ttyfp2 == NULL)
    printf("No TTY for video\n");

  memset(video_th_data, 0, sizeof(decode_data));
  video_th_data->url = url;
  video_th_data->subtitles = subtitles;
  pthread_mutex_init(&video_th_data->mutex, NULL);

  printf("Starting video decode thread\n");
  pthread_create(&video_decode_thread, NULL, *generate_frames, (void *)video_th_data);

  memset(audio_th_data, 0, sizeof(decode_data));
  audio_th_data->url = url;
  audio_th_data->sample_rate = sample_rate;
  pthread_mutex_init(&audio_th_data->mutex, NULL);

  printf("Starting decode thread (charset %s, monochrome %d, scale %d)\n", translit, monochrome, scale);
  pthread_create(&audio_decode_thread, NULL, *ffmpeg_decode_snd, (void *)audio_th_data);

  while(!ready && !stop && err != -1) {
    pthread_mutex_lock(&audio_th_data->mutex);
    pthread_mutex_lock(&video_th_data->mutex);
    ready = audio_th_data->data_ready && video_th_data->data_ready;
    stop = audio_th_data->decoding_end && video_th_data->decoding_end;
    err = video_th_data->decoding_ret;
    pthread_mutex_unlock(&audio_th_data->mutex);
    pthread_mutex_unlock(&video_th_data->mutex);
    usleep(100);
  }

  printf("Decode thread state: %s\n", ready ? "ready" : stop ? "failure" : "unknown");

  if (!ready && (stop || err)) {
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    goto cleanup_thread;
  }

  pthread_mutex_lock(&audio_th_data->mutex);
  img_data = audio_th_data->img_data;
  img_size = audio_th_data->img_size;
  pthread_mutex_unlock(&audio_th_data->mutex);

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

  pthread_mutex_lock(&audio_th_data->mutex);
  send_metadata("has_video", audio_th_data->has_video ? "1":"0", translit);
  send_metadata("artist", audio_th_data->artist, translit);
  send_metadata("album", audio_th_data->album, translit);
  send_metadata("title", audio_th_data->title, translit);
  send_metadata("track", audio_th_data->track, translit);
  pthread_mutex_unlock(&audio_th_data->mutex);

  if (diffs == NULL) {
    diffs = malloc(HGR_LEN * sizeof(byte_diff *));
    for (j = 0; j < HGR_LEN; j++) {
      diffs[j] = malloc(sizeof(byte_diff));
    }
  }

  vhgr_file = open(tmp_filename, O_RDONLY);

  unlink(tmp_filename);
  if (vhgr_file == -1) {
    printf("Error opening %s\n", tmp_filename);
    // simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    // goto cleanup_thread;
  }

  memset(changes_buffer, 0, sizeof changes_buffer);

  offset = cur_base = 0;
  send_offset(0, ttyfp2);
  send_base(0, ttyfp2);

  printf("AV: Client ready\n");
  if (hgr_buf) {
    simple_serial_putc(SURL_ANSWER_STREAM_ART);
    if (simple_serial_getc() == SURL_CLIENT_READY) {
      printf("Sending image\n");
      simple_serial_write_fast((char *)hgr_buf, img_size);
      if (simple_serial_getc() != SURL_CLIENT_READY) {
        goto cleanup_thread;
      }
    } else {
      printf("Skip image sending\n");
    }
  }

  simple_serial_putc(SURL_ANSWER_STREAM_START);
  if (simple_serial_getc() != SURL_CLIENT_READY) {
    pthread_mutex_lock(&audio_th_data->mutex);
    audio_th_data->stop = 1;
    printf("audio stop\n");
    pthread_mutex_unlock(&audio_th_data->mutex);
    ret = -1;
    goto cleanup_thread;
  }

  audio_max = 256;
  sleep(1); /* Let ffmpeg have a bit of time to push data so we don't starve */

  audio_ready = 0;
  pthread_create(&audio_push_thread, NULL, *audio_push, NULL);
  pthread_create(&video_push_thread, NULL, *video_push, NULL);
  while (1) {
    pthread_mutex_lock(&audio_th_data->mutex);
    cancelled = audio_th_data->stop;
    pthread_mutex_unlock(&audio_th_data->mutex);
    if (cancelled) {
      printf("Audio thread cancelled\n");
      pthread_mutex_lock(&video_th_data->mutex);
      video_th_data->stop = 1;
      pthread_mutex_unlock(&video_th_data->mutex);
      pthread_join(audio_push_thread, NULL);
      pthread_join(video_push_thread, NULL);
      break;
    }

    pthread_mutex_lock(&video_th_data->mutex);
    playback_stop = video_th_data->stop;
    pthread_mutex_unlock(&video_th_data->mutex);
    if (playback_stop) {
      printf("Video playback stop\n");
      break;
    }
    sleep(1);
  }
  if (!cancelled) {
    pthread_join(audio_push_thread, NULL);
    pthread_join(video_push_thread, NULL);
  }

  fflush(ttyfp);
  if (ttyfp2)
    fflush(ttyfp2);

  printf("done (cancelled: %d)\n", cancelled);
  send_av_sample(AV_MAX_LEVEL/2);
  send_end_of_av_stream();

  do {
    c = simple_serial_getc();
    printf("got %02X\n", c);
  } while (c != SURL_CLIENT_READY
        && c != SURL_METHOD_ABORT);

cleanup_thread:
  printf("Cleaning up decoder thread\n");
  pthread_mutex_lock(&audio_th_data->mutex);
  audio_th_data->stop = 1;
  pthread_mutex_unlock(&audio_th_data->mutex);
  pthread_join(audio_decode_thread, NULL);
  pthread_join(video_decode_thread, NULL);
  free(audio_th_data->img_data);
  free(audio_th_data->data);
  free(audio_th_data->artist);
  free(audio_th_data->album);
  free(audio_th_data->title);
  free(audio_th_data->track);
  free(audio_th_data);
  free(video_th_data);
  if (vhgr_file != -1) {
    close(vhgr_file);
  }
  if (ttyfp2)
    fclose(ttyfp2);

  printf("Done\n");

  return ret;
}
