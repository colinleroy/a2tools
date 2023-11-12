#ifndef __malloc0_h
#define __malloc0_h

#ifndef __CC65__
#define __fastcall__
#endif

void * __fastcall__ malloc0(size_t size);
void * __fastcall__ realloc_safe(void *ptr, size_t size);

#endif
