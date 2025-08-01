//------------------------------------------------------------------------------
// picojpeg.c v1.1 - Public domain, Rich Geldreich <richgel99@gmail.com>
// Nov. 27, 2010 - Initial release
// Feb. 9, 2013 - Added H1V2/H2V1 support, cleaned up macros, signed shift fixes
// Also integrated and tested changes from Chris Phoenix <cphoenix@gmail.com>.
//------------------------------------------------------------------------------
/*
 * Adapted in 2023 by Colin Leroy-Mira <colin@colino.net> for the Apple 2
 */

/* Handle pic by horizontal bands for memory constraints reasons.
 * Bands need to be a multiple of 4px high for compression reasons
 * on QT 150/200 pictures,
 * and a multiple of 5px for nearest-neighbor scaling reasons.
 * (480 => 192 = *0.4, 240 => 192 = *0.8)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "platform.h"
#include "progress_bar.h"
#include "qt-conv.h"
#include "picojpeg.h"
#include "extrazp.h"
#include "jpeg_platform.h"

/* Shared with qt-conv.c */
char magic[5] = JPEG_EXIF_MAGIC;
char *model = "200";
uint16 *huff_ptr;

#define N_STUFF_CHARS 4
uint8 cache[CACHE_SIZE + N_STUFF_CHARS];
uint8 *cache_start = cache + N_STUFF_CHARS;
uint8 raw_image[RAW_IMAGE_SIZE];

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 600)
#pragma register-vars(push, on)

//------------------------------------------------------------------------------
// Set to 1 if right shifts on signed ints are always unsigned (logical) shifts
// When 1, arithmetic right shifts will be emulated by using a logical shift
// with special case code to ensure the sign bit is replicated.

// Define PJPG_INLINE to "inline" if your C compiler supports explicit inlining
#define PJPG_INLINE
//------------------------------------------------------------------------------

#define PJPG_MAXCOMPSINSCAN 3
//------------------------------------------------------------------------------
typedef enum
{
   M_SOF0  = 0xC0,
   M_SOF1  = 0xC1,
   M_SOF2  = 0xC2,
   M_SOF3  = 0xC3,

   M_SOF5  = 0xC5,
   M_SOF6  = 0xC6,
   M_SOF7  = 0xC7,

   M_JPG   = 0xC8,
   M_SOF9  = 0xC9,
   M_SOF10 = 0xCA,
   M_SOF11 = 0xCB,

   M_SOF13 = 0xCD,
   M_SOF14 = 0xCE,
   M_SOF15 = 0xCF,

   M_DHT   = 0xC4,

   M_DAC   = 0xCC,

   M_RST0  = 0xD0,
   M_RST1  = 0xD1,
   M_RST2  = 0xD2,
   M_RST3  = 0xD3,
   M_RST4  = 0xD4,
   M_RST5  = 0xD5,
   M_RST6  = 0xD6,
   M_RST7  = 0xD7,

   M_SOI   = 0xD8,
   M_EOI   = 0xD9,
   M_SOS   = 0xDA,
   M_DQT   = 0xDB,
   M_DNL   = 0xDC,
   M_DRI   = 0xDD,
   M_DHP   = 0xDE,
   M_EXP   = 0xDF,

   M_APP0  = 0xE0,
   M_APP15 = 0xEF,

   M_JPG0  = 0xF0,
   M_JPG13 = 0xFD,
   M_COM   = 0xFE,

   M_TEM   = 0x01,

   M_ERROR = 0x100,

   RST0    = 0xD0
} JPEG_MARKER;
//------------------------------------------------------------------------------
// 128 bytes
int16 gCoeffBuf[8*8];

#ifndef __CC65__
uint8 ZAG_Coeff[] =
{
   0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
};
#else
uint8 ZAG_Coeff[] =
{
   0*2,  1*2,  8*2,  16*2,  9*2,  2*2,  3*2, 10*2,
   17*2, 24*2, 32*2, 25*2, 18*2, 11*2,  4*2,  5*2,
   12*2, 19*2, 26*2, 33*2, 40*2, 48*2, 41*2, 34*2,
   27*2, 20*2, 13*2,  6*2,  7*2, 14*2, 21*2, 28*2,
   35*2, 42*2, 49*2, 56*2, 57*2, 50*2, 43*2, 36*2,
   29*2, 22*2, 15*2, 23*2, 30*2, 37*2, 44*2, 51*2,
   58*2, 59*2, 52*2, 45*2, 38*2, 31*2, 39*2, 46*2,
   53*2, 60*2, 61*2, 54*2, 47*2, 55*2, 62*2, 63*2,
};
#endif

uint8 gMCUBufG[128];
// 256 bytes
uint16 gQuant0[8*8];
uint16 gQuant1[8*8];

// 6 bytes
uint16 gLastDC[3];

// DC - 192
HuffTable gHuffTab0;
uint8 gHuffVal0[16];

HuffTable gHuffTab1;
uint8 gHuffVal1[16];

// AC - 672
HuffTable gHuffTab2;
uint8 gHuffVal2[256];

HuffTable gHuffTab3;
uint8 gHuffVal3[256];

static uint8 gValidHuffTables;
static uint8 gValidQuantTables;

static uint8 gTemFlag;

#ifndef __CC65__
uint16 gBitBuf;
uint8 gBitsLeft;
#else
#define gBitBuf zp6i
#define gBitsLeft zp8
#endif
//------------------------------------------------------------------------------
uint8 gCompsInFrame;
uint8 gCompIdent[3];
uint8 gCompHSamp[3];
uint8 gCompVSamp[3];
uint8 gCompQuant[3];

uint16 gNextRestartNum;
uint16 gRestartInterval;
uint16 gRestartsLeft;

