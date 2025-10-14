#ifndef __qt_conv_h
#define __qt_conv_h

#include "platform.h"
#include "extended_conio.h"
#include "extrazp.h"

extern uint8 cache[];
extern uint8 *cache_start;
extern uint8 *cache_end;

extern int ifd;

#ifdef __CC65__
/* Can't use zp* as they're used by codecs,
 * but can use serial-reserved ZP vars as
 * we don't do serial */
#define cur_cache_ptr prev_ram_irq_vector
#else
extern uint8 *cur_cache_ptr;
#endif

#define RAW_IMAGE_SIZE ((BAND_HEIGHT) * RAW_WIDTH)

#define QTKT_MAGIC      "qktk"
#define QTKN_MAGIC      "qktn"
#define JPEG_EXIF_MAGIC "\377\330\377\341"

#define FILE_HEIGHT HGR_HEIGHT
#define THUMB_WIDTH 80
#define THUMB_HEIGHT 60

#ifdef __CC65__

//Save temp file to HD instead of RAM when debugging decoders
#ifdef DEBUG_HD
#define TMP_NAME "/HD/GREY"
#else
#define TMP_NAME "/RAM/GREY"
#endif

#define HIST_NAME "/RAM/HIST"
#define THUMBNAIL_NAME "/RAM/THUMBNAIL"
#else
#define TMP_NAME "GREY"
#define HIST_NAME "HIST"
#define THUMBNAIL_NAME "THUMBNAIL"
#endif

extern uint16 height, width;
extern uint8 raw_image[];
extern uint16 raw_width, raw_image_size;

extern char magic[5];
extern char *model;

void qt_load_raw(uint16 top);

#endif
