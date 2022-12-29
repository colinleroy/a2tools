#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "simple_serial.h"
#include "extended_conio.h"

#define BUFSIZE 255
static char buf[BUFSIZE];

int main(int argc, char **argv) {
  char *response_buf = NULL;
  int response_code;
  long response_size;

#ifdef __CC65__
  simple_serial_open(2, SER_BAUD_9600);
#else
simple_serial_open(argv[1], B9600);
#endif
again:

  printf("Enter URL: ");
  cgets(buf, BUFSIZE);

  simple_serial_printf("GET %s\n", buf);

  /* Read code and size */
  simple_serial_gets(buf, 255);
  if (strchr(buf, ',') == NULL) {
    printf("Unexpected response '%s'\n", buf);
    goto again;
  }

  response_size = atol(strchr(buf, ',') + 1);
  *strchr(buf,',') = '\0';
  response_code = atoi(buf);

  printf("HTTP code %d (%ld bytes)\n", response_code, response_size);

  response_buf = malloc(response_size + 1);
  if (response_buf == NULL) {
    printf("Could not malloc %ld bytes\n", response_size);
  }

  simple_serial_read(response_buf, sizeof(char), response_size + 1);
  response_buf[response_size] = '\0';
  printf("%s\n", response_buf);
  
  free(response_buf);
  response_buf = NULL;

  goto again;
  
  exit(0);
}
