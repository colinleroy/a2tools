#ifndef __features_h
#define __features_h

#ifdef __CC65__

  extern unsigned char is_iigs;
  extern unsigned char is_iieenh;
  extern unsigned char has_80cols;

  #ifndef CH_CURS_UP
    #define CH_CURS_UP   0x0B
    #define CH_CURS_DOWN 0x0A
  #endif

#else

  #define is_iigs 0
  #define is_iieenh 0
  #define has_80cols 1

#endif

#endif
