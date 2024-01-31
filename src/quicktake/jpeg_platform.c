#include "jpeg_platform.h"
#include "qt-conv.h"

#define getExtendTest(i) ((i > 15) ? 0 : extendTests[i])
#define getExtendOffset(i) ((i > 15) ? 0 : extendOffsets[i])

int16 __fastcall__ huffExtend(uint16 x, uint8 s)
{
  uint16 lx = x, t = 0;
  uint8 ls = s;

  if (ls < 16)
    t = extendTests[ls];

  if (t > lx)
    return (int16)lx + getExtendOffset(ls);

  return (int16)lx;
}

extern uint16 gBitBuf;
extern uint8 gBitsLeft;

uint16 __fastcall__ getBits(uint8 numBits, uint8 FFCheck)
{
  uint8 n = numBits, tmp;
  uint8 ff = FFCheck;
  uint8 final_shift = 16 - n;
  uint16 ret = gBitBuf;

  if (n > 8) {
    n -= 8;

    gBitBuf <<= gBitsLeft;
    gBitBuf |= getOctet(ff);
    gBitBuf <<= (8 - gBitsLeft);
    ret = (ret & 0xFF00) | (gBitBuf >> 8);
  }

  if (gBitsLeft < n) {
    gBitBuf <<= gBitsLeft;
    gBitBuf |= getOctet(ff);
    tmp = n - gBitsLeft;
    gBitBuf <<= tmp;
    gBitsLeft = 8 - tmp;
  } else {
    gBitsLeft = gBitsLeft - n;
    gBitBuf <<= n;
  }

  return ret >> final_shift;
}
uint16 __fastcall__ getBits1(uint8 numBits) {
  return getBits(numBits, 0);
}
uint16 __fastcall__ getBits2(uint8 numBits) {
  return getBits(numBits, 1);
}

extern uint8 *cur_cache_ptr;
extern uint8 *cache_end;

uint8 getOctet(uint8 FFCheck)
{
  uint8 c, n;
  if (cur_cache_ptr == cache_end) {
    fillInBuf();
  }
  c = *(cur_cache_ptr++);
  if (!FFCheck)
    goto out;
  if (c != 0xFF)
    goto out;
  if (cur_cache_ptr == cache_end)
    fillInBuf();
  n = *(cur_cache_ptr++);
  if (n)
  {
     *(cur_cache_ptr--) = n;
     *(cur_cache_ptr--) = 0xFF;
  }
out:
  return c;
}

uint8 getBit(void)
{
  uint8 ret = 0;

  if (gBitBuf & 0x8000)
      ret = 1;

  if (!gBitsLeft)
  {
      gBitBuf |= getOctet(1);
      gBitsLeft = 7;
      gBitBuf <<= 1;
      return ret;
  }

  gBitsLeft--;
  gBitBuf <<= 1;

  return ret;
}

