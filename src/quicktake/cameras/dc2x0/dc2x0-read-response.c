#include "a2_features.h"
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "dc2x0.h"
#include "dc2x0-read-response.h"
#include "../pc-debug.h"
#include "../qt-serial.h"
#include "../../ui/ui.h"

#pragma code-name(push, "DC2X0")
#pragma rodata-name(push, "DC2X0")
#pragma data-name(push, "DC2X0")
#pragma bss-name(push, "DC2X0")

static uint8 c;

/* Read a reply from the camera */
uint8 dc2x0_read_response(char *dest, uint16 len) {
  if (serial_read_byte_no_irq() != 0x01) {
    PC_DEBUG_PRINTF("Unexpected header 0x%02X", c);
    return -1;
  }
  if (simple_serial_read_no_irq(dest, len) == EOF) {
    PC_DEBUG_BUFFER("Short data", dest, len);
    return -1;
  }
  PC_DEBUG_BUFFER("Data", dest, len);
  /* Checksum */
  c = serial_read_byte_no_irq();
  PC_DEBUG_PRINTF("checksum %02X\n", c);

  return 0;
}
