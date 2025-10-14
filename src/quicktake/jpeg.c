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
#include "qt-conv.h"
#include "picojpeg.h"
#include "extrazp.h"
#include "jpeg_platform.h"

/* Shared with qt-conv.c */
char magic[5] = JPEG_EXIF_MAGIC;
char *model = "200";
uint16 *huff_ptr;

#define N_STUFF_CHARS 4
#ifndef __CC65__
uint8 cache[CACHE_SIZE + N_STUFF_CHARS];
#else
extern uint8 cache[CACHE_SIZE + N_STUFF_CHARS];
#endif
uint8 *cache_start = cache + N_STUFF_CHARS;
extern uint8 raw_image[RAW_IMAGE_SIZE];

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
uint16 gLastDC;

#ifndef __CC65__
int16 gCoeffBuf[8*8];

uint8 ZAG_Coeff[] =
{
   0,     1,   8,  16,    9,   2,0xFF,  10,
   17, 0xFF,0xFF,0xFF,   18,0xFF,0xFF,0xFF,
   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

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

uint8 gMCUBufG[128];
// 256 bytes
uint8 gQuant0_l[64];
uint8 gQuant0_h[64];
uint8 gQuant1_l[64];
uint8 gQuant1_h[64];

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

#else
// 256 bytes
extern uint8 gHuffVal2[256];
// 256 bytes
extern uint8 gHuffVal3[256];

// 256 bytes
extern uint8 gMCUBufG[128];
extern HuffTable gHuffTab0;
extern uint8 gHuffVal0[16];
extern uint8 gHuffVal1[16];

// 256 bytes
extern HuffTable gHuffTab1;
extern HuffTable gHuffTab2;

// 96 bytes
extern HuffTable gHuffTab3;

#endif

static uint8 gValidQuantTables;

static uint8 gTemFlag;

#ifndef __CC65__
uint8 gBitBuf;
uint8 gBitsLeft;
#else
#define gBitBuf zp2i
#define gBitsLeft zp8
#endif
//------------------------------------------------------------------------------
uint8 gCompsInFrame;
uint8 gCompIdent[3];
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
  cputsxy(0, 7, "Reading       ");
  // Reserve a few bytes at the beginning of the buffer for putting back ("stuffing") chars.
  read(ifd, cur_cache_ptr = cache_start, CACHE_SIZE);
  cputsxy(0, 7, "Decoding      ");
}

//------------------------------------------------------------------------------

#ifndef __CC65__
uint16 extendTests[] = {
  0,     0x1,   0x2,   0x4,   0x8,   0x10,   0x20,   0x40,
  0x80,  0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000
};

uint16 extendOffsets[] = {
  ((-1)<<0) + 1,  ((-1)<<1) + 1,  ((-1)<<2) + 1,  ((-1)<<3) + 1,  ((-1)<<4) + 1,
  ((-1)<<5) + 1,  ((-1)<<6) + 1,  ((-1)<<7) + 1,  ((-1)<<8) + 1,  ((-1)<<9) + 1,
  ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1, ((-1)<<13) + 1, ((-1)<<14) + 1,
  ((-1)<<15) + 1
};
#else
// see jpeg_arrays.s
#endif

