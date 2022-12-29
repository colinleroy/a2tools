#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http.h"
#include "simple_serial.h"
#include "extended_conio.h"

#define BUFSIZE 255

#ifndef __CC65__
#include "http_curl.c"

#else

void http_connect_proxy(void) {
#ifdef __CC65__
  simple_serial_open(2, SER_BAUD_9600);
  simple_serial_set_timeout(10);
#endif
}

static char buf[BUFSIZE];

http_response *http_request(const char *method, const char *url, const char **headers, int n_headers) {
  http_response *resp = malloc(sizeof(http_response));
  int i;

  simple_serial_printf("%s %s\n", method, url);
  for (i = 0; i < n_headers; i++) {
    simple_serial_printf("%s\n", headers[i]);
  }
  simple_serial_puts("\n");

  simple_serial_gets_with_timeout(buf, BUFSIZE);
  if (buf == NULL || buf[0] == '\0') {
    resp->size = 0;
    resp->code = 0;
    return resp;
  }

  if (strchr(buf, ',') == NULL) {
    printf("Unexpected response '%s'\n", buf);
    free(resp);
    return NULL;
  }

  resp->size = atol(strchr(buf, ',') + 1);
  *strchr(buf,',') = '\0';
  resp->code = atoi(buf);

  resp->body = malloc(resp->size + 1);
  if (resp->body == NULL) {
    printf("Could not malloc %ld bytes\n", resp->size);
    free(resp);
    return NULL;
  }

  simple_serial_read(resp->body, sizeof(char), resp->size + 1);
  resp->body[resp->size] = '\0';

  return resp;
}

void http_response_free(http_response *resp) {
  free(resp->body);
  free(resp);
}
#endif