uint8 gCompsInScan;
uint8 gCompList[3];
uint8 gCompDCTab[3]; // 0,1
uint8 gCompACTab[3]; // 0,1

uint8 gMaxBlocksPerMCU;

uint8 gNumMCUSRemainingX, gNumMCUSRemainingY;

uint8 gMCUOrg[6];

//------------------------------------------------------------------------------

void fillInBuf(void)
{
   // Reserve a few bytes at the beginning of the buffer for putting back ("stuffing") chars.
  read(ifd, cur_cache_ptr = cache_start, CACHE_SIZE);
}

//------------------------------------------------------------------------------

uint16 extendTests[] = {
  0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
  0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000
};
uint16 extendOffsets[] = {
  0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1, ((-1)<<5) + 1,
  ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1, ((-1)<<9) + 1, ((-1)<<10) + 1,
  ((-1)<<11) + 1, ((-1)<<12) + 1, ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1
};

//------------------------------------------------------------------------------
static void huffCreate(uint8* pBits, HuffTable* pHuffTable)
{
  uint8 i = 0;
  uint8 j = 0;
  uint16 code = 0;
  uint8 *l_pBits = pBits;
  register uint16 *curMaxCode = pHuffTable->mMaxCode;
  register uint16 *curMinCode = pHuffTable->mMinCode;
  register uint16 *curValPtr = pHuffTable->mValPtr;

   for ( ; ; )
   {
      uint8 num = *l_pBits;

      if (!num)
      {
         *curMinCode = 0x0000;
         *curMaxCode = 0xFFFF;
         *curValPtr = 0;
      }
      else
      {
         *curMinCode = code;
         *curMaxCode = code + num - 1;
         *curValPtr = j;

         j = (uint8)(j + num);

         code += num;
      }

#ifndef __CC65__
      code <<= 1;
#else
      __asm__("asl %v", code);
      __asm__("rol %v+1", code);
#endif

      i++;
      curMaxCode++;
      curMinCode++;
      curValPtr++;
      l_pBits++;
      if (i > 15)
         break;
   }
}
//------------------------------------------------------------------------------
static HuffTable* getHuffTable(uint8 index)
{
   // 0-1 = DC
   // 2-3 = AC
   switch (index)
   {
      case 0: return &gHuffTab0;
      case 1: return &gHuffTab1;
      case 2: return &gHuffTab2;
      case 3: return &gHuffTab3;
      default: return 0;
   }
}
//------------------------------------------------------------------------------
static uint8* getHuffVal(uint8 index)
{
   // 0-1 = DC
   // 2-3 = AC
   switch (index)
   {
      case 0: return gHuffVal0;
      case 1: return gHuffVal1;
      case 2: return gHuffVal2;
      case 3: return gHuffVal3;
      default: return 0;
   }
}
//------------------------------------------------------------------------------
static uint16 getMaxHuffCodes(uint8 index)
{
   return (index < 2) ? 12 : 255;
}
//------------------------------------------------------------------------------
static uint8 readDHTMarker(void)
{
   uint8 bits[16];
   uint16 left = getBits1(16);
   register uint8 *ptr;

   if (left < 2)
      return PJPG_BAD_DHT_MARKER;

   left -= 2;

   while (left)
   {
      uint8 i, tableIndex, index;
      uint8* pHuffVal;
      HuffTable* pHuffTable;
      uint16 count, totalRead;

      index = (uint8)getBits1(8);

      if ( ((index & 0xF) > 1) || ((index & 0xF0) > 0x10) )
         return PJPG_BAD_DHT_INDEX;

      tableIndex = ((index >> 3) & 2) + (index & 1);

      pHuffTable = getHuffTable(tableIndex);
      pHuffVal = getHuffVal(tableIndex);

      gValidHuffTables |= (1 << tableIndex);

      count = 0;
      ptr = bits;
      for (i = 0; i <= 15; i++)
      {
         uint8 n = (uint8)getBits1(8);
         *(ptr++) = n;
         count = (uint16)(count + n);
      }

      if (count > getMaxHuffCodes(tableIndex))
         return PJPG_BAD_DHT_COUNTS;

      ptr = pHuffVal;
      for (i = 0; i < count; i++)
         *(ptr++) = (uint8)getBits1(8);

      totalRead = count + (1+16);

      if (left < totalRead)
         return PJPG_BAD_DHT_MARKER;

      left = (uint16)(left - totalRead);

      huffCreate(bits, pHuffTable);
   }

   return 0;
}
//------------------------------------------------------------------------------
static void createWinogradQuant(uint16* pQuant);

