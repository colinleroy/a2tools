#include <tgi.h>
#include <conio.h>
#include <string.h>
#include <unistd.h>
#include <mouse.h>

extern void flip_page_after_vbl(void);
extern void init_mouse(void);
unsigned char freq;

void main(void) {
  memset(0x2000, 0x00, 0x2000);
  memset(0x4000, 0xff, 0x2000);

again:
  cputs("Frequency (0: 60Hz, 1: 50Hz)\r\n");
  freq = cgetc();
  if (freq < '0' || freq > '1') {
    goto again;
  }

  cputs("Installing mouse driver\r\n");
  init_mouse();
  cputs("Mouse driver installed\r\n");

  tgi_install (a2_hi_tgi);
  tgi_init ();

  while (1) {
    flip_page_after_vbl();
    if (kbhit()) {
      cgetc();
      break;
    }
  }
  tgi_done();
  cputs("Cleaning up\r\n");
}
