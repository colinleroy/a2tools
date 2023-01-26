#ifndef __surl_h
#define __surl_h

typedef struct _surl_response surl_response;
struct _surl_response {
  int code;
  size_t size;
  size_t cur_pos;
  char *content_type;

  size_t header_size;
  size_t cur_hdr_pos;
};

int surl_connect_proxy(void);
void surl_close_proxy(void);

surl_response *surl_start_request(const char *method, const char *url, char **headers, int n_headers);

void surl_read_response_header(surl_response *resp);

size_t surl_receive_data(surl_response *resp, char *buffer, size_t max_len);
size_t surl_receive_headers(surl_response *resp, char *buffer, size_t max_len);

size_t surl_receive_lines(surl_response *resp, char *buffer, size_t max_len);

int surl_send_data_params(surl_response *resp, size_t total, int raw);
size_t surl_send_data(surl_response *resp, char *buffer, size_t len);

int surl_find_line(surl_response *resp, char *buffer, size_t max_len, char *search_str);
int surl_get_json(surl_response *resp, char *buffer, size_t max_len, char striphtml, char *translit, char *selector);

void surl_response_free(surl_response *resp);

#endif
