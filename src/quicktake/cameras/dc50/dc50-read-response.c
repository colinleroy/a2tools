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

static uint8 c;

/* Read a reply from the camera */
uint8 dc50_read_response(char *dest, uint16 len) {
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
