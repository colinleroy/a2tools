#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <semaphore.h>

#include "../surl_protocol.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "ffmpeg.h"
#include "array_sort.h"
#include "hgr-convert.h"
#include "char-convert.h"
#include "extended_conio.h"
#include "strsplit.h"
#include "printer.h"
#include "../log.h"

#define MAX_VIDEO_OFFSET 126
#define NUM_VIDEO_BASES  (HGR_LEN/MAX_VIDEO_OFFSET)+1
#define MIN_VIDEO_REPS   3
#define MAX_VIDEO_REPS   10

#define MAX_AV_OFFSET    126
#define NUM_AV_BASES     (HGR_LEN/MAX_AV_OFFSET)+4+1
#define AV_TEXT_BASE_0   (HGR_LEN/MAX_AV_OFFSET)+1

#define SAMPLE_RATE (115200 / (1+8+1))
#define AUDIO_MAX          256
#define BUFFER_LEN         (60*10)

#define AUDIO_MAX_LEVEL          31
#define AUDIO_NUM_LEVELS         (AUDIO_MAX_LEVEL+1)
#define AUDIO_STREAM_TITLE       AUDIO_NUM_LEVELS
#define AUDIO_END_OF_STREAM      AUDIO_NUM_LEVELS + 1

#define AV_MAX_LEVEL             31
#define AV_NUM_LEVELS            (AV_MAX_LEVEL+1)
#define AV_END_OF_STREAM         AV_NUM_LEVELS
#define AV_KBD_LOAD_LEVEL        16

/* Set very high because it looks nicer to drop frames than to
 * artifact all the way
 */
#define MAX_BYTES_PER_FRAME 600

#define DOUBLE_BUFFER

#if 0
  #define DEBUG LOG
#else
 #define DEBUG if (0) LOG
#endif

unsigned long bytes_sent = 0, data_bytes = 0, offset_bytes = 0, base_bytes = 0;

char video_bytes_buffer[HGR_LEN*2];
int num_video_bytes = 0;

int cur_base, offset;

extern int serial_delay;

char output_file[64];

char tmp_filename[FILENAME_MAX];

int video_len;

unsigned int vol_mult = 10;

#define PREDECODE_SECS 10

#define IO_BARRIER(msg) do {                            \
    int r;                                              \
    LOG("IO Barrier (%s)\n", msg);                      \
    do {                                                \
      r = simple_serial_getc();                         \
    } while (r != SURL_CLIENT_READY                     \
          && r != SURL_METHOD_ABORT && r != 0x00);      \
} while (0)

static int update_eta(int eta) {
  int r;

  simple_serial_putc(SURL_ANSWER_STREAM_LOAD);
  /* Quiet IO Barrier */
  simple_serial_getc_with_timeout();
  if (eta == ETA_MAX) {
    eta = 255;
  } else {
    if (eta < 0) {
      eta = 0;
    }
    eta /= 8;
    if (eta > 254) {
      eta = 254;
    }
    if (eta < 0) {
      eta = 0;
    }
  }
  usleep(10000);
  simple_serial_putc(eta);

  r = simple_serial_getc_with_timeout();
  if (r != SURL_CLIENT_READY) {
    LOG("Client abort (%02x)\n", r);
    return -1;
  }

  return 0;
}

static void *ffmpeg_video_decode_thread(void *th_data) {
  decode_data *data = (decode_data *)th_data;
  unsigned char *buf;
  int frameno = 0;
  int vhgr_file;
  struct timeval decode_start, cur_time;
  unsigned long secs;
  unsigned long microsecs;
  unsigned long elapsed, prev_elapsed = 0;
  int decode_slow = 0;

  LOG("Generating frames for %s...\n", data->url);

  vhgr_file = -1;
  sprintf(tmp_filename, "/tmp/vhgr-XXXXXX");
  if ((vhgr_file = mkstemp(tmp_filename)) < 0) {
    LOG("Could not open output file %s (%s).\n", tmp_filename, strerror(errno));
    pthread_mutex_lock(&data->mutex);
    data->decoding_end = 1;
    data->decoding_ret = -1;
    pthread_mutex_unlock(&data->mutex);
    return NULL;
  }

  if (ffmpeg_video_decode_init(data, &video_len) != 0) {
    LOG("Could not init ffmpeg.\n");
    pthread_mutex_lock(&data->mutex);
    data->decoding_end = 1;
    data->decoding_ret = -1;
    pthread_mutex_unlock(&data->mutex);
    return NULL;
  }

  pthread_mutex_lock(&data->mutex);

  if (data->decoding_end == 1) {
    goto out; /* out unlocks mutex */
  }
  pthread_mutex_unlock(&data->mutex);

  gettimeofday(&decode_start, 0);
  while ((buf = ffmpeg_video_decode_frame(data, video_len*FPS, frameno)) != NULL) {
    signed long usecs_per_frame, remaining_frames, done_len, remaining_len;
    if (write(vhgr_file, buf, HGR_LEN) != HGR_LEN) {
      LOG("Could not write data\n");
      close(vhgr_file);
      vhgr_file = -1;
      pthread_mutex_lock(&data->mutex);
      data->decoding_end = 1;
      data->decoding_ret = -1;
      pthread_mutex_unlock(&data->mutex);
      break;
    }
    frameno++;

    gettimeofday(&cur_time, 0);
    secs      = (cur_time.tv_sec - decode_start.tv_sec)*1000000;
    microsecs = cur_time.tv_usec - decode_start.tv_usec;
    elapsed   = secs + microsecs;

    usecs_per_frame = elapsed / frameno;
    remaining_frames = video_len*FPS - frameno;
    remaining_len = remaining_frames * usecs_per_frame;
    done_len = frameno / FPS;

    if (elapsed - prev_elapsed > 2000000) {
      LOG("decoded %d frames (%lds) in %lds, remaining %ld frames, should take %lds\n",
            frameno, done_len, elapsed/1000000, remaining_frames, remaining_len/1000000);
      prev_elapsed = elapsed;
    }

    pthread_mutex_lock(&data->mutex);
    data->decode_remaining = remaining_frames;
    data->max_seekable = frameno/2;
    data->total_frames = frameno;

    if (frameno == FPS * PREDECODE_SECS) {
      if (elapsed / 1000000 > PREDECODE_SECS / 3) {
        LOG("decoding too slow, not starting early\n");
        decode_slow = 1;
      } else {
        data->data_ready = 1;
      }
    } else if (decode_slow) {
      /* We can start streaming as soon as what is remaining should
       * take less than the time we already spent decoding */
      if ((remaining_len/1000000)/2 < done_len) {
        data->data_ready = 1;
        decode_slow = 0;
      }
    }
    /* Send ping every 10% (and only once for every 25 frames matching the modulo)*/
    if ((frameno) % 100 == 0) {
      data->eta = (remaining_len/1000000) - (done_len*2);
      
      if (data->eta > 0) {
        LOG("Frame %d ETA %d\n", frameno, data->eta);
      }
    }

    if (data->stop) {
      LOG("Aborting video decode\n");
      pthread_mutex_unlock(&data->mutex);
      break;
    } else {
      pthread_mutex_unlock(&data->mutex);
    }
  }

  pthread_mutex_lock(&data->mutex);
  data->decode_remaining = 0;
  data->max_seekable = frameno;
  data->total_frames = frameno;
  data->data_ready = 1;
  data->decoding_end = 1;
  data->decoding_ret = 0;
out:
  pthread_mutex_unlock(&data->mutex);

  if (vhgr_file > 0) {
    close(vhgr_file);
  }

  ffmpeg_video_decode_deinit(data);
  return NULL;
}

