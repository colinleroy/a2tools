#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "simple_serial.h"

extern FILE *ttyfp;

static void set_parenb(void) {
  struct termios tty;

  if(tcgetattr(fileno(ttyfp), &tty) != 0) {
    printf("tcgetattr error\n");
    exit(1);
  }
  tty.c_iflag |= INPCK;
  tty.c_cflag |= PARENB;

  if (tcsetattr(fileno(ttyfp), TCSANOW, &tty) != 0) {
    printf("tcgetattr error\n");
    exit(1);
  }
  tcgetattr(fileno(ttyfp), &tty);
}

static void get_ack(void) {
  char c = simple_serial_getc();
  printf("==> 0x%02x\n", (unsigned char)c);
  if (c != 0) {
    printf("unexpected reply\n");
    exit(1);
  }
  printf("ACK =>\n");
}

static void send_ack() {
  simple_serial_putc('\6');
  printf("<= ACK\n");
}

static void send_separator(void) {
  char str_start[] = {'\26','\0','\0','\0','\0','\0','\0'};
  simple_serial_write(str_start, sizeof str_start);
  printf("<= SEPARATOR\n");
  get_ack();
}

static void get_hello(void) {
  int i = 0;
  while (i++ < 7) {
    char c = simple_serial_getc();
    printf("HELLO => 0x%02x (%c)\n", (unsigned char)c, (unsigned char)c);
  }
}

static void send_hello(void) {
  char str_hello[] = {'Z','\245','U','\5','\0','\0','%','\200','\0','\200','\2','\0','\200'};
  int i;
  simple_serial_write(str_hello, sizeof str_hello);
  printf("<= HELLO\n");

  i = 0;
  while (i++ < 10) {
    char c = simple_serial_getc();
    printf("HELLO_REPLY => 0x%02x (%c)\n", (unsigned char)c, (unsigned char)c);
  }
}

static void send_first_photo_command(void) {
  char str[] = {'\26','(','\0','0','\0','\0','\0','\0','\0','\200','\0'};
  int i;
  simple_serial_write(str, sizeof str);
  printf("<= PHOTO 1\n");

  get_ack();
  send_ack(); // line 1211
}

static void send_second_photo_command(void) {
  char str[] = {'\26','(','\0','\0','\0','\0','\1','\0','\t','`','\0'};
  int i;
  simple_serial_write(str, sizeof str);
  printf("<= PHOTO 2\n");

  get_ack();
  send_ack(); // line 1211
}

static void send_third_photo_command(void) {
  char str[] = {'\26','(','\0','!','\0','\0','\1','\0','\0','@','\0'};
  int i;
  simple_serial_write(str, sizeof str);
  printf("<= PHOTO 3\n");

  get_ack();
  send_ack(); // line 1211
}

static void send_fourth_photo_command(void) {
  char str[] = {'\26','(','\0','\20','\0','\0','\1','\0','p','\200','\0'};
  int i;
  simple_serial_write(str, sizeof str);
  printf("<= PHOTO 3\n");

  get_ack();
  send_ack(); // line 1211
}

int main(void) {
  int c, i;
  char str_photos[] = {'\26','*','\0','\3','\0','\0','\0','\0','\0','\5','\0',
                       '\3','\3','\10','\4','\0'};
  unsigned int b;
  FILE *fp;

  simple_serial_open();

  tcflush(fileno(ttyfp), TCIOFLUSH);
  
  b = TIOCM_DTR;
  ioctl(fileno(ttyfp), TIOCMBIS, &b);
  b = TIOCM_RTS;
  ioctl(fileno(ttyfp), TIOCMBIC, &b);
  
  b = TIOCM_DTR|TIOCM_CTS|TIOCM_DSR;
  b = ioctl(fileno(ttyfp), TIOCMGET, &b);
  
  b = 0x1e0;
  ioctl(fileno(ttyfp), TIOCMIWAIT, b);
  
  b = TIOCM_DTR|TIOCM_RTS;
  ioctl(fileno(ttyfp), TIOCMBIC, &b);
  
  get_hello();
  send_hello();

  tcflush(fileno(ttyfp), TCIOFLUSH);
  set_parenb();

  sleep(1);
  simple_serial_write(str_photos, sizeof str_photos);
  printf("<= PHOTOS command\n");

  /* get ack */
  get_ack();
  send_ack();

  /* Get a full kB of 0xaa ?? */
  for (b = 0; b < 1024; b++) {
    c = simple_serial_getc();
    printf("==> 0x%02x\n", (unsigned char)c);
  }

  send_ack();
  get_ack();
  
  send_separator();

  send_first_photo_command();
  for (b = 0; b < 128; b++) {
    c = simple_serial_getc();
    printf("PHOTO1_REPLY(%d) => 0x%02x (%c)\n", b, (unsigned char)c, (unsigned char)c);
  }

  sleep(1);
  send_second_photo_command();
  for (i = 0; i < 2400; i++) {
    if (i > 0 && i % 512 == 0)
      send_ack();
    c = simple_serial_getc();
    printf("PHOTO2_REPLY(%d) => 0x%02x (%c)\n", i, (unsigned char)c, (unsigned char)c);
  }

  send_third_photo_command();
  for (i = 0; i < 64; i++) {
    c = simple_serial_getc();
    printf("PHOTO3_REPLY(%d) => 0x%02x (%c)\n", i, (unsigned char)c, (unsigned char)c);
  }

  fp = fopen("out.serial", "wb");
  send_fourth_photo_command();
  for (i = 0; i < 28799; i++) {
    if (i > 0 && i % 512 == 0)
      send_ack();
    c = simple_serial_getc();
    fwrite(&c, 1, 1, fp);
    printf("PHOTO4_REPLY(%d) => 0x%02x (%c)\n", i, (unsigned char)c, (unsigned char)c);
  }
  fclose(fp);
}
