#include "jpeg_platform.h"
#include "qt-conv.h"

static uint8 skipBits = 0;
uint8 raw_image[RAW_IMAGE_SIZE];

int16 __fastcall__ huffExtend(uint16 x, uint8 s)
{
  if (s < 16) {
    uint16 t = extendTests[s];
    if (t > x) {
      return (int16)x + extendOffsets[s];
    }
  }

  return (int16)x;
}

extern uint8 gBitBuf;
extern uint8 gBitsLeft;
uint8 FFCheck;

uint8 getOctet(void)
{
  uint8 c, n;

  c = *(cur_cache_ptr);
  cur_cache_ptr++;
  if (cur_cache_ptr == cache_end) {
    fillInBuf();
  }
  if (!FFCheck)
    goto out;
  if (c != 0xFF)
    goto out;

  n = *(cur_cache_ptr);
  if (n)
  {
     *(--cur_cache_ptr) = 0xFF;
  } else {
    cur_cache_ptr++;
    if (cur_cache_ptr == cache_end)
      fillInBuf();
  }
out:
  return c;
}

void setFFCheck(uint8 on) {
  FFCheck = on;
}

uint8 getByteNoFF(void) {
  FFCheck = 0;
  return getOctet();
}

static inline uint8 getBit(void)
{
  uint8 ret = 0;

  if (!gBitsLeft)
  {
      gBitBuf = getOctet();
      gBitsLeft = 8;
  }

  gBitsLeft--;
  if (gBitBuf & 0x80)
    ret = 1;
  gBitBuf <<= 1;
  return ret;
}

uint8 FFCheck = 0;
uint16 __fastcall__ getBits(uint8 numBits)
{
  uint16 r = 0;
  while (numBits--) {
    r <<= 1;
    r |= getBit();
  }
  return r;
}
 
uint16 __fastcall__ getBitsNoFF(uint8 numBits) {
  FFCheck = 0;
  return getBits(numBits);
}
uint16 __fastcall__ getBitsFF(uint8 numBits) {
  FFCheck = 1;
  if (skipBits) {
    while (numBits--) {
      getBit();
    }
    return 0;
  } else {
    return getBits(numBits);
  }
}

extern uint8 *cur_cache_ptr;
extern uint8 *cache_end;

