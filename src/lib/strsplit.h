#ifndef __strsplit_h
#define __strsplit_h

#ifndef _CC65__
#define __fastcall__
#endif

/* Allocates out */
int __fastcall__ _strsplit_int(char in_place, char *in, char split, char ***out);
#define strsplit(in, split, out) _strsplit_int(0, in, split, out)
#define strsplit_in_place(in, split, out) _strsplit_int(1, in, split, out)

/* Does not allocate out! */
int __fastcall__ _strnsplit_int(char in_place, char *in, char split, char **tokens, size_t max_tokens);
#define strnsplit(in, split, out, max_tokens) _strnsplit_int(0, in, split, out, max_tokens)
#define strnsplit_in_place(in, split, out, max_tokens) _strnsplit_int(1, in, split, out, max_tokens)

#endif
