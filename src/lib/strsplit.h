#ifndef __strsplit_h
#define __strsplit_h

#ifndef _CC65__
#define __fastcall__
#endif

int __fastcall__ strsplit(char *in, char split, char ***out);
int __fastcall__ strsplit_in_place(char *in, char split, char ***out);

int __fastcall__ strnsplit(char *in, char split, char ***out, size_t max_tokens);
int __fastcall__ strnsplit_in_place(char *in, char split, char ***out, size_t max_tokens);

#endif
