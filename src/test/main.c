#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
int main(int argc, char *argv[]) {
  struct stat st;
  struct statvfs sv;
  int r;
  printf("argc %d, argv1 %s\n", argc, argc > 1 ? argv[1]:"NULL");
  r = stat("/RAM", &st);
  printf("/RAM: %d (%d), %lu\n", r, errno, st.st_size);
  r = stat("/qsdf", &st);
  printf("/qsdf: %d (%d), %lu\n", r, errno, st.st_size);
  r = stat("PRODOS", &st);
  printf("PRODOS: %d (%d), %lu\n", r, errno, st.st_size);
  
  r = statvfs("/RAM", &sv);
  printf("/RAM: %d(%d), %lu/%lu\n", r, errno, sv.f_bfree, sv.f_blocks);
  r = statvfs("/qsdf", &sv);
  printf("/qsdf: %d(%d), %lu/%lu\n", r, errno, sv.f_bfree, sv.f_blocks);
  r = statvfs("/ADTPRO.2.1.0/PRODOS", &sv);
  printf("/ADTPRO.2.1.0/PRODOS: %d(%d), %lu/%lu\n", r, errno, sv.f_bfree, sv.f_blocks);
  r = statvfs("/RAM", &sv);
  printf("/RAM: %d(%d), %lu/%lu\n", r, errno, sv.f_bfree, sv.f_blocks);
  cgetc();
  r = exec("/prout/test", "path");
  printf("exec: %d (%d)\n", r, errno);
  cgetc();
  r = exec("/ADTPRO.2.1.0/test", "path");
  printf("exec: %d (%d)\n", r, errno);
  cgetc();
}
