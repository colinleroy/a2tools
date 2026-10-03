#include <string.h>
#include "dct_data.h"
#include "../qt-conv.h"
#include "extended_conio.h"

#define DATASIZE_IDX 0x186

uint32 data_size;

#ifdef __CC65__
void setup_floppy_restart(void);
#endif

char qt_setup_decode(void) {
  cur_cache_ptr = cache + INITIAL_CACHE_OFFSET;
  if (memcmp (cur_cache_ptr, DCT_MAGIC, 4)) {
err_out:
    cputs("Invalid file.\r\n");
    return -1;
  }

  data_size =
        ((uint32)cur_cache_ptr[DATASIZE_IDX]) |
        ((uint32)cur_cache_ptr[DATASIZE_IDX+1] << 8) |
        ((uint32)cur_cache_ptr[DATASIZE_IDX+2] << 16) |
        ((uint32)cur_cache_ptr[DATASIZE_IDX+3] << 24);

  cur_cache_ptr += (0x200);

#ifdef __CC65__
  cache_read = cur_cache_ptr;
  setup_floppy_restart();
#endif
  bits_table = normal_bits;
  shift_table = normal_shift;
  actual_width = width = 320;
  height = 240;

  switch (data_size) {
  case 24000:
    actual_width = 160;
    blocks_per_row = 20;
    total_blocks = 600;
    blocks_per_band = 600/(DECODE_HEIGHT/BAND_HEIGHT);
    break;
  case 192000:
    bits_table = superfine_bits;
    shift_table = superfine_shift;
    /* Fallthrough */
  case 96000:
    blocks_per_row = 40;
    total_blocks = 2400;
    blocks_per_band = 2400/(DECODE_HEIGHT/BAND_HEIGHT);
    break;
  default:
    goto err_out;
  }

  /* zero coef so we don't have to store zeroes in the
   * always-zero coefficients for normal/fine pictures
   */
  bzero(coef, sizeof(coef));
  return 0;
}
