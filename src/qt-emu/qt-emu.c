#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_serial.h"
#include "extended_conio.h"

static unsigned char QT_HELLO[] = {0xA5, 0x5A, 0x01, 0x01, 0x01, 0x00, 0x02};
static unsigned char QT_HELLO_REPLY[] = {0x00, 0x00, 0x00, 0x00, 0xE1, 0x00, 0x00, 0x80, 0x02, 0x00};
#define SEND 0
#define RECV 1
int debug(int way, unsigned char *data, int len) {
  int i;
  printf("%s %d: ", way == SEND ? "SEND":"RECV", len);
  for (i = 0; i < len; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
}

int main(void) {
  unsigned char buf[512];
  int speed;

  simple_serial_set_speed(B9600);
  simple_serial_set_parity(0);

  simple_serial_read_opts();

  simple_serial_close();
  simple_serial_open();

  printf("type a key to wake up\n");
  cgetc();
  simple_serial_flush();
  /* Send camera hello */
  simple_serial_write(QT_HELLO, sizeof(QT_HELLO));
  debug(SEND, QT_HELLO, sizeof(QT_HELLO));

  /* Read computer */
  simple_serial_read(buf, 13);
  debug(RECV, buf, 13);

  speed = (buf[6]<<8)|buf[7];
  printf("computer wants %d bps\n", speed);
  
  QT_HELLO_REPLY[4] = buf[6];
  QT_HELLO_REPLY[5] = buf[7];

  simple_serial_write(QT_HELLO_REPLY, sizeof(QT_HELLO_REPLY));
  debug(SEND, QT_HELLO_REPLY, sizeof(QT_HELLO_REPLY));

  // simple_serial_set_parity(PARENB);

  /* Set speed */
  simple_serial_read(buf, 15);
  debug(RECV, buf, 15);

  switch (buf[0x0D]) {
    case 0x08: printf("computer still wants 9600 bps\n"); speed = 9600; break;
    case 0x10: printf("computer still wants 19200 bps\n"); speed = 19200; break;
    case 0x20: printf("computer still wants 38400 bps\n"); speed = 38400; break;
    case 0x30: printf("computer still wants 57600 bps\n"); speed = 57600; break;
  }

  /* Send ack */
  simple_serial_putc(0x00);

  simple_serial_read(buf, 1);
  if (buf[0] != 0x06) {
    printf("protocol error %02X\n", buf[0]);
    exit(1);
  }
  printf("ack done\n");

  switch(speed) {
    case 9600: simple_serial_set_speed(B9600); break;
    case 19200: simple_serial_set_speed(B19200); break;
    case 38400: simple_serial_set_speed(B38400); break;
    case 57600: simple_serial_set_speed(B57600); break;
  }

  simple_serial_read(buf, 1);
  if (buf[0] != 0x06) {
    printf("protocol error\n");
    exit(1);
  }
  simple_serial_putc(0x00);

  /* Speed neg done */

}
