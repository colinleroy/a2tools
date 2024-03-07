#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>

#include "ffmpeg_to_hgr.h"
#include "simple_serial.h"
#include "extended_conio.h"

extern FILE *ttyfp;

#define SAMPLE_OFFSET 0x40
#define MAX_LEVEL       31
#define END_OF_STREAM   32

#define send_sample(i) simple_serial_putc((i) + SAMPLE_OFFSET)

int sample_rate;


static void send_end_of_stream(void) {
  send_sample(END_OF_STREAM);
}

int main(int argc, char *argv[]) {
  int num = 0;
  unsigned char c;
  struct timeval samp_start, samp_end;
  unsigned long secs;
  unsigned long microsecs;
  unsigned long elapsed;
  int max = 0;
  int i;
  unsigned char *data = NULL;
  size_t cur = 0, size = 0;

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

  ffmpeg_to_raw_snd(argv[1], sample_rate, &data, &size);

  for (cur = 0; cur < size; cur++) {
    if (data[cur] > max)
      max = data[cur];
  }

  if (max == 0) {
    max = 255;
  }

  printf("Max volume: %d\n", max);

  gettimeofday(&samp_start, 0);

  for (cur = 0; cur < size; cur++) {
    send_sample(data[cur] * MAX_LEVEL/max);
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
            goto done;
        }
      }
    }

    num++;
  }

done:
  send_sample(0);
  free(data);
  //send_end_of_stream();
}
