#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>
#include <unistd.h>
#include "hgr.h"
#include "simple_serial.h"
#include "surl.h"
#include "pwm.h"

static char buf[1024];
int main(int argc, char *argv[]) {
  surl_connect_proxy();
  simple_serial_set_irq(0);
  pwm();
}
