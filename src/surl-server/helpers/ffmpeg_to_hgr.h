#ifndef __ffmpeg_to_hgr_h
#define __ffmpeg_to_hgr_h

#define FPS        25
#define FPS_STR   "25"

int ffmpeg_to_hgr_init(const char *filename, int *video_len);
void ffmpeg_to_hgr_deinit(void);
unsigned char *ffmpeg_convert_frame(void);

#endif
