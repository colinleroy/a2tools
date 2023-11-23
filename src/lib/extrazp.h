#ifndef __extra_zp_h
#define __extra_zp_h

#include "platform.h"

/* https://fadden.com/apple2/dl/zero-page.txt
 * Of course don't use them randomly
 */

extern uint8 zp6;
extern uint8 zp7;
extern uint8 zp8;
extern uint8 zp9;
extern uint8 zp10;
extern uint8 zp11;
extern uint8 zp12;
extern uint8 zp13;

extern int8 zp6s;
extern int8 zp7s;
extern int8 zp8s;
extern int8 zp9s;
extern int8 zp10s;
extern int8 zp11s;
extern int8 zp12s;
extern int8 zp13s;

extern uint8 *zp6p;
extern uint8 *zp8p;
extern uint8 *zp10p;
extern uint8 *zp12p;

extern uint16 *zp6ip;
extern uint16 *zp8ip;
extern uint16 *zp10ip;
extern uint16 *zp12ip;

extern uint16 zp6i;
extern uint16 zp8i;
extern uint16 zp10i;
extern uint16 zp12i;

extern int16 zp6si;
extern int16 zp8si;
extern int16 zp10si;
extern int16 zp12si;

extern int16 *zp6sip;
extern int16 *zp8sip;
extern int16 *zp10sip;
extern int16 *zp12sip;


#ifdef __CC65__
#pragma zpsym ("zp6")
#pragma zpsym ("zp7")
#pragma zpsym ("zp8")
#pragma zpsym ("zp9")
#pragma zpsym ("zp10")
#pragma zpsym ("zp11")
#pragma zpsym ("zp12")
#pragma zpsym ("zp13")

#pragma zpsym ("zp6p")
#pragma zpsym ("zp8p")
#pragma zpsym ("zp10p")
#pragma zpsym ("zp12p")

#pragma zpsym ("zp6ip")
#pragma zpsym ("zp8ip")
#pragma zpsym ("zp10ip")
#pragma zpsym ("zp12ip")

#pragma zpsym ("zp6i")
#pragma zpsym ("zp8i")
#pragma zpsym ("zp10i")
#pragma zpsym ("zp12i")

#pragma zpsym ("zp6si")
#pragma zpsym ("zp8si")
#pragma zpsym ("zp10si")
#pragma zpsym ("zp12si")

#pragma zpsym ("zp6sip")
#pragma zpsym ("zp8sip")
#pragma zpsym ("zp10sip")
#pragma zpsym ("zp12sip")
#endif
#endif