extern int ttyfd;
extern char *opt_aux_tty_path;
extern int aux_ttyfd;

static void flush_video_bytes(int fd) {
  struct timeval vid_send_start;
  struct timeval vid_send_end;
  gettimeofday(&vid_send_start, 0);


  simple_serial_write_fast_fd(fd, video_bytes_buffer, num_video_bytes);

  gettimeofday(&vid_send_end, 0);

  unsigned long secs      = (vid_send_end.tv_sec - vid_send_start.tv_sec)*1000000;
  unsigned long microsecs = vid_send_end.tv_usec - vid_send_start.tv_usec;
  unsigned long elapsed   = secs + microsecs;
  unsigned long real_elapsed;
  DEBUG("video flushed %d bytes in %lu microsecs\n", num_video_bytes, elapsed);

  /* Force wait on emulation (where pushing to "serial port" is
   * not constrained by the laws of physics)
   * In reality at 115200bps we flush one byte every 86 µs */
  real_elapsed = num_video_bytes*86;
  if (elapsed < real_elapsed/2) {
    DEBUG("video: sleeping %luµs\n", real_elapsed-elapsed);
    usleep(real_elapsed-elapsed);
  }

  num_video_bytes = 0;
}

static void enqueue_video_byte(unsigned char b, int fd) {
  if (fd < 0) {
    return;
  }
  bytes_sent++;
  /* Video only ? */
  if (1) {
    video_bytes_buffer[num_video_bytes++] = b;
    return;
  }
  /* Otherwise don't buffer */
  write(fd, &b, 1);
  //fflush(fp);
}

static int last_sent_base = -1;

static void buffer_video_base(unsigned char b, int fd) {
  DEBUG(" new base %d\n", b);
  DEBUG(" base %d offset %d\n", b, offset);
  if ((b| 0x80) == 0xFF) {
    LOG("Base error! Should not!\n");
    exit(1);
  }
  enqueue_video_byte(b, fd);
  last_sent_base = b;
  base_bytes++;
}

int last_sent_offset = -1;
static void buffer_video_offset(unsigned char o, int fd) {
  DEBUG("offset %d\n", o);
  if ((o| 0x80) == 0xFF) {
    LOG("Offset error! Should not!\n");
    exit(1);
  }
  enqueue_video_byte(o, fd);

  last_sent_offset = o;
  offset_bytes++;
}
static void buffer_video_num_reps(unsigned char b, int fd) {
  DEBUG("  => %d * ", b);
  enqueue_video_byte(0x7F, fd);
  if ((b & 0x80) != 0) {
    LOG("Reps error! Should not!\n");
    exit(1);
  }
  enqueue_video_byte(b, fd);

  data_bytes += 2;
}
static void buffer_video_byte(unsigned char b, int fd) {
  DEBUG("  => %d\n", b);
  if ((b & 0x80) != 0) {
    LOG("Byte error! Should not!\n");
    exit(1);
  }
  enqueue_video_byte(b|0x80, fd);

  data_bytes++;
}

