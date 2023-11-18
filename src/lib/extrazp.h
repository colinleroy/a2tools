#ifndef __extra_zp_h
#define __extra_zp_h

#include "platform.h"

/* https://fadden.com/apple2/dl/zero-page.txt
 * Of course don't use them randomly
 */

extern uint8 zp1;
extern uint8 zp2;
extern uint8 zp3;
extern uint8 zp4;
extern uint8 zp5;
extern uint8 zp6;
extern uint8 zp7;
extern uint8 zp8;
extern uint8 zp9;
extern uint8 zp10;
extern uint8 zp11;
extern uint8 zp12;
extern uint8 zp13;

extern int8 zp1s;
extern int8 zp2s;
extern int8 zp3s;
extern int8 zp4s;
extern int8 zp5s;
extern int8 zp6s;
extern int8 zp7s;
extern int8 zp8s;
extern int8 zp9s;
extern int8 zp10s;
extern int8 zp11s;
extern int8 zp12s;
extern int8 zp13s;

extern uint8 *zp1p;
extern uint8 *zp3p;
extern uint8 *zp6p;
extern uint8 *zp8p;
extern uint8 *zp10p;
extern uint8 *zp12p;

extern uint16 *zp1ip;
extern uint16 *zp3ip;
extern uint16 *zp6ip;
extern uint16 *zp8ip;
extern uint16 *zp10ip;
extern uint16 *zp12ip;

extern uint16 zp1i;
extern uint16 zp3i;
extern uint16 zp6i;
extern uint16 zp8i;
extern uint16 zp10i;
extern uint16 zp12i;

extern int16 zp1si;
extern int16 zp3si;
extern int16 zp6si;
extern int16 zp8si;
extern int16 zp10si;
extern int16 zp12si;

#ifdef __CC65__
#pragma zpsym ("zp1")
#pragma zpsym ("zp2")
#pragma zpsym ("zp3")
#pragma zpsym ("zp4")
#pragma zpsym ("zp5")
#pragma zpsym ("zp6")
#pragma zpsym ("zp7")
#pragma zpsym ("zp8")
#pragma zpsym ("zp9")
#pragma zpsym ("zp10")
#pragma zpsym ("zp11")
#pragma zpsym ("zp12")
#pragma zpsym ("zp13")

#pragma zpsym ("zp1p")
#pragma zpsym ("zp3p")
#pragma zpsym ("zp6p")
#pragma zpsym ("zp8p")
#pragma zpsym ("zp10p")
#pragma zpsym ("zp12p")

#pragma zpsym ("zp1ip")
#pragma zpsym ("zp3ip")
#pragma zpsym ("zp6ip")
#pragma zpsym ("zp8ip")
#pragma zpsym ("zp10ip")
#pragma zpsym ("zp12ip")

#pragma zpsym ("zp1i")
#pragma zpsym ("zp3i")
#pragma zpsym ("zp6i")
#pragma zpsym ("zp8i")
#pragma zpsym ("zp10i")
#pragma zpsym ("zp12i")
#endif
#endif
