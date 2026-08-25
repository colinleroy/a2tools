#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "../qt-conv.h"

#define WH_OFFSET 544

extern uint8 kodak_cbpp;

#define EXIF_ENTRY_SIZE 12

#ifndef __CC65__
#define READ32(value, offset) do { value = ((cache[offset]<<24)|(cache[offset+1]<<16)|(cache[offset+2]<<8)|(cache[offset+3])); } while (0)
#define READ16(value, offset) do { value = ((cache[offset]<<8)|(cache[offset+1])); } while (0)
#else
#define READ32(value, offset) do {                \
  ((unsigned char *)&value)[0] = cache[offset+3]; \
  ((unsigned char *)&value)[1] = cache[offset+2]; \
  ((unsigned char *)&value)[3] = cache[offset+0]; \
  ((unsigned char *)&value)[2] = cache[offset+1]; \
} while (0)

#define READ16(value, offset) do {                \
  ((unsigned char *)&value)[0] = cache[offset+1]; \
  ((unsigned char *)&value)[1] = cache[offset+0]; \
} while (0)
#endif

static uint16 data_offset;

static void kdc_get_parameters(void) {
  uint32 ifd_offset;
  uint16 num_entries;
  uint16 cur_entry;
  uint8 found_subifd = 0;
  uint8 found_width = 0, found_height = 0;

  /* Default values */
  width = 0;
  height = 0;

  READ32(ifd_offset, 4);
  READ16(num_entries, 8);

search_exif:
  for (cur_entry = 0; cur_entry < num_entries; cur_entry++) {
    uint16 entry_offset = ifd_offset + sizeof(num_entries) + (EXIF_ENTRY_SIZE*cur_entry);
    uint32 value;
    uint16 tag;

    READ16(tag, entry_offset);
    if (!found_subifd) {
      /* Look for the SubIFDs field */
      if (tag != 0x014A) {
        continue;
      }
      READ32(value, entry_offset+8);
      ifd_offset = value;
      READ16(num_entries, ifd_offset);
      found_subifd = 1;
      goto search_exif;
    } else {
      /* We're now looking for width/height */
      READ32(value, entry_offset+8);
      switch(tag) {
      case 0x0100: READ16(width, entry_offset+8);       break;
      case 0x0101: READ16(height, entry_offset+8);      break;
      case 0x0111: READ32(data_offset, entry_offset+8); break;
      case 0x9102: READ32(value, entry_offset+8);
                   kodak_cbpp = cache[value+3];         break;
      }
    }
  }
}

char qt_setup_decode(void) {
  uint16 v;

  if (!memcmp (cache_start, QKTN_MAGIC, 4)) {
    width = 640/2;
    height = 480/2;
    ((unsigned char *)&v)[1] = cache[WH_OFFSET + 8];
    ((unsigned char *)&v)[0] = cache[WH_OFFSET + 9];
    if (v == 30)
      data_offset = 738;
    else
      data_offset = 736;
  } else if (!memcmp (cache_start, KDC_MAGIC, 4)) {
    kdc_get_parameters();
    if (width == 756 && height == 504) {
      width = 768/2;
      height = 512/2;
    }
    kodak_cbpp = kodak_cbpp == 243 ? 2 : 3;
  } else {
    cputs("Invalid file.\r\n");
    return -1;
  }
  lseek(ifd, data_offset, SEEK_SET);
  read(ifd, cur_cache_ptr = cache, CACHE_SIZE);

  return 0;
}
