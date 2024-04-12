#ifndef __ffmpeg_to_hgr_h
#define __ffmpeg_to_hgr_h

#include <pthread.h>

#define FPS        24
#define FPS_STR   "24"

int ffmpeg_to_hgr_init(char *filename, int *video_len, char subtitles);
void ffmpeg_to_hgr_deinit(void);
unsigned char *ffmpeg_convert_frame(int total_frames, int current_frame);

typedef struct _decode_data {
  pthread_mutex_t mutex;

  /* params */
  char *url;
  int sample_rate;
  char subtitles;

  /* data */
  unsigned char *img_data;
  size_t img_size;
  int data_ready;
  unsigned char *data;
  size_t size;

  /* flags */
  int stop;
  int decoding_ret;
  int decoding_end;

  /* metadata */
  char has_video; 
  char *artist;
  char *album;
  char *title;
  char *track;
} decode_data;

int ffmpeg_to_raw_snd(decode_data *data);

#endif
