#ifndef __header_h
#define __header_h

#include "list.h"

char __fastcall__ print_header(list *l, status *root_status, notification *root_notif);
void show_help (list *l, status *root_status, notification *root_notif);
void __fastcall__ print_free_ram(void);

#endif
