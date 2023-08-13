#ifndef __dgets_h
#define __dgets_h

#include <string.h>
#ifndef __CC65__
#define __fastcall__
#endif

typedef char (*cmd_handler_func) (char);
char * __fastcall__ dget_text(char *buf, size_t size, cmd_handler_func cmd_cb, char enter_accepted);

void __fastcall__  echo(int on);

#endif
