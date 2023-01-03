#ifndef __http_h
#define __http_h

typedef struct _http_response http_response;
struct _http_response {
  int code;
  size_t size;
  size_t cur_pos;
  char *content_type;
};

void http_connect_proxy(void);
void http_close_proxy(void);

http_response *http_start_request(const char *method, const char *url, const char **headers, int n_headers);

size_t http_receive_data(http_response *resp, char *buffer, size_t max_len);
size_t http_receive_lines(http_response *resp, char *buffer, size_t max_len);

void http_response_free(http_response *resp);

#endif
