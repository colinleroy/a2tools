/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2024 Colin Leroy-Mira - convert frames to HGR
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * API example for decoding and filtering
 * @example filtering_video.c
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

#include <ffmpeg.h>

/* Final buffer size, possibly including black borders */
#define WIDTH 280
#define HEIGHT 192

/* Video size */
#define VIDEO_SIZE "140:96"

#define GREYSCALE_TO_HGR 1

const char *video_filter_descr = /* Set frames per second to a known value */
                                 "fps="FPS_STR","
                                 /* Scale to VIDEO_SIZE, respect aspect ratio, scale fast (no interpolation) */
                                 "scale="VIDEO_SIZE":force_original_aspect_ratio=decrease:flags=neighbor,"
                                 /* Pad in the middle of the HGR screen */
                                 "pad=width=280:height=192:x=-1:y=-1,"
                                 /* Add subtitles, bigger than necessary because anti-aliasing ruins legibility */
                                 "subtitles=force_style='Fontsize=24,Hinting=light':filename=";

static AVFormatContext *audio_fmt_ctx, *video_fmt_ctx;
static AVCodecContext *audio_dec_ctx, *video_dec_ctx;
AVFilterContext *audio_buffersink_ctx, *video_buffersink_ctx;
AVFilterContext *audio_buffersrc_ctx, *video_buffersrc_ctx;
AVFilterGraph *audio_filter_graph, *video_filter_graph;
static int audio_stream_index = -1, video_stream_index = -1;
AVPacket *video_packet, *audio_packet;
AVFrame *video_frame, *audio_frame;
AVFrame *video_filt_frame, *audio_filt_frame;
static unsigned baseaddr[HEIGHT];

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

