#include "network.h"
#include "simple_serial.h"

http_response *get_url(const char *url) {
  const char *headers[1] = {"Accept: text/*"};
  http_response *resp;
  
  simple_serial_set_activity_indicator(1, 39, 0);
  resp = http_request("GET", url, headers, 1);
  simple_serial_set_activity_indicator(0, 0, 0);

  return resp;
}