static uint8 readDQTMarker(void)
{
   uint16 left = getBits1(16);
   register uint16 *ptr_quant1, *ptr_quant0;
   if (left < 2)
      return PJPG_BAD_DQT_MARKER;

   left -= 2;

   while (left)
   {
      uint8 i;
      uint8 n = (uint8)getBits1(8);
      uint8 prec = n >> 4;
      uint16 totalRead;

      n &= 0x0F;

      if (n > 1)
         return PJPG_BAD_DQT_TABLE;

      gValidQuantTables |= (n ? 2 : 1);

      // read quantization entries, in zag order
      ptr_quant0 = gQuant0;
      ptr_quant1 = gQuant1;
      for (i = 0; i < 64; i++)
      {
         uint16 temp = getBits1(8);

         if (prec)
            temp = (temp << 8) + getBits1(8);

         if (n)
            *ptr_quant1 = (int16)temp;
         else
            *ptr_quant0 = (int16)temp;
        
         ptr_quant0++;
         ptr_quant1++;
      }

      createWinogradQuant(n ? gQuant1 : gQuant0);

      totalRead = 64 + 1;

      if (prec)
         totalRead += 64;

      if (left < totalRead)
         return PJPG_BAD_DQT_LENGTH;

      left = (uint16)(left - totalRead);
   }

   return 0;
}
//------------------------------------------------------------------------------
static uint8 readSOFMarker(void)
{
  uint8 i;
  uint16 left = getBits1(16);
  uint16 gImageXSize;
  uint16 gImageYSize;
  register uint8 *ptr_ident, *ptr_quant, *ptr_hsamp;

   if (getBits1(8) != 8)
      return PJPG_BAD_PRECISION;

   gImageYSize = getBits1(16);

   if (gImageYSize != QT200_HEIGHT)
      return PJPG_BAD_HEIGHT;

   gImageXSize = getBits1(16);

   if (gImageXSize  != QT200_WIDTH)
      return PJPG_BAD_WIDTH;

   gCompsInFrame = (uint8)getBits1(8);

   if (gCompsInFrame > 3)
      return PJPG_TOO_MANY_COMPONENTS;

   if (left != (gCompsInFrame + gCompsInFrame + gCompsInFrame + 8))
      return PJPG_BAD_SOF_LENGTH;

   ptr_ident = gCompIdent;
   ptr_quant = gCompQuant;
   ptr_hsamp = gCompHSamp;
   for (i = 0; i < gCompsInFrame; i++)
   {
      *(ptr_ident++) = (uint8)getBits1(8);
      *(ptr_hsamp++) = (uint8)getBits1(4);
      gCompVSamp[i] = (uint8)getBits1(4);
      *(ptr_quant) = (uint8)getBits1(8);

      if (*ptr_quant > 1)
         return PJPG_UNSUPPORTED_QUANT_TABLE;
      ptr_quant++;
   }

   return 0;
}
//------------------------------------------------------------------------------
// Used to skip unrecognized markers.
uint8 skipVariableMarker(void)
{
   uint16 left = getBits1(16);

   if (left < 2)
      return PJPG_BAD_VARIABLE_MARKER;

   left -= 2;

   while (left)
   {
      getBits1(8);
      left--;
   }

   return 0;
}
//------------------------------------------------------------------------------
// Read a define restart interval (DRI) marker.
static uint8 readDRIMarker(void)
{
   if (getBits1(16) != 4)
      return PJPG_BAD_DRI_LENGTH;

   gRestartInterval = getBits1(16);

   return 0;
}
//------------------------------------------------------------------------------
// Read a start of scan (SOS) marker.
static uint8 readSOSMarker(void)
{
   uint8 i;
   uint16 left = getBits1(16);
   uint8 spectral_start, spectral_end, successive_high, successive_low;

   gCompsInScan = (uint8)getBits1(8);

   left -= 3;

   if ( (left != (gCompsInScan + gCompsInScan + 3)) || (gCompsInScan < 1) || (gCompsInScan > PJPG_MAXCOMPSINSCAN) )
      return PJPG_BAD_SOS_LENGTH;

   for (i = 0; i < gCompsInScan; i++)
   {
      uint8 cc = (uint8)getBits1(8);
      uint8 c = (uint8)getBits1(8);
      uint8 ci;

      left -= 2;

      for (ci = 0; ci < gCompsInFrame; ci++)
         if (cc == gCompIdent[ci])
            break;

      if (ci >= gCompsInFrame)
         return PJPG_BAD_SOS_COMP_ID;

      gCompList[i]    = ci;
      gCompDCTab[ci] = (c >> 4) & 15;
      gCompACTab[ci] = (c & 15);
   }

   spectral_start  = (uint8)getBits1(8);
   spectral_end    = (uint8)getBits1(8);
   successive_high = (uint8)getBits1(4);
   successive_low  = (uint8)getBits1(4);

   left -= 3;

   while (left)
   {
      getBits1(8);
      left--;
   }

   return 0;
}
//------------------------------------------------------------------------------
static uint8 nextMarker(void)
{
   uint8 c;

   do {
      do {
         c = (uint8)getBits1(8);
      } while (c != 0xFF);

      do {
         c = (uint8)getBits1(8);
      } while (c == 0xFF);
   } while (c == 0);

   return c;
}

