#ifndef __dputc_h
#define __dputc_h

#ifndef __CC65__
#define init_fast_irq(x)
#else
void __fastcall__ init_fast_irq (void);
#endif
#endif
