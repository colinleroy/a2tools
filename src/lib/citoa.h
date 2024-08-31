#ifndef __citoa_h
#define __citoa_h

#ifdef __CC65__

void __fastcall__  citoa(int n);
void __fastcall__  cutoa(int n);

#else

#define citoa(x) do { printf("%d", x); } while(0)
#define cutoa(x) do { printf("%u", x); } while(0)

#endif

#endif
