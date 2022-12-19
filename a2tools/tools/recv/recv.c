/* No more readability now */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>

#define DELAY_MS 3
#define LONG_DELAY_MS 50

/**********************
 * NON FUNCTIONAL NOW
 */////////////////////

static void setup_tty(char *ttypath) {
  struct termios tty;
  int port = open(ttypath, O_RDWR);

  if (port < 0) {
    printf("Cannot open %s: %s\n", ttypath, strerror(errno));
    exit(1);
  }

  if(tcgetattr(port, &tty) != 0) {
    printf("tcgetattr error: %s\n", strerror(errno));
    close(port);
    exit(1);
  }
  cfsetispeed(&tty, B9600);
  cfsetospeed(&tty, B9600);

  tty.c_cflag &= ~PARENB; /* disable parity */
  tty.c_cflag &= ~CSTOPB; /* one stop bit */

  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;     /* 8bit */

  tty.c_cflag &= ~CRTSCTS;/* Disabled CRTSCTS */
  tty.c_cflag |= CREAD | CLOCAL;

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO;
  tty.c_lflag &= ~ISIG;
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;

  if (tcsetattr(port, TCSANOW, &tty) != 0) {
    printf("tcgetattr error: %s\n", strerror(errno));
    close(port);
    exit(1);
  }
  close(port);
}

int main(int argc, char **argv) {
  FILE *fp;
  int count = 0;
  char buf[128];

  if (argc < 2) {
    printf("Usage: %s [input tty]\n", argv[0]);
    exit(1);
  }
  
  setup_tty(argv[1]);

  fp = fopen(argv[1], "r+b");
  if (fp == NULL) {
    printf("Can't open %s\n", argv[1]);
    exit(1);
  }

  while (1) {
    if (fgets(buf, sizeof(buf), stdin)) {
      printf("< %s\n", buf);
      fputs(buf, fp);
    }
    if (fgets(buf, sizeof(buf), fp)) {
      printf("> %s\n", buf);
    }
  }
  fclose(fp);

  exit(0);
}
