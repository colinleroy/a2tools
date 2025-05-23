#ifndef __features_h
#define __features_h

#ifdef __CC65__

  signed char try_videomode(unsigned mode);
  #ifndef __APPLE2ENH__
  #define VIDEOMODE_40COL 0x15
  #define VIDEOMODE_80COL 0x00
  #endif
  extern unsigned char is_iigs;
  extern unsigned char is_iie;
  extern unsigned char is_iieenh;
  extern unsigned char has_80cols;
  extern unsigned char has_128k;

  #ifndef CH_CURS_UP
    #define CH_CURS_UP   0x0B
    #define CH_CURS_DOWN 0x0A
    #define CH_DEL       0x7F
  #endif

#else

  #define try_videomode()
  #define is_iigs 0
  #define is_iie 0
  #define is_iieenh 0
  #define has_80cols 1
  #define has_128k 1

#endif

#endif
