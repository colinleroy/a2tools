#ifndef __ffmpeg_to_hgr_h
#define __ffmpeg_to_hgr_h

#define FPS        24
#define FPS_STR   "24"

int ffmpeg_to_hgr_init(const char *filename, int *video_len);
void ffmpeg_to_hgr_deinit(void);
unsigned char *ffmpeg_convert_frame(void);

int ffmpeg_to_raw_snd(const char *filename, int sample_rate,
                      unsigned char **data, size_t *size,
                      char **img_data, size_t *img_size);

#endif
