#include <stdio.h>
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <dbg.h>
#include <errno.h>
#include <apple2.h>

#define BUF_SIZE 255
#define DATA_SIZE 16384

int main(void) {
  int r, w, exit_code = 0;
  char *filename = malloc(BUF_SIZE);
  char *filetype = malloc(BUF_SIZE);
  char *s_len = malloc(BUF_SIZE);
  size_t data_len = 0;
  FILE *outfp = NULL;
  char *data = NULL;


  simple_serial_open(2, SER_BAUD_9600);

  simple_serial_set_timeout(30);

read_again:
  printf("\nReady to receive (Ctrl-reset to abort)\n");

  if (simple_serial_gets(filename, BUF_SIZE) != NULL) {
    if (strlen(filename) > 8)
      filename[7] = '\0';
    printf("Filename   '%s'\n", filename);
  }
  if (simple_serial_gets(filetype, BUF_SIZE) != NULL) {
    printf("Filetype   '%s'\n", filetype);
  }

  if (simple_serial_gets(s_len, BUF_SIZE) != NULL) {
    data_len = atoi(s_len);
    printf("Data length %d\n", data_len);
  }

  if (!strcasecmp(filetype, "TXT")) {
    _filetype = PRODOS_T_TXT;
    _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  } else if (!strcasecmp(filetype,"BIN")) {
    _filetype = PRODOS_T_BIN;
    /* FIXME This shouldn't be hardcoded */
    _auxtype = 0x0803;
  } else if (!strcasecmp(filetype,"SYS")) {
    char *tmp = malloc(BUF_SIZE);
    sprintf(tmp, "%s.system", filename);
    free(filename);
    filename = tmp;
    _filetype = PRODOS_T_SYS;
  } else {
    printf("Filetype is unsupported.\n");
    exit_code = 1;
    goto err_out;
  }

  while (data_len > 0) {
    size_t block = (data_len > DATA_SIZE ? DATA_SIZE : data_len);
    data = malloc(block + 1);

    if (data == NULL) {
      printf("Couldn't allocate %d bytes of data.\n", block);
      exit (1);
    }

    printf("Reading data...\n");
    r = simple_serial_read_with_timeout(data, sizeof(char), block + 1);

    printf("Read %d bytes. Writing %s...\n", r, filename);

    if (outfp == NULL)
      outfp = fopen(filename,"w");
    if (outfp == NULL) {
      printf("Open error %d: %s\n", errno, strerror(errno));
      exit_code = 1;
      goto err_out;
    }

    w = fwrite(data, 1, r, outfp);
    if (w < r) {
      printf("Only wrote %d bytes. Error %d: %s\n", w, errno, strerror(errno));
      exit_code = 1;
      goto err_out;
    }

    free(data);
    data_len -= block;
  }

  if (fclose(outfp) != 0) {
    printf("Close error %d: %s\n", errno, strerror(errno));
    exit_code = 1;
  }

  outfp = NULL;
  
  goto read_again;

err_out:
  free(filename);
  free(filetype);
  free(data);

  simple_serial_close();
  exit(exit_code);
}
