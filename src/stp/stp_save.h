#ifndef __stp_save_h
#define __stp_save_h

#include "surl.h"

int stp_save_dialog(char *url, surl_response *resp, char confirm);
int stp_save(char *full_filename, surl_response *resp);

#endif
