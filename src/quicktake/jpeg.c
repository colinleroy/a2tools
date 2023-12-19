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
#include <arpa/inet.h>
#include "platform.h"
#include "progress_bar.h"
#include "qt-conv.h"
#include "picojpeg.h"
#include "extrazp.h"

/* Shared with qt-conv.c */
char magic[5] = JPEG_EXIF_MAGIC;
char *model = "200";
uint16 cache_size = 4096;
uint8 cache[4096];
uint16 *huff_ptr;

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
static int16 gCoeffBuf[8*8];

static int16 *ZAG_Coeff[] =
{
   gCoeffBuf+0,  gCoeffBuf+1,  gCoeffBuf+8, gCoeffBuf+16,  gCoeffBuf+9,  gCoeffBuf+2,  gCoeffBuf+3, gCoeffBuf+10,
   gCoeffBuf+17, gCoeffBuf+24, gCoeffBuf+32, gCoeffBuf+25, gCoeffBuf+18, gCoeffBuf+11,  gCoeffBuf+4,  gCoeffBuf+5,
   gCoeffBuf+12, gCoeffBuf+19, gCoeffBuf+26, gCoeffBuf+33, gCoeffBuf+40, gCoeffBuf+48, gCoeffBuf+41, gCoeffBuf+34,
   gCoeffBuf+27, gCoeffBuf+20, gCoeffBuf+13,  gCoeffBuf+6,  gCoeffBuf+7, gCoeffBuf+14, gCoeffBuf+21, gCoeffBuf+28,
   gCoeffBuf+35, gCoeffBuf+42, gCoeffBuf+49, gCoeffBuf+56, gCoeffBuf+57, gCoeffBuf+50, gCoeffBuf+43, gCoeffBuf+36,
   gCoeffBuf+29, gCoeffBuf+22, gCoeffBuf+15, gCoeffBuf+23, gCoeffBuf+30, gCoeffBuf+37, gCoeffBuf+44, gCoeffBuf+51,
   gCoeffBuf+58, gCoeffBuf+59, gCoeffBuf+52, gCoeffBuf+45, gCoeffBuf+38, gCoeffBuf+31, gCoeffBuf+39, gCoeffBuf+46,
   gCoeffBuf+53, gCoeffBuf+60, gCoeffBuf+61, gCoeffBuf+54, gCoeffBuf+47, gCoeffBuf+55, gCoeffBuf+62, gCoeffBuf+63,
};
// 8*8*4 bytes * 3 = 768
static uint8 gMCUBufG[256];
// 256 bytes
static int16 gQuant0[8*8];
static int16 gQuant1[8*8];

// 6 bytes
static int16 gLastDC[3];

typedef struct HuffTableT
{
   uint16 mMinCode[16];
   uint16 mMaxCode[16];
   uint8 mValPtr[16];
} HuffTable;

// DC - 192
static HuffTable gHuffTab0;

static uint8 gHuffVal0[16];

static HuffTable gHuffTab1;
static uint8 gHuffVal1[16];

// AC - 672
static HuffTable gHuffTab2;
static uint8 gHuffVal2[256];

static HuffTable gHuffTab3;
static uint8 gHuffVal3[256];

static uint8 gValidHuffTables;
static uint8 gValidQuantTables;

static uint8 gTemFlag;
#define PJPG_MAX_IN_BUF_SIZE 256
static uint8 gInBuf[PJPG_MAX_IN_BUF_SIZE];

static uint16 gBitBuf;
static uint8 gBitsLeft;
//------------------------------------------------------------------------------
static uint8 gCompsInFrame;
static uint8 gCompIdent[3];
static uint8 gCompHSamp[3];
static uint8 gCompVSamp[3];
static uint8 gCompQuant[3];

static uint16 gRestartInterval;
static uint16 gNextRestartNum;
static uint16 gRestartsLeft;

static uint8 gCompsInScan;
static uint8 gCompList[3];
static uint8 gCompDCTab[3]; // 0,1
static uint8 gCompACTab[3]; // 0,1

static uint8 gMaxBlocksPerMCU;

static uint16 gNumMCUSRemainingX, gNumMCUSRemainingY;

static uint8 gMCUOrg[6];

//------------------------------------------------------------------------------

#ifndef __CC65__
static uint8 *curInBufPtr;
#else
#define curInBufPtr prev_rom_irq_vector
#endif
static uint8 *endInBufPtr;
#define N_STUFF_CHARS 4
//------------------------------------------------------------------------------
static void fillInBuf(void)
{
   // Reserve a few bytes at the beginning of the buffer for putting back ("stuffing") chars.
   curInBufPtr = gInBuf + N_STUFF_CHARS;

   src_file_get_bytes(curInBufPtr, PJPG_MAX_IN_BUF_SIZE - N_STUFF_CHARS);
}