//------------------------------------------------------------------------------
// Process markers. Returns when an SOFx, SOI, EOI, or SOS marker is
// encountered.
static uint8 processMarkers(uint8* pMarker)
{
   for ( ; ; )
   {
      uint8 c = nextMarker();

      switch (c)
      {
         case M_SOF0:
         case M_SOF1:
         case M_SOF2:
         case M_SOF3:
         case M_SOF5:
         case M_SOF6:
         case M_SOF7:
         //      case M_JPG:
         case M_SOF9:
         case M_SOF10:
         case M_SOF11:
         case M_SOF13:
         case M_SOF14:
         case M_SOF15:
         case M_SOI:
         case M_EOI:
         case M_SOS:
         {
            *pMarker = c;
            return 0;
         }
         case M_DHT:
         {
            readDHTMarker();
            break;
         }
         // Sorry, no arithmetic support at this time. Dumb patents!
         case M_DAC:
         {
            return PJPG_NO_ARITHMITIC_SUPPORT;
         }
         case M_DQT:
         {
            readDQTMarker();
            break;
         }
         case M_DRI:
         {
            readDRIMarker();
            break;
         }
         //case M_APP0:  /* no need to read the JFIF marker */

         case M_JPG:
         case M_RST0:    /* no parameters */
         case M_RST1:
         case M_RST2:
         case M_RST3:
         case M_RST4:
         case M_RST5:
         case M_RST6:
         case M_RST7:
         case M_TEM:
         {
            return PJPG_UNEXPECTED_MARKER;
         }
         default:    /* must be DNL, DHP, EXP, APPn, JPGn, COM, or RESn or APP0 */
         {
            skipVariableMarker();
            break;
         }
      }
   }
//   return 0;
}
//------------------------------------------------------------------------------
// Finds the start of image (SOI) marker.
static uint8 locateSOIMarker(void)
{
   uint16 bytesleft;

   uint8 lastchar = (uint8)getBits1(8);

   uint8 thischar = (uint8)getBits1(8);

   /* ok if it's a normal JPEG file without a special header */

   if ((lastchar == 0xFF) && (thischar == M_SOI))
      return 0;

   bytesleft = 4096; //512;

   for ( ; ; )
   {
      if (--bytesleft == 0)
         return PJPG_NOT_JPEG;

      lastchar = thischar;

      thischar = (uint8)getBits1(8);

      if (lastchar == 0xFF)
      {
         if (thischar == M_SOI)
            break;
         else if (thischar == M_EOI)	//getBits1 will keep returning M_EOI if we read past the end
            return PJPG_NOT_JPEG;
      }
   }

   /* Check the next character after marker: if it's not 0xFF, it can't
   be the start of the next marker, so the file is bad */

   thischar = (uint8)((gBitBuf >> 8) & 0xFF);

   if (thischar != 0xFF)
      return PJPG_NOT_JPEG;

   return 0;
}
//------------------------------------------------------------------------------
// Find a start of frame (SOF) marker.
static uint8 locateSOFMarker(void)
{
   uint8 c;

   uint8 status = locateSOIMarker();
   if (status)
      return status;

   status = processMarkers(&c);
   if (status)
      return status;

   switch (c)
   {
      case M_SOF2:
      {
         // Progressive JPEG - not supported by picojpeg (would require too
         // much memory, or too many IDCT's for embedded systems).
         return PJPG_UNSUPPORTED_MODE;
      }
      case M_SOF0:  /* baseline DCT */
      {
         status = readSOFMarker();
         if (status)
            return status;

         break;
      }
      case M_SOF9:
      {
         return PJPG_NO_ARITHMITIC_SUPPORT;
      }
      case M_SOF1:  /* extended sequential DCT */
      default:
      {
         return PJPG_UNSUPPORTED_MARKER;
      }
   }

   return 0;
}

//------------------------------------------------------------------------------
// Find a start of scan (SOS) marker.
static uint8 locateSOSMarker(uint8* pFoundEOI)
{
   uint8 c;
   uint8 status;

   *pFoundEOI = 0;

   status = processMarkers(&c);
   if (status)
      return status;

   if (c == M_EOI)
   {
      *pFoundEOI = 1;
      return 0;
   }
   else if (c != M_SOS)
      return PJPG_UNEXPECTED_MARKER;

   return readSOSMarker();
}

