#ifndef __extra_zp_h
#define __extra_zp_h

#include "platform.h"

/* https://fadden.com/apple2/dl/zero-page.txt
 * Of course don't use them randomly
 */

/* Reserved for serial use */
extern uint8 *prev_ram_irq_vector;
extern uint8 *prev_rom_irq_vector;
extern uint8 a_backup;

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

extern uint8 *zp2p;
extern uint8 *zp4p;
extern uint8 *zp6p;
extern uint8 *zp8p;
extern uint8 *zp10p;
extern uint8 *zp12p;

extern int8 *zp2sp;
extern int8 *zp4sp;
extern int8 *zp6sp;
extern int8 *zp8sp;
extern int8 *zp10sp;
extern int8 *zp12sp;

extern uint16 *zp2ip;
extern uint16 *zp4ip;
extern uint16 *zp6ip;
extern uint16 *zp8ip;
extern uint16 *zp10ip;
extern uint16 *zp12ip;

extern uint16 zp2i;
extern uint16 zp4i;
extern uint16 zp6i;
extern uint16 zp8i;
extern uint16 zp10i;
extern uint16 zp12i;

extern int16 zp2si;
extern int16 zp4si;
extern int16 zp6si;
extern int16 zp8si;
extern int16 zp10si;
extern int16 zp12si;

extern int16 *zp2sip;
extern int16 *zp4sip;
extern int16 *zp6sip;
extern int16 *zp8sip;
extern int16 *zp10sip;
extern int16 *zp12sip;


#ifdef __CC65__
#pragma zpsym ("prev_ram_irq_vector")
#pragma zpsym ("prev_rom_irq_vector")
#pragma zpsym ("a_backup")

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

#pragma zpsym ("zp2s")
#pragma zpsym ("zp3s")
#pragma zpsym ("zp4s")
#pragma zpsym ("zp5s")
#pragma zpsym ("zp6s")
#pragma zpsym ("zp7s")
#pragma zpsym ("zp8s")
#pragma zpsym ("zp9s")
#pragma zpsym ("zp10s")
#pragma zpsym ("zp11s")
#pragma zpsym ("zp12s")
#pragma zpsym ("zp13s")

#pragma zpsym ("zp2p")
#pragma zpsym ("zp4p")
#pragma zpsym ("zp6p")
#pragma zpsym ("zp8p")
#pragma zpsym ("zp10p")
#pragma zpsym ("zp12p")

#pragma zpsym ("zp2sp")
#pragma zpsym ("zp4sp")
#pragma zpsym ("zp6sp")
#pragma zpsym ("zp8sp")
#pragma zpsym ("zp10sp")
#pragma zpsym ("zp12sp")

#pragma zpsym ("zp2ip")
#pragma zpsym ("zp4ip")
#pragma zpsym ("zp6ip")
#pragma zpsym ("zp8ip")
#pragma zpsym ("zp10ip")
#pragma zpsym ("zp12ip")

#pragma zpsym ("zp2i")
#pragma zpsym ("zp4i")
#pragma zpsym ("zp6i")
#pragma zpsym ("zp8i")
#pragma zpsym ("zp10i")
#pragma zpsym ("zp12i")

#pragma zpsym ("zp2si")
#pragma zpsym ("zp4si")
#pragma zpsym ("zp6si")
#pragma zpsym ("zp8si")
#pragma zpsym ("zp10si")
#pragma zpsym ("zp12si")

#pragma zpsym ("zp2sip")
#pragma zpsym ("zp4sip")
#pragma zpsym ("zp6sip")
#pragma zpsym ("zp8sip")
#pragma zpsym ("zp10sip")
#pragma zpsym ("zp12sip")
#endif
#endif
