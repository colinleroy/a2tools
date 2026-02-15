#include <tgi.h>
#include <conio.h>
#include <string.h>
#include <unistd.h>
#include <mouse.h>

extern void flip_page_after_vbl(void);

void main(void) {
  memset(0x2000, 0x00, 0x2000);
  memset(0x4000, 0xff, 0x2000);

  tgi_install (a2_hi_tgi);
  tgi_init ();

  mouse_install (&mouse_def_callbacks, mouse_static_stddrv);

  while (1) {
    flip_page_after_vbl();
    if (kbhit()) {
      break;
    }
  }
  mouse_uninstall();
}
