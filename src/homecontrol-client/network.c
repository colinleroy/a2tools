#include "network.h"

http_response *get_url(const char *url) {
  const char *headers[1] = {"Accept: text/*"};

  return http_request("GET", url, headers, 1);
}
