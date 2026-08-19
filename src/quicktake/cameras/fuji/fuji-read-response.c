#include "a2_features.h"
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "fuji.h"
#include "fuji-read-response.h"
#include "../qt-serial.h"
#include "../../ui/ui.h"

extern uint16 response_len;
extern uint8 response_continues;

/* Read a reply from the camera */
uint8 fuji_read_response(void) {
  uint8 eot_buf[3];
  uint16 i, j;

  /* Read the header */
  if (simple_serial_read_no_irq((char *)buffer, 6) == EOF) {
    if (do_debug) {
      cputs("Timeout reading response.\r\n");
    }
    return -1;
  }

  if (buffer[0] != ESC || buffer[1] != STX) {
    return -1;
  }

  response_len = (buffer[5] << 8) | buffer[4];

  i = 0;
  j = response_len; /* Don't change response_len, callers use it */
  while (j) {
    buffer[i] = serial_read_byte_no_irq();
    if (buffer[i] == ESC) {
      /* Skip escape */
      buffer[i] = serial_read_byte_no_irq();
    }
    i++;
    j--;
  }

  /* Read footer */
  simple_serial_read_no_irq((char *)eot_buf, 3);

  /* If cur_buf[1] == ETB, there will be more to read */
  response_continues = (eot_buf[1] == ETB);
  return 0;
}
