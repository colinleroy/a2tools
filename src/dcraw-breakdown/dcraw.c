#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"

#define WIDTH 640
#define HEIGHT 480
#define BAND_HEIGHT 20
uint8 raw_image[WIDTH*BAND_HEIGHT];

FILE *ifp, *ofp;

#define RAW(row,col) \
        raw_image[(row)*WIDTH+(col)]
#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))

unsigned get_four_bits (void)
{
  static uint8 bitbuf = 0;
  static uint8 vbits = 0;

  if(vbits) {
    vbits--; 
    return bitbuf & 0x0F;
  } else {
    bitbuf = fgetc(ifp);
    vbits = 1;
    return (bitbuf) >> 4;
  }
}

void quicktake_100_load_raw(uint16 top)
{
  uint8 pixel[BAND_HEIGHT+4][WIDTH+4];
  static const short gstep[16] =
  { -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
  int row, col, val=0;
  int first_line = 0, cnt;

  if (top == 0) {
    memset (pixel, 0x80, sizeof pixel);
    first_line = 1;
  } else {
    /* Copy the two full last computed lines of the last band,
     * plus the two last computed pixels of the last band.
     * They're set in the first loop below, if col < 4. If
     * we don't copy them, a clear diagonal haze descends
     * from the top left corner of the picture.
     */
    memcpy(pixel[0], pixel[BAND_HEIGHT], 2*(WIDTH+4)+2);

    /* Reset the rest to grey */
    memset(pixel[2] + 2, 0x80, BAND_HEIGHT*(WIDTH+4)-2);
  }
  for (row=2; row < BAND_HEIGHT+2; row++) {
    int first_col = 1;
    cnt = 0;
    for (col=2+(row & 1); col < WIDTH+2; col+=2) {
      val = get_four_bits();
      val = ((pixel[row-1][col-1] + 2*pixel[row-1][col+1] +
                pixel[row][col-2]) >> 2) + gstep[val];

      pixel[row][col] = val = LIM(val,0,255);
      if (col < 4) {
        pixel[row][col-2] = pixel[row+1][~row & 1] = val;
      }
      if (first_line)
        pixel[row-1][col+1] = pixel[row-1][col+3] = val;
    }
    pixel[row][col] = val;
    first_line=0;
  }
  for (row=2; row < BAND_HEIGHT+2; row++) {
    for (col=3-(row & 1); col < WIDTH+2; col+=2) {
      val = ((pixel[row][col-1] + (pixel[row][col] << 2) +
              pixel[row][col+1]) >> 1) - 0x100;
      pixel[row][col] = LIM(val,0,255);
    }
  }

  for (row=0; row < BAND_HEIGHT; row++)
    memcpy(raw_image + (WIDTH*row), pixel[row+2]+2, WIDTH);
}

int main(int argc, char *argv[]) {
  uint16 h;
  ifp = fopen("test.qtk", "r");
  ofp = fopen("out.raw","w");
  
  fseek(ifp, 736, SEEK_SET);

  for (h = 0; h < HEIGHT; h+=BAND_HEIGHT) {
    quicktake_100_load_raw(h);
    fwrite(raw_image, 1, WIDTH*BAND_HEIGHT, ofp);
  }
  fclose(ifp);
  fclose(ofp);
}