//------------------------------------------------------------------------------
static PJPG_INLINE uint8 getOctet(uint8 FFCheck)
{
#ifndef __CC65__
  uint8 c, n;
  if (curInBufPtr == endInBufPtr)
    fillInBuf();
  c = *(curInBufPtr++);
  if (!FFCheck)
    goto out;
  if (c != 0xFF)
    goto out;
  if (curInBufPtr == endInBufPtr)
    fillInBuf();
  n = *(curInBufPtr++);
  if (n)
  {
     *(curInBufPtr--) = n;
     *(curInBufPtr--) = 0xFF;
  }
out:
  return c;

#else
    /* Do we need to fill buffer? */
    __asm__("lda %v+1", curInBufPtr);
    __asm__("cmp %v+1", endInBufPtr);
    __asm__("bcc %g", buf_ok1);
    __asm__("lda %v", curInBufPtr);
    __asm__("cmp %v", endInBufPtr);
    __asm__("bcc %g", buf_ok1);
    fillInBuf();
    buf_ok1:
    /* Load char from buffer */
    __asm__("lda (%v)", curInBufPtr);
    __asm__("tax"); /* Result in X */
    /* Increment buffer pointer */
    __asm__("inc %v", curInBufPtr);
    __asm__("bne %g", cur_buf_inc_done1);
    __asm__("inc %v+1", curInBufPtr);
    cur_buf_inc_done1:
    /* Should we check for $FF? */
    __asm__("ldy #%o", FFCheck);
    __asm__("lda (sp),y");
    __asm__("beq %g", out);
    __asm__("cpx #$FF");
    __asm__("bne %g", out);

    /* Yes. Read again. */
    __asm__("lda %v+1", curInBufPtr);
    __asm__("cmp %v+1", endInBufPtr);
    __asm__("bcc %g", buf_ok2);
    __asm__("lda %v", curInBufPtr);
    __asm__("cmp %v", endInBufPtr);
    __asm__("bcc %g", buf_ok2);
    fillInBuf();
    buf_ok2:
    __asm__("lda (%v)", curInBufPtr); /* n in A */
    __asm__("inc %v", curInBufPtr);
    __asm__("bne %g", cur_buf_inc_done2);
    __asm__("inc %v+1", curInBufPtr);
    cur_buf_inc_done2:
    /* n != 0 ? */
    __asm__("cmp #$00");
    __asm__("beq %g", out);
    /* Stuff back chars */
    __asm__("sta (%v)", curInBufPtr);
    __asm__("lda %v", curInBufPtr);
    __asm__("bne %g", nouf1);
    __asm__("dec %v+1", curInBufPtr);
    nouf1:
    __asm__("dec %v", curInBufPtr);
    __asm__("lda #$FF");
    __asm__("sta (%v)", curInBufPtr);
    __asm__("lda %v", curInBufPtr);
    __asm__("bne %g", nouf2);
    __asm__("dec %v+1", curInBufPtr);
    nouf2:
    __asm__("dec %v", curInBufPtr);
out:
    /* Return result */
    __asm__("txa");
    __asm__("ldx #$00");
    __asm__("jmp incsp1");
#endif
  /* Unreachable */
  return 0;
}
//------------------------------------------------------------------------------
static uint16 getBits(uint8 numBits, uint8 FFCheck)
{
  uint8 n = numBits, tmp;
  uint8 ff = FFCheck;
  uint8 final_shift = 16 - n;
  uint16 ret = gBitBuf;

  if (n > 8) {
    n -= 8;

#ifndef __CC65__
    gBitBuf <<= gBitsLeft;
    gBitBuf |= getOctet(ff);
    gBitBuf <<= (8 - gBitsLeft);
    ret = (ret & 0xFF00) | (gBitBuf >> 8);
#else
    //gBitBuf <<= gBitsLeft;
    __asm__("ldy %v", gBitsLeft);
    __asm__("beq %g", no_lshift);
    __asm__("cpy #8");
    __asm__("bne %g", l_shift);
    __asm__("lda %v", gBitBuf);
    __asm__("sta %v+1", gBitBuf);
    __asm__("stz %v", gBitBuf);
    goto no_lshift;
    l_shift:
    __asm__("lda %v", gBitBuf);
    l_shift_again:
    __asm__("asl a");
    __asm__("rol %v+1", gBitBuf);
    __asm__("dey");
    __asm__("bne %g", l_shift_again);
    __asm__("sta %v", gBitBuf);
    no_lshift:
    //gBitBuf |= getOctet(ff);
    __asm__("lda %v", ff);
    __asm__("jsr %v", getOctet);
    __asm__("ora %v", gBitBuf);
    __asm__("sta %v", gBitBuf);
    //gBitBuf <<= (8 - gBitsLeft);
    __asm__("lda #%b", 8);
    __asm__("sec");
    __asm__("sbc %v", gBitsLeft);
    __asm__("tay");
    __asm__("beq %g", no_lshift2);
    __asm__("cpy #8");
    __asm__("bne %g", l_shift2);
    __asm__("lda %v", gBitBuf);
    __asm__("sta %v+1", gBitBuf);
    __asm__("stz %v", gBitBuf);
    goto no_lshift2;
    l_shift2:
    __asm__("lda %v", gBitBuf);
    l_shift_again2:
    __asm__("asl a");
    __asm__("rol %v+1", gBitBuf);
    __asm__("dey");
    __asm__("bne %g", l_shift_again2);
    __asm__("sta %v", gBitBuf);
    no_lshift2:
    //ret = (ret & 0xFF00) | (gBitBuf >> 8);
    __asm__("lda %v+1", gBitBuf);
    __asm__("sta %v", ret);
#endif
  }

  if (gBitsLeft < n) {
#ifndef __CC65__
    gBitBuf <<= gBitsLeft;
    gBitBuf |= getOctet(ff);
    tmp = n - gBitsLeft;
    gBitBuf <<= tmp;
    gBitsLeft = 8 - tmp;
#else
    //gBitBuf <<= gBitsLeft;
    __asm__("ldy %v", gBitsLeft);
    __asm__("beq %g", no_lshift3);
    __asm__("cpy #8");
    __asm__("bne %g", l_shift3);
    __asm__("lda %v", gBitBuf);
    __asm__("sta %v+1", gBitBuf);
    __asm__("stz %v", gBitBuf);
    goto no_lshift3;
    l_shift3:
    __asm__("lda %v", gBitBuf);
    l_shift_again3:
    __asm__("asl a");
    __asm__("rol %v+1", gBitBuf);
    __asm__("dey");
    __asm__("bne %g", l_shift_again3);
    __asm__("sta %v", gBitBuf);
    no_lshift3:
    //gBitBuf |= getOctet(ff);
    __asm__("lda %v", ff);
    __asm__("jsr %v", getOctet);
    __asm__("ora %v", gBitBuf);
    __asm__("sta %v", gBitBuf);
    // tmp = n - gBitsLeft;
    __asm__("lda %v", n);
    __asm__("sec");
    __asm__("sbc %v", gBitsLeft);
    __asm__("sta %v", tmp);
    // gBitBuf <<= tmp;
    __asm__("tay");
    __asm__("beq %g", no_lshift4);
    __asm__("cpy #8");
    __asm__("bne %g", l_shift4);
    __asm__("lda %v", gBitBuf);
    __asm__("sta %v+1", gBitBuf);
    __asm__("stz %v", gBitBuf);
    goto no_lshift4;
    l_shift4:
    __asm__("lda %v", gBitBuf);
    l_shift_again4:
    __asm__("asl a");
    __asm__("rol %v+1", gBitBuf);
    __asm__("dey");
    __asm__("bne %g", l_shift_again4);
    __asm__("sta %v", gBitBuf);
    no_lshift4:
    // gBitsLeft = 8 - tmp;
    __asm__("lda #%b", 8);
    __asm__("sec");
    __asm__("sbc %v", tmp);
    __asm__("sta %v", gBitsLeft);
#endif

  } else {
#ifndef __CC65__
    gBitsLeft = gBitsLeft - n;
    gBitBuf <<= n;
    goto no_lshift5;
#else
    // gBitsLeft = gBitsLeft - n;
    __asm__("lda %v", gBitsLeft);
    __asm__("sec");
    __asm__("sbc %v", n);
    __asm__("sta %v", gBitsLeft);
    // gBitBuf <<= n;
    __asm__("ldy %v", n);
    __asm__("beq %g", no_lshift5);
    __asm__("cpy #8");
    __asm__("bne %g", l_shift5);
    __asm__("lda %v", gBitBuf);
    __asm__("sta %v+1", gBitBuf);
    __asm__("stz %v", gBitBuf);
    goto no_lshift5;
    l_shift5:
    __asm__("lda %v", gBitBuf);
    l_shift_again5:
    __asm__("asl a");
    __asm__("rol %v+1", gBitBuf);
    __asm__("dey");
    __asm__("bne %g", l_shift_again5);
    __asm__("sta %v", gBitBuf);
#endif
  }
  no_lshift5:
  return ret >> final_shift;
}

