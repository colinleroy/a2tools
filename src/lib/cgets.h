#ifndef __cgets_h
#define __cgets_h

#ifndef __CC65__
#define __fastcall__
#endif
void echo(int on);
char * __fastcall__ cgets(char *buf, size_t size);
#endif