uint8 mul669_l[256] = {
0, 157, 58, 215, 116, 17, 174, 75, 232, 133, 34, 191, 92, 249, 150, 51, 208, 
109, 10, 167, 68, 225, 126, 27, 184, 85, 242, 143, 44, 201, 102, 3, 160, 
61, 218, 119, 20, 177, 78, 235, 136, 37, 194, 95, 252, 153, 54, 211, 112, 
13, 170, 71, 228, 129, 30, 187, 88, 245, 146, 47, 204, 105, 6, 163, 64, 
221, 122, 23, 180, 81, 238, 139, 40, 197, 98, 255, 156, 57, 214, 115, 16, 
173, 74, 231, 132, 33, 190, 91, 248, 149, 50, 207, 108, 9, 166, 67, 224, 
125, 26, 183, 84, 241, 142, 43, 200, 101, 2, 159, 60, 217, 118, 19, 176, 
77, 234, 135, 36, 193, 94, 251, 152, 53, 210, 111, 12, 169, 70, 227, 128, 
29, 186, 87, 244, 145, 46, 203, 104, 5, 162, 63, 220, 121, 22, 179, 80, 
237, 138, 39, 196, 97, 254, 155, 56, 213, 114, 15, 172, 73, 230, 131, 32, 
189, 90, 247, 148, 49, 206, 107, 8, 165, 66, 223, 124, 25, 182, 83, 240, 
141, 42, 199, 100, 1, 158, 59, 216, 117, 18, 175, 76, 233, 134, 35, 192, 
93, 250, 151, 52, 209, 110, 11, 168, 69, 226, 127, 28, 185, 86, 243, 144, 
45, 202, 103, 4, 161, 62, 219, 120, 21, 178, 79, 236, 137, 38, 195, 96, 
253, 154, 55, 212, 113, 14, 171, 72, 229, 130, 31, 188, 89, 246, 147, 48, 
205, 106, 7, 164, 65, 222, 123, 24, 181, 82, 239, 140, 41, 198, 99 };
uint8 mul669_m[256] = {
0, 2, 5, 7, 10, 13, 15, 18, 20, 23, 26, 28, 31, 33, 36, 39, 41, 
44, 47, 49, 52, 54, 57, 60, 62, 65, 67, 70, 73, 75, 78, 81, 83, 
86, 88, 91, 94, 96, 99, 101, 104, 107, 109, 112, 114, 117, 120, 122, 125, 
128, 130, 133, 135, 138, 141, 143, 146, 148, 151, 154, 156, 159, 162, 164, 167, 
169, 172, 175, 177, 180, 182, 185, 188, 190, 193, 195, 198, 201, 203, 206, 209, 
211, 214, 216, 219, 222, 224, 227, 229, 232, 235, 237, 240, 243, 245, 248, 250, 
253, 0, 2, 5, 7, 10, 13, 15, 18, 21, 23, 26, 28, 31, 34, 36, 
39, 41, 44, 47, 49, 52, 54, 57, 60, 62, 65, 68, 70, 73, 75, 78, 
81, 83, 86, 88, 91, 94, 96, 99, 102, 104, 107, 109, 112, 115, 117, 120, 
122, 125, 128, 130, 133, 135, 138, 141, 143, 146, 149, 151, 154, 156, 159, 162, 
164, 167, 169, 172, 175, 177, 180, 183, 185, 188, 190, 193, 196, 198, 201, 203, 
206, 209, 211, 214, 217, 219, 222, 224, 227, 230, 232, 235, 237, 240, 243, 245, 
248, 250, 253, 0, 2, 5, 8, 10, 13, 15, 18, 21, 23, 26, 28, 31, 
34, 36, 39, 42, 44, 47, 49, 52, 55, 57, 60, 62, 65, 68, 70, 73, 
75, 78, 81, 83, 86, 89, 91, 94, 96, 99, 102, 104, 107, 109, 112, 115, 
117, 120, 123, 125, 128, 130, 133, 136, 138, 141, 143, 146, 149, 151, 154 };
uint8 mul669_h[256] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
uint8 mul362_l[256] = {
0, 106, 212, 62, 168, 18, 124, 230, 80, 186, 36, 142, 248, 98, 204, 54, 160, 
10, 116, 222, 72, 178, 28, 134, 240, 90, 196, 46, 152, 2, 108, 214, 64, 
170, 20, 126, 232, 82, 188, 38, 144, 250, 100, 206, 56, 162, 12, 118, 224, 
74, 180, 30, 136, 242, 92, 198, 48, 154, 4, 110, 216, 66, 172, 22, 128, 
234, 84, 190, 40, 146, 252, 102, 208, 58, 164, 14, 120, 226, 76, 182, 32, 
138, 244, 94, 200, 50, 156, 6, 112, 218, 68, 174, 24, 130, 236, 86, 192, 
42, 148, 254, 104, 210, 60, 166, 16, 122, 228, 78, 184, 34, 140, 246, 96, 
202, 52, 158, 8, 114, 220, 70, 176, 26, 132, 238, 88, 194, 44, 150, 0, 
106, 212, 62, 168, 18, 124, 230, 80, 186, 36, 142, 248, 98, 204, 54, 160, 
10, 116, 222, 72, 178, 28, 134, 240, 90, 196, 46, 152, 2, 108, 214, 64, 
170, 20, 126, 232, 82, 188, 38, 144, 250, 100, 206, 56, 162, 12, 118, 224, 
74, 180, 30, 136, 242, 92, 198, 48, 154, 4, 110, 216, 66, 172, 22, 128, 
234, 84, 190, 40, 146, 252, 102, 208, 58, 164, 14, 120, 226, 76, 182, 32, 
138, 244, 94, 200, 50, 156, 6, 112, 218, 68, 174, 24, 130, 236, 86, 192, 
42, 148, 254, 104, 210, 60, 166, 16, 122, 228, 78, 184, 34, 140, 246, 96, 
202, 52, 158, 8, 114, 220, 70, 176, 26, 132, 238, 88, 194, 44, 150 };
uint8 mul362_m[256] = {
0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14, 15, 16, 18, 19, 21, 22, 
24, 25, 26, 28, 29, 31, 32, 33, 35, 36, 38, 39, 41, 42, 43, 45, 
46, 48, 49, 50, 52, 53, 55, 56, 57, 59, 60, 62, 63, 65, 66, 67, 
69, 70, 72, 73, 74, 76, 77, 79, 80, 82, 83, 84, 86, 87, 89, 90, 
91, 93, 94, 96, 97, 98, 100, 101, 103, 104, 106, 107, 108, 110, 111, 113, 
114, 115, 117, 118, 120, 121, 123, 124, 125, 127, 128, 130, 131, 132, 134, 135, 
137, 138, 139, 141, 142, 144, 145, 147, 148, 149, 151, 152, 154, 155, 156, 158, 
159, 161, 162, 164, 165, 166, 168, 169, 171, 172, 173, 175, 176, 178, 179, 181, 
182, 183, 185, 186, 188, 189, 190, 192, 193, 195, 196, 197, 199, 200, 202, 203, 
205, 206, 207, 209, 210, 212, 213, 214, 216, 217, 219, 220, 222, 223, 224, 226, 
227, 229, 230, 231, 233, 234, 236, 237, 238, 240, 241, 243, 244, 246, 247, 248, 
250, 251, 253, 254, 255, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14, 15, 
16, 18, 19, 21, 22, 23, 25, 26, 28, 29, 31, 32, 33, 35, 36, 38, 
39, 40, 42, 43, 45, 46, 48, 49, 50, 52, 53, 55, 56, 57, 59, 60, 
62, 63, 64, 66, 67, 69, 70, 72, 73, 74, 76, 77, 79, 80, 81, 83, 
84, 86, 87, 89, 90, 91, 93, 94, 96, 97, 98, 100, 101, 103, 104 };
uint8 mul362_h[256] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
uint8 mul277_l[256] = {
0, 21, 42, 63, 84, 105, 126, 147, 168, 189, 210, 231, 252, 17, 38, 59, 80, 
101, 122, 143, 164, 185, 206, 227, 248, 13, 34, 55, 76, 97, 118, 139, 160, 
181, 202, 223, 244, 9, 30, 51, 72, 93, 114, 135, 156, 177, 198, 219, 240, 
5, 26, 47, 68, 89, 110, 131, 152, 173, 194, 215, 236, 1, 22, 43, 64, 
85, 106, 127, 148, 169, 190, 211, 232, 253, 18, 39, 60, 81, 102, 123, 144, 
165, 186, 207, 228, 249, 14, 35, 56, 77, 98, 119, 140, 161, 182, 203, 224, 
245, 10, 31, 52, 73, 94, 115, 136, 157, 178, 199, 220, 241, 6, 27, 48, 
69, 90, 111, 132, 153, 174, 195, 216, 237, 2, 23, 44, 65, 86, 107, 128, 
149, 170, 191, 212, 233, 254, 19, 40, 61, 82, 103, 124, 145, 166, 187, 208, 
229, 250, 15, 36, 57, 78, 99, 120, 141, 162, 183, 204, 225, 246, 11, 32, 
53, 74, 95, 116, 137, 158, 179, 200, 221, 242, 7, 28, 49, 70, 91, 112, 
133, 154, 175, 196, 217, 238, 3, 24, 45, 66, 87, 108, 129, 150, 171, 192, 
213, 234, 255, 20, 41, 62, 83, 104, 125, 146, 167, 188, 209, 230, 251, 16, 
37, 58, 79, 100, 121, 142, 163, 184, 205, 226, 247, 12, 33, 54, 75, 96, 
117, 138, 159, 180, 201, 222, 243, 8, 29, 50, 71, 92, 113, 134, 155, 176, 
197, 218, 239, 4, 25, 46, 67, 88, 109, 130, 151, 172, 193, 214, 235 };
uint8 mul277_m[256] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 
18, 19, 20, 21, 22, 23, 24, 25, 27, 28, 29, 30, 31, 32, 33, 34, 
35, 36, 37, 38, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 
53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68, 69, 
70, 71, 72, 73, 74, 75, 76, 77, 78, 80, 81, 82, 83, 84, 85, 86, 
87, 88, 89, 90, 91, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 
104, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 119, 120, 121, 
122, 123, 124, 125, 126, 127, 128, 129, 130, 132, 133, 134, 135, 136, 137, 138, 
139, 140, 141, 142, 143, 144, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 
156, 157, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 172, 173, 
174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 185, 186, 187, 188, 189, 190, 
191, 192, 193, 194, 195, 196, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 
208, 209, 210, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 225, 
226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 238, 239, 240, 241, 242, 
243, 244, 245, 246, 247, 248, 249, 251, 252, 253, 254, 255, 0, 1, 2, 3, 
4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
uint8 mul277_h[256] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
uint8 mul196_l[256] = {
0, 196, 136, 76, 16, 212, 152, 92, 32, 228, 168, 108, 48, 244, 184, 124, 64, 
4, 200, 140, 80, 20, 216, 156, 96, 36, 232, 172, 112, 52, 248, 188, 128, 
68, 8, 204, 144, 84, 24, 220, 160, 100, 40, 236, 176, 116, 56, 252, 192, 
132, 72, 12, 208, 148, 88, 28, 224, 164, 104, 44, 240, 180, 120, 60, 0, 
196, 136, 76, 16, 212, 152, 92, 32, 228, 168, 108, 48, 244, 184, 124, 64, 
4, 200, 140, 80, 20, 216, 156, 96, 36, 232, 172, 112, 52, 248, 188, 128, 
68, 8, 204, 144, 84, 24, 220, 160, 100, 40, 236, 176, 116, 56, 252, 192, 
132, 72, 12, 208, 148, 88, 28, 224, 164, 104, 44, 240, 180, 120, 60, 0, 
196, 136, 76, 16, 212, 152, 92, 32, 228, 168, 108, 48, 244, 184, 124, 64, 
4, 200, 140, 80, 20, 216, 156, 96, 36, 232, 172, 112, 52, 248, 188, 128, 
68, 8, 204, 144, 84, 24, 220, 160, 100, 40, 236, 176, 116, 56, 252, 192, 
132, 72, 12, 208, 148, 88, 28, 224, 164, 104, 44, 240, 180, 120, 60, 0, 
196, 136, 76, 16, 212, 152, 92, 32, 228, 168, 108, 48, 244, 184, 124, 64, 
4, 200, 140, 80, 20, 216, 156, 96, 36, 232, 172, 112, 52, 248, 188, 128, 
68, 8, 204, 144, 84, 24, 220, 160, 100, 40, 236, 176, 116, 56, 252, 192, 
132, 72, 12, 208, 148, 88, 28, 224, 164, 104, 44, 240, 180, 120, 60 };
uint8 mul196_m[256] = {
0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 
13, 13, 14, 15, 16, 16, 17, 18, 19, 19, 20, 21, 22, 22, 23, 24, 
25, 26, 26, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36, 
37, 38, 39, 39, 40, 41, 42, 42, 43, 44, 45, 45, 46, 47, 48, 49, 
49, 50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 58, 58, 59, 60, 61, 
62, 62, 63, 64, 65, 65, 66, 67, 68, 68, 69, 70, 71, 71, 72, 73, 
74, 75, 75, 76, 77, 78, 78, 79, 80, 81, 81, 82, 83, 84, 84, 85, 
86, 87, 88, 88, 89, 90, 91, 91, 92, 93, 94, 94, 95, 96, 97, 98, 
98, 99, 100, 101, 101, 102, 103, 104, 104, 105, 106, 107, 107, 108, 109, 110, 
111, 111, 112, 113, 114, 114, 115, 116, 117, 117, 118, 119, 120, 120, 121, 122, 
123, 124, 124, 125, 126, 127, 127, 128, 129, 130, 130, 131, 132, 133, 133, 134, 
135, 136, 137, 137, 138, 139, 140, 140, 141, 142, 143, 143, 144, 145, 146, 147, 
147, 148, 149, 150, 150, 151, 152, 153, 153, 154, 155, 156, 156, 157, 158, 159, 
160, 160, 161, 162, 163, 163, 164, 165, 166, 166, 167, 168, 169, 169, 170, 171, 
172, 173, 173, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 182, 182, 183, 
184, 185, 186, 186, 187, 188, 189, 189, 190, 191, 192, 192, 193, 194, 195 };

