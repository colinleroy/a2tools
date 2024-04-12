#ifndef __stream_h
#define __stream_h

int surl_stream_video(char *url);
int surl_stream_audio(char *url, char *translit, char monochrome, enum HeightScale scale);

int surl_stream_audio_video(char *url, char *translit, char monochrome, enum HeightScale scale, char subtitles);

#endif
