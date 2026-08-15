#ifndef qt_thumbs_h
#define qt_thumbs_h

#define THUMB_WIDTH 80
#define THUMB_HEIGHT 60

/* The fastest way to iterate when dithering is by dey for full-size pictures.
 * This means X for thumbnails start at 256-160. Reflect that when loading
 * thumbnail data to the buffer. */
#define THUMBNAIL_BUFFER_OFFSET ((256-160)/2)
#define THUMBNAIL_BUF_START (buffer+THUMBNAIL_BUFFER_OFFSET)

extern uint8 thumb_buf[THUMB_WIDTH * 2];

#ifdef __CC65__

  #define cur_histogram zp6ip
  #define cur_opt_histogram zp8p
  #define cur_thumb_data zp10p
  extern int8 err_buf[512+2];
  extern uint8 opt_histogram[256];

#else

  extern uint8 *cur_thumb_data;
  extern int8 err_buf[512+2];
  extern uint16 histogram[256];
  extern uint8 opt_histogram[256];

#endif

void __fastcall__ thumb_histogram_qt1x0(void);
void __fastcall__ thumb_histogram_qt200(void);

#endif
