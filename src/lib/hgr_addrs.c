
#include "platform.h"
#include "hgr.h"
#include "hgr_addrs.h"

uint8 *hgr_baseaddr[HGR_HEIGHT];
uint8 div7_table[HGR_WIDTH];
uint8 mod7_table[HGR_WIDTH];

void init_hgr_base_addrs (void)
{
  static uint8 y, base_init_done = 0;
  uint16 x, group_of_eight, line_of_eight, group_of_sixtyfour;
  uint8 *a, *b;
  if (base_init_done) {
    return;
  }

  /* Fun with HGR memory layout! */
  for (y = 0; y < HGR_HEIGHT; ++y)
  {
    line_of_eight = y % 8;
    group_of_eight = (y % 64) / 8;
    group_of_sixtyfour = y / 64;

    hgr_baseaddr[y] = (uint8 *)HGR_PAGE + line_of_eight * 1024 + group_of_eight * 128 + group_of_sixtyfour * 40;
  }

  /* Precompute /7 and %7 from 0 to HGR_WIDTH */
  a = div7_table + 0;
  b = mod7_table + 0;
  for (x = 0; x < HGR_WIDTH; x++) {
    *a = x / 7;
    *b = 1 << (x % 7);
    a++;
    b++;
  }
  base_init_done = 1;
}
