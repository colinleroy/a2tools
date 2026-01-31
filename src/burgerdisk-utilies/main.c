#include <conio.h>

#include "citoa.h"
#include "smartport.h"

void main(void) {
  unsigned char slot, unit;
  unsigned char *buffer;
  unsigned long device_blocks;

  for (slot = 7; slot > 0; slot--) {
    if (is_slot_smartport(slot)) {
      cputs("Slot ");
      cutoa(slot);
      cputs(": SmartPort device\r\n");
      for (unit = 1; unit < 8; unit++) {
        buffer = smartport_get_status(slot, unit, SP_STATUS_DIB);
        if (buffer) {
          cputs(" Unit ");
          cutoa(unit);
          cputs(" size ");
          cutoa(smartport_dev_size(buffer)/1024);
          cputs("kB - ");
          cputs(smartport_dev_name(buffer));
          cputs("\r\n");
        }
      }
    }
  }
  cgetc();
}
