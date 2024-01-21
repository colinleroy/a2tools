#ifndef __char_convert_h
#define __char_convert_h

char *do_charset_convert(char *in, int way, char *a2charset, int lowercase, size_t *new_len);

enum {
  INCOMING,
  OUTGOING
};

#endif
