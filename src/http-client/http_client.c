#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http.h"
#include "simple_serial.h"
#include "extended_conio.h"

#define BUFSIZE 255
static char buf[BUFSIZE];

int main(int argc, char **argv) {
  http_response *response = NULL;
  const char *headers[1] = {"Accept: text/*"};

  http_connect_proxy();

again:
  printf("Enter URL: ");
  cgets(buf, BUFSIZE);

  response = http_request("GET", buf, headers, 1);

  printf("Got response %d (%ld bytes)\n", response->code, response->size);

  printf("%s\n", response->body);
  
  http_response_free(response);

  goto again;
  
  exit(0);
}