// useless, all zeroes
// uint8 mul196_h[256] = {
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//------------------------------------------------------------------------------
static uint8 init(void)
{
   gCompsInFrame = 
     gRestartInterval = 
     gCompsInScan = 
     gValidHuffTables = 
     gValidQuantTables = 
     gTemFlag = 
     gBitBuf = 0;
   gBitsLeft = 8;

   // Multiplication tables are pre-built using:
   // i = 0;
   // do {
   //   r = (uint32)i * 669U;
   //   mul669_l[i] = (r & (0x000000ff));
   //   mul669_m[i] = (r & (0x0000ff00)) >> 8;
   //   mul669_h[i] = (r & (0x00ff0000)) >> 16;
   // 
   //   r = (uint32)i * 362U;
   //   mul362_l[i] = (r & (0x000000ff));
   //   mul362_m[i] = (r & (0x0000ff00)) >> 8;
   //   mul362_h[i] = (r & (0x00ff0000)) >> 16;
   // 
   //   r = (uint32)i * 277U;
   //   mul277_l[i] = (r & (0x000000ff));
   //   mul277_m[i] = (r & (0x0000ff00)) >> 8;
   //   mul277_h[i] = (r & (0x00ff0000)) >> 16;
   // 
   //   r = (uint32)i * 196U;
   //   mul196_l[i] = (r & (0x000000ff));
   //   mul196_m[i] = (r & (0x0000ff00)) >> 8;
   //   mul196_h[i] = (r & (0x00ff0000)) >> 16;
   // } while(++i < 256);
   // 
   // printf("uint8 mul669_l[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul669_l[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // printf("uint8 mul669_m[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul669_m[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // printf("uint8 mul669_h[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul669_h[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // 
   // printf("uint8 mul362_l[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul362_l[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // printf("uint8 mul362_m[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul362_m[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // printf("uint8 mul362_h[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul362_h[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // 
   // printf("uint8 mul277_l[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul277_l[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // printf("uint8 mul277_m[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul277_m[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // printf("uint8 mul277_h[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul277_h[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // 
   // printf("uint8 mul196_l[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul196_l[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // printf("uint8 mul196_m[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul196_m[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");
   // printf("uint8 mul196_h[256] = {\n");
   // for (i = 0; i < 256; i++) {
   //   printf("%d%s ", mul196_h[i], i < 255 ? ",":"");
   //   if (i && i%16 == 0) {
   //     printf("\n");
   //   }
   // }
   // printf("};\n");

   /* Buf pre-filled by qt-conv.c::identify */
   //fillInBuf();

   getBits1(8);
   getBits1(8);

   return 0;
}
//------------------------------------------------------------------------------
// This method throws back into the stream any bytes that where read
// into the bit buffer during initial marker scanning.
static void fixInBuffer(void)
{
   /* In case any 0xFF's where pulled into the buffer during marker scanning */

   if (gBitsLeft > 0)
      *(cur_cache_ptr--) = (uint8)gBitBuf;

   *(cur_cache_ptr--) = (uint8)(gBitBuf >> 8);

   gBitsLeft = 8;
   getBits2(8);
   getBits2(8);
}
//------------------------------------------------------------------------------
// Restart interval processing.
uint8 processRestart(void)
{
   // Let's scan a little bit to find the marker, but not _too_ far.
   // 1536 is a "fudge factor" that determines how much to scan.
   uint16 i;
   uint8 c = 0;

   for (i = 1536; i > 0; i--) {
#ifndef __CC65__
      if (cur_cache_ptr == cache_end)
        fillInBuf();
      c = *(cur_cache_ptr++);
#else
      __asm__("ldy #0");
      __asm__("lda %v+1", cur_cache_ptr);
      __asm__("cmp %v+1", cache_end);
      __asm__("bcc %g", buf_ok3);
      __asm__("lda %v", cur_cache_ptr);
      __asm__("cmp %v", cache_end);
      __asm__("bcc %g", buf_ok3);
      fillInBuf();
      __asm__("ldy #0");
      buf_ok3:
      __asm__("lda (%v),y", cur_cache_ptr);
      __asm__("sta %v", c);
      __asm__("inc %v", cur_cache_ptr);
      __asm__("bne %g", cur_buf_inc_done3);
      __asm__("inc %v+1", cur_cache_ptr);
      cur_buf_inc_done3:
#endif
      if (c == 0xFF)
         break;
   }
   if (i == 0)
      return PJPG_BAD_RESTART_MARKER;

   for ( ; i > 0; i--) {
#ifndef __CC65__
      if (cur_cache_ptr == cache_end)
        fillInBuf();
      c = *(cur_cache_ptr++);
#else
      __asm__("ldy #0");
      __asm__("lda %v+1", cur_cache_ptr);
      __asm__("cmp %v+1", cache_end);
      __asm__("bcc %g", buf_ok4);
      __asm__("lda %v", cur_cache_ptr);
      __asm__("cmp %v", cache_end);
      __asm__("bcc %g", buf_ok4);
      fillInBuf();
      __asm__("ldy #0");
      buf_ok4:
      __asm__("lda (%v),y", cur_cache_ptr);
      __asm__("sta %v", c);
      __asm__("inc %v", cur_cache_ptr);
      __asm__("bne %g", cur_buf_inc_done4);
      __asm__("inc %v+1", cur_cache_ptr);
      cur_buf_inc_done4:
#endif
      if (c != 0xFF)
         break;
   }

   if (i == 0)
      return PJPG_BAD_RESTART_MARKER;

   // Is it the expected marker? If not, something bad happened.
   if (c != (gNextRestartNum + M_RST0))
      return PJPG_BAD_RESTART_MARKER;

   // Reset each component's DC prediction values.
   gLastDC[0] = 0;
   gLastDC[1] = 0;
   gLastDC[2] = 0;

   gRestartsLeft = gRestartInterval;

   gNextRestartNum = (gNextRestartNum + 1) & 7;

   // Get the bit buffer going again

   gBitsLeft = 8;
   getBits2(8);
   getBits2(8);

   return 0;
}

