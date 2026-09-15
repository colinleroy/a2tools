#include "a2_features.h"
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "dc50.h"
#include "dc50-read-response.h"
#include "../pc-debug.h"
#include "../qt-serial.h"
#include "../../ui/ui.h"

#pragma code-name(push, "DC50")
#pragma rodata-name(push, "DC50")
#pragma data-name(push, "DC50")
#pragma bss-name(push, "DC50")

extern uint16 response_len;
extern uint8 response_continues;

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
  PC_DEBUG_BUFFER("Data", dest, len);
  /* Checksum */
  simple_serial_read_no_irq((char *)&c, 1);
  PC_DEBUG_PRINTF("checksum %02X\n", c);

  return 0;
}
