#include <conio.h>
#include <device.h>
#include <dio.h>
#include <string.h>
#include <unistd.h>

#include "citoa.h"
#include "smartport.h"
#include "get_dev_from_path.h"

void main(void) {
  unsigned char slot, unit_count, unit;
  smartport_status_response status_buf;
  smartport_error response;
  char dev;
  dhandle_t dev_handle;
  size_t r = 0;
  char *data = NULL, *verif_data = NULL;
  unsigned int w;
  size_t i, num_blocks;

  dev = get_dev_from_path("/TEST");
  if (dev == INVALID_DEVICE) {
    cputs("Error finding device");
    cgetc();
  }
  dev_handle = dio_open(dev);
  if (dev_handle == NULL) {
    cputs("Error finding device");
    cgetc();
  }

  sleep(2);
  num_blocks = dio_query_sectcount(dev_handle);

  data = malloc(512);
  verif_data = malloc(512);

  /* Keep ProDOS index */
  for (i = 3; i < num_blocks; i++) {
    for (w = 0; w < 512; w++) {
      data[w] = (unsigned char)(w+i);
    }
    if (dio_write(dev_handle, i, data) != 0) {
      cputs("Error writing ");
      cutoa(i);
      cgetc();
    }
    if (dio_read(dev_handle, i, verif_data) != 0) {
      cputs("Error reading ");
      cutoa(i);
      cgetc();
    }
    if (memcmp(data, verif_data, 512)) {
      cputs("Error verifying ");
      cutoa(i);
      cgetc();
    }
    if (i % 256 == 0) {
      cutoa(i);
      cputs("\r\n");
    }
  }

  cputs("Done");
  cgetc();


  slot = getfirstsmartportslot();
  while (slot != INVALID_DEVICE) {
    cputs("Slot ");
    cutoa(slot);
    cputs(": SmartPort device, ");
    unit_count = smartportgetunitcount(slot);
    cutoa(unit_count);
    cputs(" units:\r\n");
    for (unit = 1; unit <= unit_count; unit++) {
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
