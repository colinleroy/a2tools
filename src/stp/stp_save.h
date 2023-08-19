#ifndef __stp_save_h
#define __stp_save_h

#include "surl.h"

int stp_save_dialog(char *url, const surl_response *resp, char *out_dir);
int stp_save(char *full_filename, char *out_dir, const surl_response *resp);
char *stp_confirm_save_all(void);

#endif
