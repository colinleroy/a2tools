#include "a2_features.h"
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "dc50.h"
#include "dc50-read-response.h"
#include "../qt-serial.h"
#include "../../ui/ui.h"

#pragma code-name(push, "DC50")
#pragma rodata-name(push, "DC50")
#pragma data-name(push, "DC50")
#pragma bss-name(push, "DC50")

extern uint16 response_len;
extern uint8 response_continues;

#ifdef __CC65__
#define PC_DEBUG(op, str, len)
#else
static void PC_DEBUG(char *op, const char *str, int len) {
  if (do_debug) {
    printf("%s:", op);
    for (int i = 0; i < len; i++) {
      printf("%s %02X", i%16 == 0 ? "\n":"", (uint8)str[i]);
    }
    printf("\n");
  }
}
#endif

/* Read a reply from the camera */
uint8 dc50_read_response(char *dest, uint16 len) {
  int8 c;

  if (len == 0) {
    return 0;
  }

  // bzero(buffer, sizeof buffer);
  c = simple_serial_read_no_irq(dest, len);
  if (c == EOF) {
    return -1;
  }
  PC_DEBUG("Data", dest, len);
  /* Checksum */
  simple_serial_read_no_irq((char *)&c, 1);
  PC_DEBUG("checksum", &c, 1);

  return 0;
}
