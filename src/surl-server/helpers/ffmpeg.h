#ifndef __ffmpeg_to_hgr_h
#define __ffmpeg_to_hgr_h

#include <pthread.h>

extern int FPS;

int ffmpeg_to_hgr_init(char *filename, int *video_len, char subtitles, char *translit);
void ffmpeg_to_hgr_deinit(void);

typedef struct _decode_data {
  pthread_mutex_t mutex;

  /* params */
  char *url;
  int sample_rate;
  char subtitles;
  char *translit;

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

  /* sync */
  long pts;
  long decode_remaining;
  long max_seekable;
} decode_data;

int ffmpeg_to_raw_snd(decode_data *data);
int ffmpeg_decode_subs(const char *filename, const char *translit);
const char *ffmpeg_sub_at_frame(unsigned long frame);
unsigned char *ffmpeg_convert_frame(decode_data *data, int total_frames, int current_frame);
void free_subs(void);

#endif
