#ifndef __surl_h
#define __surl_h

#include <arpa/inet.h>

#include "../surl-server/surl_protocol.h"
#include "simple_serial.h"

#ifndef __CC65__
#define __fastcall__
#endif

typedef struct _surl_response surl_response;
struct _surl_response {
#ifdef __CC65__
  unsigned int code;
  size_t size;
  size_t header_size;
  size_t content_type_size;
  size_t cur_pos;
  size_t cur_hdr_pos;
#else
  unsigned short code;
  unsigned short size;
  unsigned short header_size;
  unsigned short content_type_size;
  unsigned short cur_pos;
  unsigned short cur_hdr_pos;
#endif
  char *content_type;

};

char __fastcall__ surl_connect_proxy(void);
void surl_disconnect_proxy(void);

const surl_response * __fastcall__ surl_start_request(const char method, char *url, char **headers, int n_headers);

void __fastcall__ surl_response_done(void);
char __fastcall__ surl_response_ok(void);
int __fastcall__ surl_response_code(void);

void __fastcall__ surl_read_response_header(void);

size_t __fastcall__ surl_receive_data(char *buffer, size_t max_len);
size_t __fastcall__ surl_receive_headers(char *buffer, size_t max_len);

size_t __fastcall__ surl_receive_lines(char *buffer, size_t max_len);

void surl_strip_html(char strip_level);
void surl_translit(char *charset);

int __fastcall__ surl_send_data_params(size_t total, int raw);
#define surl_send_data(buffer, len) simple_serial_write(buffer, len)

int __fastcall__ surl_find_line(char *buffer, size_t max_len, char *search_str);
int __fastcall__ surl_get_json(char *buffer, size_t max_len, char striphtml, const char *translit, const char *selector);

/* Helper to set the date */
void __fastcall__ surl_set_time(void);

/* Helper to check connectivity */
void __fastcall__ surl_ping(void);

/* Multipart helpers */
#define surl_multipart_send_num_fields(x) simple_serial_putc(x)

#define surl_multipart_send_field_desc(name, len, type) do { \
  unsigned short h_len = htons(len);                         \
  simple_serial_puts(name);                                  \
  simple_serial_putc('\n');                                  \
  simple_serial_puts(type);                                  \
  simple_serial_putc('\n');                                  \
  simple_serial_write((char *)&h_len, 2);                    \
} while (0)

#define surl_multipart_send_field_data(data, len) surl_send_data(data, len)

#ifdef SER_DEBUG
void surl_do_debug(const char *file, int line, const char *format, ...);
#define surl_debug(...) surl_do_debug(__FILE__, __LINE__, __VA_ARGS__);
#else
#define surl_debug(...) do {} while(0)
#endif

#endif
