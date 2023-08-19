#ifndef __stp_cli_h
#define __stp_cli_h

#include "surl.h"

void stp_print_header(char *url);
void stp_print_footer(void);
void stp_print_result(const surl_response *response);

#endif
