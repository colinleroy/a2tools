#include <fcntl.h>
#include <unistd.h>

void load_bg(void) {
  int fd = open("BG1", O_RDONLY);
  read(fd, (void *)0x2000, 0x2000);
  close(fd);
}
