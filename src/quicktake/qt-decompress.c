#include <zx02.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "platform.h"

uint8 zx02_decompress_in_place(const char *filename, char *destination_start, char *destination_end) {
  int fd = open(filename, O_RDONLY);
  size_t comp_size;
  char *comp_start;

  if (fd == -1) {
    return -1;
  }
  comp_size = read(fd, (char *)destination_start, (size_t)destination_end-(size_t)destination_start);
  close(fd);

  comp_start = (char *)(destination_end-comp_size);
  memmove(comp_start, destination_start, comp_size);
  decompress_zx02(comp_start, destination_start);

  return 0;
}
