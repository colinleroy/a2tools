#ifndef __http_h
#define __http_h

typedef struct _http_response http_response;
struct _http_response {
  char *body;
  int code;
  long size;
};

void http_connect_proxy(void);

http_response *http_request(const char *method, const char *url, const char **headers, int n_headers);

void http_response_free(http_response *resp);

#endif
