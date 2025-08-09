#include "jpeg_platform.h"
#include "qt-conv.h"

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
  return getBits(numBits);
}

extern uint8 *cur_cache_ptr;
extern uint8 *cache_end;

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
  uint16 code;
  HuffTable *curTable = pHuffTable;

  FFCheck = 1;
  code = getBit();

  pHuffTable->totalCalls++;
  pHuffTable->totalGetBit++;

  /* First loop only checking low bytes as long
   * as code < 0x100 */
  for ( ; ; ) {
    if (pHuffTable->mGetMore[i])
      goto incrementS;

    if (pHuffTable->mMaxCode_l[i] >= code)
      goto loopDone;
incrementS:
    code <<= 1;
    code |= getBit();
    pHuffTable->totalGetBit++;
    i++;
    if (i == 7)
      goto long_search;
  }
loopDone:
  j = pHuffTable->mValPtr[i] + (uint8)code ;
  return pHuffVal[j];

/* No find, keep going with 16bits */
long_search:
  for ( ; ; ) {

    if (pHuffTable->mGetMore[i])
      goto incrementL;

    if (pHuffTable->mMaxCode_h[i] < (code>>8))
      goto incrementL;
    else if (pHuffTable->mMaxCode_h[i] > (code>>8))
      goto loopDone;

    if (pHuffTable->mMaxCode_l[i] >= (code&0xFF))
      goto loopDone;

incrementL:
    i++;
    if (i == 16)
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
       x4  = *(rowSrc_5) - *(rowSrc_3);
       x5  = *(rowSrc_1) + *(rowSrc_7);
       x6  = *(rowSrc_1) - *(rowSrc_7);
       x31 = *(rowSrc) - *(rowSrc_4);
       x30 = *(rowSrc) + *(rowSrc_4);
       x13 = *(rowSrc_2) + *(rowSrc_6);

       /* update rowSrc */
       x17 = x5 + x7;
       *(rowSrc) = x30 + x13 + x17;

       x32 = imul_b1_b3(*(rowSrc_2) - *(rowSrc_6)) - x13;
       res1 = imul_b5(x4 - x6);
       res2 = imul_b4(x6) - res1 - x17;
       res3 = imul_b1_b3(x5 - x7) - res2;

       /* Update rowSrc_2 */
       *(rowSrc_2) = (res3 + x31) - x32;

       /* update rowSrc_4 */
       x24 = res1 - imul_b2(x4);
       *(rowSrc_4) = x24 + x30 + res3 - x13;

       /* update rowSrc_6 */
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
   int16 x4, x7, x5, x6, res1, x24, x15, x17, res2, res3, x44, x30, x31, x12, x13, x32, x43, x41, x42;
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
       t = (*pSrc_0_8 >> PJPG_DCT_SCALE_BITS) + 128;
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

       x17 = x5 + x7;

       res1 = imul_b5(x4 - x6);
       res2 = imul_b4(x6) - res1 - x17;
       res3 = imul_b1_b3(x5 - x7) - res2;

       x31 = *(pSrc_0_8) - *(pSrc_4_8);
       x30 = *(pSrc_0_8) + *(pSrc_4_8);
       x12 = *(pSrc_2_8) - *(pSrc_6_8); // only one
       x13 = *(pSrc_2_8) + *(pSrc_6_8);

       // descale, convert to unsigned and clamp to 8-bit
       t = ((x30 + x13 + x17) >> PJPG_DCT_SCALE_BITS) + 128;
       if (t < 0)
         *pSrc_0_8 = 0; 
       else if (t & 0xFF00)
          *pSrc_0_8 = 255;
       else 
         *pSrc_0_8 = (uint8)t;

       x32 = imul_b1_b3(*(pSrc_2_8) - *(pSrc_6_8)) - x13;
       x42 = x31 - x32;
       t = ((x42 + res3) >> PJPG_DCT_SCALE_BITS) + 128;
       if (t < 0)
         *pSrc_2_8 = 0; 
       else if (t & 0xFF00)
          *pSrc_2_8 = 255;
       else 
         *pSrc_2_8 = (uint8)t;

       x24 = res1 - imul_b2(x4); // only one
       t = ((x30 - x13 + res3 + x24) >> PJPG_DCT_SCALE_BITS) + 128;
       if (t < 0)
         *pSrc_4_8 = 0; 
       else if (t & 0xFF00)
          *pSrc_4_8 = 255;
       else 
         *pSrc_4_8 = (uint8)t;

       x41 = x31 + x32;
       t = ((x41 - res2) >> PJPG_DCT_SCALE_BITS) + 128;
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
  uint8 *cur_pQ_l, *cur_pQ_h;
  uint8 *cur_ZAG_coeff;
  uint8 *end_ZAG_coeff;


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

    compDCTab = gCompDCTab[componentID];
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

    cur_ZAG_coeff = ZAG_Coeff + 1;
    end_ZAG_coeff = ZAG_Coeff + 64;

    compACTab = gCompACTab[componentID];
    cur_pQ_l = pQ_l + 1;
    cur_pQ_h = pQ_h + 1;

    for (; cur_ZAG_coeff != end_ZAG_coeff; cur_ZAG_coeff++, cur_pQ_l++, cur_pQ_h++) {
      if (compACTab)
        s = huffDecode(&gHuffTab3, gHuffVal3);
      else
        s = huffDecode(&gHuffTab2, gHuffVal2);

      extraBits = 0;
      numExtraBits = s & 0xF;
      if (numExtraBits)
        extraBits = getBitsFF(numExtraBits);

      r = s >> 4;
      s = numExtraBits;

      if (s) {
        while (r) {
          gCoeffBuf[*cur_ZAG_coeff] = 0;

          cur_ZAG_coeff++;
          cur_pQ_l++;
          cur_pQ_h++;
          r--;
        }

        ac = huffExtend(extraBits, s);
        gCoeffBuf[*cur_ZAG_coeff] =  ac * (*cur_pQ_l|(*cur_pQ_h << 8));
      } else {
        if (r == 15) {
          cur_pQ_l+=15;
          cur_pQ_h+=15;
        } else {
          while (cur_ZAG_coeff != end_ZAG_coeff) {
            gCoeffBuf[*cur_ZAG_coeff] = 0;
            cur_ZAG_coeff++;
          }
          break;
        }
      }
    }

    transformBlock(mcuBlock);
  }

   /* Skip the other blocks, do the minimal work, only consuming
    * input bits
    */
  for (mcuBlock = 2; mcuBlock < gMaxBlocksPerMCU; mcuBlock++) {
    componentID = gMCUOrg[mcuBlock];

    if (gCompDCTab[componentID])
      s = huffDecode(&gHuffTab1, gHuffVal1);
    else
      s = huffDecode(&gHuffTab0, gHuffVal0);

    compACTab = gCompACTab[componentID];

    if (s & 0xF)
      getBitsFF(s & 0xF);

    for (i = 1; i != 64; i++) {
      if (compACTab)
        s = huffDecode(&gHuffTab3, gHuffVal3);
      else
        s = huffDecode(&gHuffTab2, gHuffVal2);

      numExtraBits = s & 0xF;
      if (numExtraBits)
        getBitsFF(numExtraBits);

      if (s) {
        i += s >> 4;
      } else {
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

  if (mcuBlock == 0) {
    pGDst = gMCUBufG;
  } else {
    pGDst = gMCUBufG + 32;
  }

  idctRows();
  idctCols();

  pSrc = gCoeffBuf;
  for (iTB = 32; iTB; iTB--) {
    *pGDst++ = (uint8)*pSrc;
    pSrc += 2;
  }
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

void copy_decoded_to(uint8 *pDst_row)
{
  uint8 by;
  register uint8 *pDst1, *pDst2;
  register uint8 s = 0;

  pDst1 = pDst_row;
  pDst2 = pDst1 + (8>>1);

  by = 4;
  while (1) {
    *(pDst1+s) = gMCUBufG[s];
    *(pDst2+s) = (gMCUBufG+32)[s];
    s++;
    *(pDst1+s) = gMCUBufG[s];
    *(pDst2+s) = (gMCUBufG+32)[s];
    s++;
    *(pDst1+s) = gMCUBufG[s];
    *(pDst2+s) = (gMCUBufG+32)[s];
    s++;
    *(pDst1+s) = gMCUBufG[s];
    *(pDst2+s) = (gMCUBufG+32)[s];

    if (!--by)
      break;
    pDst1 += (DECODED_WIDTH-8);
    pDst2 += (DECODED_WIDTH-8);
    s += (4+1);
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
   register uint8 *ptr_winograd = gWinogradQuant;

   for (i = 0; i < 64; i++)
   {
      long x = (long)(gQuant0_l[i] | (gQuant0_h[i]<<8));
      int16 r;
      x *= gWinogradQuant[i];
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
   register uint8 *ptr_winograd = gWinogradQuant;

   for (i = 0; i < 64; i++)
   {
      long x = (long)(gQuant1_l[i] | (gQuant1_h[i]<<8));
      int16 r;
      x *= *(ptr_winograd++);
      r = (int16)((x + (1 << (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS - 1))) >> (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS));
      gQuant1_l[i] = (uint8)(r);
      gQuant1_h[i] = (uint8)(r >> 8);
   }
}