static int open_video_file(char *filename)
{
    const AVCodec *dec;
    int ret;
    init_base_addrs();

    if (!strncasecmp("sftp://", filename, 7)) {
      memcpy(filename, "sftp", 4);
    } else if (!strncasecmp("ftp://", filename, 6)) {
      memcpy(filename, "ftp", 3);
    }

    if ((ret = avformat_open_input(&video_fmt_ctx, filename, NULL, NULL)) < 0) {
        printf("Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(video_fmt_ctx, NULL)) < 0) {
        printf("Cannot find stream information\n");
        return ret;
    }

    /* select the stream */
    ret = av_find_best_stream(video_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        printf("Cannot find a corresponding stream in the input file\n");
        return ret;
    }

    video_stream_index = ret;

    /* create decoding context */
    video_dec_ctx = avcodec_alloc_context3(dec);
    if (!video_dec_ctx)
        return AVERROR(ENOMEM);
    avcodec_parameters_to_context(video_dec_ctx, video_fmt_ctx->streams[video_stream_index]->codecpar);

    /* init the decoder */
    if ((ret = avcodec_open2(video_dec_ctx, dec, NULL)) < 0) {
        printf("Cannot open decoder\n");
        return ret;
    }

    return 0;
}
static int open_audio_file(char *filename)
{
    const AVCodec *dec;
    int ret;
    init_base_addrs();

    if (!strncasecmp("sftp://", filename, 7)) {
      memcpy(filename, "sftp", 4);
    } else if (!strncasecmp("ftp://", filename, 6)) {
      memcpy(filename, "ftp", 3);
    }

    if ((ret = avformat_open_input(&audio_fmt_ctx, filename, NULL, NULL)) < 0) {
        printf("Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(audio_fmt_ctx, NULL)) < 0) {
        printf("Cannot find stream information\n");
        return ret;
    }

    /* select the stream */
    ret = av_find_best_stream(audio_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
    if (ret < 0) {
        printf("Cannot find a corresponding stream in the input file\n");
        return ret;
    }

    audio_stream_index = ret;

    /* create decoding context */
    audio_dec_ctx = avcodec_alloc_context3(dec);
    if (!audio_dec_ctx)
        return AVERROR(ENOMEM);
    avcodec_parameters_to_context(audio_dec_ctx, audio_fmt_ctx->streams[audio_stream_index]->codecpar);

    /* init the decoder */
    if ((ret = avcodec_open2(audio_dec_ctx, dec, NULL)) < 0) {
        printf("Cannot open decoder\n");
        return ret;
    }

    return 0;
}

static int init_video_filters(const char *filters_descr)
{
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = video_fmt_ctx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

    video_filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !video_filter_graph) {
        return AVERROR(ENOMEM);
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            video_dec_ctx->width, video_dec_ctx->height, video_dec_ctx->pix_fmt,
            time_base.num, time_base.den,
            video_dec_ctx->sample_aspect_ratio.num, video_dec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&video_buffersrc_ctx, buffersrc, "in",
                                       args, NULL, video_filter_graph);
    if (ret < 0) {
        printf("Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&video_buffersink_ctx, buffersink, "out",
                                       NULL, NULL, video_filter_graph);
    if (ret < 0) {
        printf("Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(video_buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        printf("Cannot set output pixel format\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = video_buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = video_buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(video_filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(video_filter_graph, NULL)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}


void ffmpeg_to_hgr_deinit(void) {
    av_packet_unref(video_packet);
    avfilter_graph_free(&video_filter_graph);
    avcodec_free_context(&video_dec_ctx);
    avformat_close_input(&video_fmt_ctx);
    av_frame_free(&video_frame);
    av_frame_free(&video_filt_frame);
    av_packet_free(&video_packet);
}


static int frameno = 0;
uint8_t out_buf[WIDTH*HEIGHT];

static uint8_t *dither_and_center_frame(const AVFrame *frame, AVRational time_base, int progress)
{
  int x, y;
  uint8_t *p0, *p, *out;
  static int x_offset = 0, y_offset = 0;

  // Ordered dither kernel
  uint8_t map[8][8] = {
    { 1, 49, 13, 61, 4, 52, 16, 64 },
    { 33, 17, 45, 29, 36, 20, 48, 32 },
    { 9, 57, 5, 53, 12, 60, 8, 56 },
    { 41, 25, 37, 21, 44, 28, 40, 24 },
    { 3, 51, 15, 63, 2, 50, 14, 62 },
    { 25, 19, 47, 31, 34, 18, 46, 30 },
    { 11, 59, 7, 55, 10, 58, 6, 54 },
    { 43, 27, 39, 23, 42, 26, 38, 22 }
  };

  frameno++;

  if (frame->width < WIDTH) {
    x_offset = (WIDTH - frame->width) / 2;
  }
  if (frame->height < HEIGHT) {
    y_offset = (HEIGHT - frame->height) / 2;
  }
  memset(out_buf, 0, sizeof out_buf);

  /* Trivial ASCII grayscale display. */
  p0 = frame->data[0];
  out = out_buf + (y_offset * WIDTH);

  for (y = 0; y < frame->height; y++) {
    p = p0;
    out += x_offset;
    for (x = 0; x < frame->width; x++) {
      uint16_t val = *p + *p * map[y % 8][x % 8] / 63;

      if(val >= 128)
        val = 255;
      else
        val = 0;
      *(out++) = val;
      p++;
    }
    p0 += frame->linesize[0];
    out += WIDTH - (frame->width + x_offset);
  }
  /* Progress bar */

  for (x = 0; x < progress; x++) {
    out_buf[(HEIGHT-1)*WIDTH + x] = 255;
  }

  return out_buf;
}

static char *escape_filename_for_ffmpeg_filter(char *filename) {
  char *out = malloc(2*strlen(filename)+1);
  int i, o;
  for (i = 0, o = 0; i < strlen(filename); i++) {
    if (filename[i] == ':' || filename[i] == ',' || filename[i] == '=') {
      out[o++] = '\\';
    }
    out[o++] = filename[i];
  }
  out[o++] = '\0';

  return out;
}

int ffmpeg_to_hgr_init(char *filename, int *video_len, char subtitles) {
    int ret = 0;
    char *esc_filename = escape_filename_for_ffmpeg_filter(filename);
    char *vf_str = malloc(strlen(video_filter_descr) + strlen(esc_filename) + 6);
    int try_with_subs = subtitles * 2; /* 2 = try embedded, 1 = try .srt, 0 = no subtitles. */

    video_frame = av_frame_alloc();
    video_filt_frame = av_frame_alloc();
    video_packet = av_packet_alloc();

    if (!video_frame || !video_filt_frame || !video_packet) {
        fprintf(stderr, "Could not allocate frame or packet\n");
        ret = -ENOMEM;
        goto end;
    }

    if ((ret = open_video_file(filename)) < 0) {
        goto end;
    }

    sprintf(vf_str, "%s'%s'", video_filter_descr, esc_filename);
    free(esc_filename);

try_again:
    if (!try_with_subs) {
      *(strstr(vf_str, ",subtitles")) = '\0';
    } else if (try_with_subs == 1) {
      /* Try .srt file? */
      if (strchr(vf_str, '.'))
        sprintf(strrchr(vf_str, '.'), "%s", ".srt");

    } /* First try with embedded subs */

    printf("Filter string '%s'\n", vf_str);
    if ((ret = init_video_filters(vf_str)) < 0) {
        if (try_with_subs) {
          try_with_subs--;
          goto try_again;
        }
        goto end;
    }

    printf("Duration %lus\n", video_fmt_ctx->duration/1000000);
    *video_len = video_fmt_ctx->duration/1000000;
end:
    free(vf_str);
    if (ret < 0)
        printf("Init error occurred: %s\n", av_err2str(ret));

    return ret;
}

uint8_t hgr[0x2000];
unsigned char *ffmpeg_convert_frame(int total_frames, int current_frame) {
    int progress = 0;
    uint8_t *buf = NULL;
    int ret;

    if (total_frames > 0) {
      progress = WIDTH*current_frame / total_frames;
    }

    while (1) {
        /* read one packets */
        if (av_read_frame(video_fmt_ctx, video_packet) < 0)
            goto end;

        if (video_packet->stream_index == video_stream_index) {
            ret = avcodec_send_packet(video_dec_ctx, video_packet);
            if (ret < 0) {
                printf("Error while sending a packet to the decoder\n");
                goto end;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(video_dec_ctx, video_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    printf("Error while receiving a frame from the decoder\n");
                    goto end;
                }

                video_frame->pts = video_frame->best_effort_timestamp;

                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(video_buffersrc_ctx, video_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    printf("Error while feeding the filtergraph\n");
                    goto end;
                }

                /* pull filtered frames from the filtergraph */
                while (1) {
                    ret = av_buffersink_get_frame(video_buffersink_ctx, video_filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0) {
                        printf("Error: %s\n", av_err2str(ret));
                        goto end;
                    }
                    buf = dither_and_center_frame(video_filt_frame, video_buffersink_ctx->inputs[0]->time_base, progress);
                    av_frame_unref(video_filt_frame);
                }
                av_frame_unref(video_frame);
            }
        }
        if (buf) {
            /* We got our frame */
            break;
        }
    }

end:
    if (buf) {
#ifdef GREYSCALE_TO_HGR
        int x, y, base, xoff, pixel;
        unsigned char *ptr;
        unsigned char dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
        unsigned char dhwmono[] = {0x1,0x2,0x4,0x8,0x10,0x20,0x40};

        memset(hgr, 0x00, 0x2000);

        for (y = 0; y < HEIGHT; y++) {
          base = baseaddr[y];
          for (x = 0; x < WIDTH; x++) {
            xoff = base + x/7;
            pixel = x%7;
            ptr = hgr + xoff;
            // printf("%d,%d: buf %d\n", x, y, buf[y*WIDTH + x]);
            if (buf[y*WIDTH + x] != 0x00) {
              ptr[0] |= dhwmono[pixel];
            } else {
              ptr[0] &= dhbmono[pixel];
            }
          }
        }
        return hgr;
#else
        return buf;
#endif
    } else {
        printf("done.\n");
    }
    return NULL;
}

static int init_audio_filters(const char *filters_descr, int sample_rate)
{
    char args[512];
    int ret = 0;
    const AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_U8, -1 };
    static const int64_t out_channel_layouts[] = { AV_CH_LAYOUT_MONO, -1 };
    static int out_sample_rates[] = { 0, -1 };

    AVRational time_base = audio_fmt_ctx->streams[audio_stream_index]->time_base;

    out_sample_rates[0] = sample_rate;

    audio_filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !audio_filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer audio source: the decoded frames from the decoder will be inserted here. */
    ret = snprintf(args, sizeof(args),
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%lx",
             time_base.num, time_base.den, audio_dec_ctx->sample_rate,
             av_get_sample_fmt_name(audio_dec_ctx->sample_fmt),
             audio_dec_ctx->channel_layout);

    ret = avfilter_graph_create_filter(&audio_buffersrc_ctx, abuffersrc, "in",
                                       args, NULL, audio_filter_graph);
    if (ret < 0) {
        printf("Cannot create audio buffer source\n");
        goto end;
    }

    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&audio_buffersink_ctx, abuffersink, "out",
                                       NULL, NULL, audio_filter_graph);
    if (ret < 0) {
        printf("Cannot create audio buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio_buffersink_ctx, "sample_fmts", out_sample_fmts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        printf("Cannot set output sample format\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio_buffersink_ctx, "channel_layouts", out_channel_layouts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        printf("Cannot set output channel layout\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio_buffersink_ctx, "sample_rates", out_sample_rates, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        printf("Cannot set output sample rate\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = audio_buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = audio_buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(audio_filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(audio_filter_graph, NULL)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int ffmpeg_to_raw_snd(decode_data *data) {
    int ret = 0;
    char audio_filter_descr[200];
    const AVDictionaryEntry *tag = NULL;
    const AVCodec *vdec;
    char has_video = 0;

    audio_frame = av_frame_alloc();
    audio_filt_frame = av_frame_alloc();
    audio_packet = av_packet_alloc();

    pthread_mutex_lock(&data->mutex);
    data->data_ready = 0;
    data->data = NULL;
    data->size = 0;
    data->img_data = NULL;
    data->img_size = 0;
    pthread_mutex_unlock(&data->mutex);

    if (!audio_frame || !audio_filt_frame || !audio_packet) {
      fprintf(stderr, "Could not allocate frame or packet\n");
      ret = -ENOMEM;
      goto end;
    }

    if ((ret = open_audio_file(data->url)) < 0) {
      goto end;
    }

    if (av_find_best_stream(audio_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &vdec, 0) == 0) {
      has_video = 1;
    } else {
      has_video = 0;
    }

    sprintf(audio_filter_descr, "aresample=%d,aformat=sample_fmts=u8:channel_layouts=mono",
            data->sample_rate);

    if ((ret = init_audio_filters(audio_filter_descr, data->sample_rate)) < 0) {
        goto end;
    }

    pthread_mutex_lock(&data->mutex);
    data->has_video = has_video;
    while ((tag = av_dict_get(audio_fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
      if (!strcmp(tag->key, "artist")) {
        data->artist = strdup(tag->value);
      } else if (!strcmp(tag->key, "album")) {
        data->album = strdup(tag->value);
      } else if (!strcmp(tag->key, "title")) {
        data->title = strdup(tag->value);
      } else if (!strcmp(tag->key, "track")) {
        data->track = strdup(tag->value);
      }
    }
    if (data->artist == NULL) {
      while ((tag = av_dict_get(audio_fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (!strcmp(tag->key, "album_artist")) {
          data->artist = strdup(tag->value);
        }
      }
    }
    pthread_mutex_unlock(&data->mutex);

    for (int i = 0; i < audio_fmt_ctx->nb_streams; i++) {
      if (audio_fmt_ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
        AVPacket img = audio_fmt_ctx->streams[i]->attached_pic;

        printf("Pushing image\n");
        pthread_mutex_lock(&data->mutex);
        data->img_data = malloc(img.size);
        memcpy(data->img_data, img.data, img.size);
        data->img_size = img.size;
        pthread_mutex_unlock(&data->mutex);
      }
    }

    /* read all packets */
    while (1) {
        if ((ret = av_read_frame(audio_fmt_ctx, audio_packet)) < 0)
            break;

        if (audio_packet->stream_index == audio_stream_index) {
            ret = avcodec_send_packet(audio_dec_ctx, audio_packet);
            if (ret < 0) {
                printf("Error while sending a packet to the decoder\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(audio_dec_ctx, audio_frame);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                } else if (ret == AVERROR_EOF) {
                  printf("Last frame...\n");
                  goto push;
                } else if (ret < 0) {
                    printf("Error while receiving a frame from the decoder\n");
                    goto end;
                }
push:
                if (ret >= 0) {
                    /* push the audio data from decoded frame into the filtergraph */
                    if (av_buffersrc_add_frame_flags(audio_buffersrc_ctx, audio_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        printf("Error while feeding the audio filtergraph\n");
                        break;
                    }

                    /* pull filtered audio from the filtergraph */
                    while (1) {
                        ret = av_buffersink_get_frame(audio_buffersink_ctx, audio_filt_frame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            break;
                        if (ret < 0)
                            goto end;

                        pthread_mutex_lock(&data->mutex);
                        if (data->stop) {
                          printf("Parent thread requested stopping\n");
                          pthread_mutex_unlock(&data->mutex);
                          goto end;
                        }

                        if (data->size == 0) {
                          printf("Started pushing data\n");
                        }

                        data->data = (unsigned char*) realloc(data->data,
                                  (data->size + audio_filt_frame->nb_samples) * sizeof(unsigned char));
                        memcpy(data->data + data->size, 
                               audio_filt_frame->data[0], audio_filt_frame->nb_samples * sizeof(unsigned char));
                        data->size += audio_filt_frame->nb_samples;
                        data->data_ready = 1;
                        pthread_mutex_unlock(&data->mutex);

                        av_frame_unref(audio_filt_frame);
                    }
                    av_frame_unref(audio_frame);
                }
            }
        }
        av_packet_unref(audio_packet);
    }

end:
    if (ret == AVERROR_EOF)
    ret = 0;

    pthread_mutex_lock(&data->mutex);
    printf("Decoding finished at %zu\n", data->size);
    data->decoding_end = 1;
    data->decoding_ret = ret;
    pthread_mutex_unlock(&data->mutex);
    // clean up
    av_packet_unref(audio_packet);
    avfilter_graph_free(&audio_filter_graph);
    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&audio_fmt_ctx);
    av_frame_free(&audio_frame);
    av_frame_free(&audio_filt_frame);
    av_packet_free(&audio_packet);

    if (ret < 0)
        printf("Error occurred: %s\n", av_err2str(ret));

    return ret;
}
