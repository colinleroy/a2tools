#include <conio.h>
#include <device.h>

#include "citoa.h"
#include "smartport.h"

void main(void) {
  unsigned char slot, unit_count, unit;
  smartport_status_response status_buf;
  smartport_error response;
  slot = getfirstsmartportslot();
  while (slot != INVALID_DEVICE) {
    cputs("Slot ");
    cutoa(slot);
    cputs(": SmartPort device, ");
    unit_count = smartportgetunitcount(slot);
    cutoa(unit_count);
    cputs(" units:\r\n");
    for (unit = 1; unit <= unit_count+2; unit++) {
      response = smartport_get_status(slot, unit, SP_DEVICE_INFORMATION_BLOCK, &status_buf);
      cputs(" Unit ");
      cutoa(unit);
      if (response == SP_OK) {
        cputs(" size ");
        cutoa(smartport_unit_size(&status_buf)/1024);
        cputs("kB - ");
        cputs(smartport_unit_name(&status_buf));
      } else {
        cputs(" - error ");
        cutoa(response);
      }
      cputs("\r\n");
    }
    slot = getnextsmartportslot(slot);
  }
  cgetc();
}