//------------------------------------------------------------------------------
static uint8 initScan(void)
{
   uint8 foundEOI;
   uint8 status = locateSOSMarker(&foundEOI);
   if (status)
      return status;
   if (foundEOI)
      return PJPG_UNEXPECTED_MARKER;

   gLastDC[0] = 0;
   gLastDC[1] = 0;
   gLastDC[2] = 0;

   if (gRestartInterval)
   {
      gRestartsLeft = gRestartInterval;
      gNextRestartNum = 0;
   }

   fixInBuffer();

   return 0;
}

//------------------------------------------------------------------------------
static uint8 initFrame(void)
{
   gMaxBlocksPerMCU = 4;
   gMCUOrg[0] = 0;
   gMCUOrg[1] = 0;
   gMCUOrg[2] = 1;
   gMCUOrg[3] = 2;

   gNumMCUSRemainingX = gMaxMCUSPerRow;
   gNumMCUSRemainingY = gMaxMCUSPerCol;

   return 0;
}
//----------------------------------------------------------------------------
// Winograd IDCT: 5 multiplies per row/col, up to 80 muls for the 2D IDCT

#define PJPG_DCT_SCALE_BITS 7

#define PJPG_DESCALE(x) ((x) >> PJPG_DCT_SCALE_BITS)

