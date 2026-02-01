#include <conio.h>
#include <device.h>

#include "citoa.h"
#include "smartport.h"

void main(void) {
  unsigned char slot, unit_count, unit;
  unsigned char *buffer;
  unsigned long device_blocks;

  slot = getfirstsmartportslot();
  while (slot != INVALID_DEVICE) {
    cputs("Slot ");
    cutoa(slot);
    cputs(": SmartPort device, ");
    unit_count = smartportgetunitcount(slot);
    cutoa(unit_count);
    cputs(" units:\r\n");
    for (unit = 1; unit <= unit_count; unit++) {
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
    slot = getnextsmartportslot(slot);
  }
  cgetc();
}
