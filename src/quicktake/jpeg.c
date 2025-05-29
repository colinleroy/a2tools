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

#define QT200_WIDTH 640
#define QT200_HEIGHT 480

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

// 8*8*4 bytes * 3 = 768
uint8 gMCUBufG[256];
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

uint16 gBitBuf;
uint8 gBitsLeft;
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

uint16 gNumMCUSRemainingX, gNumMCUSRemainingY;

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

uint8 mul669_l[256], mul669_m[256], mul669_h[256];
uint8 mul362_l[256], mul362_m[256], mul362_h[256];
uint8 mul277_l[256], mul277_m[256], mul277_h[256];
uint8 mul196_l[256], mul196_m[256], mul196_h[256];

//------------------------------------------------------------------------------
static uint8 init(void)
{
   uint8 i;
   uint32 r;

   gCompsInFrame = 0;
   gRestartInterval = 0;
   gCompsInScan = 0;
   gValidHuffTables = 0;
   gValidQuantTables = 0;
   gTemFlag = 0;
   gBitBuf = 0;
   gBitsLeft = 8;
   i = 0;
   do {
     r = (uint32)i * 669U;
     mul669_l[i] = (r & (0x000000ff));
     mul669_m[i] = (r & (0x0000ff00)) >> 8;
     mul669_h[i] = (r & (0x00ff0000)) >> 16;

     r = (uint32)i * 362U;
     mul362_l[i] = (r & (0x000000ff));
     mul362_m[i] = (r & (0x0000ff00)) >> 8;
     mul362_h[i] = (r & (0x00ff0000)) >> 16;

     r = (uint32)i * 277U;
     mul277_l[i] = (r & (0x000000ff));
     mul277_m[i] = (r & (0x0000ff00)) >> 8;
     mul277_h[i] = (r & (0x00ff0000)) >> 16;

     r = (uint32)i * 196U;
     mul196_l[i] = (r & (0x000000ff));
     mul196_m[i] = (r & (0x0000ff00)) >> 8;
     mul196_h[i] = (r & (0x00ff0000)) >> 16;
   } while(++i);
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

   #define gMaxMCUXSize 16
   #define gMaxMCUYSize 8

   #define gMaxMCUSPerRow ((QT200_WIDTH + (gMaxMCUXSize - 1)) >> 4)
   #define gMaxMCUSPerCol ((QT200_HEIGHT + (gMaxMCUYSize - 1)) >> 3)

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
unsigned char pjpeg_decode_mcu(void)
{
   uint8 status;

   if ((!gNumMCUSRemainingX) && (!gNumMCUSRemainingY))
      return PJPG_NO_MORE_BLOCKS;

   status = decodeNextMCU();
   if (status)
      return status;

   gNumMCUSRemainingX--;
   if (!gNumMCUSRemainingX)
   {
      gNumMCUSRemainingY--;
	  if (gNumMCUSRemainingY > 0)
		  gNumMCUSRemainingX = gMaxMCUSPerRow;
   }

   return 0;
}
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
static uint8 *pDst_row;

#define DECODED_WIDTH (QT200_WIDTH>>1)
#define DECODED_HEIGHT (QT200_HEIGHT>>1)

void qt_load_raw(uint16 top)
{
  uint8 *pDst_block;
  register uint8 *pDst;
  register uint8 *pSrcG;

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
    #define row_pitch DECODED_WIDTH
  }
  pDst_row = raw_image;

  for ( ; ; ) {
    uint8 bx, by;
    status = pjpeg_decode_mcu();

    if (status) {
       if (status != PJPG_NO_MORE_BLOCKS) {
        printf("pjpeg_decode_mcu() failed with status %u\n", status);
        return;
       }
       break;
    }

    pSrcG = gMCUBufG;

    pDst_block = pDst_row;

    for (by = 3; ; by--) {
      pDst = pDst_block;

      for (bx = 4; bx; bx--) {
        *pDst++ = *pSrcG;
        pSrcG+=2;
      }

      pSrcG += 8;
      if (!by)
        break;
      pDst_block += row_pitch;
    }

    pDst_block = pDst_row + (8>>1);

    for (by = 3; ; by--) {
      pDst = pDst_block;

      for (bx = 4; bx; bx--) {
        *pDst++ = *pSrcG;
        pSrcG+=2;
      }

      pSrcG += 8;
      if (!by)
        break;
      pDst_block += row_pitch;
    }

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

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