#define getBits1(n) getBits(n, 0)
#define getBits2(n) getBits(n, 1)
//------------------------------------------------------------------------------
static PJPG_INLINE uint8 getBit(void)
{
   uint8 ret = 0;
   if (gBitBuf & 0x8000)
      ret = 1;

   if (!gBitsLeft)
   {
#ifndef __CC65__
      gBitBuf |= getOctet(1);
#else
    __asm__("lda #1");
    __asm__("jsr %v", getOctet);
      __asm__("ora %v", gBitBuf);
      __asm__("sta %v", gBitBuf);
#endif

      gBitsLeft += 8;
   }

   gBitsLeft--;
#ifndef __CC65__
   gBitBuf <<= 1;
#else
  __asm__("asl %v", gBitBuf);
  __asm__("rol %v+1", gBitBuf);
#endif
   return ret;
}
//------------------------------------------------------------------------------
static uint16 getExtendTest(uint8 i)
{
   switch (i)
   {
      case 0: return 0;
      case 1: return 0x0001;
      case 2: return 0x0002;
      case 3: return 0x0004;
      case 4: return 0x0008;
      case 5: return 0x0010;
      case 6: return 0x0020;
      case 7: return 0x0040;
      case 8:  return 0x0080;
      case 9:  return 0x0100;
      case 10: return 0x0200;
      case 11: return 0x0400;
      case 12: return 0x0800;
      case 13: return 0x1000;
      case 14: return 0x2000;
      case 15: return 0x4000;
      default: return 0;
   }
}
//------------------------------------------------------------------------------
static int16 getExtendOffset(uint8 i)
{
   switch (i)
   {
      case 0: return 0;
      case 1: return ((-1)<<1) + 1;
      case 2: return ((-1)<<2) + 1;
      case 3: return ((-1)<<3) + 1;
      case 4: return ((-1)<<4) + 1;
      case 5: return ((-1)<<5) + 1;
      case 6: return ((-1)<<6) + 1;
      case 7: return ((-1)<<7) + 1;
      case 8: return ((-1)<<8) + 1;
      case 9: return ((-1)<<9) + 1;
      case 10: return ((-1)<<10) + 1;
      case 11: return ((-1)<<11) + 1;
      case 12: return ((-1)<<12) + 1;
      case 13: return ((-1)<<13) + 1;
      case 14: return ((-1)<<14) + 1;
      case 15: return ((-1)<<15) + 1;
      default: return 0;
   }
};
//------------------------------------------------------------------------------
static PJPG_INLINE int16 huffExtend(uint16 x, uint8 s)
{
   uint8 lx = x, ls = s;
   return ((lx < getExtendTest(ls)) ? ((int16)lx + getExtendOffset(ls)) : (int16)lx);
}
//------------------------------------------------------------------------------
static PJPG_INLINE uint8 huffDecode(HuffTable* pHuffTable, const uint8* pHuffVal)
{
  uint8 i = 0;
  uint8 j;
  uint16 code = getBit();
  HuffTable *curTable = pHuffTable;
  register uint16 *curMaxCode = curTable->mMaxCode;
  register uint16 *curMinCode = curTable->mMinCode;
  register uint8 *curValPtr = curTable->mValPtr;
  // This func only reads a bit at a time, which on modern CPU's is not terribly efficient.
  // But on microcontrollers without strong integer shifting support this seems like a
  // more reasonable approach.
  for ( ; ; ) {
    if (i == 16)
      return 0;

    if (*curMaxCode != 0xFFFF) {
      if (*curMaxCode >= code)
        break;
    }

    i++;
    curMaxCode++;
    curMinCode++;
    curValPtr++;

#ifndef __CC65__
    code <<= 1;
    code |= getBit();
#else
    __asm__("asl %v", code);
    __asm__("rol %v+1", code);
    __asm__("jsr %v", getBit);
    __asm__("ora %v", code);
    __asm__("sta %v", code);
#endif
  }

#ifndef __CC65__
  j = (uint8)*curValPtr + (uint8)code - (uint8)*curMinCode;
#else
  __asm__("clc");
  __asm__("lda (%v)", curValPtr);
  __asm__("adc %v", code);
  __asm__("sec");
  __asm__("sbc (%v)", curMinCode);
  __asm__("sta %v", j);
#endif

  return pHuffVal[j];
}
//------------------------------------------------------------------------------
static void huffCreate(uint8* pBits, HuffTable* pHuffTable)
{
  uint8 i = 0;
  uint8 j = 0;
  uint16 code = 0;
  uint8 *l_pBits = pBits;
  register uint16 *curMaxCode = pHuffTable->mMaxCode;
  register uint16 *curMinCode = pHuffTable->mMinCode;
  register uint8 *curValPtr = pHuffTable->mValPtr;

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

      totalRead = 1 + 16 + count;

      if (left < totalRead)
         return PJPG_BAD_DHT_MARKER;

      left = (uint16)(left - totalRead);

      huffCreate(bits, pHuffTable);
   }

   return 0;
}
//------------------------------------------------------------------------------
static void createWinogradQuant(int16* pQuant);

