#include "extended_conio.h"
#include "platform.h"

void ui_get_image_header_str(void) {
  cputs("  Getting header...");
}

void ui_get_thumbnail_str(uint8 n) {
  gotox(0);
  cprintf("  Getting thumbnail %d...\r\n", n);
}

void ui_get_image_str(uint16 width, uint16 height, unsigned long size) {
  gotox(0);
  cprintf("  Getting image...\r\n"
          "  Width %u, height %u, %lu bytes\r\n",
          width, height, size);
}
