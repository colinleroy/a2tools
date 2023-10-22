#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "qt-conv.h"
#include "dgets.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

#define BUF_SIZE 64
static char imgname[BUF_SIZE];
char magic[5] = "????";

int main (int argc, const char **argv)
{

#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
  printf("Free: %zu/%zuB\n\n", _heapmaxavail(), _heapmemavail());
#endif

next_file:
  cputs("Enter image to convert: ");
  dget_text(imgname, BUF_SIZE, NULL, 0);

  if (imgname[0]) {
    FILE *fp = fopen(imgname, "rb");
    if (fp) {
      fread(magic, 1, 4, fp);
      magic[4] = '\0';
      fclose(fp);
    }
    if (!strcmp(magic, QT100_MAGIC)) {
      exec("qt100conv", imgname);
    } else if (!strcmp(magic, QT150_MAGIC)) {
      exec("qt150conv", imgname);
    } else {
      cputs("Unknown file type.\r\n");
    }
  }
  goto next_file;
  return 0;
}

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
