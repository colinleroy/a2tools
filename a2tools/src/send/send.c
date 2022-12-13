#include <stdio.h>
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <dbg.h>
#include <errno.h>
#include <apple2.h>

#define BUF_SIZE 255

/**********************
 * NON FUNCTIONAL NOW
 */////////////////////
int main(void) {
//  int r, w, exit_code = 0;
  int exit_code = 0, e;
  char c;
  char *buf = malloc(BUF_SIZE);
  // char *filetype = malloc(BUF_SIZE);
  // char *s_len = malloc(BUF_SIZE);
  // size_t data_len = 0;
  // FILE *outfp;
  // char *data;

  printf("Opening serial port\n");
  simple_serial_open(2, SER_BAUD_9600);

  simple_serial_set_timeout(10);
  printf("Sending data\n");
  while (1) {
    char i;
    for (i = 'a'; i < 'h'; i++) {
      ser_put(i);
    }
    ser_put('\n');
    
    while ((e = ser_get(&c)) != SER_ERR_NO_DATA)
      printf("got code %d from serial: '%c'\n", e, c);
  }
// 
// read_again:
//   printf("\nReady to receive (Ctrl-reset to abort)\n");
// 
//   if (simple_serial_gets(filename, BUF_SIZE) != NULL) {
//     if (strlen(filename) > 8)
//       filename[7] = '\0';
//     printf("Filename   '%s'\n", filename);
//   }
//   if (simple_serial_gets(filetype, BUF_SIZE) != NULL) {
//     printf("Filetype   '%s'\n", filetype);
//   }
// 
//   if (simple_serial_gets(s_len, BUF_SIZE) != NULL) {
//     data_len = atoi(s_len);
//     printf("Data length %d\n", data_len);
//   }
// 
//   data = malloc(data_len);
// 
//   if (data == NULL) {
//     printf("Couldn't allocate %d bytes of data.\n", data_len + 1);
//     exit (1);
//   }
// 
//   if (!strcasecmp(filetype, "TXT")) {
//     _filetype = PRODOS_T_TXT;
//     _auxtype  = PRODOS_AUX_T_TXT_SEQ;
//   } else if (!strcasecmp(filetype,"BIN")) {
//     _filetype = PRODOS_T_BIN;
//     /* FIXME This shouldn't be hardcoded */
//     _auxtype = 0x0803;
//   } else if (!strcasecmp(filetype,"SYS")) {
//     _filetype = PRODOS_T_SYS;
//   } else {
//     printf("Filetype is unsupported.\n");
//     exit_code = 1;
//     goto err_out;
//   }
// 
//   printf("Reading data...\n");
//   r = simple_serial_read_with_timeout(data, sizeof(char), data_len + 1);
// 
//   printf("Read %d bytes. Opening %s...\n", r, filename);
// 
//   outfp = fopen(filename,"w");
//   if (outfp == NULL) {
//     printf("Open error %d: %s\n", errno, strerror(errno));
//     exit_code = 1;
//     goto err_out;
//   }
// 
//   w = fwrite(data, 1, r, outfp);
//   if (w < r) {
//     printf("Only wrote %d bytes. Error %d: %s\n", w, errno, strerror(errno));
//     exit_code = 1;
//   }
// 
//   free(data);
// 
//   if (fclose(outfp) != 0) {
//     printf("Close error %d: %s\n", errno, strerror(errno));
//     exit_code = 1;
//   }
// 
//   goto read_again;
// 
// err_out:
//   free(filename);
//   free(filetype);
//   free(data);

  simple_serial_close();
  exit(exit_code);
}