static uint8 readDQTMarker(void)
{
   uint16 left = getBits1(16);
   register int16 *ptr_quant1, *ptr_quant0;
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
static uint8 skipVariableMarker(void)
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

static uint8 mul669_l[256], mul669_m[256], mul669_h[256];
static uint8 mul362_l[256], mul362_m[256], mul362_h[256];
static uint8 mul277_l[256], mul277_m[256], mul277_h[256];
static uint8 mul196_l[256], mul196_m[256], mul196_h[256];

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
   //gInBufLeft = 0;
   gBitBuf = 0;
   gBitsLeft = 8;
   endInBufPtr = gInBuf + PJPG_MAX_IN_BUF_SIZE;
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
   fillInBuf();

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
      *(curInBufPtr--) = (uint8)gBitBuf;

   *(curInBufPtr--) = (uint8)(gBitBuf >> 8);

   gBitsLeft = 8;
   getBits2(8);
   getBits2(8);
}
//------------------------------------------------------------------------------
// Restart interval processing.
static uint8 processRestart(void)
{
   // Let's scan a little bit to find the marker, but not _too_ far.
   // 1536 is a "fudge factor" that determines how much to scan.
   uint16 i;
   uint8 c = 0;

   for (i = 1536; i > 0; i--) {
#ifndef __CC65__
      if (curInBufPtr == endInBufPtr)
        fillInBuf();
      c = *(curInBufPtr++);
#else
      __asm__("lda %v+1", curInBufPtr);
      __asm__("cmp %v+1", endInBufPtr);
      __asm__("bcc %g", buf_ok3);
      __asm__("lda %v", curInBufPtr);
      __asm__("cmp %v", endInBufPtr);
      __asm__("bcc %g", buf_ok3);
      fillInBuf();
      buf_ok3:
      __asm__("lda (%v)", curInBufPtr);
      __asm__("sta %v", c);
      __asm__("inc %v", curInBufPtr);
      __asm__("bne %g", cur_buf_inc_done3);
      __asm__("inc %v+1", curInBufPtr);
      cur_buf_inc_done3:
#endif
      if (c == 0xFF)
         break;
   }
   if (i == 0)
      return PJPG_BAD_RESTART_MARKER;

   for ( ; i > 0; i--) {
#ifndef __CC65__
      if (curInBufPtr == endInBufPtr)
        fillInBuf();
      c = *(curInBufPtr++);
#else
      __asm__("lda %v+1", curInBufPtr);
      __asm__("cmp %v+1", endInBufPtr);
      __asm__("bcc %g", buf_ok4);
      __asm__("lda %v", curInBufPtr);
      __asm__("cmp %v", endInBufPtr);
      __asm__("bcc %g", buf_ok4);
      fillInBuf();
      buf_ok4:
      __asm__("lda (%v)", curInBufPtr);
      __asm__("sta %v", c);
      __asm__("inc %v", curInBufPtr);
      __asm__("bne %g", cur_buf_inc_done4);
      __asm__("inc %v+1", curInBufPtr);
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
static void createWinogradQuant(int16* pQuant)
{
   uint8 i;
   register int16 *ptr_quant = pQuant;
   register uint8 *ptr_winograd = gWinogradQuant;

   for (i = 0; i < 64; i++)
   {
      long x = *ptr_quant;
      x *= *(ptr_winograd++);
      *(ptr_quant++) = (int16)((x + (1 << (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS - 1))) >> (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS));
   }
}

// These multiply helper functions are the 4 types of signed multiplies needed by the Winograd IDCT.
// A smart C compiler will optimize them to use 16x8 = 24 bit muls, if not you may need to tweak
// these functions or drop to CPU specific inline assembly.

// 1/cos(4*pi/16)
// 362, 256+106
static uint16 __fastcall__ imul_b1_b3(int16 w)
{
  uint32 x;
  int16 val = w;
  uint8 neg = 0;
#ifndef __CC65__
  x = (uint32)w * 362;
#else
  // if (val < 0) {
  //   val = -val;
  //   neg = 1;
  // }
  __asm__("ldx %v+1", val);
  __asm__("cpx #$80");
  __asm__("bcc %g", positive);
  __asm__("stx %v", neg);
  // val = -val;
  __asm__("clc");
  __asm__("lda %v", val);
  __asm__("eor #$FF");
  __asm__("adc #1");
  __asm__("sta %v", val);
  __asm__("txa");
  __asm__("eor #$FF");
  __asm__("adc #0");
  __asm__("sta %v+1", val);

  positive:
  // x = mul362_l[l] | mul362_m[l] <<8 | mul362_h[l] <<16;
  __asm__("ldy %v", val);
  __asm__("lda %v,y", mul362_l);
  __asm__("sta %v", x);
  __asm__("lda %v,y", mul362_m);
  __asm__("sta %v+1", x);
  __asm__("lda %v,y", mul362_h);
  __asm__("sta %v+2", x);
  /* We can ignore high byte */
  // __asm__("stz %v+3", x);

  // x += (mul362_l[h]) << 8;
  __asm__("clc");
  __asm__("ldy %v+1", val);
  __asm__("lda %v+1", x);
  __asm__("adc %v,y", mul362_l);
  __asm__("sta %v+1", x);

  // x += (mul362_m[h]) << 16;
  __asm__("lda %v+2", x);
  __asm__("adc %v,y", mul362_m);
  __asm__("sta %v+2", x);

  /* We can ignore high byte */
  // x += (mul362_h[h]) << 24;
  // __asm__("lda %v+3", x);
  // __asm__("adc %v,y", mul362_h);
  // __asm__("sta %v+3", x);

  /* Was val negative? */
  __asm__("lda %v", neg);
  __asm__("beq %g", do_shift);
    // x ^= 0xffffffff;
  __asm__("lda %v", x);
  __asm__("eor #$FF");
  __asm__("sta %v", x);

  __asm__("lda %v+1", x);
  __asm__("eor #$FF");
  __asm__("sta %v+1", x);

  __asm__("lda %v+2", x);
  __asm__("eor #$FF");
  __asm__("sta %v+2", x);

  /* We can ignore high byte */
  // __asm__("lda %v+3", x);
  // __asm__("eor #$FF");
  // __asm__("sta %v+3", x);

    // x++;
  __asm__("inc %v", x);
  __asm__("bne %g", do_shift);
  __asm__("inc %v+1", x);
  __asm__("bne %g", do_shift);
  __asm__("inc %v+2", x);
  /* We can ignore high byte */
  // __asm__("bne %g", do_shift);
  // __asm__("inc %v+3", x);
  do_shift:
#endif
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

   return (uint16)x;
}

// 1/cos(6*pi/16)
// 669, 256+256+157
static uint16 __fastcall__ imul_b2(int16 w)
{
  uint32 x;
  int16 val = w;
  uint8 neg = 0;
#ifndef __CC65__
  x = (uint32)w * 669;
#else
  // if (val < 0) {
  //   val = -val;
  //   neg = 1;
  // }
  __asm__("ldx %v+1", val);
  __asm__("cpx #$80");
  __asm__("bcc %g", positive);
  __asm__("stx %v", neg);
  // val = -val;
  __asm__("clc");
  __asm__("lda %v", val);
  __asm__("eor #$FF");
  __asm__("adc #1");
  __asm__("sta %v", val);
  __asm__("txa");
  __asm__("eor #$FF");
  __asm__("adc #0");
  __asm__("sta %v+1", val);

  positive:
  // x = mul669_l[l] | mul669_m[l] <<8 | mul669_h[l] <<16;
  __asm__("ldy %v", val);
  __asm__("lda %v,y", mul669_l);
  __asm__("sta %v", x);
  __asm__("lda %v,y", mul669_m);
  __asm__("sta %v+1", x);
  __asm__("lda %v,y", mul669_h);
  __asm__("sta %v+2", x);
  /* We can ignore high byte */
  // __asm__("stz %v+3", x);

  // x += (mul669_l[h]) << 8;
  __asm__("clc");
  __asm__("ldy %v+1", val);
  __asm__("lda %v+1", x);
  __asm__("adc %v,y", mul669_l);
  __asm__("sta %v+1", x);

  // x += (mul669_m[h]) << 16;
  __asm__("lda %v+2", x);
  __asm__("adc %v,y", mul669_m);
  __asm__("sta %v+2", x);

  /* We can ignore high byte */
  // x += (mul669_h[h]) << 24;
  // __asm__("lda %v+3", x);
  // __asm__("adc %v,y", mul669_h);
  // __asm__("sta %v+3", x);

  __asm__("lda %v", neg);
  __asm__("beq %g", do_shift);
    // x ^= 0xffffffff;
  __asm__("lda %v", x);
  __asm__("eor #$FF");
  __asm__("sta %v", x);

  __asm__("lda %v+1", x);
  __asm__("eor #$FF");
  __asm__("sta %v+1", x);

  __asm__("lda %v+2", x);
  __asm__("eor #$FF");
  __asm__("sta %v+2", x);

  /* We can ignore this one */
  // __asm__("lda %v+3", x);
  // __asm__("eor #$FF");
  // __asm__("sta %v+3", x);

    // x++;
  __asm__("inc %v", x);
  __asm__("bne %g", do_shift);
  __asm__("inc %v+1", x);
  __asm__("bne %g", do_shift);
  __asm__("inc %v+2", x);
  /* We can ignore high byte */
  // __asm__("bne %g", do_shift);
  // __asm__("inc %v+3", x);
  do_shift:
#endif
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

   return (uint16)x;
}

// 1/cos(2*pi/16)
// 277, 256+21
static uint16 __fastcall__ imul_b4(int16 w)
{
  uint32 x;
  int16 val = w;
  uint8 neg = 0;
#ifndef __CC65__
  x = (uint32)w * 277;
#else
  // if (val < 0) {
  //   val = -val;
  //   neg = 1;
  // }
  __asm__("ldx %v+1", val);
  __asm__("cpx #$80");
  __asm__("bcc %g", positive);
  __asm__("stx %v", neg);
  // val = -val;
  __asm__("clc");
  __asm__("lda %v", val);
  __asm__("eor #$FF");
  __asm__("adc #1");
  __asm__("sta %v", val);
  __asm__("txa");
  __asm__("eor #$FF");
  __asm__("adc #0");
  __asm__("sta %v+1", val);

  positive:
  // x = mul277_l[l] | mul277_m[l] <<8 | mul277_h[l] <<16;
  __asm__("ldy %v", val);
  __asm__("lda %v,y", mul277_l);
  __asm__("sta %v", x);
  __asm__("lda %v,y", mul277_m);
  __asm__("sta %v+1", x);
  __asm__("lda %v,y", mul277_h);
  __asm__("sta %v+2", x);
  /* We can ignore high byte */
  // __asm__("stz %v+3", x);

  // x += (mul277_l[h]) << 8;
  __asm__("clc");
  __asm__("ldy %v+1", val);
  __asm__("lda %v+1", x);
  __asm__("adc %v,y", mul277_l);
  __asm__("sta %v+1", x);

  // x += (mul277_m[h]) << 16;
  __asm__("lda %v+2", x);
  __asm__("adc %v,y", mul277_m);
  __asm__("sta %v+2", x);

  /* We can ignore high byte */
  // x += (mul277_h[h]) << 24;
  // __asm__("lda %v+3", x);
  // __asm__("adc %v,y", mul277_h);
  // __asm__("sta %v+3", x);

  __asm__("lda %v", neg);
  __asm__("beq %g", do_shift);
    // x ^= 0xffffffff;
  __asm__("lda %v", x);
  __asm__("eor #$FF");
  __asm__("sta %v", x);

  __asm__("lda %v+1", x);
  __asm__("eor #$FF");
  __asm__("sta %v+1", x);

  __asm__("lda %v+2", x);
  __asm__("eor #$FF");
  __asm__("sta %v+2", x);

  /* We can ignore this one */
  // __asm__("lda %v+3", x);
  // __asm__("eor #$FF");
  // __asm__("sta %v+3", x);

    // x++;
  __asm__("inc %v", x);
  __asm__("bne %g", do_shift);
  __asm__("inc %v+1", x);
  __asm__("bne %g", do_shift);
  __asm__("inc %v+2", x);
  /* We can ignore high byte */
  // __asm__("bne %g", do_shift);
  // __asm__("inc %v+3", x);
  do_shift:
#endif
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

   return (uint16)x;
}

// 1/(cos(2*pi/16) + cos(6*pi/16))
// 196, 196
static uint16 __fastcall__ imul_b5(int16 w)
{
  uint32 x;
  int16 val = w;
  uint8 neg = 0;
#ifndef __CC65__
  x = (uint32)w * 196;
#else
  // if (val < 0) {
  //   val = -val;
  //   neg = 1;
  // }
  __asm__("ldx %v+1", val);
  __asm__("cpx #$80");
  __asm__("bcc %g", positive);
  __asm__("stx %v", neg);
  // val = -val;
  __asm__("clc");
  __asm__("lda %v", val);
  __asm__("eor #$FF");
  __asm__("adc #1");
  __asm__("sta %v", val);
  __asm__("txa");
  __asm__("eor #$FF");
  __asm__("adc #0");
  __asm__("sta %v+1", val);

  positive:
  // x = mul196_l[l] | mul196_m[l] <<8 | mul196_h[l] <<16;
  __asm__("ldy %v", val);
  __asm__("lda %v,y", mul196_l);
  __asm__("sta %v", x);
  __asm__("lda %v,y", mul196_m);
  __asm__("sta %v+1", x);
  __asm__("lda %v,y", mul196_h);
  __asm__("sta %v+2", x);
  /* We can ignore high byte */
  // __asm__("stz %v+3", x);

  // x += (mul196_l[h]) << 8;
  __asm__("clc");
  __asm__("ldy %v+1", val);
  __asm__("lda %v+1", x);
  __asm__("adc %v,y", mul196_l);
  __asm__("sta %v+1", x);

  // x += (mul196_m[h]) << 16;
  __asm__("lda %v+2", x);
  __asm__("adc %v,y", mul196_m);
  __asm__("sta %v+2", x);

  /* We can ignore high byte */
  // x += (mul196_h[h]) << 24;
  // __asm__("lda %v+3", x);
  // __asm__("adc %v,y", mul196_h);
  // __asm__("sta %v+3", x);

  /* was val negative? */
  __asm__("lda %v", neg);
  __asm__("beq %g", do_shift);

    // x ^= 0xffffffff;
  __asm__("lda %v", x);
  __asm__("eor #$FF");
  __asm__("sta %v", x);

  __asm__("lda %v+1", x);
  __asm__("eor #$FF");
  __asm__("sta %v+1", x);

  __asm__("lda %v+2", x);
  __asm__("eor #$FF");
  __asm__("sta %v+2", x);

  /* We can ignore this one */
  // __asm__("lda %v+3", x);
  // __asm__("eor #$FF");
  // __asm__("sta %v+3", x);

    // x++;
  __asm__("inc %v", x);
  __asm__("bne %g", do_shift);
  __asm__("inc %v+1", x);
  __asm__("bne %g", do_shift);
  __asm__("inc %v+2", x);
  /* We can ignore high byte */
  // __asm__("bne %g", do_shift);
  // __asm__("inc %v+3", x);
  do_shift:
#endif
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

   return (uint16)x;
}

#define clamp(d, s) {int16 t = (s); if (t < 0) d = 0; else if (t & 0xFF00) d = 255; else d = (uint8)t; }

static void idctRows(void)
{
   uint8 i;
   register int16* pSrc;
   register int16* pSrc_1;
   register int16* pSrc_2;
#ifdef __CC65__
   #define pSrc_3 zp6sip
   #define pSrc_4 zp8sip
   #define pSrc_5 zp10sip
   #define pSrc_6 zp12sip
#else
   int16* pSrc_3;
   int16* pSrc_4;
   int16* pSrc_5;
   int16* pSrc_6;
#endif
   int16* pSrc_7;
   int16 x7, x5, x15, x17, x6, x4, tmp1, stg26, x24, tmp2, tmp3, x30, x31, x12, x13, x32;

   pSrc = gCoeffBuf;
   pSrc_1 = gCoeffBuf + 1;
   pSrc_2 = gCoeffBuf + 2;
   pSrc_3 = gCoeffBuf + 3;
   pSrc_4 = gCoeffBuf + 4;
   pSrc_5 = gCoeffBuf + 5;
   pSrc_6 = gCoeffBuf + 6;
   pSrc_7 = gCoeffBuf + 7;

   for (i = 0; i < 8; i++)
   {
#ifndef __CC65__
      if (*pSrc_1 != 0 || *pSrc_2 != 0 || *pSrc_3 != 0 || *pSrc_4 != 0 || *pSrc_5 != 0 || *pSrc_6 != 0 || *pSrc_7 != 0)
        goto full_idct_rows;
#else
      __asm__("ldy #2");
      test_again:
      __asm__("lda %v,y", gCoeffBuf);
      __asm__("bne %g", full_idct_rows);
      __asm__("iny");
      __asm__("cpy #16");
      __asm__("bne %g", test_again);
#endif
       // Short circuit the 1D IDCT if only the DC component is non-zero
#ifndef __CC65__
       *(pSrc_2) = *(pSrc_4) = *(pSrc_6) = *pSrc;
#else
      __asm__("lda (%v)", pSrc);
      __asm__("sta (%v)", pSrc_2);
      __asm__("sta (%v)", pSrc_4);
      __asm__("sta (%v)", pSrc_6);
      __asm__("ldy #$01");
      __asm__("lda (%v),y", pSrc);
      __asm__("sta (%v),y", pSrc_2);
      __asm__("sta (%v),y", pSrc_4);
      __asm__("sta (%v),y", pSrc_6);
#endif
      goto cont_idct_rows;
      full_idct_rows:
#ifndef __CC65__
       x7  = *(pSrc_5) + *(pSrc_3);
       x5  = *(pSrc_1) + *(pSrc_7);

       x6  = *(pSrc_1) - *(pSrc_7);
       x4  = *(pSrc_5) - *(pSrc_3);

       x30 = *(pSrc) + *(pSrc_4);
       x13 = *(pSrc_2) + *(pSrc_6);

       x31 = *(pSrc) - *(pSrc_4);
       x12 = *(pSrc_2) - *(pSrc_6);
#else
       __asm__("lda %v", pSrc_7);
       __asm__("sta ptr1");
       __asm__("lda %v+1", pSrc_7);
       __asm__("sta ptr1+1");

       __asm__("ldy #$01");
       __asm__("clc");
       __asm__("lda (%v)", pSrc_5);
       __asm__("adc (%v)", pSrc_3);
       __asm__("sta %v", x7);
       __asm__("lda (%v),y", pSrc_5);
       __asm__("adc (%v),y", pSrc_3);
       __asm__("sta %v+1", x7);

       __asm__("clc");
       __asm__("lda (%v)", pSrc_1);
       __asm__("adc (ptr1)");
       __asm__("sta %v", x5);
       __asm__("lda (%v),y", pSrc_1);
       __asm__("adc (ptr1),y");
       __asm__("sta %v+1", x5);

       __asm__("sec");
       __asm__("lda (%v)", pSrc_1);
       __asm__("sbc (ptr1)");
       __asm__("sta %v", x6);
       __asm__("lda (%v),y", pSrc_1);
       __asm__("sbc (ptr1),y");
       __asm__("sta %v+1", x6);

       __asm__("sec");
       __asm__("lda (%v)", pSrc_5);
       __asm__("sbc (%v)", pSrc_3);
       __asm__("sta %v", x4);
       __asm__("lda (%v),y", pSrc_5);
       __asm__("sbc (%v),y", pSrc_3);
       __asm__("sta %v+1", x4);

       __asm__("clc");
       __asm__("lda (%v)", pSrc);
       __asm__("adc (%v)", pSrc_4);
       __asm__("sta %v", x30);
       __asm__("lda (%v),y", pSrc);
       __asm__("adc (%v),y", pSrc_4);
       __asm__("sta %v+1", x30);

       __asm__("clc");
       __asm__("lda (%v)", pSrc_2);
       __asm__("adc (%v)", pSrc_6);
       __asm__("sta %v", x13);
       __asm__("lda (%v),y", pSrc_2);
       __asm__("adc (%v),y", pSrc_6);
       __asm__("sta %v+1", x13);

       __asm__("sec");
       __asm__("lda (%v)", pSrc);
       __asm__("sbc (%v)", pSrc_4);
       __asm__("sta %v", x31);
       __asm__("lda (%v),y", pSrc);
       __asm__("sbc (%v),y", pSrc_4);
       __asm__("sta %v+1", x31);

       __asm__("sec");
       __asm__("lda (%v)", pSrc_2);
       __asm__("sbc (%v)", pSrc_6);
       __asm__("sta %v", x12);
       __asm__("lda (%v),y", pSrc_2);
       __asm__("sbc (%v),y", pSrc_6);
       __asm__("sta %v+1", x12);
#endif
       x15 = x5 - x7;
       x17 = x5 + x7;

       tmp1 = imul_b5(x4 - x6);
       stg26 = imul_b4(x6) - tmp1;

       x24 = tmp1 - imul_b2(x4);

       tmp2 = stg26 - x17;
       tmp3 = imul_b1_b3(x15) - tmp2;

       x32 = imul_b1_b3(x12) - x13;

       *(pSrc) = x30 + x13 + x17;
       *(pSrc_2) = x31 - x32 + tmp3;
       *(pSrc_4) = x30 + tmp3 + x24 - x13;
       *(pSrc_6) = x31 + x32 - tmp2;

      cont_idct_rows:
      pSrc += 8;
      pSrc_1 += 8;
      pSrc_2 += 8;
      pSrc_3 += 8;
      pSrc_4 += 8;
      pSrc_5 += 8;
      pSrc_6 += 8;
      pSrc_7 += 8;
   }
}

static void idctCols(void)
{
   uint8 i;

   register int16* pSrc = gCoeffBuf;
   register int16* pSrc_0_8 = gCoeffBuf+0*8;
   register int16* pSrc_2_8 = gCoeffBuf+2*8;
#ifdef __CC65__
   #define pSrc_4_8 zp6sip
   #define pSrc_6_8 zp8sip
   #define pSrc_8_8 zp10sip
   #define pSrc_1_8 zp12sip
#else
   int16 *pSrc_4_8;
   int16 *pSrc_6_8;
   int16 *pSrc_8_8;
   int16 *pSrc_1_8;
#endif
   int16 *pSrc_3_8 = gCoeffBuf+3*8;
   int16 *pSrc_5_8 = gCoeffBuf+5*8;
   int16 *pSrc_7_8 = gCoeffBuf+7*8;
   int16 x4, x7, x5, x6, tmp1, stg26, x24, x15, x17, tmp2, tmp3, x44, x30, x31, x12, x13, x32, x40, x43, x41, x42;
   uint8 c;

   pSrc_4_8 = gCoeffBuf+4*8;
   pSrc_6_8 = gCoeffBuf+6*8;
   pSrc_8_8 = gCoeffBuf+8*8;
   pSrc_1_8 = gCoeffBuf+1*8;

   for (i = 0; i < 8; i++)
   {
#ifndef __CC65__
      if (*pSrc_2_8 != 0)
        goto full_idct_cols;
      if (*pSrc_4_8 != 0)
        goto full_idct_cols;
      if (*pSrc_6_8 != 0)
        goto full_idct_cols;
#else
      __asm__("ldy #1");
      __asm__("lda (%v)", pSrc_2_8);
      __asm__("bne %g", full_idct_cols);
      __asm__("lda (%v)", pSrc_4_8);
      __asm__("bne %g", full_idct_cols);
      __asm__("lda (%v)", pSrc_6_8);
      __asm__("bne %g", full_idct_cols);
      __asm__("lda (%v),y", pSrc_2_8);
      __asm__("bne %g", full_idct_cols);
      __asm__("lda (%v),y", pSrc_4_8);
      __asm__("bne %g", full_idct_cols);
      __asm__("lda (%v),y", pSrc_6_8);
      __asm__("bne %g", full_idct_cols);
#endif
       // Short circuit the 1D IDCT if only the DC component is non-zero
       clamp(c, PJPG_DESCALE(*pSrc_0_8) + 128);
#ifndef __CC65__
       *(pSrc_0_8) =
         *(pSrc_2_8) =
         *(pSrc_4_8) =
         *(pSrc_6_8) = c;
#else
      __asm__("lda %v", c);
      __asm__("sta (%v)", pSrc_0_8);
      __asm__("sta (%v)", pSrc_2_8);
      __asm__("sta (%v)", pSrc_4_8);
      __asm__("sta (%v)", pSrc_6_8);
      __asm__("ldy #$01");
      __asm__("lda #$00");
      __asm__("sta (%v),y", pSrc_0_8);
      __asm__("sta (%v),y", pSrc_2_8);
      __asm__("sta (%v),y", pSrc_4_8);
      __asm__("sta (%v),y", pSrc_6_8);
#endif
      goto cont_idct_cols;
      full_idct_cols:
#ifndef __CC65__
       x4  = *(pSrc_5_8) - *(pSrc_3_8);
       x7  = *(pSrc_5_8) + *(pSrc_3_8);
       x6  = *(pSrc_1_8) - *(pSrc_7_8);
       x5  = *(pSrc_1_8) + *(pSrc_7_8);
#else
      __asm__("ldy #1");
      __asm__("sec");
      __asm__("lda %v", pSrc_3_8);
      __asm__("sta ptr1");
      __asm__("lda %v+1", pSrc_3_8);
      __asm__("sta ptr1+1");
      __asm__("lda %v", pSrc_5_8);
      __asm__("sta ptr2");
      __asm__("lda %v+1", pSrc_5_8);
      __asm__("sta ptr2+1");
      __asm__("lda (ptr2)");
      __asm__("sbc (ptr1)");
      __asm__("sta %v", x4);
      __asm__("lda (ptr2),y");
      __asm__("sbc (ptr1),y");
      __asm__("sta %v+1", x4);

      __asm__("clc");
      __asm__("lda (ptr2)");
      __asm__("adc (ptr1)");
      __asm__("sta %v", x7);
      __asm__("lda (ptr2),y");
      __asm__("adc (ptr1),y");
      __asm__("sta %v+1", x7);

      __asm__("sec");
      __asm__("lda %v", pSrc_7_8);
      __asm__("sta ptr1");
      __asm__("lda %v+1", pSrc_7_8);
      __asm__("sta ptr1+1");
      __asm__("lda (%v)", pSrc_1_8);
      __asm__("sbc (ptr1)");
      __asm__("sta %v", x6);
      __asm__("lda (%v),y", pSrc_1_8);
      __asm__("sbc (ptr1),y");
      __asm__("sta %v+1", x6);

      __asm__("clc");
      __asm__("lda (%v)", pSrc_1_8);
      __asm__("adc (ptr1)");
      __asm__("sta %v", x5);
      __asm__("lda (%v),y", pSrc_1_8);
      __asm__("adc (ptr1),y");
      __asm__("sta %v+1", x5);
#endif
       tmp1 = imul_b5(x4 - x6);
       stg26 = imul_b4(x6) - tmp1;

       x24 = tmp1 - imul_b2(x4);

       x15 = x5 - x7;
       x17 = x5 + x7;

       tmp2 = stg26 - x17;
       tmp3 = imul_b1_b3(x15) - tmp2;
       x44 = tmp3 + x24;

#ifndef __CC65__
       x31 = *(pSrc_0_8) - *(pSrc_4_8);
       x30 = *(pSrc_0_8) + *(pSrc_4_8);
       x12 = *(pSrc_2_8) - *(pSrc_6_8);
       x13 = *(pSrc_2_8) + *(pSrc_6_8);
#else
      __asm__("ldy #1");
      __asm__("sec");
      __asm__("lda (%v)", pSrc_0_8);
      __asm__("sbc (%v)", pSrc_4_8);
      __asm__("sta %v", x31);
      __asm__("lda (%v),y", pSrc_0_8);
      __asm__("sbc (%v),y", pSrc_4_8);
      __asm__("sta %v+1", x31);

      __asm__("clc");
      __asm__("lda (%v)", pSrc_0_8);
      __asm__("adc (%v)", pSrc_4_8);
      __asm__("sta %v", x30);
      __asm__("lda (%v),y", pSrc_0_8);
      __asm__("adc (%v),y", pSrc_4_8);
      __asm__("sta %v+1", x30);

      __asm__("sec");
      __asm__("lda (%v)", pSrc_2_8);
      __asm__("sbc (%v)", pSrc_6_8);
      __asm__("sta %v", x12);
      __asm__("lda (%v),y", pSrc_2_8);
      __asm__("sbc (%v),y", pSrc_6_8);
      __asm__("sta %v+1", x12);

      __asm__("clc");
      __asm__("lda (%v)", pSrc_2_8);
      __asm__("adc (%v)", pSrc_6_8);
      __asm__("sta %v", x13);
      __asm__("lda (%v),y", pSrc_2_8);
      __asm__("adc (%v),y", pSrc_6_8);
      __asm__("sta %v+1", x13);
#endif
       x32 = imul_b1_b3(x12) - x13;

       x40 = x30 + x13;
       x43 = x30 - x13;
       x41 = x31 + x32;
       x42 = x31 - x32;

       // descale, convert to unsigned and clamp to 8-bit
       clamp(*(pSrc_0_8), PJPG_DESCALE(x40 + x17)  + 128);
       clamp(*(pSrc_2_8), PJPG_DESCALE(x42 + tmp3) + 128);
       clamp(*(pSrc_4_8), PJPG_DESCALE(x43 + x44)  + 128);
       clamp(*(pSrc_6_8), PJPG_DESCALE(x41 - tmp2) + 128);
       // *(pSrc_1_8) = clamp(PJPG_DESCALE(x41 + tmp2) + 128);
       // *(pSrc_3_8) = clamp(PJPG_DESCALE(x43 - x44)  + 128);
       // *(pSrc_5_8) = clamp(PJPG_DESCALE(x42 - tmp3) + 128);
       // *(pSrc_7_8) = clamp(PJPG_DESCALE(x40 - x17)  + 128);

      cont_idct_cols:
      pSrc_0_8++;
      pSrc_2_8++;
      pSrc_4_8++;
      pSrc_6_8++;
      pSrc_1_8++;
      pSrc_3_8++;
      pSrc_5_8++;
      pSrc_7_8++;
   }
}

/*----------------------------------------------------------------------------*/
static void transformBlock(uint8 mcuBlock)
{
#ifndef __CC65__
  uint8* pGDst;
  int16* pSrc;
#else
  #define pGDst zp6p
  #define pSrc zp8sip
#endif
  uint8 i;

    idctRows();
    idctCols();

  if (mcuBlock == 0) {
    pGDst = gMCUBufG;
  } else {
    pGDst = gMCUBufG + 64;
  }

  pSrc = gCoeffBuf;
  for (i = 64; i; i--) {
    *pGDst++ = (uint8)*pSrc++;
  }

}
//------------------------------------------------------------------------------
static uint8 decodeNextMCU(void)
{
  uint8 status;
  uint8 mcuBlock;
  /* Do not use zp vars here, it'll be destroyed by transformBlock
  * and idct*
  */
  register uint8 *cur_gMCUOrg;

  if (gRestartInterval) {
    if (gRestartsLeft == 0) {
      status = processRestart();
      if (status)
        return status;
    }
    gRestartsLeft--;
  }

  cur_gMCUOrg = gMCUOrg + 0;
  for (mcuBlock = 0; mcuBlock < 2; mcuBlock++) {
    uint8 componentID = *cur_gMCUOrg;
    uint8 compQuant = gCompQuant[componentID];	
    uint8 compDCTab = gCompDCTab[componentID];
    uint8 numExtraBits, compACTab;
    int16* pQ = compQuant ? gQuant1 : gQuant0;
    uint16 r, dc;
    uint8 s;
#ifndef __CC65__
    int16 *cur_pQ;
    int16 **cur_ZAG_coeff;
    int16 **end_ZAG_coeff;
#else
    /* We can use 6 and 8 there as we'll be done with them by the time
     * we reach transformBlock()
     */
    #define cur_pQ zp6sip
    #define cur_ZAG_coeff zp8sip
    int16 *end_ZAG_coeff;
#endif
    if (compDCTab)
      s = huffDecode(&gHuffTab1, gHuffVal1);
    else
      s = huffDecode(&gHuffTab0, gHuffVal0);

    cur_gMCUOrg++;

    r = 0;
    numExtraBits = s & 0xF;
    if (numExtraBits)
       r = getBits2(numExtraBits);
    dc = huffExtend(r, s);

    dc = dc + gLastDC[componentID];
    gLastDC[componentID] = dc;

    gCoeffBuf[0] = dc * pQ[0];
    compACTab = gCompACTab[componentID];

    // Decode and dequantize AC coefficients

#ifndef __CC65__
    cur_ZAG_coeff = (ZAG_Coeff + 1);
    end_ZAG_coeff = ZAG_Coeff + (64 - 1);
#else
    cur_ZAG_coeff = (int16 *)(ZAG_Coeff + 1);
    end_ZAG_coeff = (int16 *)(ZAG_Coeff + (64 - 1));
#endif
    cur_pQ = pQ + 1;
    for (; cur_ZAG_coeff != end_ZAG_coeff; cur_ZAG_coeff++, cur_pQ++) {
      uint16 extraBits, tmp16;

      if (compACTab)
        s = huffDecode(&gHuffTab3, gHuffVal3);
      else
        s = huffDecode(&gHuffTab2, gHuffVal2);

      extraBits = 0;
      numExtraBits = s & 0xF;
      if (numExtraBits)
        extraBits = getBits2(numExtraBits);

      r = s >> 4;
      s &= 15;

      if (s) {
        uint16 ac;

        while (r) {
#ifndef __CC65__
          **cur_ZAG_coeff = 0;
#else
          __asm__("lda (%v)", cur_ZAG_coeff);
          __asm__("sta ptr1");
          __asm__("ldy #1");
          __asm__("lda (%v),y", cur_ZAG_coeff);
          __asm__("sta ptr1+1");
          __asm__("lda #0");
          __asm__("sta (ptr1)");
          __asm__("sta (ptr1),y");
#endif
          cur_ZAG_coeff++;
          cur_pQ++;
          r--;
        }

        ac = huffExtend(extraBits, s);

#ifndef __CC65__
        **cur_ZAG_coeff =  ac * *cur_pQ;
#else
        tmp16 = ac * *cur_pQ;
        __asm__("lda (%v)", cur_ZAG_coeff);
        __asm__("sta ptr1");
        __asm__("ldy #1");
        __asm__("lda (%v),y", cur_ZAG_coeff);
        __asm__("sta ptr1+1");
        __asm__("lda %v", tmp16);
        __asm__("sta (ptr1)");
        __asm__("lda %v+1", tmp16);
        __asm__("sta (ptr1),y");
#endif
      } else {
        if (r == 15) {
          if ((cur_ZAG_coeff + 16) > end_ZAG_coeff)
            return PJPG_DECODE_ERROR;

          for (r = 16; r > 0; r--) {
#ifndef __CC65__
            **cur_ZAG_coeff = 0;
#else
            __asm__("lda (%v)", cur_ZAG_coeff);
            __asm__("sta ptr1");
            __asm__("ldy #1");
            __asm__("lda (%v),y", cur_ZAG_coeff);
            __asm__("sta ptr1+1");
            __asm__("lda #0");
            __asm__("sta (ptr1)");
            __asm__("sta (ptr1),y");
#endif
            cur_ZAG_coeff++;
            cur_pQ++;
          }
          cur_ZAG_coeff--;
          cur_pQ--;
        } else
          break;
      }
    }

    while (cur_ZAG_coeff != end_ZAG_coeff) {
#ifndef __CC65__
      **cur_ZAG_coeff = 0;
#else
      __asm__("lda (%v)", cur_ZAG_coeff);
      __asm__("sta ptr1");
      __asm__("ldy #1");
      __asm__("lda (%v),y", cur_ZAG_coeff);
      __asm__("sta ptr1+1");
      __asm__("lda #0");
      __asm__("sta (ptr1)");
      __asm__("sta (ptr1),y");
#endif
      cur_ZAG_coeff++;
      cur_pQ++;
    }

    if (mcuBlock < 2)
      transformBlock(mcuBlock);
  }

   /* Skip the other blocks, do the minimal work */
  for (mcuBlock = 2; mcuBlock < gMaxBlocksPerMCU; mcuBlock++) {
    uint8 componentID = *cur_gMCUOrg;
    uint8 compDCTab = gCompDCTab[componentID];
    uint8 numExtraBits, compACTab;
    uint16 r, dc;
    uint8 s, i;

    if (compDCTab)
      s = huffDecode(&gHuffTab1, gHuffVal1);
    else
      s = huffDecode(&gHuffTab0, gHuffVal0);

    compACTab = gCompACTab[componentID];
    r = 0;
    numExtraBits = s & 0xF;
    if (numExtraBits)
      r = getBits2(numExtraBits);

    dc = huffExtend(r, s);

    for (i = 1; i != 64; i++) {
      uint16 extraBits;

      if (compACTab)
        s = huffDecode(&gHuffTab3, gHuffVal3);
      else
        s = huffDecode(&gHuffTab2, gHuffVal2);

      extraBits = 0;
      numExtraBits = s & 0xF;
      if (numExtraBits)
        extraBits = getBits2(numExtraBits);

      r = s >> 4;
      s &= 15;

      if (s) {
        int16 ac;

        i += r;
        ac = huffExtend(extraBits, s);
      } else {
        if (r == 15) {
          i+=15;
        } else
          break;
      }
    }
  }
  return 0;
}
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

    for (by = 0; ; by++) {
      pDst = pDst_block;

      for (bx = 0; bx < 4; bx++) {
        *pDst++ = *pSrcG++;
        pSrcG++;
      }

      pSrcG += 8;
      if (by == 3)
        break;
      pDst_block += row_pitch;
    }

    pDst_block = pDst_row + (8>>1);

    for (by = 0; ; by++) {
      pDst = pDst_block;

      for (bx = 0; bx < 4; bx++) {
        *pDst++ = *pSrcG++;
        pSrcG++;
      }

      pSrcG += 8;
      if (by == 3)
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
      if (dst_y % (QT_BAND*2) == 0) {
        break;
      }
    }
  }
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