// 1/cos(4*pi/16)
// 362, 256+106
uint16 __fastcall__ imul_b1_b3(int16 w)
{
  uint32 x;
  int16 val = w;
  uint8 neg = 0;
  x = (uint32)w * 362;
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint16 __fastcall__ imul_b2(int16 w)
{
  uint32 x;
  int16 val = w;
  uint8 neg = 0;
  x = (uint32)w * 669;
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint16 __fastcall__ imul_b4(int16 w)
{
  uint32 x;
  int16 val = w;
  uint8 neg = 0;
  x = (uint32)w * 277;
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint16 __fastcall__ imul_b5(int16 w)
{
  uint32 x;
  int16 val = w;
  uint8 neg = 0;
  x = (uint32)w * 196;
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(x);

  return (uint16)x;
}

uint8 huffDecode(HuffTable* pHuffTable, const uint8* pHuffVal)
{
  uint8 i = 0;
  uint8 j;
  uint16 code = getBit();
  HuffTable *curTable = pHuffTable;
  register uint16 *curMaxCode = curTable->mMaxCode;
  register uint16 *curMinCode = curTable->mMinCode;
  register uint8 *curValPtr = curTable->mValPtr;

  for ( ; ; ) {
    if (i == 16)
      return 0;

    if (*curMaxCode == 0xFFFF) {
      goto noTest;
    }
    if (*curMaxCode < code) {
      goto noTest;
    }
    break;
    noTest:
    i++;
    curMaxCode++;
    curMinCode++;
    curValPtr++;

    code <<= 1;
    code |= getBit();
  }

  j = (uint8)*curValPtr + (uint8)code - (uint8)*curMinCode;
  return pHuffVal[j];
}

void idctRows(void)
{
   uint8 i;
   register int16* rowSrc;
   register int16* rowSrc_1;
   register int16* rowSrc_2;
   int16* rowSrc_3;
   int16* rowSrc_4;
   int16* rowSrc_5;
   int16* rowSrc_6;
   int16 stg26, x12;
   int16* rowSrc_7;
   int16 x7, x5, x15, x17, x6, x4, res1, x24, res2, res3, x30, x31, x13, x32;


   rowSrc = gCoeffBuf;
   i = 8;
nextIdctRowsLoop:
       rowSrc_1 = rowSrc + 1;
       rowSrc_2 = rowSrc + 2;
       rowSrc_3 = rowSrc + 3;
       rowSrc_4 = rowSrc + 4;
       rowSrc_5 = rowSrc + 5;
       rowSrc_6 = rowSrc + 6;
       rowSrc_7 = rowSrc + 7;
      if (*rowSrc_1 != 0 || *rowSrc_2 != 0 || *rowSrc_3 != 0 || *rowSrc_4 != 0 || *rowSrc_5 != 0 || *rowSrc_6 != 0 || *rowSrc_7 != 0)
        goto full_idct_rows;
       // Short circuit the 1D IDCT if only the DC component is non-zero
     *(rowSrc_2) = *(rowSrc_4) = *(rowSrc_6) = *rowSrc;
      goto cont_idct_rows;

      full_idct_rows:
       x7  = *(rowSrc_5) + *(rowSrc_3);
       x5  = *(rowSrc_1) + *(rowSrc_7);

       x6  = *(rowSrc_1) - *(rowSrc_7);
       x4  = *(rowSrc_5) - *(rowSrc_3);

       x30 = *(rowSrc) + *(rowSrc_4);
       x13 = *(rowSrc_2) + *(rowSrc_6);

       x31 = *(rowSrc) - *(rowSrc_4);
       x12 = *(rowSrc_2) - *(rowSrc_6);

       x32 = imul_b1_b3(x12) - x13;

       x15 = x5 - x7;
       x17 = x5 + x7;

       *(rowSrc) = x30 + x13 + x17;

       res1 = imul_b5(x4 - x6);
       stg26 = imul_b4(x6) - res1;
       res2 = stg26 - x17;
       res3 = imul_b1_b3(x15) - res2;

       x24 = res1 - imul_b2(x4);

       *(rowSrc_2) = x31 - x32 + res3;
       *(rowSrc_4) = x30 + res3 + x24 - x13;
       *(rowSrc_6) = x31 + x32 - res2;

      cont_idct_rows:
      rowSrc += 8;
      i--;
      if (i)
        goto nextIdctRowsLoop;
}

#define PJPG_DCT_SCALE_BITS 7

void idctCols(void)
{
   uint8 idctCC;

   register int16* pSrc_0_8;
   register int16* pSrc_2_8;
   register int16 *pSrc_3_8;
   int16 *pSrc_4_8;
   int16 *pSrc_6_8;
   int16 *pSrc_5_8;
   int16 *pSrc_1_8;
   int16 stg26;
   int16 *pSrc_7_8;
   int16 x4, x7, x5, x6, res1, x24, x15, x17, res2, res3, x44, x30, x31, x12, x13, x32, x40, x43, x41, x42;
   uint8 c;
   int16 t;

   pSrc_0_8 = gCoeffBuf+0*8;
   pSrc_1_8 = gCoeffBuf+1*8;
   pSrc_2_8 = gCoeffBuf+2*8;
   pSrc_3_8 = gCoeffBuf+3*8;
   pSrc_4_8 = gCoeffBuf+4*8;
   pSrc_5_8 = gCoeffBuf+5*8;
   pSrc_6_8 = gCoeffBuf+6*8;
   pSrc_7_8 = gCoeffBuf+7*8;

   for (idctCC = 0; idctCC < 8; idctCC++)
   {
      if (*pSrc_2_8 != 0)
        goto full_idct_cols;
      if (*pSrc_4_8 != 0)
        goto full_idct_cols;
      if (*pSrc_6_8 != 0)
        goto full_idct_cols;

       // Short circuit the 1D IDCT if only the DC component is non-zero
       t = (*pSrc_0_8 >> PJPG_DCT_SCALE_BITS)+128;
       if (t < 0)
         c = 0; 
       else if (t & 0xFF00)
          c = 255;
       else 
         c = (uint8)t;

       *(pSrc_0_8) =
         *(pSrc_2_8) =
         *(pSrc_4_8) =
         *(pSrc_6_8) = c;
      goto cont_idct_cols;
      full_idct_cols:

       x4  = *(pSrc_5_8) - *(pSrc_3_8);
       x7  = *(pSrc_5_8) + *(pSrc_3_8);
       x6  = *(pSrc_1_8) - *(pSrc_7_8);
       x5  = *(pSrc_1_8) + *(pSrc_7_8);

       x15 = x5 - x7;
       x17 = x5 + x7;

       res1 = imul_b5(x4 - x6);
       x24 = res1 - imul_b2(x4);

       stg26 = imul_b4(x6) - res1;
       res2 = stg26 - x17;

       res3 = imul_b1_b3(x15) - res2;
       x44 = res3 + x24;

       x31 = *(pSrc_0_8) - *(pSrc_4_8);
       x30 = *(pSrc_0_8) + *(pSrc_4_8);
       x12 = *(pSrc_2_8) - *(pSrc_6_8);
       x13 = *(pSrc_2_8) + *(pSrc_6_8);

       x32 = imul_b1_b3(x12) - x13;

       x40 = x30 + x13;
       x43 = x30 - x13;
       x41 = x31 + x32;
       x42 = x31 - x32;

       // descale, convert to unsigned and clamp to 8-bit
       t = ((x40 + x17) >> PJPG_DCT_SCALE_BITS) +128;
       if (t < 0)
         *pSrc_0_8 = 0; 
       else if (t & 0xFF00)
          *pSrc_0_8 = 255;
       else 
         *pSrc_0_8 = (uint8)t;

       t = ((x42 + res3) >> PJPG_DCT_SCALE_BITS) +128;
       if (t < 0)
         *pSrc_2_8 = 0; 
       else if (t & 0xFF00)
          *pSrc_2_8 = 255;
       else 
         *pSrc_2_8 = (uint8)t;

       t = ((x43 + x44) >> PJPG_DCT_SCALE_BITS) +128;
       if (t < 0)
         *pSrc_4_8 = 0; 
       else if (t & 0xFF00)
          *pSrc_4_8 = 255;
       else 
         *pSrc_4_8 = (uint8)t;

       t = ((x41 - res2) >> PJPG_DCT_SCALE_BITS) +128;
       if (t < 0)
         *pSrc_6_8 = 0; 
       else if (t & 0xFF00)
          *pSrc_6_8 = 255;
       else 
         *pSrc_6_8 = (uint8)t;

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

uint8 decodeNextMCU(void)
{
  uint8 status;
  uint8 mcuBlock;
  /* Do not use zp vars here, it'll be destroyed by transformBlock
  * and idct*
  */
  register uint8 *cur_gMCUOrg;
  uint8 componentID;
  uint16* pQ;
  uint8 compACTab;
  uint16 r;
  uint8 s, i;
  uint16 extraBits;
  uint8 compQuant;	
  uint8 compDCTab;
  uint8 numExtraBits;
  uint16 dc;
  uint16 ac;
  uint16 *cur_pQ;
  int16 **cur_ZAG_coeff;
  int16 **end_ZAG_coeff;


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
    componentID = *cur_gMCUOrg;
    compQuant = gCompQuant[componentID];	
    compDCTab = gCompDCTab[componentID];
    pQ = compQuant ? gQuant1 : gQuant0;
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

    cur_ZAG_coeff = (ZAG_Coeff + 1);
    end_ZAG_coeff = ZAG_Coeff + (64 - 1);

    compACTab = gCompACTab[componentID];
    cur_pQ = pQ + 1;

    for (; cur_ZAG_coeff != end_ZAG_coeff; cur_ZAG_coeff++, cur_pQ++) {
      if (compACTab)
        s = huffDecode(&gHuffTab3, gHuffVal3);
      else
        s = huffDecode(&gHuffTab2, gHuffVal2);

      extraBits = 0;
      numExtraBits = s & 0xF;
      if (numExtraBits)
        extraBits = getBits2(numExtraBits);

      r = s >> 4;
      s = numExtraBits;

      if (s) {
        while (r) {
          **cur_ZAG_coeff = 0;

          cur_ZAG_coeff++;
          cur_pQ++;
          r--;
        }

        ac = huffExtend(extraBits, s);
        **cur_ZAG_coeff =  ac * *cur_pQ;
      } else {
        if (r == 15) {
          for (s = r; s > 0; s--) {
            cur_ZAG_coeff++;
            cur_pQ++;
          }
        } else {
          break;
        }
      }
    }
    while (cur_ZAG_coeff != end_ZAG_coeff) {
      **cur_ZAG_coeff = 0;
      cur_ZAG_coeff++;
      cur_pQ++;
    }

    transformBlock(mcuBlock);
  }
   /* Skip the other blocks, do the minimal work */
  for (mcuBlock = 2; mcuBlock < gMaxBlocksPerMCU; mcuBlock++) {
    componentID = *cur_gMCUOrg;

    if (gCompDCTab[componentID])
      numExtraBits = huffDecode(&gHuffTab1, gHuffVal1) & 0xF;
    else
      numExtraBits = huffDecode(&gHuffTab0, gHuffVal0) & 0xF;

    compACTab = gCompACTab[componentID];

    r = 0;
    if (numExtraBits)
      r = getBits2(numExtraBits);

    huffExtend(r, s);

    for (i = 1; i != 64; i++) {
      if (compACTab)
        s = huffDecode(&gHuffTab3, gHuffVal3);
      else
        s = huffDecode(&gHuffTab2, gHuffVal2);

      extraBits = 0;
      numExtraBits = s & 0xF;
      if (numExtraBits)
        extraBits = getBits2(numExtraBits);

      r = s >> 4;
      s = numExtraBits;

      if (s) {
        i += r;
        huffExtend(extraBits, s);
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

void transformBlock(uint8 mcuBlock)
{
  uint8* pGDst;
  int16* pSrc;
  uint8 iTB;
  uint8 mB = mcuBlock;

  idctRows();
  idctCols();

  if (mB == 0) {
    pGDst = gMCUBufG;
  } else {
    pGDst = gMCUBufG + 64;
  }

  pSrc = gCoeffBuf;
  for (iTB = 64; iTB; iTB--) {
    *pGDst++ = (uint8)*pSrc++;
  }
}
