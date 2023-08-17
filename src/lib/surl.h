#ifndef __surl_h
#define __surl_h
#ifndef __CC65__
#include <arpa/inet.h>
#endif
#include "../surl-server/surl_protocol.h"

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

#ifdef __APPLE2ENH__
#define ntohs(x) (((x) >> 8) + (((x) & 0xff) << 8))
#define htons(x) ntohs(x)

#define ntohl(x) ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8) \
                  | (((x) & 0x0000ff00u) << 8) | (((x) & 0x000000ffu) << 24))
#define htonl(x) ntohl(x)
#endif

int __fastcall__ surl_connect_proxy(void);
#define surl_close_proxy() simple_serial_close()

surl_response * __fastcall__ surl_start_request(const char method, char *url, char **headers, int n_headers);

void __fastcall__ surl_response_free(surl_response *resp);
char __fastcall__ surl_response_ok(surl_response *resp);

void __fastcall__ surl_read_response_header(surl_response *resp);

size_t __fastcall__ surl_receive_data(surl_response *resp, char *buffer, size_t max_len);
size_t __fastcall__ surl_receive_headers(surl_response *resp, char *buffer, size_t max_len);

size_t __fastcall__ surl_receive_lines(surl_response *resp, char *buffer, size_t max_len);

/* No resp parameter but are expected to be called after
 * surl_start_request() and surl_response_ok().
 */
int __fastcall__ surl_send_data_params(size_t total, int raw);
#define surl_send_data(buffer, len) simple_serial_write(buffer, len)

int __fastcall__ surl_find_line(char *buffer, size_t max_len, char *search_str);
int __fastcall__ surl_get_json(char *buffer, size_t max_len, char striphtml, char *translit, char *selector);

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
#endif