//------------------------------------------------------------------------------
#pragma code-name(push, "LC")
static void huffCreate(uint8* pBits, HuffTable* pHuffTable)
{
  int8 i = 15;
  uint8 j = 0;
  uint16 code = 0;
  uint8 *l_pBits = pBits;

   for ( ; ; )
   {
      uint8 num;

      num = *l_pBits;

      if (!num)
      {
         pHuffTable->mGetMore[i] = 1;
      }
      else
      {
         /* JPEG Huffman tables never use bitstrings
            which are composed of only 1’s. "111" is
            out. "1111" is forbidden. And you can just
            forget about "111111".
            -- https://alexdowad.github.io/huffman-coding/
         */
         uint16 max = (code + num);
         pHuffTable->mMaxCode_l[i] = max & 0xFF;
         pHuffTable->mMaxCode_h[i] = (max & 0xFF00) >> 8;
         pHuffTable->mValPtr[i] = j-(code & 0xFF);

         code += num;

         j = (uint8)(j + num);
      }

#ifndef __CC65__
      code <<= 1;
#else
      __asm__("asl %v", code);
      __asm__("rol %v+1", code);
#endif

      if (--i < 0)
         break;
      l_pBits++;
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


#define getLong() (getByteNoFF()<<8|getByteNoFF())

//------------------------------------------------------------------------------
static uint8 readDHTMarker(void)
{
   uint8 bits[16];
   uint16 left = getLong();
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

      index = (uint8)getByteNoFF();

      if ( ((index & 0xF) > 1) || ((index & 0xF0) > 0x10) )
         return PJPG_BAD_DHT_INDEX;

      tableIndex = ((index >> 3) & 2) + (index & 1);


      pHuffTable = getHuffTable(tableIndex);
      pHuffVal = getHuffVal(tableIndex);

      count = 0;
      ptr = bits;
      for (i = 0; i <= 15; i++)
      {
         uint8 n = (uint8)getByteNoFF();
         *(ptr++) = n;
         count = (uint16)(count + n);
      }

      if (count > getMaxHuffCodes(tableIndex))
         return PJPG_BAD_DHT_COUNTS;

      ptr = pHuffVal;
      for (i = 0; i < count; i++)
         *(ptr++) = (uint8)getByteNoFF();

      totalRead = count + (1+16);

      if (left < totalRead)
         return PJPG_BAD_DHT_MARKER;

      left = (uint16)(left - totalRead);

      huffCreate(bits, pHuffTable);
   }

   return 0;
}
#pragma code-name(pop)

//------------------------------------------------------------------------------

static uint8 readDQTMarker(void)
{
   uint16 left = getLong();
   if (left < 2)
      return PJPG_BAD_DQT_MARKER;

   left -= 2;

   while (left)
   {
      uint8 i;
      uint8 n = (uint8)getByteNoFF();
      uint8 prec = n >> 4;
      uint16 totalRead;

      n &= 0x0F;

      if (n > 1)
         return PJPG_BAD_DQT_TABLE;

      if (n) {
        gValidQuantTables |= 2;
        for (i = 0; i < 64; i++)
        {
           if (prec) {
             gQuant1_h[i] = getByteNoFF();
             gQuant1_l[i] = getByteNoFF();
           } else {
             gQuant1_h[i] = 0;
             gQuant1_l[i] = getByteNoFF();
           }
        }
        createWinogradQuant1();
      } else {
        gValidQuantTables |= 1;
        for (i = 0; i < 64; i++)
        {
           if (prec) {
             gQuant0_h[i] = getByteNoFF();
             gQuant0_l[i] = getByteNoFF();
           } else {
             gQuant0_h[i] = 0;
             gQuant0_l[i] = getByteNoFF();
           }
        }
        createWinogradQuant0();
      }

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
  uint16 left = getLong();
  uint16 gImageXSize;
  uint16 gImageYSize;

   if (getByteNoFF() != 8)
      return PJPG_BAD_PRECISION;

   gImageYSize = getLong();

   if (gImageYSize != INPUT_HEIGHT)
      return PJPG_BAD_HEIGHT;

   gImageXSize = getLong();

   if (gImageXSize  != INPUT_WIDTH)
      return PJPG_BAD_WIDTH;

   gCompsInFrame = (uint8)getByteNoFF();

   if (gCompsInFrame > 3)
      return PJPG_TOO_MANY_COMPONENTS;

   if (left != (gCompsInFrame + gCompsInFrame + gCompsInFrame + 8))
      return PJPG_BAD_SOF_LENGTH;

   for (i = 0; i < gCompsInFrame; i++)
   {
      gCompIdent[i] = (uint8)getByteNoFF();
      getByteNoFF();
      gCompQuant[i] = getByteNoFF();
      if (gCompQuant[i] > 1)
         return PJPG_UNSUPPORTED_QUANT_TABLE;
   }

   setQuant(gCompQuant[0]);

   return 0;
}
//------------------------------------------------------------------------------
// Read a define restart interval (DRI) marker.
static uint8 readDRIMarker(void)
{
    uint16 tmp = getLong();
   if (tmp != 4)
      return PJPG_BAD_DRI_LENGTH;

   gRestartInterval = getLong();

   return 0;
}
//------------------------------------------------------------------------------
// Read a start of scan (SOS) marker.
static uint8 readSOSMarker(void)
{
   uint8 i;
   uint16 left = getLong();
   uint8 spectral_start, spectral_end, successive_high, successive_low;

   gCompsInScan = (uint8)getByteNoFF();

   left -= 3;

   if ( (left != (gCompsInScan + gCompsInScan + 3)) || (gCompsInScan < 1) || (gCompsInScan > PJPG_MAXCOMPSINSCAN) )
      return PJPG_BAD_SOS_LENGTH;

   for (i = 0; i < gCompsInScan; i++)
   {
      uint8 cc = (uint8)getByteNoFF();
      uint8 c = (uint8)getByteNoFF();
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

   setACDCTabs();

   spectral_start  = (uint8)getByteNoFF();
   spectral_end    = (uint8)getByteNoFF();
   i = getByteNoFF();
   successive_high = i>>4;
   successive_low  = i&0x0F;

   left -= 3;

   while (left)
   {
      getByteNoFF();
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
         c = (uint8)getByteNoFF();
      } while (c != 0xFF);

      do {
         c = (uint8)getByteNoFF();
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

   uint8 lastchar = (uint8)getByteNoFF();

   uint8 thischar = (uint8)getByteNoFF();

   /* ok if it's a normal JPEG file without a special header */

   if ((lastchar == 0xFF) && (thischar == M_SOI))
      return 0;

   bytesleft = 4096; //512;

   for ( ; ; )
   {
      if (--bytesleft == 0)
         return PJPG_NOT_JPEG;

      lastchar = thischar;

      thischar = (uint8)getByteNoFF();

      if (lastchar == 0xFF)
      {
         if (thischar == M_SOI)
            break;
         if (thischar == M_EOI)
            return PJPG_NOT_JPEG;
      }
   }

   /* Check the next character after marker: if it's not 0xFF, it can't
   be the start of the next marker, so the file is bad */

   thischar = gBitBuf;

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
   if (c != M_SOS)
      return PJPG_UNEXPECTED_MARKER;

   return readSOSMarker();
}

//------------------------------------------------------------------------------
static uint8 init(void)
{
   gCompsInFrame = 
     gRestartInterval = 
     gCompsInScan = 
     gValidQuantTables = 
     gTemFlag = 
     gBitBuf = 0;

   gBitsLeft = 0;

   return 0;
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
      c = *(cur_cache_ptr++);
      if (cur_cache_ptr == cache_end) {
        fillInBuf();
      }
      if (c == 0xFF)
         break;
   }
   if (i == 0)
      return PJPG_BAD_RESTART_MARKER;

   for ( ; i > 0; i--) {
      c = *(cur_cache_ptr++);
      if (cur_cache_ptr == cache_end) {
        fillInBuf();
      }
      if (c != 0xFF)
         break;
   }

   if (i == 0)
      return PJPG_BAD_RESTART_MARKER;

   // Is it the expected marker? If not, something bad happened.
   if (c != (gNextRestartNum + M_RST0))
      return PJPG_BAD_RESTART_MARKER;

   // Reset DC prediction values.
   gLastDC = 0;

   gRestartsLeft = gRestartInterval;

   gNextRestartNum = (gNextRestartNum + 1) & 7;
   setFFCheck(1);

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

   gLastDC = 0;

   if (gRestartInterval)
   {
      gRestartsLeft = gRestartInterval;
      gNextRestartNum = 0;
   }

  setFFCheck(1);

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

   gNumMCUSRemainingX = GMAXMCUSPERROW;
   gNumMCUSRemainingY = GMAXMCUSPERCOL;

   return 0;
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

   return 0;
}

static uint8 mcu_x = 0;
static uint8 status;
static uint8 dst_y;

void qt_load_raw(uint16 top)
{
  static uint8 *pDst_row;

  if (top == 0) {
#ifdef __CC65__
    initFloppyStarter();
#endif

    status = pjpeg_decode_init();

    if (status) {
      cputs("pjpeg_decode_init() failed\r\n");
      return;
    }
  }

  dst_y = 0;
  pDst_row = raw_image;
  output0 = pDst_row;
  output1 = output0+RAW_WIDTH;
  output2 = output1+RAW_WIDTH;
  output3 = output2+RAW_WIDTH;
  outputIdx = 0;
  for ( ; ; ) {
    if (pjpeg_decode_mcu()) {
      cputs("pjpeg_decode_mcu() failed\r\n");
      return;
    }

    #define M_MCUHEIGHT (GMAXMCUYSIZE/2)

    if (++mcu_x == GMAXMCUSPERROW) {
      mcu_x = 0;
      if ((++dst_y == (BAND_HEIGHT/M_MCUHEIGHT))) {
        break;
      }
      pDst_row += M_MCUHEIGHT*RAW_WIDTH;
      output0 = pDst_row;
      output1 = output0+RAW_WIDTH;
      output2 = output1+RAW_WIDTH;
      output3 = output2+RAW_WIDTH;
      outputIdx = 0;
    }
  }
}

//------------------------------------------------------------------------------
// Used to skip unrecognized markers.
uint8 skipVariableMarker(void)
{
   uint16 left = getLong();
   uint16 safeSkip;
   uint16 avail = cache_end - cur_cache_ptr;

   if (left < 2)
      return PJPG_BAD_VARIABLE_MARKER;

   left -= 2;
   if (left == 0) {
     return 0;
   }
   if (left == 1) {
     getByteNoFF();
     return 0;
   }
   safeSkip = left - 2;

   /* Skip faster than consuming every bit */
skip_again:
   if (avail > safeSkip) {
     cur_cache_ptr += safeSkip;
     left -= safeSkip;
   } else {
     safeSkip -= avail;
     left -= avail;
     while (safeSkip > CACHE_SIZE) {
       /* A full page discardable */
       lseek(ifd, CACHE_SIZE, SEEK_CUR);
       safeSkip -= CACHE_SIZE;
       left -= CACHE_SIZE;
     }
     fillInBuf();
     avail = CACHE_SIZE;
     goto skip_again;
   }

   while(left--) {
     getByteNoFF();
   }

   return 0;
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
