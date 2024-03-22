#ifndef __charsets_h
#define __charsets_h

/* Refs: https://www.aivosto.com/articles/charsets-7bit.html
 * and https://archive.org/details/Apple_IIgs_Hardware_Reference_HiRes/page/n275/mode/1up?view=theater
 * FIXME make that configurable */
#define US_CHARSET "US-ASCII"
#define FR_CHARSET "ISO646-FR1"
#define ES_CHARSET "ISO646-ES"
#define IT_CHARSET "ISO646-IT"
#define DE_CHARSET "ISO646-DE"
extern char *translit_charset;

#endif
