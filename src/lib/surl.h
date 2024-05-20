#ifndef __surl_h
#define __surl_h

#include <arpa/inet.h>
#include "platform.h"
#include "../surl-server/surl_protocol.h"
#include "simple_serial.h"

#ifndef __CC65__
#define __fastcall__
#endif

typedef struct _surl_response surl_response;
struct _surl_response {
  /* Mind the alignment */
  uint32 size;
  uint16 code;
  uint16 header_size;
  uint16 content_type_size;
  uint16 cur_hdr_pos;
  uint32 cur_pos;
  char *content_type;

};

char __fastcall__ surl_connect_proxy(void);
void surl_disconnect_proxy(void);

const surl_response * __fastcall__ surl_start_request(const char method, char *url, char **headers, int n_headers);

void __fastcall__ surl_response_done(void);
char __fastcall__ surl_response_ok(void);
int __fastcall__ surl_response_code(void);

void __fastcall__ surl_read_response_header(void);

void __fastcall__ surl_read_with_barrier(char *buffer, size_t nmemb);

// size_t __fastcall__ surl_receive_data(char *buffer, size_t max_len);
size_t __fastcall__ surl_receive_bindata(char *buffer, size_t max_len, char binary);
#define surl_receive_data(buffer, max_len) surl_receive_bindata((buffer), (max_len), 0)

size_t __fastcall__ surl_receive_headers(char *buffer, size_t max_len);

size_t __fastcall__ surl_receive_lines(char *buffer, size_t max_len);

void surl_strip_html(char strip_level);
void surl_translit(char *charset);

int __fastcall__ surl_send_data_params(uint32 total, int raw);
#define surl_send_data(buffer, len) simple_serial_write(buffer, len)

int __fastcall__ surl_find_line(char *buffer, size_t max_len, char *search_str);
int __fastcall__ surl_get_json(char *buffer, size_t max_len, char striphtml, const char *translit, const char *selector);

#ifdef __CC65__
int surl_wait_for_stream(void);
int __fastcall__ surl_stream_video(void);
int __fastcall__ surl_stream_audio(char vu_x, char vu_y);
int __fastcall__ surl_stream_av(void);
#else
#define surl_stream_video()
#define surl_stream_audio()
#define surl_stream_av()
#endif
/* Helper to set the date */
void __fastcall__ surl_set_time(void);

/* Helper to check connectivity */
void __fastcall__ surl_ping(void);

/* Multipart helpers */
#define surl_multipart_send_num_fields(x) simple_serial_putc(x)

#define surl_multipart_send_field_desc(name, len, type) do { \
  unsigned long h_len = htonl(len);                          \
  simple_serial_puts(name);                                  \
  simple_serial_putc('\n');                                  \
  simple_serial_puts(type);                                  \
  simple_serial_putc('\n');                                  \
  simple_serial_write((char *)&h_len, 4);                    \
} while (0)

#define surl_multipart_send_field_data(data, len) surl_send_data(data, len)

#ifdef SER_DEBUG
void surl_do_debug(const char *file, int line, const char *format, ...);
#define surl_debug(...) surl_do_debug(__FILE__, __LINE__, __VA_ARGS__);
#else
#define surl_debug(...) do {} while(0)
#endif

#endif
