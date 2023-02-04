#ifndef __surl_h
#define __surl_h

#include "surl_protocol.h"

#ifndef __CC65__
#define __fastcall__
#endif

typedef struct _surl_response surl_response;
struct _surl_response {
  unsigned int code;
  size_t size;
  size_t header_size;

  size_t cur_pos;
  size_t cur_hdr_pos;

  char *content_type;

};

#define BUFSIZE 255
extern char surl_buf[BUFSIZE];

int __fastcall__ surl_connect_proxy(void);
#define surl_close_proxy() simple_serial_close()

surl_response * __fastcall__ surl_start_request(const char method, char *url, char **headers, int n_headers);

void __fastcall__ surl_read_response_header(surl_response *resp);

size_t __fastcall__ surl_receive_data(surl_response *resp, char *buffer, size_t max_len);
size_t __fastcall__ surl_receive_headers(surl_response *resp, char *buffer, size_t max_len);

size_t __fastcall__ surl_receive_lines(surl_response *resp, char *buffer, size_t max_len);

int __fastcall__ surl_send_data_params(surl_response *resp, size_t total, int raw);
#define surl_send_data(resp, buffer, len) simple_serial_write(buffer, 1, len)

int __fastcall__ surl_find_line(surl_response *resp, char *buffer, size_t max_len, char *search_str);
int __fastcall__ surl_get_json(surl_response *resp, char *buffer, size_t max_len, char striphtml, char *translit, char *selector);

void __fastcall__ surl_response_free(surl_response *resp);
char __fastcall__ surl_response_ok(surl_response *resp);

#endif
