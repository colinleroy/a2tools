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
#include <stdint.h>

#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

#include <ffmpeg.h>

#include <curl/curl.h>

#include "char-convert.h"
#include "../surl_protocol.h"
#include "../log.h"

/* Final buffer size, possibly including black borders */
#define HGR_WIDTH 280
#define HGR_HEIGHT 192

#define STAB_VALUE 20

/* Video size - the bigger, the less fps */
#define MAX_BYTES_PER_FRAME 1600
int pic_width, pic_height;

int ditherer = 0; /* 0 = bayer 1 = burkes */

#define GREYSCALE_TO_HGR 1

#define SUB_DEFAULT_FPS 25

const char *video_filter_descr_s = /* Set frames per second to a known value */
                                 "fps=%d,"
                                 /* Scale to VIDEO_SIZE, scale fast (no interpolation) */
                                 "scale=%d:%d:flags=neighbor,"
                                 /* Equalize histogram (Use 1/x instead of 0.100 because of LC_ALL, decimal separator, etc) */
                                 "histeq=strength=1/20,"
                                 /* Black border */
                                 "pad=width=%d:height=%d:x=-1:y=-1:color=Black,"
                                 /* White border */
                                 "pad=width=%d:height=%d:x=-1:y=-1:color=White,"
                                 /* Pad in the middle of the HGR screen */
                                 "pad=width=%d:height=%d:x=-1:y=-1:color=Black";
                                 // WIP dither with ffmpeg
                                 //" [o1]; [o1][1:v] paletteuse=dither=bayer:bayer_scale=1/2:diff_mode=rectangle";

const char *video_filter_descr_l = /* Set frames per second to a known value */
                                 "fps=%d,"
                                 /* Scale to VIDEO_SIZE, scale fast (no interpolation) */
                                 "scale=%d:%d:flags=neighbor,"
                                 /* Equalize histogram (Use 1/x instead of 0.100 because of LC_ALL, decimal separator, etc) */
                                 "histeq=strength=1/20,"
                                 /* Pad in the middle of the HGR screen */
                                 "pad=width=%d:height=%d:x=-1:y=%d:color=Black";
                                 // WIP dither with ffmpeg
                                 //" [o1]; [o1][1:v] paletteuse=dither=bayer:bayer_scale=1/2:diff_mode=rectangle";

static AVFormatContext *audio_fmt_ctx, *video_fmt_ctx;
static AVCodecContext *audio_dec_ctx, *video_dec_ctx;
AVFilterContext *audio_buffersink_ctx, *video_buffersink_ctx;
AVFilterContext *audio_buffersrc_ctx, *video_buffersrc_ctx;
AVFilterGraph *audio_filter_graph, *video_filter_graph;
static int audio_stream_index = -1, video_stream_index = -1;
AVPacket *video_packet, *audio_packet;
AVFrame *video_frame, *audio_frame;
AVFrame *video_filt_frame, *audio_filt_frame;

static unsigned baseaddr[HGR_HEIGHT];

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

/* Very simple libcurl handler for streaming art */
static size_t curl_art_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  decode_data *data = (decode_data *)userp;
  unsigned char *ptr = realloc(data->img_data, data->img_size + realsize + 1);

  if(!ptr) {
    /* out of memory! */
    LOG("not enough memory (realloc returned NULL)\n");
    free(data->img_data);
    data->img_data = NULL;
    data->img_size = 0;
    return 0;
  }

  data->img_data = ptr;
  memcpy(&(data->img_data[data->img_size]), contents, realsize);
  data->img_size += realsize;
  data->img_data[data->img_size] = 0;

  return realsize;
}
static int open_interrupt_cb(void *ctx)
{
  decode_data *data = (decode_data *)ctx;
  if (data->stop) {
    return -1;
  } else {
    return 0;
  }
}

static int open_video_file(decode_data *data)
{
    const AVCodec *dec;
    int ret;
    AVDictionary *video_options = NULL;

    init_base_addrs();

    if (!strncasecmp("sftp://", data->url, 7)) {
      memcpy(data->url, "sftp", 4);
    } else if (!strncasecmp("ftp://", data->url, 6)) {
      memcpy(data->url, "ftp", 3);
    }

    av_dict_set(&video_options, "reconnect_on_network_error", "1", 0);
    av_dict_set(&video_options, "reconnect", "1", 0);
    av_dict_set(&video_options, "icy", "0", 0);
    // av_dict_set(&video_options, "probesize", "500000", 0);
    // av_dict_set(&video_options, "analyzeduration", "5000", 0);

    video_fmt_ctx = avformat_alloc_context();
    video_fmt_ctx->interrupt_callback.callback = open_interrupt_cb;
    video_fmt_ctx->interrupt_callback.opaque = data;

    if ((ret = avformat_open_input(&video_fmt_ctx, data->url, NULL, &video_options)) < 0) {
      av_dict_free(&video_options);
      LOG("Video: Cannot open input file\n");
      return ret;
    }

    // WIP dither via ffmpeg
    // if ((ret = avformat_open_input(&video_fmt_ctx, "/tmp/palette.png", NULL, &video_options)) < 0) {
    //   av_dict_free(&video_options);
    //   LOG("Video: Cannot open palette file (%d)\n", AVERROR(ret));
    //   return ret;
    // }

    av_dict_free(&video_options);

    if ((ret = avformat_find_stream_info(video_fmt_ctx, NULL)) < 0) {
      LOG("Video: Cannot find stream information\n");
      return ret;
    }

    /* select the stream */
    ret = av_find_best_stream(video_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
      LOG("Video: Cannot find a corresponding stream in the input file\n");
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
        LOG("Video: Cannot open decoder\n");
        return ret;
    }

    return 0;
}

