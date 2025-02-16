#include "hgr_addrs.h"
#include "extrazp.h"

extern char *palette[8];

#define sprite zp6p
#define line   zp8p

void draw(char x, char y) {
  char sprite_num = x%8;
  
  char cur_y = y;
  char end_y = y + 15;
  char cur_byte = 0;

  sprite = palette[sprite_num];
  while(cur_y < end_y) {
    char i;
    
    line = hgr_baseaddr[cur_y] + (x/8);
    for (i = 0; i < 6; i++) {
      line[i] = sprite[cur_byte++];
    }
    cur_y++;
  }
}
