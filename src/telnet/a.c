#include "extended_conio.h"
#include <stdio.h>

void __fastcall__ dputs (const char* s);
void __fastcall__ dputsxy (unsigned char x, unsigned char y, const char* s);
void __fastcall__ dputcxy (unsigned char x, unsigned char y, char c);
void __fastcall__ dputc (char c);

int main(void) {
  int i, j = 0;
  char buf[10];
  char *str[2] = {
    "123456789012345678901234567890123456789012345678901234567890123X\10U!\r\n",
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkX\10l!\r\n"
  };

  videomode(VIDEOMODE_80COL);
  clrscr();
  while (1) {
    unsigned char cur_x, cur_y;
    gotoxy(0,0);
    j = 1 - j;
    for (i = 0; i < 30; i++) {
      // cur_x = wherex();
      // cur_y = wherey();
      sprintf(buf, "%d,%d",wherex(),wherey());
      // dputsxy(74, 0, buf);
      // gotoxy(cur_x, cur_y);
      dputs(buf);
      dputs(str[j] + i);
      cgetc();
      // cur_y = wherey();
      // gotox(0);
      // if (cur_y < 23)
      //   gotoy(++cur_y);
      // else
      //   printf("\n");
    }
    cgetc();
  }
  return 0;
}