static int open_audio_file(decode_data *data)
{
    const AVCodec *dec;
    int ret;
    AVDictionary *audio_options = NULL;

    init_base_addrs();

    if (!strncasecmp("sftp://", data->url, 7)) {
      memcpy(data->url, "sftp", 4);
    } else if (!strncasecmp("ftp://", data->url, 6)) {
      memcpy(data->url, "ftp", 3);
    }

    av_dict_set(&audio_options, "icy", "1", 0);
    // av_dict_set(&audio_options, "probesize", "500000", 0);
    // av_dict_set(&audio_options, "analyzeduration", "5000", 0);

    audio_fmt_ctx = avformat_alloc_context();
    audio_fmt_ctx->interrupt_callback.callback = open_interrupt_cb;
    audio_fmt_ctx->interrupt_callback.opaque = data;

    if ((ret = avformat_open_input(&audio_fmt_ctx, data->url, NULL, &audio_options)) < 0) {
      av_dict_free(&audio_options);
      LOG("Audio: Cannot open input file\n");
      return ret;
    }

    av_dict_free(&audio_options);

    if ((ret = avformat_find_stream_info(audio_fmt_ctx, NULL)) < 0) {
      LOG("Audio: Cannot find stream information\n");
      return ret;
    }

    /* select the stream */
    ret = av_find_best_stream(audio_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
    if (ret < 0) {
      LOG("Audio: Cannot find a corresponding stream in the input file\n");
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
      LOG("Audio: Cannot open decoder\n");
      return ret;
    }

    return 0;
}

int FPS = 24;

static int init_video_filters(decode_data *data)
{
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = video_fmt_ctx->streams[video_stream_index]->time_base;
    AVRational fps_tb = video_fmt_ctx->streams[video_stream_index]->r_frame_rate;
    double fps;

    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };
    char *filters_descr = malloc(strlen(video_filter_descr_s) * 2);
    float aspect_ratio = (float)video_dec_ctx->width / (float)video_dec_ctx->height;

    if (fps_tb.num > 0 && fps_tb.den > 0) {
      fps = av_q2d(fps_tb);
    } else {
      fps = 24.0;
    }

    data->orig_fps = fps;

    if (fps >= 30.0) {
      FPS = 30;
    } else {
      FPS = 24;
    }

    LOG("Original video %dx%d (%.2f), %.2ffps, doing %d fps with size %d, %ssubs\n", video_dec_ctx->width, video_dec_ctx->height, aspect_ratio,
           data->orig_fps, FPS, data->video_size, data->has_subtitles?"":"no ");

    /* Get final resolution. We don't want too much "square pixels". */
    for (pic_width = HGR_WIDTH - 4; pic_width > HGR_WIDTH/4; pic_width--) {
      pic_height = pic_width / aspect_ratio;
      if (pic_height > 187)
        continue;
      if (data->has_subtitles && pic_height > 156)
        continue;
      if (pic_width * pic_height < (data->video_size != HGR_SCALE_HALF ? 0x2000 : MAX_BYTES_PER_FRAME) * 8) {
        break;
      }
    }
    LOG("Rescaling to %dx%d (%d pixels)\n", pic_width, pic_height, pic_width * pic_height);

    if (data->video_size == HGR_SCALE_HALF)
      sprintf(filters_descr, video_filter_descr_s,
              FPS,
              pic_width, pic_height,
              pic_width + 2, pic_height + 2, /* Black border */
              pic_width + 4, pic_height + 4, /* White border */
              HGR_WIDTH, HGR_HEIGHT);
    else
      sprintf(filters_descr, video_filter_descr_l,
              FPS,
              pic_width, pic_height,
              HGR_WIDTH, HGR_HEIGHT,
              data->has_subtitles ? 2 : -1);

    video_filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !video_filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
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
        LOG("Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&video_buffersink_ctx, buffersink, "out",
                                       NULL, NULL, video_filter_graph);
    if (ret < 0) {
        LOG("Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(video_buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOG("Cannot set output pixel format\n");
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
    free(filters_descr);
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

void ffmpeg_subtitles_free(decode_data *data) {
  unsigned long i;
  pthread_mutex_lock(&data->sub_mutex);
  for (i = 0; i < data->nsubs; i++) {
    if (data->subs[i])
      free(data->subs[i]);
  }
  free(data->subs);
  data->subs = NULL;
  data->nsubs = 0;
  pthread_mutex_unlock(&data->sub_mutex);
}

static uint8_t dithered_buf[HGR_WIDTH*HGR_HEIGHT];
static uint8_t prev_dithered_buf[HGR_WIDTH*HGR_HEIGHT];
static uint8_t *prev_grayscale_buf = NULL;
static uint8_t hgr_buf[0x2000];
static uint8_t byte_age[0x2000];
static uint8_t prev_hgr_buf[0x2000];

void ffmpeg_video_decode_deinit(decode_data *data) {
    av_packet_unref(video_packet);
    avfilter_graph_free(&video_filter_graph);
    avcodec_free_context(&video_dec_ctx);
    avformat_close_input(&video_fmt_ctx);
    av_frame_free(&video_frame);
    av_frame_free(&video_filt_frame);
    av_packet_free(&video_packet);
    if (prev_grayscale_buf) {
      free(prev_grayscale_buf);
      prev_grayscale_buf = NULL;
    }
    if (data->enable_subtitles)
      pthread_join(data->sub_thread, NULL);
}


static uint8_t *bayer_dither_frame(const AVFrame *frame, AVRational time_base, int progress)
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

  if (frame->width < HGR_WIDTH) {
    x_offset = (HGR_WIDTH - frame->width) / 2;
  }
  if (frame->height < HGR_HEIGHT) {
    y_offset = (HGR_HEIGHT - frame->height) / 2;
  }
  memset(dithered_buf, 0, sizeof dithered_buf);

  if (prev_grayscale_buf == NULL) {
    prev_grayscale_buf = malloc(frame->height*frame->width);
    memset(prev_grayscale_buf, 0x7F, frame->height*frame->width);
    memset(prev_dithered_buf, 0, HGR_WIDTH*HGR_HEIGHT);
    memset(prev_hgr_buf, 0, 0x2000);
  }

  /* Trivial ASCII grayscale display. */
  p0 = frame->data[0];
  out = dithered_buf + (y_offset * HGR_WIDTH);

  for (y = 0; y < frame->height; y++) {
    p = p0;
    out += x_offset;
    for (x = 0; x < frame->width; x++) {
      uint16_t coord = y*frame->width + x;
      uint16_t val;
      uint16_t time_stab_val;

      /* Stabilise source video */
      /*
       No stabilisation: 1183b/frame avg (904 data, 216 offset, 61 base),  9.49fps
       3/5:              1112b/frame avg (875 data, 236 offset, 0 base),  10,11fps
      */
      time_stab_val = ((*p * 3) + (prev_grayscale_buf[coord]) * 2) / 5;

      /* Drop single-pixel changes */
      if (time_stab_val != prev_grayscale_buf[coord]
      && (x == 0               || *(p-1) == prev_grayscale_buf[coord-1])
      && (x == frame->width-1  || *(p+1) == prev_grayscale_buf[coord+1])
      && (y == 0               || *(p-frame->linesize[0]) == prev_grayscale_buf[coord-frame->height])
      && (y == frame->height-1 || *(p+frame->linesize[0]) == prev_grayscale_buf[coord+frame->height])) {
        time_stab_val = prev_grayscale_buf[coord];
      }
      prev_grayscale_buf[coord] = time_stab_val;

      val = time_stab_val + time_stab_val * map[y % 8][x % 8] / 63;

      if(val >= 128)
        val = 255;
      else
        val = 0;
      *(out++) = val;
      p++;
    }
    p0 += frame->linesize[0];
    out += HGR_WIDTH - (frame->width + x_offset);
  }

  /* Progress bar */
  for (x = 0; x < progress; x++) {
    dithered_buf[x] = 255;
  }

  return dithered_buf;
}

static uint8_t *burkes_dither_frame(const AVFrame *frame, AVRational time_base, int progress)
{
  int x, y;
  uint8_t *p0, *p, *out;
  static int x_offset = 0, y_offset = 0;
#define ERR_X_OFF 2 //avoid special cases at start/end of line
  int error_table[192+5][280+5] = {0};
  int threshold = 128;

  if (pic_width < HGR_WIDTH) {
    x_offset = (HGR_WIDTH - pic_width) / 2;
  }
  if (pic_height < HGR_HEIGHT) {
    y_offset = (HGR_HEIGHT - pic_height) / 2;
  }
  memset(dithered_buf, 0, sizeof dithered_buf);

  /* Trivial ASCII grayscale display. */
  p0 = frame->data[0];
  out = dithered_buf + (y_offset * HGR_WIDTH);

  for (y = 0; y < frame->height; y++) {
    p = p0;

    if (y < y_offset || y > y_offset + pic_height) {
      goto skip_line;
    }
    for (x = 0; x < frame->width; x++) {
      int current_error, time_stab_val;

      if (x < x_offset || x > x_offset + pic_width) {
        p++;
        out++;
        continue;
      }

      /*
       No stabilisation: 1605 avg (1263 data, 276 offset, 64 base)
       1/2:              480 avg (275 data, 156 offset, 47 base)
       2/3:              668 avg (404 data, 208 offset, 54 base)
       3/5:              576 avg (341 data, 182 offset, 51 base)
      */
      time_stab_val = ((*p * 3) / 5) + ((prev_dithered_buf[y*HGR_WIDTH+x] * 2) / 5);
      if (threshold > time_stab_val + error_table[y][x+ERR_X_OFF]) {
        *(out++) = 0;
        current_error = time_stab_val + error_table[y][x + ERR_X_OFF];
      } else {
        *(out++) = 255;
        current_error = time_stab_val + error_table[y][x + ERR_X_OFF] - 255;
      }
      error_table[y][x + 1 + ERR_X_OFF] += (int)(8.0L / 32.0L * current_error);
      error_table[y][x + 2 + ERR_X_OFF] += (int)(4.0L / 32.0L * current_error);
      error_table[y + 1][x + ERR_X_OFF] += (int)(8.0L / 32.0L * current_error);
      error_table[y + 1][x + 1 + ERR_X_OFF] += (int)(4.0L / 32.0L * current_error);
      error_table[y + 1][x + 2 + ERR_X_OFF] += (int)(2.0L / 32.0L * current_error);
      error_table[y + 1][x - 1 + ERR_X_OFF] += (int)(4.0L / 32.0L * current_error);
      error_table[y + 1][x - 2 + ERR_X_OFF] += (int)(2.0L / 32.0L * current_error);

      p++;
    }
skip_line:
    p0 += frame->linesize[0];
  }

  /* Progress bar */
  for (x = 0; x < progress; x++) {
    dithered_buf[x] = 255;
  }

  return dithered_buf;
}

static void *ffmpeg_subtitles_decode_thread(void *data) {
  decode_data *th_data = (decode_data *)data;

  th_data->has_subtitles = 0;

  if (th_data->subtitles_url == NULL) {
    if (ffmpeg_subtitles_decode(th_data, th_data->url) < 0) {
      char *srt = malloc(strlen(th_data->url) + 10);
      LOG("No embedded subtitles.\n");
      strcpy(srt, th_data->url);
      if (strchr(srt, '.'))
        strcpy(strrchr(srt, '.'), ".srt");
      if (ffmpeg_subtitles_decode(data, srt) < 0) {
        LOG("No srt subtitles.\n");
      }
      free(srt);
    }
  } else if (ffmpeg_subtitles_decode(data, th_data->subtitles_url) < 0) {
    LOG("No subtitles at URL %s.\n", th_data->subtitles_url);
  }

  /* We're ready. */
  sem_post(&th_data->sub_thread_ready);

  return NULL;
}

int ffmpeg_video_decode_init(decode_data *data, int *video_len) {
    int ret = 0;

    video_frame = av_frame_alloc();
    video_filt_frame = av_frame_alloc();
    video_packet = av_packet_alloc();

    if (!video_frame || !video_filt_frame || !video_packet) {
        fprintf(stderr, "Could not allocate frame or packet\n");
        ret = -ENOMEM;
        goto end;
    }

    if ((ret = open_video_file(data)) < 0) {
        goto end;
    }

    if (data->enable_subtitles) {
      pthread_mutex_init(&data->sub_mutex, NULL);
      pthread_create(&data->sub_thread, NULL, *ffmpeg_subtitles_decode_thread, (void *)data);
      LOG("Waiting for subtitle thread.\n");
      sem_wait(&data->sub_thread_ready);
    }

    if ((ret = init_video_filters(data)) < 0) {
        goto end;
    }

    /* Now we know our FPS, recalculate subtitles frames */
    if (data->has_subtitles && data->subs) {
      char **new_subs = malloc(data->nsubs*sizeof(char *));
      int i;
      memset(new_subs, 0, data->nsubs*sizeof(char *));
      for (i = 0; i < data->nsubs; i++) {
        if (data->subs[i]) {
          int j = (i * (1000.0/SUB_DEFAULT_FPS))/(1000.0/FPS);
          if (i != j) {
            LOG("Moving sub from %d to %d (%s)\n", i, j, data->subs[i]);
            new_subs[j] = data->subs[i];
          }
        }
      }
      free(data->subs);
      data->subs = new_subs;
    }

    LOG("Duration %lus\n", video_fmt_ctx->duration/1000000);
    *video_len = video_fmt_ctx->duration/1000000;
end:
    if (ret < 0)
        LOG("Init error occurred: %s\n", av_err2str(ret));

    return ret;
}

unsigned char *ffmpeg_video_decode_frame(decode_data *data, int total_frames, int current_frame) {
    int progress = 0;
    uint8_t *buf = NULL;
    int ret;
    int first = 1;

    if (current_frame == 0) {
      memset(prev_dithered_buf, 0, sizeof(prev_dithered_buf));
      memset(prev_hgr_buf, 0, 0x2000);
    }

    if (total_frames > 0) {
      progress = HGR_WIDTH*current_frame / total_frames;
    }

    while (1) {
        /* read one packets */
        if (av_read_frame(video_fmt_ctx, video_packet) < 0)
            goto end;

        if (video_packet->stream_index == video_stream_index) {
            ret = avcodec_send_packet(video_dec_ctx, video_packet);
            if (ret < 0) {
                LOG("Error while sending a packet to the decoder\n");
                av_packet_unref(video_packet);
                continue;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(video_dec_ctx, video_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    LOG("Error while receiving a frame from the decoder\n");
                    av_packet_unref(video_packet);
                    goto end;
                }

                video_frame->pts = video_frame->best_effort_timestamp;

                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(video_buffersrc_ctx, video_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    LOG("Error while feeding the filtergraph\n");
                    av_frame_unref(video_frame);
                    av_packet_unref(video_packet);
                    goto end;
                }

                /* pull filtered frames from the filtergraph */
                while (1) {
                    ret = av_buffersink_get_frame(video_buffersink_ctx, video_filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0) {
                        LOG("Error: %s\n", av_err2str(ret));
                        av_frame_unref(video_frame);
                        av_packet_unref(video_packet);
                        goto end;
                    }

                    if (first && current_frame == 0) {
                      AVRational time_base = video_fmt_ctx->streams[video_stream_index]->time_base;
                      data->pts = (float)video_filt_frame->pts * 1000.0 * av_q2d(time_base);
                      first = 0;
                      LOG("video frame pts %ld duration %ld timebase %d/%d %f\n",
                      data->pts, video_filt_frame->pkt_duration, time_base.num, time_base.den, av_q2d(time_base));
                    }

                    if (ditherer == 0)
                      buf = bayer_dither_frame(video_filt_frame, video_buffersink_ctx->inputs[0]->time_base, progress);
                    else
                      buf = burkes_dither_frame(video_filt_frame, video_buffersink_ctx->inputs[0]->time_base, progress);
                    memcpy(prev_dithered_buf, buf, sizeof(prev_dithered_buf));
                    av_frame_unref(video_filt_frame);
                }
                av_frame_unref(video_frame);
            }
        }
        av_packet_unref(video_packet);
        if (buf) {
            /* We got our frame */
            break;
        }
    }

end:
    if (buf) {
#ifdef GREYSCALE_TO_HGR
        int x, y, base, xoff, pixel;
        // int changes = 0;
        unsigned char *ptr;
        unsigned char dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
        unsigned char dhwmono[] = {0x1,0x2,0x4,0x8,0x10,0x20,0x40};

        memset(hgr_buf, 0x00, 0x2000);

        if (current_frame == 0) {
          memset(byte_age, 0, 0x2000);
        }

        for (y = 0; y < HGR_HEIGHT; y++) {
          base = baseaddr[y];
          for (x = 0; x < HGR_WIDTH; x++) {
            xoff = base + x/7;
            pixel = x%7;
            ptr = hgr_buf + xoff;

            if (buf[y*HGR_WIDTH + x] != 0x00) {
              ptr[0] |= dhwmono[pixel];
            } else {
              ptr[0] &= dhbmono[pixel];
            }
          }
        }

        /* Count changes */
        // for (x = 0; x < 0x2000; x++) {
        //   if (hgr_buf[x] != prev_hgr_buf[x]) {
        //     changes++;
        //     if (changes > MAX_BYTES_PER_FRAME/2) {
        //       break;
        //     }
        //   }
        // }
        //
        // /* Filter out single byte changes */
        // if (changes > MAX_BYTES_PER_FRAME/2) {
        //   for (y = 0; y < HGR_HEIGHT; y++) {
        //     base = baseaddr[y];
        //     for (x = 0; x < 40; x++) {
        //       int prev_line_xoff, next_line_xoff;
        //       xoff = base + x;
        //       prev_line_xoff = y > 0 ? baseaddr[y-1] + x : 0;
        //       next_line_xoff = y < HGR_HEIGHT-1 ? baseaddr[y+1] + x : 0;
        //
        //       if (*(hgr_buf + xoff) != *(prev_hgr_buf + xoff)
        //        && byte_age[xoff] < 5
        //        && (x == 0            || *(hgr_buf + xoff - 1) == *(prev_hgr_buf + xoff - 1))
        //        && (x == 39           || *(hgr_buf + xoff + 1) == *(prev_hgr_buf + xoff + 1))
        //        && (y == 0            || *(hgr_buf + prev_line_xoff) == *(prev_hgr_buf + prev_line_xoff))
        //        && (y == HGR_HEIGHT-1 || *(hgr_buf + next_line_xoff) == *(prev_hgr_buf + next_line_xoff))) {
        //         LOG("frame %d single byte change at %04x, pixel age %d.\n", current_frame, xoff+0x2000, byte_age[xoff]);
        //         *(hgr_buf + xoff) = *(prev_hgr_buf + xoff);
        //         byte_age[xoff]++;
        //       } else {
        //         byte_age[xoff] = 0;
        //       }
        //     }
        //   }
        // }
        // memcpy(prev_hgr_buf, hgr_buf, 0x2000);
        return hgr_buf;
#else
        return buf;
#endif
    } else {
        LOG("done.\n");
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
        LOG("Cannot create audio buffer source\n");
        goto end;
    }

    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&audio_buffersink_ctx, abuffersink, "out",
                                       NULL, NULL, audio_filter_graph);
    if (ret < 0) {
        LOG("Cannot create audio buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio_buffersink_ctx, "sample_fmts", out_sample_fmts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOG("Cannot set output sample format\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio_buffersink_ctx, "channel_layouts", out_channel_layouts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOG("Cannot set output channel layout\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio_buffersink_ctx, "sample_rates", out_sample_rates, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOG("Cannot set output sample rate\n");
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

#define SUBS_BLOCK (3600*4*30)

int ffmpeg_subtitles_decode(decode_data *data, const char *filename) {
    int ret = 0, index;
    static AVFormatContext *ctx;
    static AVCodecContext *dec;
    const AVCodec *codec;
    AVPacket *packet = av_packet_alloc();
    unsigned long prev_end_frame = 0;

    ffmpeg_subtitles_free(data);

    if (packet == NULL) {
      ret = AVERROR(ENOMEM);
      goto end;
    }

    ctx = avformat_alloc_context();
    ctx->interrupt_callback.callback = open_interrupt_cb;
    ctx->interrupt_callback.opaque = data;

    if ((ret = avformat_open_input(&ctx, filename, NULL, NULL)) < 0) {
      LOG("Subtitles: Cannot open input file\n");
      goto end;
    }

    if ((ret = avformat_find_stream_info(ctx, NULL)) < 0) {
      LOG("Subtitles: Cannot find stream information\n");
      goto end;
    }

    /* select the stream */
    ret = av_find_best_stream(ctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, &codec, 0);
    if (ret < 0) {
      LOG("Subtitles: Cannot find a corresponding stream in the input file\n");
      goto end;
    }

    index = ret;

    dec = avcodec_alloc_context3(codec);
    if (!dec) {
      ret = AVERROR(ENOMEM);
      goto end;
    }

    avcodec_parameters_to_context(dec, ctx->streams[index]->codecpar);

    /* init the decoder */
    if ((ret = avcodec_open2(dec, codec, NULL)) < 0) {
      LOG("Subtitles: Cannot open decoder\n");
      goto end;
    }

    LOG("got subtitles stream\n");
    pthread_mutex_lock(&data->sub_mutex);
    data->nsubs = SUBS_BLOCK;
    data->subs = malloc(data->nsubs*sizeof(char *));
    memset(data->subs, 0, data->nsubs*sizeof(char *));
    pthread_mutex_unlock(&data->sub_mutex);
    while (1) {
      pthread_mutex_lock(&data->mutex);
      if (data->stop) {
        LOG("Stopping subtitles thread\n");
        pthread_mutex_unlock(&data->mutex);
        goto end;
      }
      pthread_mutex_unlock(&data->mutex);

skip:
      if ((ret = av_read_frame(ctx, packet)) < 0) {
        if (ret == AVERROR_EOF) {
          ret = 0;
        }
        break;
      }

      if (packet->stream_index == index) {
        AVSubtitle subtitle;
        int gotSub;
        float start, end;
        unsigned long start_frame, end_frame;
        const char *text = NULL;

        ret = avcodec_decode_subtitle2(dec, &subtitle, &gotSub, packet);

        if (gotSub) {
          size_t l;
          start = packet->pts * (1000.0*av_q2d(ctx->streams[index]->time_base))
                      + subtitle.start_display_time;
          end = (packet->pts + packet->duration) * (1000.0*av_q2d(ctx->streams[index]->time_base))
                      + subtitle.end_display_time;
          // LOG("start %f (pts %lu timebase %d/%d start_display_time %u)\n",
          //        start, packet->pts, ctx->streams[index]->time_base.num, ctx->streams[index]->time_base.den,
          //        subtitle.start_display_time);
          if (packet->data && packet->data[0] != '\0') {
            text = (char *)packet->data;
            //LOG("dat '%s'\n", text);
          } else {
            for (int i = 0; i < subtitle.num_rects; i++) {
              AVSubtitleRect *rect = subtitle.rects[i];
              if (rect->type == SUBTITLE_ASS) {
                text = rect->ass;
                //LOG("ass '%s'\n", text);
                for (int j = 0; j < 8; j++) {
                  text = strchr(text, ',');
                  if (!text) {
                    goto skip;
                  }
                  text++;
                }
                if (text[0] == ',') {
                  text++; /* FIXME that's ugly */
                }
              } else if (rect->type == SUBTITLE_TEXT) {
                text = rect->text;
                //LOG("srt '%s'\n", text);
              } else {
                continue;
              }
            }
          }
          if (text) {
            char *idx;
            /* Compute frames @25fps, we don't know real fps yet. */
            start_frame = (unsigned long)(start)/(1000.0/SUB_DEFAULT_FPS);
            end_frame = (unsigned long)(end)/(1000.0/SUB_DEFAULT_FPS);

            pthread_mutex_lock(&data->sub_mutex);

            if (start_frame >= data->nsubs || end_frame >= data->nsubs) {
              pthread_mutex_unlock(&data->sub_mutex);
              goto end;
            }

            /* Remove end of previous if it ends after this one starts */
            if (prev_end_frame >= start_frame && start_frame > 1) {
              LOG("moving previous subtitle end from %ld to %ld\n", prev_end_frame, start_frame-2);
              data->subs[start_frame-2] = data->subs[prev_end_frame];
              data->subs[prev_end_frame] = NULL;
            }

            while ((idx = strchr(text, '\r'))) {
              *idx = ' ';
            }
            while ((idx = strchr(text, '\n'))) {
              *idx = ' ';
            }
            while ((idx = strstr(text, "\\N"))) {
              *idx = ' ';
              *(idx+1) = '\n';
            }
            while ((idx = strchr(text, '\\'))) {
              *idx = ' ';
              if (*(idx+1) != '\0')
                *(idx+1) = ' ';
            }
            while ((idx = strstr(text, "&nbsp;"))) {
              *idx = ' ';
              memmove(idx+1, idx+6, strlen(idx)-5);
            }
            idx = strdup(text);
            data->subs[start_frame] = do_charset_convert(idx, OUTGOING,
                                        data->translit ? data->translit:"US_ASCII", 0, &l);
            data->subs[end_frame] = strdup("");
            LOG("sub frames %ld-%ld: %s\n", start_frame, end_frame, text);
            free(idx);
            prev_end_frame = end_frame;
            pthread_mutex_unlock(&data->sub_mutex);
          }
          avsubtitle_free(&subtitle);
        }
      }
      av_packet_unref(packet);
    }

end:
    data->has_subtitles = (prev_end_frame > 0);
    av_packet_free(&packet);
    avcodec_free_context(&dec);
    avformat_close_input(&ctx);

    return ret;
}

char *ffmpeg_get_subtitle_at_frame(decode_data *data, unsigned long frame) {
  char *sub = NULL;

  pthread_mutex_lock(&data->sub_mutex);
  if (data->subs != NULL && frame < data->nsubs && data->subs[frame]) {
    sub = strdup(data->subs[frame]);
  }
  pthread_mutex_unlock(&data->sub_mutex);
  return sub;
}

void ffmpeg_shift_subtitle_at_frame(decode_data *data, unsigned long frame) {
  pthread_mutex_lock(&data->sub_mutex);
  if (data->subs && frame < data->nsubs) {
    if (data->subs[frame] && frame + 2 < data->nsubs) {
      LOG("pushing sub %s to %ld\n", data->subs[frame], frame+2);
      data->subs[frame + 2] = data->subs[frame];
      data->subs[frame] = NULL;
    }
  }
  pthread_mutex_unlock(&data->sub_mutex);
}

static void update_title_locked(decode_data *data) {
  char* icy_metadata = NULL;

  /* Check for cast metadata */
  av_opt_get(audio_fmt_ctx, "icy_metadata_packet", AV_OPT_SEARCH_CHILDREN, (uint8_t**) &icy_metadata);

  if (icy_metadata != NULL) {
    char *new_title = icy_metadata;

    /* Cleanup cast title */
    if (strstr(icy_metadata, "StreamTitle='")) {
      new_title = strstr(icy_metadata, "StreamTitle='") + strlen("StreamTitle='");

      if (strstr(new_title, "';Stream"))
        *(strstr(new_title, "';Stream")) = '\0';
      else if (strchr(new_title, '\''))
        *(strrchr(new_title, '\'')) = '\0';

      while(strchr(new_title, '\\')) {
        *(strchr(new_title, '\\')) = ' ';
      }
    }

    /* Refresh cast title */
    if (data->title != NULL && strcmp(data->title, new_title)) {
      free(data->title);
      data->title = strdup(new_title);
      data->title_changed = 1;
    } else  if (data->title == NULL) {
      data->title = strdup(new_title);
      data->title_changed = 1;
    }
  }

  av_free(icy_metadata);
  icy_metadata = NULL;
}

int ffmpeg_audio_decode(decode_data *data) {
    int ret = 0;
    char audio_filter_descr[200];
    const AVDictionaryEntry *tag = NULL;
    const AVCodec *vdec;
    char has_video = 0;
    int first = 1;

    audio_frame = av_frame_alloc();
    audio_filt_frame = av_frame_alloc();
    audio_packet = av_packet_alloc();

    pthread_mutex_lock(&data->mutex);
    data->data_ready = 0;
    data->data = NULL;
    data->size = 0;
    data->img_data = NULL;
    data->img_size = 0;
    data->max_audio_volume = 0;
    pthread_mutex_unlock(&data->mutex);

    if (!audio_frame || !audio_filt_frame || !audio_packet) {
      fprintf(stderr, "Could not allocate frame or packet\n");
      ret = -ENOMEM;
      goto end;
    }

    if ((ret = open_audio_file(data)) < 0) {
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

        LOG("Pushing image\n");
        pthread_mutex_lock(&data->mutex);
        data->img_data = malloc(img.size);
        memcpy(data->img_data, img.data, img.size);
        data->img_size = img.size;
        pthread_mutex_unlock(&data->mutex);
      }
    }

    if (data->artist == NULL || data->img_data == NULL) {
        char *icy_header = NULL;
        av_opt_get(audio_fmt_ctx, "icy_metadata_headers", AV_OPT_SEARCH_CHILDREN, (uint8_t**) &icy_header);
        if (icy_header != NULL) {
          LOG("ICY headers: %s\n", icy_header);

          /* Cast name */
          if (strcasestr(icy_header, "\nicy-name: ") != NULL) {
            data->artist = strdup(strcasestr(icy_header, "\nicy-name: ") + strlen("\nicy-name: "));
            if (strchr(data->artist, '\r'))
              *(strchr(data->artist, '\r')) = '\0';
            if (strchr(data->artist, '\n'))
              *(strchr(data->artist, '\n')) = '\0';
          }

          /* Cast logo */
          if (strcasestr(icy_header, "\nicy-logo: ") != NULL) {
            char *art_url = strdup(strcasestr(icy_header, "\nicy-logo: ") + strlen("\nicy-logo: "));
            CURL *curl_handle;
            CURLcode res;

            if (strchr(art_url, '\r'))
              *(strchr(art_url, '\r')) = '\0';
            if (strchr(art_url, '\n'))
              *(strchr(art_url, '\n')) = '\0';

            curl_handle = curl_easy_init();
            curl_easy_setopt(curl_handle, CURLOPT_URL, art_url);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_art_cb);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, data);
            res = curl_easy_perform(curl_handle);

            if (res != CURLE_OK) {
              data->img_data = NULL;
              data->img_size = 0;
              LOG("Failed fetching %s\n", art_url);
            }
            curl_easy_cleanup(curl_handle);
            free(art_url);
          }

          av_free(icy_header);
          icy_header = NULL;
        }
    }

    /* read all packets */
    while (1) {
        if ((ret = av_read_frame(audio_fmt_ctx, audio_packet)) < 0)
            break;

        if (audio_packet->stream_index == audio_stream_index) {
            ret = avcodec_send_packet(audio_dec_ctx, audio_packet);
            if (ret < 0) {
                LOG("Error while sending a packet to the decoder\n");
                av_packet_unref(audio_packet);
                continue;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(audio_dec_ctx, audio_frame);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                } else if (ret == AVERROR_EOF) {
                  LOG("Last frame...\n");
                } else if (ret < 0) {
                    LOG("Error while receiving a frame from the decoder\n");
                    goto end;
                }

                audio_frame->pts = audio_frame->best_effort_timestamp;

                if (ret >= 0) {
                    /* push the audio data from decoded frame into the filtergraph */
                    if (av_buffersrc_add_frame_flags(audio_buffersrc_ctx, audio_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        LOG("Error while feeding the audio filtergraph\n");
                        break;
                    }

                    /* pull filtered audio from the filtergraph */
                    while (1) {
                        ret = av_buffersink_get_frame(audio_buffersink_ctx, audio_filt_frame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            break;
                        if (ret < 0)
                            goto end;

                        if (first) {
                          AVRational time_base = audio_fmt_ctx->streams[audio_stream_index]->time_base;
                          data->pts = (float)audio_filt_frame->pts * 1000.0 * av_q2d(time_base);
                          first = 0;
                          LOG("audio frame pts %ld duration %ld timebase %d/%d %f\n",
                          data->pts, audio_filt_frame->pkt_duration, time_base.num, time_base.den, av_q2d(time_base));
                        }

                        pthread_mutex_lock(&data->mutex);
                        if (data->stop) {
                          LOG("Parent thread requested stopping\n");
                          pthread_mutex_unlock(&data->mutex);
                          goto end;
                        }

                        if (data->size == 0) {
                          LOG("Started pushing data\n");
                        }

                        data->data = (unsigned char*) realloc(data->data,
                                  (data->size + audio_filt_frame->nb_samples) * sizeof(unsigned char));
                        memcpy(data->data + data->size,
                               audio_filt_frame->data[0], audio_filt_frame->nb_samples * sizeof(unsigned char));
                        data->size += audio_filt_frame->nb_samples;
                        for (int i = 0; i < audio_filt_frame->nb_samples; i++) {
                          if (audio_filt_frame->data[0][i] > data->max_audio_volume)
                            data->max_audio_volume = audio_filt_frame->data[0][i];
                        }
                        data->data_ready = 1;

                        update_title_locked(data);

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
    LOG("Audio decoding finished at %zu\n", data->size);
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
        LOG("Error occurred: %s\n", av_err2str(ret));

    return ret;
}
