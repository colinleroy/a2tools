#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef __CC65__
#include <device.h>
#include <apple2.h>
#endif

char get_dev_from_path(const char *path) {
  char dev;
  char *buf = malloc(18);

  if (!buf) {
    return -1;
  }

  dev = getfirstdevice();
  do {
    if (getdevicedir(dev, buf, 17) == NULL) {
      continue;
    }
    if (!strcmp(path, buf)) {
      break;
    }
  } while ((dev = getnextdevice(dev)) != INVALID_DEVICE);

  free(buf);
  return dev;
}
