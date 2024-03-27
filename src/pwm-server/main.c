#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pthread.h>

#include "ffmpeg.h"
#include "simple_serial.h"
#include "extended_conio.h"

extern FILE *ttyfp;



int sample_rate;

#define SAMPLE_OFFSET 0x40
#define MAX_LEVEL       32
#define END_OF_STREAM   (MAX_LEVEL+1)

#define send_sample(i) do {                 \
  if ((i) + SAMPLE_OFFSET < 0x40) {         \
    printf("woah %d\n", i);                 \
  }                                         \
  simple_serial_putc((i) + SAMPLE_OFFSET);  \
} while (0)

static void send_end_of_stream(void) {
  send_sample(END_OF_STREAM);
}

#define ABS(x) ((x) < 0 ? -(x) : (x))

void *ffmpeg_decode(void *arg) {
  decode_data *th_data = (decode_data *)arg;

  ffmpeg_to_raw_snd(th_data);
  return (void *)0;
}

int main(int argc, char *argv[]) {
  int num = 0;
  unsigned char c;
  struct timeval samp_start, samp_end;
  unsigned long secs;
  unsigned long microsecs;
  unsigned long elapsed;
  int max = 256;
  int i;
  unsigned char *data = NULL;
  unsigned char *img_data = NULL;
  size_t cur = 0, size = 0, img_size = 0;
  pthread_t decode_thread;
  decode_data *th_data = malloc(sizeof(decode_data));
  int ready = 0;
  int stop = 0;

  if (simple_serial_open() != 0) {
    printf("Can't open serial\n");
    exit(1);
  }

  sample_rate = 115200 / (1+8+1);
  if (argc < 2) {
    printf("Usage: %s [audio url]\n", argv[0]);
    exit(1);
  }
  if (argc == 3) {
    sample_rate = atoi(argv[2]);
  }

  memset(th_data, 0, sizeof(decode_data));
  th_data->url = argv[1];
  th_data->sample_rate = sample_rate;
  pthread_mutex_init(&th_data->mutex, NULL);

  pthread_create(&decode_thread, NULL, *ffmpeg_decode, (void *)th_data);

  printf("waiting for data\n");
  while(!ready && !stop) {
    pthread_mutex_lock(&th_data->mutex);
    ready = th_data->data_ready;
    stop = th_data->stop;
    pthread_mutex_unlock(&th_data->mutex);
  }

  /* test samples */
  for (i = 0; i < END_OF_STREAM; i++) {
    printf("send %d\n", i);
    for (argc = 200; argc > 0; argc--) {
      send_sample(i);
      usleep(10000);
    }
    printf("send %d done\n", i);
    send_sample(i);
    cgetc();
  }

  printf("starting\n");
  cur = 0;
  while (1) {
    unsigned char *data = NULL;
    size_t size;

    pthread_mutex_lock(&th_data->mutex);
    data = th_data->data;
    size = th_data->size;
    pthread_mutex_unlock(&th_data->mutex);

    if (cur == size) {
      break;
    }
    send_sample(data[cur] * MAX_LEVEL/max);
    cur++;
    if (num == sample_rate) {
      gettimeofday(&samp_end, 0);
      secs      = (samp_end.tv_sec - samp_start.tv_sec)*1000000;
      microsecs = samp_end.tv_usec - samp_start.tv_usec;
      elapsed   = secs + microsecs;
      printf("%d samples in %luµs\n", num, elapsed);
      num = 0;
      gettimeofday(&samp_start, 0);
    }

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
            send_sample(0);
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
            pthread_mutex_lock(&th_data->mutex);
            th_data->stop = 1;
            pthread_mutex_unlock(&th_data->mutex);
            goto done;
        }
      }
    }

    num++;
  }

done:
  send_sample(0);
  free(th_data->data);
  free(th_data->img_data);
  free(th_data);
  //send_end_of_stream();
}