static void buffer_video_repetitions(int min_reps, int ident_vals, int last_val, int fd) {
  if (last_val == -1) {
    return;
  }
  if (ident_vals > min_reps) {
    buffer_video_num_reps(ident_vals, fd);
    buffer_video_byte(last_val, fd);
  } else {
    while (ident_vals > 0) {
      buffer_video_byte(last_val, fd);
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

static inline void check_duration(const char *str, struct timeval *start) {
  struct timeval frame_end;
  gettimeofday(&frame_end, 0);

  unsigned long secs      = (frame_end.tv_sec - start->tv_sec)*1000000;
  unsigned long microsecs = frame_end.tv_usec - start->tv_usec;
  unsigned long elapsed   = secs + microsecs;
  DEBUG("%s took %lu microsecs\n", str, elapsed);

  /* Force wait on emulation (where pushing to "serial port" is
   * not constrained by the laws of physics) */
  if (!strcmp(str,"audio") && elapsed < 1000) {
    usleep((1000000/FPS)-elapsed);
  }
}

static inline int video_sync_fps(struct timeval *start) {
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
  if (lateness > (1000000/FPS)) {
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


static int sort_by_score(byte_diff *a, byte_diff *b) {
  return a->changed < b->changed;
}

static int sort_by_offset(byte_diff *a, byte_diff *b) {
  return a->offset > b->offset;
}

static byte_diff **diffs = NULL;

int audio_sample_offset = 0;
int audio_sample_multiplier = 2;

static int num_audio_samples = 0;
static unsigned char audio_samples_buffer[SAMPLE_RATE*2];

static inline void buffer_audio_sample(unsigned char i) {
  audio_samples_buffer[num_audio_samples++] = (i + audio_sample_offset)*audio_sample_multiplier;
}

static inline int flush_audio_samples(void) {
  if (write(ttyfd, (char *)audio_samples_buffer, num_audio_samples) < num_audio_samples) {
    LOG("Audio flush: write error %d (%s)\n", errno, strerror(errno));
    return EOF;
  }

  num_audio_samples = 0;
  return 0;
}

static int send_end_of_audio_stream(void) {
  buffer_audio_sample(AUDIO_END_OF_STREAM);
  return flush_audio_samples();
}

static int audio_numcols = 40;
static void send_audio_title(char *title, char *translit) {
  int i, w;
  size_t l;
  char *translit_buf;
  char space = ' ';

  if (title == NULL) {
    return;
  }

  translit_buf = do_charset_convert(title, OUTGOING, translit, 0, &l);
  buffer_audio_sample(AUDIO_STREAM_TITLE);
  flush_audio_samples();

  /* Limit to audio_numcols chars */
  w = write(ttyfd, translit_buf, MIN(audio_numcols, strlen(translit_buf)));

  if (w < 0)
    w = 0;

  free(translit_buf);

  /* Fill to audio_numcols chars */
  for (i = 0; i < audio_numcols - w; i++) {
    write(ttyfd, &space, 1);
  }
}

static int send_end_of_av_stream(void) {
  buffer_audio_sample(AV_END_OF_STREAM);
  return flush_audio_samples();
}

static void *ffmpeg_audio_decode_thread(void *arg) {
  decode_data *th_data = (decode_data *)arg;

  ffmpeg_audio_decode(th_data);
  return (void *)0;
}

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
  LOG("=> %s %s\n", key, value);
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

static unsigned char *audio_get_stream_art(decode_data *audio_data, int monochrome, int scale, int dhgr) {
  unsigned char *img_data = NULL;
  size_t img_size = 0;
  unsigned char *buffer = NULL;

  pthread_mutex_lock(&audio_data->mutex);
  img_data = audio_data->img_data;
  img_size = audio_data->img_size;
  pthread_mutex_unlock(&audio_data->mutex);

  if (img_data && img_size) {
    FILE *fp = fopen("/tmp/imgdata", "w+b");
    if (fp) {
      if (fwrite(img_data, 1, img_size, fp) == img_size) {
        fclose(fp);
        if (dhgr) {
          buffer = sdl_to_dhgr("/tmp/imgdata", monochrome, 0, &img_size, 0, scale);
        } else {
          buffer = sdl_to_hgr("/tmp/imgdata", monochrome, 0, &img_size, 0, scale);
        }

        if (img_size != HGR_LEN && img_size != HGR_LEN*2) {
          buffer = NULL;
        }
      } else {
        fclose(fp);
      }
    }
  }
  return buffer;
}

int skip_secs (unsigned char c) {
  int s = 0;
  switch (c) {
    case APPLE_CH_CURS_LEFT:
    case APPLE_CH_CURS_RIGHT:
      s = 10;
      break;
    case APPLE_CH_CURS_UP:
    case APPLE_CH_CURS_DOWN:
    case 'u':
    case 'U':
    case 'j':
    case 'J':
      s = 60;
      break;
  }
  return s;
}

int surl_stream_audio(char *url, char *translit, char monochrome, HGRScale scale) {
  int num = 0;
  unsigned char c;
  size_t cur = 0;
  unsigned char *hgr_buf = NULL;
  size_t size = 0;
  int ret = 0;
  int vol_adj_done = 0;
  int auto_vol = vol_mult;
  struct timeval frame_start;
  pthread_t decode_thread;
  decode_data *th_data = malloc(sizeof(decode_data));
  int ready = 0;
  int stop = 0;

  memset(th_data, 0, sizeof(decode_data));
  th_data->url = url;
  th_data->sample_rate = SAMPLE_RATE;
  th_data->eta = ETA_MAX;
  pthread_mutex_init(&th_data->mutex, NULL);

  LOG("Starting decode thread (charset %s, monochrome %d, scale %d)\n", translit, monochrome, scale);
  pthread_create(&decode_thread, NULL, *ffmpeg_audio_decode_thread, (void *)th_data);

  while(!ready && !stop) {
    int eta;
    pthread_mutex_lock(&th_data->mutex);
    ready = th_data->data_ready;
    stop = th_data->decoding_end;
    eta = th_data->eta;
    pthread_mutex_unlock(&th_data->mutex);
    if (update_eta(eta) != 0) {
      stop = 1;
    }
    usleep(10000);
  }

  LOG("Decode thread state: %s\n", ready ? "ready" : stop ? "failure" : "unknown");

  if (!ready && stop) {
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    goto cleanup_thread;
  } else {
    hgr_buf = audio_get_stream_art(th_data, monochrome, scale, 0);

    pthread_mutex_lock(&th_data->mutex);
    send_metadata("has_video", th_data->has_video ? "1":"0", translit);
    send_metadata("artist", th_data->artist, translit);
    send_metadata("album", th_data->album, translit);
    send_metadata("title", th_data->title, translit);
    send_metadata("track", th_data->track, translit);
    th_data->title_changed = 0;
    pthread_mutex_unlock(&th_data->mutex);

    LOG("Client ready\n");
    if (hgr_buf) {
      unsigned char dhgr;
      simple_serial_putc(SURL_ANSWER_STREAM_ART);
      dhgr = simple_serial_getc() == 'D';
      if (simple_serial_getc() == SURL_CLIENT_READY) {
        LOG("Sending %s image\n", dhgr ? "DHGR":"HGR");
        if (dhgr) {
          hgr_buf = audio_get_stream_art(th_data, monochrome, scale, 1);
          simple_serial_write_fast((char *)hgr_buf, HGR_LEN);
          simple_serial_getc();
          simple_serial_write_fast((char *)hgr_buf+HGR_LEN, HGR_LEN);
        } else {
          simple_serial_write_fast((char *)hgr_buf, HGR_LEN);
        }
        if (simple_serial_getc() != SURL_CLIENT_READY) {
          return -1;
        }
      } else {
        LOG("Skip image sending\n");
      }
    }

    simple_serial_putc(SURL_ANSWER_STREAM_START);
    if (simple_serial_getc() != SURL_CLIENT_READY) {
      pthread_mutex_lock(&th_data->mutex);
      th_data->stop = 1;
      LOG("stop\n");
      pthread_mutex_unlock(&th_data->mutex);
      ret = -1;
      goto cleanup_thread;
    }
  }

  audio_numcols = simple_serial_getc();
  LOG("Client has %d columns\n", audio_numcols);
  audio_sample_offset = simple_serial_getc();
  audio_sample_multiplier = simple_serial_getc();
  LOG("Sample offset is %d mult %d\n", audio_sample_offset, audio_sample_multiplier);

  sleep(1); /* Let ffmpeg have a bit of time to push data so we don't starve */
  vol_mult = 10;

  /* Debug for cycle-counting */
  // for (int samp_val = 0; samp_val < AUDIO_NUM_LEVELS; samp_val++) {
  //   int count;
  //   for (count = 0; count < 100; count++) {
  //     buffer_audio_sample((uint8_t)samp_val);
  //   }
  //   flush_audio_samples();
  // }

  gettimeofday(&frame_start, 0);
  while (1) {
    int32_t cur_val, samp_val;

    pthread_mutex_lock(&th_data->mutex);
    if (cur > SAMPLE_RATE*(2*BUFFER_LEN)) {
      /* Avoid ever-expanding buffer */
      memmove(th_data->data, th_data->data + SAMPLE_RATE*BUFFER_LEN, SAMPLE_RATE*BUFFER_LEN);
      th_data->data = realloc(th_data->data, th_data->size - SAMPLE_RATE*BUFFER_LEN);
      th_data->size -= SAMPLE_RATE*BUFFER_LEN;
      cur -= SAMPLE_RATE*BUFFER_LEN;
    }
    size = th_data->size;
    if (cur < size) {
      cur_val = (int32_t)th_data->data[cur];
    }
    stop = th_data->decoding_end;

    /* Update max volume for auto-leveling after decoding */
    if (th_data->decoding_end && th_data->max_audio_volume != 0) {
      /* Adjust volume */
      if (th_data->max_audio_volume != 0 && vol_mult == 10 && !vol_adj_done) {
        vol_mult = ((AUDIO_MAX/2) * vol_mult) / (th_data->max_audio_volume-127);
        LOG("Max detected level now %d, vol set to %d\n", th_data->max_audio_volume, vol_mult);
        vol_adj_done = 1;
        auto_vol = vol_mult;
      }
    }
    if (th_data->title_changed) {
      if (th_data->title) {
        LOG("Title change: %s\n", th_data->title);
        send_audio_title(th_data->title, translit);
      }
      th_data->title_changed = 0;
    }
    pthread_mutex_unlock(&th_data->mutex);

    if (cur == size) {
      if (stop) {
        LOG("Stopping at %zu/%zu\n", cur, size);
        break;
      } else {
        /* We're starved but not done :-( */
        goto handle_kbd;
      }
    }

    samp_val = (int32_t)(((cur_val-((int32_t)AUDIO_MAX/2))*(int32_t)vol_mult)/10)+((int32_t)AUDIO_MAX/2);
    if (samp_val < 0) {
      samp_val = 0;
    } else if (samp_val >= AUDIO_MAX) {
      samp_val = AUDIO_MAX-1;
    }
    buffer_audio_sample((uint8_t)samp_val*AUDIO_NUM_LEVELS/AUDIO_MAX);
    if (cur % (SAMPLE_RATE/FPS) == 0) {
      check_duration("audio", &frame_start);
      gettimeofday(&frame_start, 0);
      if (flush_audio_samples() == EOF) {
        goto cleanup_thread;
      }
    }
    cur++;

handle_kbd:
    /* Kbd input polled directly for no wait at all */
    {
      struct pollfd fds[1];
      fds[0].fd = ttyfd;
      fds[0].events = POLLIN;
      if (poll(fds, 1, 0) > 0 && (fds[0].revents & POLLIN)) {
        c = simple_serial_getc();
        switch (c) {
          case '+':
            if (vol_mult < 80) {
              vol_mult ++;
              LOG("volume %d\n", vol_mult);
            }
            break;
          case '-':
            if (vol_mult > 2) {
              vol_mult --;
              LOG("volume %d\n", vol_mult);
            }
            break;
          case '=':
            vol_mult = auto_vol;
            LOG("volume %d\n", vol_mult);
            break;
          case ' ':
            LOG("Pause\n");
            buffer_audio_sample(AUDIO_MAX_LEVEL/2);
            if (flush_audio_samples() == EOF) {
              goto cleanup_thread;
            }
            simple_serial_getc();
            break;
          case APPLE_CH_CURS_LEFT:
          case APPLE_CH_CURS_DOWN:
            LOG("Rewind\n");
            if (cur < SAMPLE_RATE * skip_secs(c)) {
              cur = 0;
            } else {
              cur -= SAMPLE_RATE * skip_secs(c);
            }
            break;
          case APPLE_CH_CURS_RIGHT:
          case APPLE_CH_CURS_UP:
            if (cur + SAMPLE_RATE * skip_secs(c) < size) {
              cur += SAMPLE_RATE * skip_secs(c);
            } else {
              cur = size;
            }
            LOG("Forward\n");
            break;
          case CH_ESC:
            LOG("Stop\n");
            goto done;
          case SURL_METHOD_ABORT:
          case 0x00:
            LOG("Connection reset\n");
            goto cleanup_thread;
        }
      }
    }

    num++;
  }

done:
  buffer_audio_sample(AUDIO_MAX_LEVEL/2);
  if (send_end_of_audio_stream() == EOF) {
    goto cleanup_thread;
  }

  do {
    c = simple_serial_getc();
    LOG("got %02X\n", c);
  } while (c != SURL_CLIENT_READY
        && c != SURL_METHOD_ABORT
        && c != 0x00
        && c != (unsigned char)EOF);

cleanup_thread:
  LOG("Cleaning up audio decoder thread\n");
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
  LOG("Done\n");

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
  int skipped = 0, duration;
  int command, page = 0;
  int num_diffs = 0;
  int vhgr_file = -1;
  pthread_t decode_thread;
  decode_data *th_data = malloc(sizeof(decode_data));
  int ready = 0;
  int stop = 0;
  int err = 0;

  /* Reset stats */
  bytes_sent = data_bytes = offset_bytes = base_bytes = 0;

  memset(th_data, 0, sizeof(decode_data));
  th_data->url = url;
  th_data->video_size = HGR_SCALE_HALF;
  th_data->eta = ETA_MAX;
  pthread_mutex_init(&th_data->mutex, NULL);

  LOG("Starting video decode thread\n");
  pthread_create(&decode_thread, NULL, *ffmpeg_video_decode_thread, (void *)th_data);

  sleep(1);

  while(!ready && !stop) {
    int eta;
    pthread_mutex_lock(&th_data->mutex);
    ready = th_data->data_ready;
    stop = th_data->decoding_end;
    err = th_data->decoding_ret;
    eta = th_data->eta;
    pthread_mutex_unlock(&th_data->mutex);
    if (update_eta(eta) != 0) {
      stop = 1;
    }
    usleep(10000);
  }

  if (stop && err) {
    LOG("Error generating frames\n");
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
    LOG("Error opening %s\n", tmp_filename);
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    goto cleanup;
  }

  simple_serial_putc(SURL_ANSWER_STREAM_READY);

  if (simple_serial_getc() != SURL_CLIENT_READY) {
    LOG("Client not ready\n");
    goto cleanup;
  }

  LOG("Starting stream\n");
  simple_serial_putc(SURL_ANSWER_STREAM_START);
  LOG("sent\n");

  memset(video_bytes_buffer, 0, sizeof video_bytes_buffer);

  i = 0;
  page = 1;

  /* Fill cache */
  read(vhgr_file, buf[page], HGR_LEN);
  lseek(vhgr_file, 0, SEEK_SET);

  memset(buf_prev[0], 0x00, HGR_LEN);
  memset(buf_prev[1], 0x00, HGR_LEN);
  /* Send 30 full-black bytes first to make sure everything
  * started client-side (but don't change the first one) */
  memset(buf_prev[1] + 1, 0x7F, 30);

  memset(buf[page], 0x00, HGR_LEN);
  goto send;

next_file:
  i++;
#ifdef  DOUBLE_BUFFER
  page = !page;
#endif

  if ((r = read(vhgr_file, buf[page], HGR_LEN)) != HGR_LEN) {
    goto close_last;
  }

  /* count diffs */
  last_diff = 0;
  cur_base = 0;
  ident_vals = 0;


  if (video_sync_fps(&frame_start)) {
    gettimeofday(&frame_start, 0);
    DEBUG("skipping frame\n");
    skipped++;
#ifdef DOUBLE_BUFFER
    page = !page;
#endif
    goto next_file;
  }

send:
  gettimeofday(&frame_start, 0);

  /* Sync point - force a switch to base 0 */
  offset = cur_base = 0;
  buffer_video_offset(0, ttyfd);
  buffer_video_base(0, ttyfd);
  flush_video_bytes(ttyfd);

  if (i > FPS && (i % (15*FPS)) == 0) {
    duration = i/FPS;
    LOG("%d seconds, %d frames skipped / %d: %.2f fps\n", duration,
         skipped, i, (float)(i-skipped)/duration);

  }

  DEBUG("sync point %d\n", i);
  command = simple_serial_getc_immediate();
  switch (command) {
    case CH_ESC:
    case SURL_METHOD_ABORT:
    case 0x00:
      LOG("Abort, exiting.\n");
      goto close_last;
    case ' ':
      LOG("Pause.\n");
      command = simple_serial_getc();
      if (command == SURL_METHOD_ABORT || command == 0x00)
        goto close_last;
      LOG("Play.\n");
      lateness = 0;
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

    offset = pixel - (cur_base*MAX_VIDEO_OFFSET);
    /* If there's no hole in updated bytes, we can let offset
     * increment up to 255 */
    if ((offset >= MAX_VIDEO_OFFSET && pixel != last_diff+1)
      || offset > 255) {
      /* must flush ident */
      buffer_video_repetitions(MIN_VIDEO_REPS, ident_vals, last_val, ttyfd);
      ident_vals = 0;

      /* we have to update base */
      cur_base = pixel / MAX_VIDEO_OFFSET;
      offset = pixel - (cur_base*MAX_VIDEO_OFFSET);

      if (offset < last_sent_offset && cur_base == last_sent_base + 1) {
        DEBUG("skip sending base (offset %d => %d, base %d => %d)\n",
                last_sent_offset, offset, last_sent_base, cur_base);
        buffer_video_offset(offset, ttyfd);
        last_sent_base = cur_base;
      } else {
        DEBUG("must send base (offset %d => %d, base %d => %d)\n",
                last_sent_offset, offset, last_sent_base, cur_base);
        buffer_video_offset(offset, ttyfd);
        buffer_video_base(cur_base, ttyfd);
      }
    } else if (pixel != last_diff+1) {
      /* must flush ident */
      buffer_video_repetitions(MIN_VIDEO_REPS, ident_vals, last_val, ttyfd);
      ident_vals = 0;
      /* We have to send offset */
      buffer_video_offset(offset, ttyfd);
    }
    if (last_val == -1 ||
       (ident_vals < MAX_VIDEO_REPS && buf[page][pixel] == last_val && pixel == last_diff+1)) {
      ident_vals++;
    } else {
      buffer_video_repetitions(MIN_VIDEO_REPS, ident_vals, last_val, ttyfd);
      ident_vals = 1;
    }
    last_val = buf[page][pixel];
    if (last_val & 0x80) {
      LOG("wrong val\n");
    }

    last_diff = pixel;

    /* Note diff done */
    buf_prev[page][pixel] = buf[page][pixel];
  }
  buffer_video_repetitions(MIN_VIDEO_REPS, ident_vals, last_val, ttyfd);
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
  buffer_video_offset(0, ttyfd);
  buffer_video_base(NUM_VIDEO_BASES+2, ttyfd); /* Done */
  flush_video_bytes(ttyfd);

  /* Get rid of possible last ack */
  simple_serial_flush();


  if (i - skipped > 0 && i/FPS > 0) {
    LOG("Max: %d, Min: %d, Average: %d\n", max, min, total / (i-skipped));
    LOG("Sent %lu bytes for %d non-skipped frames: %lu/s, %lu/frame avg (%lu data, %lu offset, %lu base)\n",
            bytes_sent, (i-skipped), bytes_sent/(i/FPS), bytes_sent/(i-skipped),
            data_bytes/(i-skipped), offset_bytes/(i-skipped), base_bytes/(i-skipped));
  }
  if (i - skipped > FPS && i/FPS > 0) {
    duration = i/FPS;
    LOG("%d seconds, %d frames skipped / %d: %.2f fps\n", duration,
          skipped, i, (float)(i-skipped)/duration);
  }

cleanup:
  if (vhgr_file != -1) {
    close(vhgr_file);
  }
  if (th_data) {
    pthread_mutex_lock(&th_data->mutex);
    th_data->stop = 1;
    LOG("cleanup\n");
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

int vhgr_file;

sem_t av_sem;

static void *audio_push(void *unused) {
  /* Audio vars */
  unsigned char c;
  size_t cur = 0;
  int stop, pause = 0;
  struct timeval frame_start;
  int vol_adj_done = 0;
  int auto_vol = vol_mult;
  size_t audio_size = 0;

  gettimeofday(&frame_start, 0);
  vol_mult = 10;

  while (1) {
    int32_t cur_val, samp_val;

    pthread_mutex_lock(&audio_th_data->mutex);
    if (!audio_th_data->fake_data) {
      if (cur > SAMPLE_RATE*(2*BUFFER_LEN)) {
        /* Avoid ever-expanding buffer */
        memmove(audio_th_data->data, audio_th_data->data + SAMPLE_RATE*BUFFER_LEN, SAMPLE_RATE*BUFFER_LEN);
        audio_th_data->data = realloc(audio_th_data->data, audio_th_data->size - SAMPLE_RATE*BUFFER_LEN);
        audio_th_data->size -= SAMPLE_RATE*BUFFER_LEN;
        cur -= SAMPLE_RATE*BUFFER_LEN;
      }
      audio_size = audio_th_data->size;
      if (cur < audio_size) {
        cur_val = audio_th_data->data[cur];
      }
      stop = audio_th_data->decoding_end;

      /* Update max volume for auto-leveling after decoding */
      if (audio_th_data->decoding_end && audio_th_data->max_audio_volume != 0) {
        /* Adjust volume */
        if (audio_th_data->max_audio_volume != 0 && vol_mult == 10 && !vol_adj_done) {
          vol_mult = ((AUDIO_MAX/2) * vol_mult) / (audio_th_data->max_audio_volume-127);
          LOG("Max detected level now %d, vol set to %d\n", audio_th_data->max_audio_volume, vol_mult);
          vol_adj_done = 1;
          auto_vol = vol_mult;
        }
      }
    } else {
      pthread_mutex_lock(&video_th_data->mutex);
      audio_th_data->size = video_th_data->total_frames * (SAMPLE_RATE/FPS);
      audio_size = audio_th_data->size;
      pthread_mutex_unlock(&video_th_data->mutex);

      cur_val = AUDIO_MAX/2;
      stop = 1;
    }

    pthread_mutex_unlock(&audio_th_data->mutex);

    if (cur == audio_size) {
      if (stop) {
        LOG("Audio stop at %zu/%zu\n", cur, audio_size);
        goto abort;
      } else {
        /* We're starved but not done :-( */
        goto handle_kbd;
      }
    }

    if (!pause) {
      samp_val = (int32_t)(((cur_val-((int32_t)AUDIO_MAX/2))*(int32_t)vol_mult)/10)+((int32_t)AUDIO_MAX/2);
      if (samp_val < 0) {
        samp_val = 0;
      } else if (samp_val >= AUDIO_MAX) {
        samp_val = AUDIO_MAX-1;
      }
      buffer_audio_sample((uint8_t)samp_val*AV_NUM_LEVELS/AUDIO_MAX);

      if (cur % (SAMPLE_RATE/FPS) == 0) {
        check_duration("audio", &frame_start);
        gettimeofday(&frame_start, 0);
        if (flush_audio_samples() == EOF) {
          goto abort;
        }
        /* Signal video thread to push a frame */
        sem_post(&av_sem);
      }
      cur++;
    } else {
      /* During pause, we have to drive client in the duty
       * cycles where it handles the keyboard. */
handle_kbd:
      buffer_audio_sample(AV_KBD_LOAD_LEVEL);
      if (flush_audio_samples() == EOF) {
        goto abort;
      }
    }

    /* Kbd input polled directly for no wait at all */
    {
      struct pollfd fds[1];
      fds[0].fd = ttyfd;
      fds[0].events = POLLIN;
      if (poll(fds, 1, 0) > 0 && (fds[0].revents & POLLIN)) {
        c = simple_serial_getc();
        switch (c) {
          case CH_ESC:
            LOG("Stop\n");
            goto abort;
          case SURL_METHOD_ABORT:
          case 0x00:
            LOG("Connection reset\n");
            goto abort;
          case APPLE_CH_CURS_LEFT:
          case APPLE_CH_CURS_DOWN:
            if (cur < SAMPLE_RATE * skip_secs(c)) {
              cur = 0;
            } else {
              cur -= SAMPLE_RATE * skip_secs(c);
            }
            LOG("Rewind to sample %zu\n", cur);
            if (cur % SAMPLE_RATE) {
              cur = cur - cur % SAMPLE_RATE;
              LOG("Fixed to %zu\n", cur);
            }
            LOG("Seek video to %zu frames (%zu bytes)\n", (cur/SAMPLE_RATE)*FPS, (cur/SAMPLE_RATE)*FPS*HGR_LEN);
            lseek(vhgr_file, (cur/SAMPLE_RATE)*FPS*HGR_LEN, SEEK_SET);
            break;
          case APPLE_CH_CURS_RIGHT:
          case APPLE_CH_CURS_UP:
            if (cur + SAMPLE_RATE * skip_secs(c) < audio_size) {
              cur += SAMPLE_RATE * skip_secs(c);
            } else {
              cur = audio_size;
            }
            LOG("Forward to sample %zu\n", cur);
            if (cur % SAMPLE_RATE) {
              cur = cur - cur % SAMPLE_RATE;
              LOG("Fixed to %zu\n", cur);
            }
            pthread_mutex_lock(&video_th_data->mutex);
            if (video_th_data->max_seekable < (cur/SAMPLE_RATE)*FPS) {
              cur = (video_th_data->max_seekable / FPS) * SAMPLE_RATE;
              LOG("Cannot skip so far, skipping to %zu samples (frame %lu)\n",
                     cur, video_th_data->max_seekable);
            }
            pthread_mutex_unlock(&video_th_data->mutex);
            LOG("Seek video to %zu frames (%zu bytes)\n", (cur/SAMPLE_RATE)*FPS, (cur/SAMPLE_RATE)*FPS*HGR_LEN);
            lseek(vhgr_file, (cur/SAMPLE_RATE)*FPS*HGR_LEN, SEEK_SET);
            break;
          case '+':
            if (vol_mult < 80) {
              vol_mult ++;
              LOG("volume %d\n", vol_mult);
            }
            break;
          case '-':
            if (vol_mult > 2) {
              vol_mult --;
              LOG("volume %d\n", vol_mult);
            }
            break;
          case '=':
            vol_mult = auto_vol;
            LOG("volume %d\n", vol_mult);
            break;
          case ' ':
            /* Pause */
            pause = !pause;
            break;
          case 'a':
          case 'A':
            pthread_mutex_lock(&video_th_data->mutex);
            video_th_data->accept_artefact = !video_th_data->accept_artefact;
            pthread_mutex_unlock(&video_th_data->mutex);
            break;
          case 's':
          case 'S':
            pthread_mutex_lock(&video_th_data->mutex);
            video_th_data->interlace = !video_th_data->interlace;
            pthread_mutex_unlock(&video_th_data->mutex);
            break;
          default:
            LOG("key '%02X'\n", c);
        }
      }
    }
  }
  return NULL;
abort:
  pthread_mutex_lock(&audio_th_data->mutex);
  audio_th_data->stop = 1;
  LOG("audio_push aborted\n");
  pthread_mutex_unlock(&audio_th_data->mutex);
  return NULL;
}

static char sub_line_off[4] = {0};
static char sub_line_len[4] = {0};
static char sub_line[4][160] = {0};

static void build_sub_display(const char *text) {
  int l;
  for (l = 0; l < 4; l++) {
    memset(sub_line[l], ' ', sub_line_len[l]);
  }
  if (*text) {
    char **words;
    char *orig_sub = strdup(text);
    size_t num_words = strsplit_in_place(orig_sub, ' ', &words);
    int cur_word;

    for (l = 0; l < 4; l++) {
      sub_line_len[l] = 0;
    }

    l = 0;
    cur_word = 0;
next_line:
    sub_line[l][0] = '\0';
next_word:
    if (cur_word < num_words) {
      char *cur_word_str = words[cur_word];
      int cur_word_len = strlen(cur_word_str);
      sub_line_len[l] = strlen(sub_line[l]);
      if (cur_word_str[0] == '\n') {
        /* Force newline */
        cur_word_str++;
        cur_word_len--;
        l++;
        if (l == 4)
          goto sub_full;
        sub_line[l][0] = '\0';
        if (cur_word_len == 0) {
          cur_word++;
          goto next_word;
        }
      }
      if (sub_line_len[l] + cur_word_len < 39) {
        strcat(sub_line[l], cur_word_str);
        strcat(sub_line[l], " ");
        sub_line_len[l] = strlen(sub_line[l]);
        sub_line_off[l] = (40 - sub_line_len[l])/2;
        cur_word++;
        goto next_word;
      } else {
        l++;
        if (l < 4) {
          goto next_line;
        }
      }
    }
sub_full:
    free(words);
    free(orig_sub);
  }
  for (l = 0; l < 4; l++) {
    DEBUG("line %d: %s (%d)\n", l, sub_line[l], sub_line_len[l]);
  }
}

static unsigned baseaddr[192];
static void init_base_addrs (void)
{
  unsigned int i, group_of_eight, line_of_eight, group_of_sixtyfour;

  for (i = 0; i < 192; ++i)
  {
    line_of_eight = i % 8;
    group_of_eight = (i % 64) / 8;
    group_of_sixtyfour = i / 64;

    baseaddr[i] = line_of_eight * 1024 + group_of_eight * 128 + group_of_sixtyfour * 40;
  }
}

void *video_push(void *unused) {
  int i, j;
  int last_diff;
  int total = 0, min = 0xFFFF, max = 0;
  size_t r;
  unsigned char buf_prev[2][HGR_LEN], buf[2][HGR_LEN];
  struct timeval frame_start;
  int skipped = 0, duration;
  int page = 0;
  int num_diffs = 0;
  int sem_val;
  int cur_frame;
  char *push_sub = NULL;
  int push_sub_page = 0;
  int vidstop;
  int has_subtitles = 1;
  int interlace = 0;
  int accept_artefact = 0;

  i = 0;
  page = 1;
  init_base_addrs();

  /* Reset stats */
  bytes_sent = data_bytes = offset_bytes = base_bytes = 0;

  pthread_mutex_lock(&video_th_data->mutex);
  has_subtitles = video_th_data->has_subtitles;
  pthread_mutex_unlock(&video_th_data->mutex);

  LOG("Video has %ssubtitles.\n", has_subtitles == 0 ? "no ":"");

  /* Fill cache */
  read(vhgr_file, buf[page], HGR_LEN);
  lseek(vhgr_file, 0, SEEK_SET);

  memset(buf_prev[0], 0x00, HGR_LEN);
  memset(buf_prev[1], 0x00, HGR_LEN);
  /* Send 30 full-black bytes first to make sure everything
  * started client-side (but don't change the first one) */
  memset(buf_prev[1] + 1, 0x7F, 30);

  memset(buf[0], 0x00, HGR_LEN);
  memset(buf[1], 0x00, HGR_LEN);

  offset = cur_base = 0x100;
  goto send;

next_file:
  i++;

  cur_frame = lseek(vhgr_file, 0, SEEK_CUR) / HGR_LEN;
  if ((r = read(vhgr_file, buf[page], HGR_LEN)) != HGR_LEN) {
    LOG("Starved!\n");
    goto close_last;
  }

  check_duration("video", &frame_start);

  /* Wait for audio thread signal that we can push a frame */
  sem_wait(&av_sem);
  sem_getvalue(&av_sem, &sem_val);

  last_diff = 0x2001;

  if (num_video_bytes > 0) {
    /* Sync point */
    enqueue_video_byte(0x7F, aux_ttyfd); /* Switch page */
    flush_video_bytes(aux_ttyfd);
    DEBUG("send page toggle\n");
    if (push_sub && push_sub_page) {
      push_sub_page--;
      if (push_sub_page == 0) {
        free(push_sub);
        push_sub = NULL;
      }
    }
  }

  /* Check if we should push a subtitle, before skipping */
  if (has_subtitles) {
    if (push_sub == NULL) {
      push_sub = ffmpeg_get_subtitle_at_frame(video_th_data, cur_frame);
      if (push_sub) {
        build_sub_display(push_sub);
        push_sub_page = 2;
      }
    } else {
      /* We already have a subtitle to push, so move the
       * potential next one forward so we don't forget it. */
      ffmpeg_shift_subtitle_at_frame(video_th_data, cur_frame);
    }
  }

  if (sem_val > 1) {
    gettimeofday(&frame_start, 0);
    skipped++;
    goto next_file;
  }

#ifdef DOUBLE_BUFFER
  page = !page;
#endif

  if (i > FPS && (i % (15*FPS)) == 0) {
    duration = i/FPS;
    LOG("%d seconds, %d frames skipped / %d: %.2f fps\n", duration,
         skipped, i, (float)(i-skipped)/duration);

  }

send:

  pthread_mutex_lock(&video_th_data->mutex);
  vidstop = video_th_data->stop;
  interlace = video_th_data->interlace;
  accept_artefact = video_th_data->accept_artefact;
  pthread_mutex_unlock(&video_th_data->mutex);
  if (vidstop) {
    LOG("Video thread stopping\n");
    goto close_last;
  }

  gettimeofday(&frame_start, 0);

  if (interlace) {
    int line;
    for (line = 1; line < 192; line+=2) {
      int line_addr = baseaddr[line];
      memset(buf[page]+line_addr, 0, 40);
    }
  }

  for (num_diffs = 0, j = 0; j < HGR_LEN; j++) {
    // uint16_t offset = (uint16_t)j;
    // uint8_t line  = ((offset & 0b0001110000000000) >> 10)
    //                |((offset & 0b0000001110000000) >> 4)
    //                |((offset & 0b0000000001100000) << 1);

    if (buf_prev[page][j] != buf[page][j]) {
      diffs[num_diffs]->offset = j;
      diffs[num_diffs]->changed = diff_score(buf_prev[page][j], buf[page][j]);
      num_diffs++;
    }
  }

  if (accept_artefact) {
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
  }

  for (j = 0; j < num_diffs; j++) {
    int pixel = diffs[j]->offset;

    offset = pixel - (cur_base*MAX_AV_OFFSET);
    /* If there's no hole in updated bytes, we can let offset
     * increment up to 255 */
    if (j == 0
        || (offset >= MAX_AV_OFFSET && pixel != last_diff+1)
        || offset > 255
        || offset < 0) {
      /* we have to update base */
      cur_base = pixel / MAX_AV_OFFSET;
      offset = pixel - (cur_base*MAX_AV_OFFSET);

      DEBUG("send base (offset %d => %d, base %d => %d)\n",
              last_sent_offset, offset, last_sent_base, cur_base);
      buffer_video_offset(offset, aux_ttyfd);
      buffer_video_base(cur_base, aux_ttyfd);
    } else if (pixel != last_diff+1) {
      DEBUG("send offset %d (base is %d)\n", offset, cur_base);
      /* We have to send offset */
      buffer_video_offset(offset, aux_ttyfd);
    }
    buffer_video_byte(buf[page][pixel], aux_ttyfd);

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

  if (push_sub) {
    int line;
    int text_base;
    for (line = 0, text_base = AV_TEXT_BASE_0; line < 4; line++, text_base++) {
      int c;
      if (sub_line_len[line] > 0) {
        buffer_video_offset(sub_line_off[line], aux_ttyfd);
        buffer_video_base(text_base, aux_ttyfd);

        for (c = 0; c < sub_line_len[line]; c++) {
          buffer_video_byte(sub_line[line][c], aux_ttyfd);
        }
      }
    }
    LOG("frame %d pushing sub to page %d\n", cur_frame, page);
  }
  goto next_file;

close_last:
  flush_video_bytes(aux_ttyfd);
  if (i - skipped > 0 && i/FPS > 0) {
    LOG("Max: %d, Min: %d, Average: %d\n", max, min, total / (i-skipped));
    LOG("Sent %lu bytes for %d non-skipped frames: %lub/s, %lub/frame avg (%lu data, %lu offset, %lu base)\n",
            bytes_sent, (i-skipped), bytes_sent/(i/FPS), bytes_sent/(i-skipped),
            data_bytes/(i-skipped), offset_bytes/(i-skipped), base_bytes/(i-skipped));
  }
  if (i - skipped > FPS && i/FPS > 0) {
    duration = i/FPS;
    LOG("%d seconds, %d frames skipped / %d: %.2f fps\n", duration,
          skipped, i, (float)(i-skipped)/duration);
  }
  LOG("avg sync duration %d\n", sync_duration);

  pthread_mutex_lock(&video_th_data->mutex);
  video_th_data->stop = 1;
  LOG("vstop\n");
  pthread_mutex_unlock(&video_th_data->mutex);

  return NULL;
}

int surl_stream_audio_video(char *url, char *translit, char monochrome, SubtitlesMode subtitles, char *subtitles_url, HGRScale size) {
  int j;
  int cancelled = 0, playback_stop = 0;
  /* Control vars */
  unsigned char c, *hgr_buf = NULL;
  int ret = 0;
  pthread_t audio_decode_thread, video_decode_thread;
  pthread_t audio_push_thread, video_push_thread;
  audio_th_data = malloc(sizeof(decode_data));
  video_th_data = malloc(sizeof(decode_data));
  ready = 0;
  stop = 0;

  if (opt_aux_tty_path) {
    stop_printer_thread();
    aux_ttyfd = simple_serial_open_file(opt_aux_tty_path, B115200);
  }

  if (aux_ttyfd < 0)
    LOG("No TTY for video\n");

  memset(video_th_data, 0, sizeof(decode_data));
  video_th_data->url = url;
  video_th_data->enable_subtitles = subtitles;
  video_th_data->video_size = size;
  video_th_data->translit = translit;
  video_th_data->subtitles_url = subtitles_url;
  video_th_data->eta = ETA_MAX;
  pthread_mutex_init(&video_th_data->mutex, NULL);

  LOG("Starting video decode thread\n");
  pthread_create(&video_decode_thread, NULL, *ffmpeg_video_decode_thread, (void *)video_th_data);

  memset(audio_th_data, 0, sizeof(decode_data));
  audio_th_data->url = url;
  audio_th_data->sample_rate = SAMPLE_RATE;
  pthread_mutex_init(&audio_th_data->mutex, NULL);

  LOG("Starting audio decode thread (charset %s, monochrome %d, size %d)\n", translit, monochrome, size);
  pthread_create(&audio_decode_thread, NULL, *ffmpeg_audio_decode_thread, (void *)audio_th_data);

  stop = 0;
  while(!ready && !stop) {
    int eta;
    pthread_mutex_lock(&audio_th_data->mutex);
    pthread_mutex_lock(&video_th_data->mutex);

    /* data_ready is set when the thread has enough data to start streaming;
     * decoding_end is set when the thread is finished (either with all data,
     * or because of an error). We want to wait for both threads to be ready
     * or done. */
    ready = (audio_th_data->data_ready || audio_th_data->decoding_end)
         && (video_th_data->data_ready || video_th_data->decoding_end);

    eta = video_th_data->eta;
    pthread_mutex_unlock(&audio_th_data->mutex);
    pthread_mutex_unlock(&video_th_data->mutex);
    if (update_eta(eta) != 0) {
      stop = 1;
    }
    usleep(10000);
  }

  pthread_mutex_lock(&audio_th_data->mutex);
  pthread_mutex_lock(&video_th_data->mutex);

  /* We can have no audio stream: video thread is ready with frames, audio thread not ready but done
   * Note: video thread could be ready AND done, so check it has frames */
  if (video_th_data->data_ready && video_th_data->total_frames > 0
   && !audio_th_data->data_ready && audio_th_data->decoding_end) {
    audio_th_data->fake_data = 1;
    LOG("Stream has no audio\n");
  }

  /* We can have no video stream: audio thread ready with data, video thread not ready but done
   * Note: audio thread could be ready AND done, so check it has data */
  if (audio_th_data->data_ready && audio_th_data->data
   && !video_th_data->data_ready && video_th_data->decoding_end) {
    LOG("Stream has no video\n");
  }

  /* We have nothing */
  if (!audio_th_data->data_ready && !video_th_data->data_ready) {
    LOG("Stream has no video nor audio.\n");
    ready = 0;
    stop = 1;
  }

  pthread_mutex_unlock(&audio_th_data->mutex);
  pthread_mutex_unlock(&video_th_data->mutex);

  LOG("Decode threads state: %s\n", ready ? "ready" : stop ? "failure" : "unknown");

  if (!ready && stop) {
    simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    goto cleanup_thread;
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
    LOG("Error opening %s\n", tmp_filename);
    // simple_serial_putc(SURL_ANSWER_STREAM_ERROR);
    // goto cleanup_thread;
  }

  memset(video_bytes_buffer, 0, sizeof video_bytes_buffer);

  offset = cur_base = 0;

  LOG("AV: Client ready\n");

  pthread_mutex_lock(&video_th_data->mutex);
  if (video_th_data->total_frames == 0) {
    /* No video ? send embedded art if there is any */
    hgr_buf = audio_get_stream_art(audio_th_data, monochrome, HGR_SCALE_FULL, 0);
  }
  pthread_mutex_unlock(&video_th_data->mutex);

  if (hgr_buf) {
    unsigned char dhgr;
    /* If we got an image from audio stream, and no frames from video stream,
     * send it.
     */
    simple_serial_putc(SURL_ANSWER_STREAM_ART);
    dhgr = simple_serial_getc() == 'D';
    if (simple_serial_getc() == SURL_CLIENT_READY) {
        LOG("Sending %s image\n", dhgr ? "DHGR":"HGR");
        if (dhgr) {
          hgr_buf = audio_get_stream_art(audio_th_data, monochrome, HGR_SCALE_FULL, 1);
          simple_serial_write_fast((char *)hgr_buf, HGR_LEN);
          simple_serial_getc();
          simple_serial_write_fast((char *)hgr_buf+HGR_LEN, HGR_LEN);
        } else {
          simple_serial_write_fast((char *)hgr_buf, HGR_LEN);
        }
      if (simple_serial_getc() != SURL_CLIENT_READY) {
        LOG("client error\n");
      }
    } else {
      LOG("Skip image sending\n");
    }
  }

  /* start point for client to enter the AV streamer */
  simple_serial_putc(SURL_ANSWER_STREAM_START);

  IO_BARRIER("Video state");

  /* Inform client whether we have video */
  LOG("Informing client that video is %s (%d)\n", aux_ttyfd > 0 ? "on":"off", aux_ttyfd);
  if (aux_ttyfd > 0) {
    simple_serial_putc(SURL_VIDEO_PORT_OK);
  } else {
    simple_serial_putc(SURL_VIDEO_PORT_NOK);
  }

  LOG("Final wait for client\n");
  if ((c = simple_serial_getc()) != SURL_CLIENT_READY) {
    LOG("Received unexpected 0x%02x\n", c);
    pthread_mutex_lock(&audio_th_data->mutex);
    audio_th_data->stop = 1;
    LOG("audio stop\n");
    pthread_mutex_unlock(&audio_th_data->mutex);
    pthread_mutex_lock(&video_th_data->mutex);
    video_th_data->stop = 1;
    LOG("video stop\n");
    pthread_mutex_unlock(&video_th_data->mutex);
    ret = -1;
    goto cleanup_thread;
  }

  LOG("Getting stream parameters\n");
  audio_sample_offset = simple_serial_getc();
  audio_sample_multiplier = simple_serial_getc();
  LOG("Sample offset is %d mult %d\n", audio_sample_offset, audio_sample_multiplier);

  sleep(1); /* Let ffmpeg have a bit of time to push data so we don't starve */

  /* Send subtitles info */
  pthread_mutex_lock(&video_th_data->mutex);
  simple_serial_putc(video_th_data->has_subtitles);
  pthread_mutex_unlock(&video_th_data->mutex);

  /* start point for client to enter the data read loop */
  if (simple_serial_getc() != SURL_CLIENT_READY) {
    pthread_mutex_lock(&audio_th_data->mutex);
    audio_th_data->stop = 1;
    LOG("audio stop\n");
    pthread_mutex_unlock(&audio_th_data->mutex);
    pthread_mutex_lock(&video_th_data->mutex);
    video_th_data->stop = 1;
    LOG("video stop\n");
    pthread_mutex_unlock(&video_th_data->mutex);
    ret = -1;
    goto read_and_cleanup_thread;
  }
  pthread_mutex_lock(&audio_th_data->mutex);
  if (!audio_th_data->fake_data) {
    long add_samples = SAMPLE_RATE / FPS;
    if (audio_th_data->pts < video_th_data->pts) {
      long cut_ms = video_th_data->pts - audio_th_data->pts;
      long cut_samples = cut_ms * SAMPLE_RATE / 1000;
      LOG("Audio starts early (A %ld V %ld), cutting %ld\n", audio_th_data->pts, video_th_data->pts, cut_samples);
      memmove(audio_th_data->data, audio_th_data->data + cut_samples, audio_th_data->size - cut_samples);
      audio_th_data->data = realloc(audio_th_data->data, audio_th_data->size - cut_samples);
      audio_th_data->size -= cut_samples;
    }

    if (audio_th_data->pts > video_th_data->pts) {
      long add_ms = audio_th_data->pts - video_th_data->pts;
      long add_samples = add_ms * SAMPLE_RATE / 1000;
      LOG("Audio starts late (A %ld V %ld)\n", audio_th_data->pts, video_th_data->pts);
      audio_th_data->data = realloc(audio_th_data->data, audio_th_data->size + add_samples);
      memmove(audio_th_data->data, audio_th_data->data + add_samples, audio_th_data->size);
      memset(audio_th_data->data, 128, add_samples);
      audio_th_data->size += add_samples;
    }

    if (audio_th_data->pts == video_th_data->pts) {
      LOG("Audio starts same as video (A %ld V %ld)\n", audio_th_data->pts, video_th_data->pts);
    }

    /* Add one frame of silence so we can start posting first video frame on time */
    audio_th_data->data = realloc(audio_th_data->data, audio_th_data->size + add_samples);
    memmove(audio_th_data->data, audio_th_data->data + add_samples, audio_th_data->size);
    memset(audio_th_data->data, 128, add_samples);
    audio_th_data->size += add_samples;
  } else {
    /* We'll have to check size from number of frames, in audio_push */
  }

  pthread_mutex_unlock(&audio_th_data->mutex);

  sem_init(&av_sem, 0, 0);
  pthread_create(&audio_push_thread, NULL, *audio_push, NULL);
  pthread_create(&video_push_thread, NULL, *video_push, NULL);
  while (1) {
    pthread_mutex_lock(&audio_th_data->mutex);
    cancelled = audio_th_data->stop;
    pthread_mutex_unlock(&audio_th_data->mutex);
    if (cancelled) {
      LOG("Audio thread cancelled\n");
      pthread_mutex_lock(&video_th_data->mutex);
      video_th_data->stop = 1;
      pthread_mutex_unlock(&video_th_data->mutex);
      /* Signal video thread so it can stop */
      sem_post(&av_sem);
      break;
    }

    pthread_mutex_lock(&video_th_data->mutex);
    playback_stop = video_th_data->stop;
    pthread_mutex_unlock(&video_th_data->mutex);
    if (playback_stop) {
      LOG("Video playback stop\n");
      break;
    }
    sleep(1);
  }

  pthread_join(audio_push_thread, NULL);
  pthread_join(video_push_thread, NULL);

  LOG("done (cancelled: %d)\n", cancelled);
  send_end_of_av_stream();

read_and_cleanup_thread:
  do {
    c = simple_serial_getc();
    LOG("got %02X\n", c);
  } while (c != SURL_CLIENT_READY
        && c != SURL_METHOD_ABORT
        && c != 0x00
        && c != (unsigned char)EOF);

cleanup_thread:
  LOG("Cleaning up A/V decoder thread\n");
  pthread_mutex_lock(&audio_th_data->mutex);
  audio_th_data->stop = 1;
  pthread_mutex_unlock(&audio_th_data->mutex);
  pthread_mutex_lock(&video_th_data->mutex);
  video_th_data->stop = 1;
  pthread_mutex_unlock(&video_th_data->mutex);
  pthread_join(audio_decode_thread, NULL);
  pthread_join(video_decode_thread, NULL);
  free(audio_th_data->img_data);
  free(audio_th_data->data);
  free(audio_th_data->artist);
  free(audio_th_data->album);
  free(audio_th_data->title);
  free(audio_th_data->track);
  ffmpeg_subtitles_free(video_th_data);
  free(audio_th_data);
  free(video_th_data);
  if (vhgr_file != -1) {
    close(vhgr_file);
  }

  if (opt_aux_tty_path) {
    if (aux_ttyfd > 0)
      close(aux_ttyfd);
    aux_ttyfd = -1;
    start_printer_thread();
  }

  LOG("Done\n");

  return ret;
}
