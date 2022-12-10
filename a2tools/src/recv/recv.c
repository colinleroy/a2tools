#include <stdio.h>
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <dbg.h>
#include <errno.h>
#include <apple2.h>

#define BUF_SIZE 255
#define MAX_DATA_SIZE 16385

int main(void) {
  int r, w, exit_code = 0;
  char *filename = malloc(BUF_SIZE);
  char *filetype = malloc(BUF_SIZE);
  char *s_len = malloc(BUF_SIZE);
  size_t data_len = 0;
  FILE *outfp;

  char *data = malloc(MAX_DATA_SIZE);

  if (data == NULL) {
    printf("Couldn't allocate data.\n");
    exit (1);
  }

  simple_serial_open(2, SER_BAUD_9600);

  simple_serial_set_timeout(10);

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

  if (data_len + 1 >= MAX_DATA_SIZE) {
    printf("Data too long (max %d bytes).\n", MAX_DATA_SIZE);
  }

  if (!strcasecmp(filetype, "TXT")) {
    _filetype = PRODOS_T_TXT;
    _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  } else if (!strcasecmp(filetype,"BIN")) {
    _filetype = PRODOS_T_BIN;
    /* FIXME This shouldn't be hardcoded */
    _auxtype = 0x0803;
  } else if (!strcasecmp(filetype,"SYS")) {
    _filetype = PRODOS_T_SYS;
  } else {
    printf("Filetype is unsupported.\n");
    exit_code = 1;
    goto err_out;
  }

  printf("Reading data...\n");
  r = simple_serial_read_with_timeout(data, sizeof(char), data_len + 1);

  printf("Read %d bytes. Opening %s...\n", r, filename);

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
  }

  if (fclose(outfp) != 0) {
    printf("Close error %d: %s\n", errno, strerror(errno));
    exit_code = 1;
  }
  
  goto read_again;

err_out:
  free(filename);
  free(filetype);
  free(data);

  simple_serial_close();
  exit(exit_code);
}
