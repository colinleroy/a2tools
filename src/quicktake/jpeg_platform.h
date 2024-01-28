#ifndef __JPEG_PLATFORM_H
#define __JPEG_PLATFORM_H

#include "platform.h"

typedef struct HuffTableT
{
   uint16 mMinCode[16];
   uint16 mMaxCode[16];
   uint8 mValPtr[16];
} HuffTable;


extern uint16 extendTests[];
extern uint16 extendOffsets[];
extern uint8 mul669_l[256], mul669_m[256], mul669_h[256];
extern uint8 mul362_l[256], mul362_m[256], mul362_h[256];
extern uint8 mul277_l[256], mul277_m[256], mul277_h[256];
extern uint8 mul196_l[256], mul196_m[256], mul196_h[256];
extern int16 gCoeffBuf[8*8];
extern uint16 gRestartInterval;
extern uint16 gRestartsLeft;
extern int16 *ZAG_Coeff[];
extern uint8 gMaxBlocksPerMCU;
extern uint8 gCompsInFrame;
extern uint8 gCompIdent[3];
extern uint8 gCompHSamp[3];
extern uint8 gCompVSamp[3];
extern uint8 gCompQuant[3];

extern uint16 gNextRestartNum;

extern uint8 gCompsInScan;
extern uint8 gCompList[3];
extern uint8 gCompDCTab[3]; // 0,1
extern uint8 gCompACTab[3]; // 0,1

extern HuffTable gHuffTab0;
extern uint8 gHuffVal0[16];

extern HuffTable gHuffTab1;
extern uint8 gHuffVal1[16];

// AC - 672
extern HuffTable gHuffTab2;
extern uint8 gHuffVal2[256];

extern HuffTable gHuffTab3;
extern uint8 gHuffVal3[256];

extern uint8 gMCUBufG[256];
// 256 bytes
extern uint16 gQuant0[8*8];
extern uint16 gQuant1[8*8];

// 6 bytes
extern uint16 gLastDC[3];

extern uint16 gNumMCUSRemainingX, gNumMCUSRemainingY;
extern uint8 gMCUOrg[6];

int16 __fastcall__ huffExtend(uint16 x, uint8 s);
uint16 __fastcall__ getBits1(uint8 numBits);
uint16 __fastcall__ getBits2(uint8 numBits);
uint8 __fastcall__ getOctet(uint8 FFCheck);
uint8 getBit(void);
void fillInBuf(void);

uint16 __fastcall__ imul_b1_b3(int16 w);
uint16 __fastcall__ imul_b2(int16 w);
uint16 __fastcall__ imul_b4(int16 w);
uint16 __fastcall__ imul_b5(int16 w);

uint8 __fastcall__ huffDecode(HuffTable* pHuffTable, const uint8* pHuffVal);
uint8 decodeNextMCU(void);
void transformBlock(uint8 mcuBlock);
void idctRows(void);
void idctCols(void);
uint8 processRestart(void);

#endif
