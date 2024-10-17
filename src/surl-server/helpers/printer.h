#ifndef __printer_h
#define __printer_h

#define N_PAPER_SIZES 7

#include "../surl_protocol.h"

int install_printer_thread(void);
int start_printer_thread(void);
int stop_printer_thread(void);

const char *printer_get_iwem(void);
#endif
