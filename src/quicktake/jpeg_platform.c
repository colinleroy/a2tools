#include "jpeg_platform.h"
#include "qt-conv.h"

static uint8 skipBits = 0;

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

// 1/cos(4*pi/16)
// 362, 256+106
uint16 __fastcall__ imul_b1_b3(int16 w)
{
  uint32 x;
  x = (uint32)w * 362;
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint16 __fastcall__ imul_b4(int16 w)
{
  uint32 x;
  x = (uint32)(-w * 473);
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint16 __fastcall__ imul_b5(int16 w)
{
  uint32 x;
  x = (uint32)(-w * 196);
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
   uint8 i;
   register int16* rowSrc;
   register int16* rowSrc_1;
   register int16* rowSrc_2;
   register int16* rowSrc_3;


   rowSrc = gCoeffBuf;
   i = 3;
nextIdctRowsLoop:
       rowSrc_1 = rowSrc + 1;
       rowSrc_2 = rowSrc + 2;
       rowSrc_3 = rowSrc + 3;

       /* don't use rowSrc+3 (3,11,19) in input,
        * but compute them for idctCols */
      // printf("row using gCoeffBuf[%d,%d,%d]\n", 
      //         rowSrc-(int16*)gCoeffBuf,
      //         rowSrc_1-(int16*)gCoeffBuf,
      //         rowSrc_2-(int16*)gCoeffBuf);

      if (*rowSrc_1 != 0 || *rowSrc_2 != 0)
        goto full_idct_rows;
       // Short circuit the 1D IDCT if only the DC component is non-zero
     *(rowSrc_1) = *(rowSrc_2) = *(rowSrc_3) = *rowSrc;
      goto cont_idct_rows;

      full_idct_rows:
         int16 x24, res1, res2, res3;
         int16 x30, x31, x13, x5;
         int16 x32;

         x5  = *(rowSrc_1);
         x30 = *(rowSrc);
         x13 = *(rowSrc_2);

         *(rowSrc) = x30 + x13 + x5;

         x32 = imul_b1_b3(x13) - x13;

         res1 = imul_b5(x5);
         res2 = imul_b4(x5) + x5;
         res3 = imul_b1_b3(x5) + x30 + res2;

         *(rowSrc_1) = res3 - x32;
         *(rowSrc_2) = res3 + res1 - x13;
         *(rowSrc_3) = x32 + x30 + res2;

      cont_idct_rows:
      rowSrc += 8;
      i--;
      if (i)
        goto nextIdctRowsLoop;
}

#define PJPG_DCT_SCALE_BITS 7

void idctCols(uint8 mcuoffset)
{
   uint8 idctCC;

   register int16* pSrc_0_8;
   register int16* pSrc_2_8;
   int16 *pSrc_1_8;
   int16 stg26;
   uint8 c;
   uint16 t;
   uint8 *output = (uint8*)gMCUBufG;

   pSrc_0_8 = gCoeffBuf+0*8;
   pSrc_1_8 = gCoeffBuf+1*8;
   pSrc_2_8 = gCoeffBuf+2*8;

   for (idctCC = 0; idctCC < 4; idctCC++)
   {

      // printf("%d cols using gCoeffBuf[%d,%d,%d]\n", idctCC,
      //         pSrc_0_8-(int16*)gCoeffBuf,
      //         pSrc_1_8-(int16*)gCoeffBuf,
      //         pSrc_2_8-(int16*)gCoeffBuf);


      if (*pSrc_1_8 != 0)
        goto full_idct_cols;
      if (*pSrc_2_8 != 0)
        goto full_idct_cols;

       // Short circuit the 1D IDCT if only the DC component is non-zero
       t = (*pSrc_0_8 >> PJPG_DCT_SCALE_BITS) + 128;
       if (t & 0xF000)
         c = 0; 
       else if (t & 0xFF00)
          c = 255;
       else 
         c = (uint8)t;

       output[mcuoffset + 0*4] = 
        output[mcuoffset + 1*4] = 
        output[mcuoffset + 2*4] = 
        output[mcuoffset + 3*4] = c;
      goto cont_idct_cols;
      full_idct_cols:
       int16 cx5, cx30, cx12;
       int16 cres1, cres2, cres3;
       int16 cx32, cx42;

       cx30 = *(pSrc_0_8);
       cx5  = *(pSrc_1_8);
       cx12 = *(pSrc_2_8);

       cres1 = imul_b5(cx5);
       cres2 = imul_b4(cx5) + cx5;
       cres3 = imul_b1_b3(cx5) + cres2;

       /* same index as before */
       // descale, convert to unsigned and clamp to 8-bit
       t = ((int16)(cx30 + cx12 + cx5) >> PJPG_DCT_SCALE_BITS) + 128;
       if (t & 0xF000)
         output[mcuoffset + 0*4] = 0;
       else if (t & 0xFF00)
          output[mcuoffset + 0*4] = 255;
       else
         output[mcuoffset + 0*4] = (uint8)t;

       cx32 = imul_b1_b3(cx12) - cx12;
       t = ((int16)(cx30 + cx32 + cres2) >> PJPG_DCT_SCALE_BITS) + 128;
       if (t & 0xF000)
         output[mcuoffset + 3*4] = 0;
       else if (t & 0xFF00)
          output[mcuoffset + 3*4] = 255;
       else
         output[mcuoffset + 3*4] = (uint8)t;

       t = ((int16)(cx30 + cres3 - cx32) >> PJPG_DCT_SCALE_BITS) + 128;
       if (t & 0xF000)
         output[mcuoffset + 1*4] = 0;
       else if (t & 0xFF00)
          output[mcuoffset + 1*4] = 255;
       else
         output[mcuoffset + 1*4] = (uint8)t;

       t = ((int16)(cx30 + cres3 + cres1 - cx12) >> PJPG_DCT_SCALE_BITS) + 128;
       if (t & 0xF000)
         output[mcuoffset + 2*4] = 0; 
       else if (t & 0xFF00)
          output[mcuoffset + 2*4] = 255;
       else 
         output[mcuoffset + 2*4] = (uint8)t;


      cont_idct_cols:
      pSrc_0_8++;
      pSrc_1_8++;
      pSrc_2_8++;
      mcuoffset++;
   }
}

uint8 decodeNextMCU(uint8 *pDst_row)
{
  uint8 status;
  uint8 mcuBlock;
  /* Do not use zp vars here, it'll be destroyed by transformBlock
  * and idct*
  */
  uint8 componentID;
  uint8 *pQ_l, *pQ_h;
  uint8 compACTab;
  uint16 r;
  uint8 s, i;
  uint16 extraBits;
  uint8 compDCTab;
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
    componentID = gMCUOrg[mcuBlock];
    if (gCompQuant[componentID]) {
      pQ_l = gQuant1_l;
      pQ_h = gQuant1_h;
    } else {
      pQ_l = gQuant0_l;
      pQ_h = gQuant0_h;
    }


    compACTab = gCompACTab[componentID];
    compDCTab = gCompDCTab[componentID];

    /* Pre-zero coeffs we'll use in idctRows */
    gCoeffBuf[1] =
      gCoeffBuf[2] =
      gCoeffBuf[8] =
      gCoeffBuf[9] =
      gCoeffBuf[10] =
      gCoeffBuf[16] =
      gCoeffBuf[17] =
      gCoeffBuf[18] = 0;

    if (compDCTab)
      s = huffDecode(&gHuffTab1, gHuffVal1);
    else
      s = huffDecode(&gHuffTab0, gHuffVal0);

    r = 0;
    numExtraBits = s & 0xF;
    if (numExtraBits)
       r = getBitsFF(numExtraBits);

    dc = huffExtend(r, s);

    dc = dc + gLastDC_l[componentID] + (gLastDC_h[componentID]<<8);
    gLastDC_l[componentID] = dc & 0xFF;
    gLastDC_h[componentID] = (dc & 0xFF00) >> 8;

    gCoeffBuf[0] = dc * (pQ_l[0]|(pQ_h[0]<<8));

    for (i = 1; i != 64; i++) {
      if (compACTab)
        s = huffDecode(&gHuffTab3, gHuffVal3);
      else
        s = huffDecode(&gHuffTab2, gHuffVal2);

      r = s >> 4;
      i += r;

      s = s & 0xF;
      if (!s) {
        if (r != 15) {
          break;
        }
      } else {
        extraBits = getBitsFF(s);

        if (ZAG_Coeff[i] != 0xFF) {
          ac = huffExtend(extraBits, s);
          // printf("computing %d\n", ZAG_Coeff[i]);
          gCoeffBuf[ZAG_Coeff[i]] = ac * (pQ_l[i]|(pQ_h[i] << 8));
        }
      }
    }
    idctRows();
    idctCols(mcuBlock == 0 ? 0 : 16);
  }

   /* Skip the other blocks, do the minimal work, only consuming
    * input bits
    */
  skipBits = 1;
  for (mcuBlock = 2; mcuBlock < gMaxBlocksPerMCU; mcuBlock++) {
    componentID = gMCUOrg[mcuBlock];

    if (gCompDCTab[componentID])
      s = huffDecode(&gHuffTab1, gHuffVal1);
    else
      s = huffDecode(&gHuffTab0, gHuffVal0);

    compACTab = gCompACTab[componentID];

    if (s & 0xF)
      getBitsFF(s & 0xF);

    for (i = 1; i != 64;) {
      if (compACTab)
        s = huffDecode(&gHuffTab3, gHuffVal3);
      else
        s = huffDecode(&gHuffTab2, gHuffVal2);

      numExtraBits = s & 0xF;
      if (numExtraBits)
        getBitsFF(numExtraBits);

      if (!s) {
        break;
      } else {
        i += (s >> 4) + 1;
      }
   }
  }
  skipBits = 0;

  return 0;
}

//------------------------------------------------------------------------------
unsigned char pjpeg_decode_mcu(uint8 *pDst_row)
{
   uint8 status;

   if ((!gNumMCUSRemainingX) && (!gNumMCUSRemainingY))
      return PJPG_NO_MORE_BLOCKS;

   status = decodeNextMCU(pDst_row);
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

void copy_decoded_to(uint8 *pDst_row)
{
  uint8 by;
  register uint8 *pDst1, *pDst2;
  register uint8 s = 0;

  pDst1 = pDst_row;
  pDst2 = pDst1 + 4;

  by = 4;
  while (1) {
    *(pDst1+s) = gMCUBufG[s];
    *(pDst2+s) = (gMCUBufG+16)[s];
    s++;
    *(pDst1+s) = gMCUBufG[s];
    *(pDst2+s) = (gMCUBufG+16)[s];
    s++;
    *(pDst1+s) = gMCUBufG[s];
    *(pDst2+s) = (gMCUBufG+16)[s];
    s++;
    *(pDst1+s) = gMCUBufG[s];
    *(pDst2+s) = (gMCUBufG+16)[s];
    s++;

    if (!--by)
      break;
    pDst1 += (DECODED_WIDTH-4);
    pDst2 += (DECODED_WIDTH-4);
  }
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
