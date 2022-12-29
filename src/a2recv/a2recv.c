#include <stdio.h>
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <errno.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif

#define BUF_SIZE 255
#define DATA_SIZE 16384
#define FLOPPY_DELAY 300000

int main(int argc, char **argv) {
  int r, w, exit_code = 0;
  int floppy_delay;
  char *filename = malloc(BUF_SIZE);
  char *filetype = malloc(BUF_SIZE);
  char *s_len = malloc(BUF_SIZE);
  char *start_addr = malloc(BUF_SIZE);
  size_t data_len = 0;
  FILE *outfp = NULL;
  char *data = NULL;


#ifdef __CC65__
  simple_serial_open(2, SER_BAUD_9600);
#else
if (argc < 2) {
  printf("Usage: %s [input tty]\n", argv[0]);
  exit(1);
}
  simple_serial_open(argv[1], B9600);
#endif
  simple_serial_set_timeout(30);

read_again:
  printf("\nReady to receive (Ctrl-reset to abort)\n");

  if (simple_serial_gets(filename, BUF_SIZE) != NULL) {
    if (strlen(filename) > 8)
      filename[8] = '\0';
    if (strchr(filename, '\n'))
      *strchr(filename, '\n') = '\0';
    printf("Filename   '%s'\n", filename);
  }
  if (simple_serial_gets(filetype, BUF_SIZE) != NULL) {
    if (strchr(filetype, '\n'))
      *strchr(filetype, '\n') = '\0';
    printf("Filetype   '%s'\n", filetype);
  }

  if (simple_serial_gets(s_len, BUF_SIZE) != NULL) {
    if (strchr(s_len, '\n'))
      *strchr(s_len, '\n') = '\0';
    data_len = atoi(s_len);
    printf("Data length %d\n", (int)data_len);
  }

#ifdef __CC65__
  if (!strcasecmp(filetype, "TXT")) {
    _filetype = PRODOS_T_TXT;
    _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  } else if (!strcasecmp(filetype,"BIN")) {
    _filetype = PRODOS_T_BIN;
    simple_serial_gets(start_addr, BUF_SIZE);
    _auxtype = strtoul(start_addr, NULL, 16);
    printf("Start address %04x\n", _auxtype);
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
#else
  char *tmp = malloc(BUF_SIZE);
  sprintf(tmp, "%s.%s", filename, filetype);
  free(filename);
  filename = tmp;
#endif

  while (data_len > 0) {
    size_t block = (data_len > DATA_SIZE ? DATA_SIZE : data_len);
    data = malloc(block + 1);

    if (data == NULL) {
      printf("Couldn't allocate %d bytes of data.\n", (int)block);
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
    if (data_len > 0) {
      for (floppy_delay = FLOPPY_DELAY; floppy_delay > 0; floppy_delay--) {
        /* Wait for floppy to be done */
      }
      printf("Telling sender we're ready\n");
      simple_serial_puts("READY\n");
    }
  }

  if (fclose(outfp) != 0) {
    printf("Close error %d: %s\n", errno, strerror(errno));
    exit_code = 1;
  }

  outfp = NULL;

  printf("Telling sender we're done\n");
  simple_serial_puts("READY\n");
  
  goto read_again;

err_out:
  free(filename);
  free(filetype);
  free(data);

  simple_serial_close();
  exit(exit_code);
}