uint16 __fastcall__ imul_b1(int16 w)
{
  uint32 x;
  x = (uint32)w * 145;
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint16 __fastcall__ imul_b2(int16 w)
{
  uint32 x;
  x = (uint32)(w * 106);
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint16 __fastcall__ imul_b4(int16 w)
{
  uint32 x;
  x = (uint32)(-w * 217);
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint16 __fastcall__ imul_b5(int16 w)
{
  uint32 x;
  x = (uint32)(-w * 51);
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}


uint8 huffDecode(HuffTable* pHuffTable, const uint8* pHuffVal)
{
  int8 i = 7;
  uint8 j;
  uint16 code;
  HuffTable *curTable = pHuffTable;

  FFCheck = 1;
  code = getBit();

  pHuffTable->totalCalls++;
  pHuffTable->totalGetBit++;

  /* First loop only checking low bytes as long
   * as code < 0x100 */
  for ( ; ; ) {
    if (pHuffTable->mGetMore[i+8])
      goto incrementS;

    if (code < pHuffTable->mMaxCode_l[i+8]) {
      j = pHuffTable->mValPtr[i+8] + (uint8)code ;
      return pHuffVal[j];
    }
incrementS:
    code <<= 1;
    code |= getBit();
    i--;
    if (i < 0)
      goto long_search;
  }

/* No find, keep going with 16bits */
long_search:
  i = 7;
  for ( ; ; ) {

    if (pHuffTable->mGetMore[i])
      goto incrementL;

    if (pHuffTable->mMaxCode_h[i] == (code>>8))
      goto checkLow;
    if (pHuffTable->mMaxCode_h[i] < (code>>8))
      goto incrementL;
checkLow:
    if ((code&0xFF) < pHuffTable->mMaxCode_l[i]) {
      j = pHuffTable->mValPtr[i] + (uint8)code ;
      return pHuffVal[j];
    }

incrementL:
    i--;
    if (i < 0)
      return 0;
    code <<= 1;
    code |= getBit();
    pHuffTable->totalGetBit++;
  }
}

void idctRows(void)
{
   uint8 idctRC;

   for (idctRC = 0; idctRC < 3*8; idctRC += 8) {

     /* don't use rowSrc+3 (3,11,19) in input,
      * but compute them for idctCols */
    // printf("row using gCoeffBuf[%d,%d,%d]\n", 
    //         rowSrc-(int16*)gCoeffBuf,
    //         rowSrc_1-(int16*)gCoeffBuf,
    //         rowSrc_2-(int16*)gCoeffBuf);

    if (gCoeffBuf[(idctRC)+1] == 0 &&
        gCoeffBuf[(idctRC)+2] == 0) {
      // Short circuit the 1D IDCT if only the DC component is non-zero
      gCoeffBuf[(idctRC)+1] =
      gCoeffBuf[(idctRC)+2] =
      gCoeffBuf[(idctRC)+3] = gCoeffBuf[(idctRC)+0];
    } else {
       int16 x24, res1, res2, res3;
       int16 x30, x31, x13, x5;
       int16 x32;

       x30 = gCoeffBuf[(idctRC)+0];
       x5  = gCoeffBuf[(idctRC)+1];
       x13 = gCoeffBuf[(idctRC)+2];

       gCoeffBuf[(idctRC)+0] = x30 + x13 + x5;

       x32 = imul_b2(x13);

       res3 = imul_b1(x5);
       gCoeffBuf[(idctRC)+1] = res3 + x30 - x32;

       res1 = imul_b5(x5);
       gCoeffBuf[(idctRC)+2] = res1 + x30 - x13;

       res2 = imul_b4(x5);
       gCoeffBuf[(idctRC)+3] = res2 + x30 + x32;
     }
  }
}

#define PJPG_DCT_SCALE_BITS 7

uint8 *output0, *output1, *output2, *output3;
uint16 outputIdx;
void idctCols(void)
{
   uint8 idctCC;
   uint8 c;
   int16 t;
   uint8 val0, val1, val2, val3;

   for (idctCC = 0; idctCC < 4; idctCC++)
   {

      // printf("%d cols using gCoeffBuf[%d,%d,%d]\n", idctCC,
      //         pSrc_0_8-(int16*)gCoeffBuf,
      //         pSrc_1_8-(int16*)gCoeffBuf,
      //         pSrc_2_8-(int16*)gCoeffBuf);

      #define DESCALE(v) (((v) >> PJPG_DCT_SCALE_BITS) + 128)
      #define CLAMP(t) ((t) < 0 ? 0 : ((t) > 255 ? 255 : (t)))
      if (gCoeffBuf[idctCC+1*8] == 0 &&
          gCoeffBuf[idctCC+2*8] == 0) {
        // Short circuit the 1D IDCT if only the DC component is non-zero
        t = DESCALE(gCoeffBuf[idctCC+0*8]);
        c = CLAMP(t);
        val0 = val1 = val2 = val3 = c;

      } else {
        int16 cx5, cx30, cx12;
        int16 cres1, cres2, cres3;
        int16 cx32;

        cx30 = gCoeffBuf[idctCC+0*8];
        cx5  = gCoeffBuf[idctCC+1*8];
        cx12 = gCoeffBuf[idctCC+2*8];

        /* same index as before */
        // descale, convert to unsigned and clamp to 8-bit
        t = DESCALE(cx30 + cx12 + cx5);
        val0 = CLAMP(t);

        cres2 = imul_b4(cx5);
        cx32 = imul_b2(cx12);
        t = DESCALE(cx32 + cx30 + cres2);
        val3 = CLAMP(t);

        cres3 = imul_b1(cx5);
        t = DESCALE(cres3 + cx30 - cx32);
        val1 = CLAMP(t);

        cres1 = imul_b5(cx5);
        t = DESCALE(cres1 + cx30 - cx12);
        val2 = CLAMP(t);
      }
      output0[outputIdx] = val0;
      output1[outputIdx] = val1;
      output2[outputIdx] = val2;
      output3[outputIdx] = val3;
      outputIdx++;
   }
}

uint8 *pQ_l, *pQ_h;

void setQuant(uint8 quantId) {
  uint8 i;
  uint8 large_mults = 0;

  if (quantId) {
    pQ_l = gQuant1_l;
    pQ_h = gQuant1_h;
  } else {
    pQ_l = gQuant0_l;
    pQ_h = gQuant0_h;
  }
  for (i = 1; i < 64; i++) {
    if (ZAG_Coeff[i] != 0xFF && pQ_h[i]) {
      large_mults = 1;
    }
  }
  printf("Quant table %d has%s 16-bits mults\n",
         quantId, large_mults ? "":" no");
}

uint8 compACTab;
uint8 compDCTab;
uint8 skipCompACTab;
uint8 skipCompDCTab;

HuffTable *DCHuff, *skipDCHuff;
uint8 *DCHuffVal, *skipDCHuffVal;
HuffTable *ACHuff, *skipACHuff;
uint8 *ACHuffVal, *skipACHuffVal;

void setACDCTabs(void) {
  compACTab = gCompACTab[0];
  if (compACTab) {
    ACHuff = &gHuffTab3;
    ACHuffVal = gHuffVal3;
  } else {
    ACHuff = &gHuffTab2;
    ACHuffVal = gHuffVal2;
  }

  compDCTab = gCompDCTab[0];
  if (compDCTab) {
    DCHuff = &gHuffTab1;
    DCHuffVal = gHuffVal1;
  } else {
    DCHuff = &gHuffTab0;
    DCHuffVal = gHuffVal0;
  }

  if (gCompACTab[1] != gCompACTab[2]) {
    printf("unsupported AC tabs\n");
  }
  if (gCompDCTab[1] != gCompDCTab[2]) {
    printf("unsupported DC tabs\n");
  }
  skipCompACTab = gCompACTab[1];
  if (skipCompACTab) {
    skipACHuff = &gHuffTab3;
    skipACHuffVal = gHuffVal3;
  } else {
    skipACHuff = &gHuffTab2;
    skipACHuffVal = gHuffVal2;
  }

  skipCompDCTab = gCompDCTab[1];
  if (skipCompDCTab) {
    skipDCHuff = &gHuffTab1;
    skipDCHuffVal = gHuffVal1;
  } else {
    skipDCHuff = &gHuffTab0;
    skipDCHuffVal = gHuffVal0;
  }
}

uint8 decodeNextMCU(void)
{
  uint8 status;
  uint8 mcuBlock;
  uint8 componentID;
  uint16 r;
  uint8 s, cur_ZAG_coeff;
  uint16 extraBits;
  uint8 numExtraBits;
  uint16 dc;
  uint16 ac;

  if (gRestartInterval) {
    if (gRestartsLeft == 0) {
      status = processRestart();
      if (status)
        return status;
    }
    gRestartsLeft--;
  }

  for (mcuBlock = 0; mcuBlock < 2; mcuBlock++) {
    if (gMCUOrg[mcuBlock] != 0) {
      /* see initFrame, componentID = 0 for mcuBlocks 0/1 */
      printf("Unexpected thingy.\n");
      return -1;
    }

    /* Pre-zero coeffs we'll use in idctRows */
    gCoeffBuf[1] =
      gCoeffBuf[2] =
      gCoeffBuf[8] =
      gCoeffBuf[9] =
      gCoeffBuf[10] =
      gCoeffBuf[16] =
      gCoeffBuf[17] =
      gCoeffBuf[18] = 0;

    s = huffDecode(DCHuff, DCHuffVal);

    r = 0;
    numExtraBits = s & 0xF;
    if (numExtraBits)
       r = getBitsFF(numExtraBits);

    dc = huffExtend(r, s);

    dc = dc + gLastDC_l[0] + (gLastDC_h[0]<<8);
    gLastDC_l[0] = dc & 0xFF;
    gLastDC_h[0] = (dc & 0xFF00) >> 8;

    gCoeffBuf[0] = dc * (pQ_l[0]|(pQ_h[0]<<8));

    for (cur_ZAG_coeff = 1; cur_ZAG_coeff != 64; cur_ZAG_coeff++) {
      s = huffDecode(ACHuff, ACHuffVal);

      r = s >> 4;
      cur_ZAG_coeff += r;

      s = s & 0xF;
      if (!s) {
        if (r != 15) {
          break;
        }
      } else {
        extraBits = getBitsFF(s);

        if (ZAG_Coeff[cur_ZAG_coeff] != 0xFF) {
          ac = huffExtend(extraBits, s);
          // printf("computing %d\n", ZAG_Coeff[cur_ZAG_coeff]);
          gCoeffBuf[ZAG_Coeff[cur_ZAG_coeff]] = ac * (pQ_l[cur_ZAG_coeff]|(pQ_h[cur_ZAG_coeff] << 8));
        }
      }
    }
    idctRows();
    idctCols();
  }

   /* Skip the other blocks, do the minimal work, only consuming
    * input bits
    */
  skipBits = 1;
  for (mcuBlock = 2; mcuBlock < gMaxBlocksPerMCU; mcuBlock++) {
    componentID = gMCUOrg[mcuBlock];

    s = huffDecode(skipDCHuff, skipDCHuffVal);

    if (s & 0xF)
      getBitsFF(s & 0xF);

    for (cur_ZAG_coeff = 1; cur_ZAG_coeff != 64;) {
      s = huffDecode(skipACHuff, skipACHuffVal);
      if (!s) {
        break;
      }
      numExtraBits = s & 0xF;
      if (numExtraBits)
        getBitsFF(numExtraBits);

      cur_ZAG_coeff += (s >> 4) + 1;
   }
  }
  skipBits = 0;

  return 0;
}

//------------------------------------------------------------------------------
unsigned char pjpeg_decode_mcu(void)
{
   if (decodeNextMCU())
      return -1;

   if (!--gNumMCUSRemainingX)
   {
	  if (--gNumMCUSRemainingY > 0)
		  gNumMCUSRemainingX = GMAXMCUSPERROW;
   }

   return 0;
}

//----------------------------------------------------------------------------
// Winograd IDCT: 5 multiplies per row/col, up to 80 muls for the 2D IDCT

#define PJPG_DCT_SCALE_BITS 7
#define PJPG_WINOGRAD_QUANT_SCALE_BITS 10

// Multiply quantization matrix by the Winograd IDCT scale factors
void createWinogradQuant0(void)
{
   uint8 i;

   for (i = 0; i < 64; i++)
   {
      long x, v = (long)(gQuant0_l[i] | (gQuant0_h[i]<<8));
      int16 r;
      x = v * gWinogradQuant[i];
      // r = (int16)((x + (1 << (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS - 1))) >> (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS));
      //r = (int16)((x + (1 << (2))) >> (3));
      r = (int16)((x + 4) >> (3));
      gQuant0_l[i] = (uint8)(r);
      gQuant0_h[i] = (uint8)(r >> 8);
   }
}
void createWinogradQuant1(void)
{
   uint8 i;

   for (i = 0; i < 64; i++)
   {
      long x, v = (long)(gQuant1_l[i] | (gQuant1_h[i]<<8));
      int16 r;
      x = v * gWinogradQuant[i];
      r = (int16)((x + (1 << (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS - 1))) >> (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS));
      gQuant1_l[i] = (uint8)(r);
      gQuant1_h[i] = (uint8)(r >> 8);
   }
}
