#ifndef __ntohs_h
#define __ntohs_h

#ifndef __CC65__
#include <arpa/inet.h>
#else

int __fastcall__ ntohs (int x);
#define htons(x) ntohs(x)

#define ntohl(x) ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8) \
                  | (((x) & 0x0000ff00u) << 8) | (((x) & 0x000000ffu) << 24))
#define htonl(x) ntohl(x)

#endif

#endif
