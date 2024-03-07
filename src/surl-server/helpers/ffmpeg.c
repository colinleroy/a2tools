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
#include <libswresample/swresample.h>

#include <ffmpeg.h>

/* Final buffer size, possibly including black borders */
#define WIDTH 280
#define HEIGHT 192

/* Video size */
#define VIDEO_SIZE "140:96"

#define GREYSCALE_TO_HGR 1
/* ffmpeg settings: Get a known fps rate, and scale to size.
 * if scaling is smaller than buffer size, we'll center the video.
 */
const char *video_filter_descr = "fps="FPS_STR",scale="VIDEO_SIZE":force_original_aspect_ratio=decrease:flags=neighbor";

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int stream_index = -1;
AVPacket *packet;
AVFrame *frame;
AVFrame *filt_frame;
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

static int open_file(const char *filename, enum AVMediaType type)
{
    const AVCodec *dec;
    int ret;

    init_base_addrs();

    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the stream */
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a corresponding stream in the input file\n");
        return ret;
    }
    stream_index = ret;

    /* create decoding context */
    dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx)
        return AVERROR(ENOMEM);
    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[stream_index]->codecpar);

    /* init the decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(const char *filters_descr)
{
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = fmt_ctx->streams[stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
            time_base.num, time_base.den,
            dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
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
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}


void ffmpeg_to_hgr_deinit(void) {
    av_packet_unref(packet);
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);
    av_packet_free(&packet);
}


static int frameno = 0;
uint8_t out_buf[WIDTH*HEIGHT];

static uint8_t *dither_and_center_frame(const AVFrame *frame, AVRational time_base)
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
  return out_buf;
}

int ffmpeg_to_hgr_init(const char *filename, int *video_len) {
    int ret = 0;

    frame = av_frame_alloc();
    filt_frame = av_frame_alloc();
    packet = av_packet_alloc();

    if (!frame || !filt_frame || !packet) {
        fprintf(stderr, "Could not allocate frame or packet\n");
        ret = -ENOMEM;
        goto end;
    }

    if ((ret = open_file(filename, AVMEDIA_TYPE_VIDEO)) < 0) {
        goto end;
    }
    if ((ret = init_filters(video_filter_descr)) < 0) {
        goto end;
    }

    printf("Duration %lu\n", fmt_ctx->duration/1000000);
    *video_len = fmt_ctx->duration/1000000;
end:
    if (ret < 0)
        printf("Error occurred: %s\n", av_err2str(ret));

    return ret;
}

uint8_t hgr[0x2000];
unsigned char *ffmpeg_convert_frame(void) {
    uint8_t *buf = NULL;
    int ret;

    while (1) {
        /* read one packets */
        if (av_read_frame(fmt_ctx, packet) < 0)
            goto end;

        if (packet->stream_index == stream_index) {
            ret = avcodec_send_packet(dec_ctx, packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                goto end;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
                    goto end;
                }

                frame->pts = frame->best_effort_timestamp;

                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    goto end;
                }

                /* pull filtered frames from the filtergraph */
                while (1) {
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0) {
                        printf("Error: %s\n", av_err2str(ret));
                        goto end;
                    }
                    buf = dither_and_center_frame(filt_frame, buffersink_ctx->inputs[0]->time_base);
                    av_frame_unref(filt_frame);
                }
                av_frame_unref(frame);
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

int ffmpeg_to_raw_snd(const char *filename, int sample_rate, unsigned char **data, size_t *size) {
    int ret = 0;
    const AVCodecParameters *codec;
    struct SwrContext* swr = swr_alloc();

    frame = av_frame_alloc();
    filt_frame = av_frame_alloc();
    packet = av_packet_alloc();

    if (!frame || !filt_frame || !packet) {
        fprintf(stderr, "Could not allocate frame or packet\n");
        ret = -ENOMEM;
        goto end;
    }

    if ((ret = open_file(filename, AVMEDIA_TYPE_AUDIO)) < 0) {
        goto end;
    }

    codec = fmt_ctx->streams[stream_index]->codecpar;

    av_opt_set_int(swr, "in_channel_count",  codec->channels, 0);
    av_opt_set_int(swr, "out_channel_count", 1, 0);
    av_opt_set_int(swr, "in_channel_layout",  codec->channel_layout, 0);
    av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
    av_opt_set_int(swr, "in_sample_rate", codec->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate", sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt",  codec->format, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_U8,  0);
    swr_init(swr);

    printf("Duration %lu (%d channels, %d sample rate)\n", fmt_ctx->duration/1000000, codec->channels, codec->sample_rate);

    while (av_read_frame(fmt_ctx, packet) >= 0) {
      // decode one frame

      if (packet->stream_index == stream_index) {
        ret = avcodec_send_packet(dec_ctx, packet);
        if (ret < 0) {
            printf("Error while sending a packet to the decoder\n");
            goto end;
        }

        while (ret >= 0) {
          unsigned char *buffer;

          ret = avcodec_receive_frame(dec_ctx, frame);
          if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
              printf("Error while receiving a frame from the decoder\n");
              goto end;
          }
          // resample frames
          av_samples_alloc((unsigned char**) &buffer, NULL, 1, frame->nb_samples, AV_SAMPLE_FMT_U8, 0);
          int frame_count = swr_convert(swr, (unsigned char**) &buffer, frame->nb_samples, (const unsigned char**) frame->data, frame->nb_samples);
          // append resampled frames to data
          *data = (unsigned char*) realloc(*data, (*size + frame->nb_samples) * sizeof(unsigned char));
          memcpy(*data + *size, buffer, frame_count * sizeof(unsigned char));
          *size += frame_count;

          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            ret = 0;
            break;
          }
        }
      }
    }
 
    // clean up
    swr_free(&swr);
    av_packet_unref(packet);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);
    av_packet_free(&packet);

end:
    if (ret < 0)
        printf("Error occurred: %s\n", av_err2str(ret));

    return ret;
}
