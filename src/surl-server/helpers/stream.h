#ifndef __stream_h
#define __stream_h

int surl_stream_video(char *url);
int surl_stream_audio(char *url, char *translit, char monochrome, HGRScale scale);

int surl_stream_audio_video(char *url, char *translit, char monochrome, SubtitlesMode subtitles, char *subtitles_url, HGRScale size);

#endif