#define PJPG_WINOGRAD_QUANT_SCALE_BITS 10

uint8 gWinogradQuant[] =
{
   128,  178,  178,  167,  246,  167,  151,  232,
   232,  151,  128,  209,  219,  209,  128,  101,
   178,  197,  197,  178,  101,   69,  139,  167,
   177,  167,  139,   69,   35,   96,  131,  151,
   151,  131,   96,   35,   49,   91,  118,  128,
   118,   91,   49,   46,   81,  101,  101,   81,
   46,   42,   69,   79,   69,   42,   35,   54,
   54,   35,   28,   37,   28,   19,   19,   10,
};

// Multiply quantization matrix by the Winograd IDCT scale factors
static void createWinogradQuant(uint16* pQuant)
{
   uint8 i;
   register uint16 *ptr_quant = pQuant;
   register uint8 *ptr_winograd = gWinogradQuant;

   for (i = 0; i < 64; i++)
   {
      long x = *ptr_quant;
      x *= *(ptr_winograd++);
      *(ptr_quant++) = (int16)((x + (1 << (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS - 1))) >> (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS));
   }
}

/*----------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
unsigned char pjpeg_decode_init(void)
{
   uint8 status;

   status = init();
   if (status)
      return status;

   status = locateSOFMarker();
   if (status)
      return status;

   status = initFrame();
   if (status)
      return status;

   status = initScan();
   if (status)
      return status;

   #define m_MCUWidth gMaxMCUXSize
   #define m_MCUHeight gMaxMCUYSize
   return 0;
}

static uint8 mcu_x = 0;
static uint8 status;
static uint16 dst_y;

void qt_load_raw(uint16 top)
{
  static uint8 *pDst_row;

  if (top == 0) {
    status = pjpeg_decode_init();

    if (status) {
      printf("pjpeg_decode_init() failed with status %u\n", status);
      if (status == PJPG_UNSUPPORTED_MODE) {
        printf("Progressive JPEG files are not supported.\n");
      }
      return;
    }
    dst_y = 0;
  }
  pDst_row = raw_image;

  for ( ; ; ) {
    status = pjpeg_decode_mcu();

    if (status) {
       if (status != PJPG_NO_MORE_BLOCKS) {
        printf("pjpeg_decode_mcu() failed with status %u\n", status);
        return;
       }
       break;
    }

    //pDst_block = pDst_row;

    copy_decoded_to(pDst_row);

    mcu_x++;
    pDst_row += 8;
    if (mcu_x == gMaxMCUSPerRow) {
      mcu_x = 0;
      dst_y += m_MCUHeight;
      pDst_row += QT200_HEIGHT * 2;

      progress_bar(-1, -1, 80*22, dst_y >> 1, height);
      if (dst_y % (BAND_HEIGHT*2) == 0) {
        break;
      }
    }
  }
}


void copy_decoded_to(uint8 *pDst_row)
{
  uint8 by;
  uint8 *pDst_block;
  register uint8 *pSrcG, *pDst;

  pDst_block = pDst_row;
  pSrcG = gMCUBufG;

  by = 3;
  while (1) {
    pDst = pDst_block;

    *pDst++ = *pSrcG++;
    *pDst++ = *pSrcG++;
    *pDst++ = *pSrcG++;
    *pDst++ = *pSrcG++;

    pSrcG += 4;
    if (!by)
      break;
    pDst_block += DECODED_WIDTH;
    by--;
  }

  pDst_block = pDst_row + (8>>1);

  by = 3;
  while (1) {
    pDst = pDst_block;

    *pDst++ = *pSrcG++;
    *pDst++ = *pSrcG++;
    *pDst++ = *pSrcG++;
    *pDst++ = *pSrcG++;

    pSrcG += 4;
    if (!by)
      break;
    pDst_block += DECODED_WIDTH;
    by--;
  }
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
