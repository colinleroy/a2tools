#ifndef __ffmpeg_to_hgr_h
#define __ffmpeg_to_hgr_h

#include <pthread.h>
#include <semaphore.h>
extern int FPS;

typedef struct _decode_data {
  pthread_mutex_t mutex;

  /* params */
  char *url;
  int sample_rate;
  char enable_subtitles;
  char video_size;
  char has_subtitles;
  uint8_t max_audio_volume;
  char *translit;
  char interlace;
  char accept_artefact;

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

  /* subs (for video stream) */
  pthread_mutex_t sub_mutex;
  pthread_t sub_thread;
  sem_t sub_thread_ready;
  unsigned long nsubs;
  char **subs;
} decode_data;

int ffmpeg_video_decode_init(decode_data *data, int *video_len);
void ffmpeg_video_decode_deinit(decode_data *data);
unsigned char *ffmpeg_video_decode_frame(decode_data *data, int total_frames, int current_frame);

int ffmpeg_audio_decode(decode_data *data);

int ffmpeg_subtitles_decode(decode_data *data, const char *filename);
char *ffmpeg_get_subtitle_at_frame(decode_data *data, unsigned long frame);
void ffmpeg_shift_subtitle_at_frame(decode_data *data, unsigned long frame);
void ffmpeg_subtitles_free(decode_data *data);


#endif
