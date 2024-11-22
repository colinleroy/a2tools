#ifndef __dgets_h
#define __dgets_h

#include <string.h>
#ifndef __CC65__
#define __fastcall__
#endif

typedef char (*cmd_handler_func) (char);
/* dget_text_multi with enter_accepted = 0 is equivalent to dget_text_single, don't link both implementations */
char * __fastcall__ dget_text_single(char *buf, size_t size, cmd_handler_func cmd_cb);
char * __fastcall__ dget_text_multi(char *buf, size_t size, cmd_handler_func cmd_cb, char enter_accepted);

extern char dgets_echo_on;

#endif
