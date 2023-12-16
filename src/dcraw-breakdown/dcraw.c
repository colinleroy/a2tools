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

#define getbits(n) getbithuff(n,0)

unsigned getbithuff (int nbits, uint16 *huff)
{
  static unsigned bitbuf=0;
  static int vbits=0, reset=0;
  unsigned c;

  if (nbits > 25) return 0;
  if (nbits < 0)
    return bitbuf = vbits = reset = 0;
  if (nbits == 0 || vbits < 0) return 0;
  while (!reset && vbits < nbits && (c = fgetc(ifp)) != EOF &&
    !(c == 0xff && fgetc(ifp))) {
    bitbuf = (bitbuf << 8) + (uint8) c;
    vbits += 8;
  }
  c = bitbuf << (32-vbits) >> (32-nbits);
  if (huff) {
    vbits -= huff[c] >> 8;
    c = (uint8) huff[c];
  } else
    vbits -= nbits;
  if (vbits < 0) exit(1);
  return c;
}

void quicktake_100_load_raw(uint16 top)
{
  uint8 pixel[BAND_HEIGHT+4][WIDTH+4];
  static const short gstep[16] =
  { -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
  static const short rstep[6][4] =
  { {  -3,-1,1,3  }, {  -5,-1,1,5  }, {  -8,-2,2,8  },
    { -13,-3,3,13 }, { -19,-4,4,19 }, { -28,-6,6,28 } };
  static const short curve[256] =
  { 0,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
    28,29,30,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,53,
    54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,74,75,76,77,78,
    79,80,81,82,83,84,86,88,90,92,94,97,99,101,103,105,107,110,112,114,116,
    118,120,123,125,127,129,131,134,136,138,140,142,144,147,149,151,153,155,
    158,160,162,164,166,168,171,173,175,177,179,181,184,186,188,190,192,195,
    197,199,201,203,205,208,210,212,214,216,218,221,223,226,230,235,239,244,
    248,252,257,261,265,270,274,278,283,287,291,296,300,305,309,313,318,322,
    326,331,335,339,344,348,352,357,361,365,370,374,379,383,387,392,396,400,
    405,409,413,418,422,426,431,435,440,444,448,453,457,461,466,470,474,479,
    483,487,492,496,500,508,519,531,542,553,564,575,587,598,609,620,631,643,
    654,665,676,687,698,710,721,732,743,754,766,777,788,799,810,822,833,844,
    855,866,878,889,900,911,922,933,945,956,967,978,989,1001,1012,1023 };
  int rb, row, col, sharp, val=0;
  int first_line = 0, cnt;

  if (top == 0) {
    getbits(-1);
    memset (pixel, 0x80, sizeof pixel);
    first_line = 1;
  } else {
    /* Copy the two last computed lines of the last band */
    for (row = 0; row < 2; row++) {
      memcpy(pixel[row], pixel[row+BAND_HEIGHT], WIDTH+4);
    }

    /* Copy the two last computed pixels of the last band.
     * They're set in the first loop below, if col < 4. If
     * we don't copy them, a clear diagonal haze descends
     * from the top left corner of the picture. */
    pixel[2][0] = pixel[BAND_HEIGHT+2][0];
    pixel[2][1] = pixel[BAND_HEIGHT+2][1];
    memset(pixel[2] + 2, 0x80, WIDTH+2);

    /* Reset the rest to grey */
    for (row = 3; row < BAND_HEIGHT+2; row++) {
      memset(pixel[row], 0x80, WIDTH+4);
    }
  }
  for (row=2; row < BAND_HEIGHT+2; row++) {
    int first_col = 1;
    cnt = 0;
    for (col=2+(row & 1); col < WIDTH+2; col+=2) {
      val = ((pixel[row-1][col-1] + 2*pixel[row-1][col+1] +
                pixel[row][col-2]) >> 2) + gstep[getbits(4)];

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
  for (row=2; row < BAND_HEIGHT+2; row++)
    for (col=3-(row & 1); col < WIDTH+2; col+=2) {
      val = ((pixel[row][col-1] + (pixel[row][col] << 2) +
              pixel[row][col+1]) >> 1) - 0x100;
      pixel[row][col] = LIM(val,0,255);
    }
   for (row=0; row < BAND_HEIGHT; row++)
     for (col=0; col < WIDTH; col++)
       RAW(row,col) = pixel[row+2][col+2];
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
