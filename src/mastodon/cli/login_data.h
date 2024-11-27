#ifndef __login_data_h
#define __login_data_h

typedef struct _login_data {
  char instance_url[70];
  char client_id[50];
  char client_secret[50];
  char login[50];
  char oauth_code[50];
  char oauth_token[50];
  char monochrome;
  char charset[16];
} login_data_t;

extern login_data_t login_data;

#endif
