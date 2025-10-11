#ifndef __JPEG_PLATFORM_H
#define __JPEG_PLATFORM_H

#include "platform.h"
#include "picojpeg.h"

#define QT200_WIDTH 640
#define QT200_HEIGHT 480

typedef struct HuffTableT
{
   uint8 mMaxCode_l[16];
   uint8 mMaxCode_h[16];
   uint8 mValPtr[16]; // actually uint8 but it's easier to have Y the same on Codes and Val
   uint8 mGetMore[16];
   uint32 totalCalls;
   uint32 totalGetBit;
} HuffTable;

uint8 getByteNoFF(void);
void setFFCheck(uint8 on);
extern uint16 extendTests[];
extern uint16 extendOffsets[];
extern int16 gCoeffBuf[8*8];
extern uint16 gRestartInterval;
extern uint16 gRestartsLeft;
extern uint8 ZAG_Coeff[];
extern uint8 ZAG_Coeff_work[];
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

extern uint8 gMCUBufG[128];
// 256 bytes
extern uint8 gQuant0_l[8*8];
extern uint8 gQuant0_h[8*8];
extern uint8 gQuant1_l[8*8];
extern uint8 gQuant1_h[8*8];

// 6 bytes
extern uint16 gLastDC;

extern uint8 gNumMCUSRemainingX, gNumMCUSRemainingY;
extern uint8 gMCUOrg[6];
extern uint8 gWinogradQuant[];

extern uint8 *output0, *output1, *output2, *output3;
extern uint16 outputIdx;
#pragma zpsym("outputIdx")

#ifdef __CC65__
void initFloppyStarter(void);
#endif

int16 __fastcall__ huffExtend(uint16 x, uint8 s);
uint16 __fastcall__ getBitsNoFF(uint8 numBits);
uint16 __fastcall__ getBitsFF(uint8 numBits);
void fillInBuf(void);

uint16 __fastcall__ imul_b1_b3(int16 w);
uint16 __fastcall__ imul_b4(int16 w);
uint16 __fastcall__ imul_b5(int16 w);

uint8 __fastcall__ huffDecode(HuffTable* pHuffTable, const uint8* pHuffVal);
void transformBlock(uint8 mcuBlock);
uint8 processRestart(void);

uint8 skipVariableMarker(void);

#define GMAXMCUXSIZE 16
#define GMAXMCUYSIZE 8

#define GMAXMCUSPERROW ((QT200_WIDTH + (GMAXMCUXSIZE - 1)) >> 4)
#define GMAXMCUSPERCOL ((QT200_HEIGHT + (GMAXMCUYSIZE - 1)) >> 3)
#define DECODED_WIDTH (QT200_WIDTH>>1)
#define DECODED_HEIGHT (QT200_HEIGHT>>1)

unsigned char pjpeg_decode_mcu(void);
void setQuant(uint8 quantId);
void setACDCTabs(void);

void createWinogradQuant0(void);
void createWinogradQuant1(void);
#endif
