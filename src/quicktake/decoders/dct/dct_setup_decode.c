#include <string.h>
#include "dct_data.h"
#include "../qt-conv.h"
#include "extended_conio.h"

#define DATASIZE_IDX 0x186
#define HEADER_SIZE  0x200

uint8 data_size;

#ifdef __CC65__
void setup_floppy_restart(void);
#endif

char qt_setup_decode(void) {
  if (memcmp (cache + INITIAL_CACHE_OFFSET, DCT_MAGIC, 4)) {
err_out:
    cputs("Invalid file.\r\n");
    return -1;
  }

  /* Take a single byte to recognize size, it's enough:
   * 005DC0
   * 017700
   * 02EE00 */
  data_size = cache[DATASIZE_IDX + INITIAL_CACHE_OFFSET + 1];

  cur_cache_ptr = cache + INITIAL_CACHE_OFFSET + HEADER_SIZE;

#ifdef __CC65__
  cache_read = cur_cache_ptr;
  setup_floppy_restart();
#endif
  bits_table = normal_bits;
  shift_table = normal_shift;
  actual_width = width = 320;
  height = 240;

  switch (data_size) {
  case 0x5D: /* Normal picture of size 24000, 0x5DC0 */
    actual_width = 160;
    blocks_per_row = 20;
    total_blocks = 600;
    blocks_per_band = 600/(DECODE_HEIGHT/BAND_HEIGHT);
    break;
  case 0xEE: /* Superfine picture of size 192000, 0x02EE00 */
    bits_table = superfine_bits;
    shift_table = superfine_shift;
    /* Fallthrough */
  case 0x77: /* Fine picture of size 24000, 0x017700 */
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
