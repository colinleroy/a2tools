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
#include <time.h>

static void dostat(const char *path) {
  struct stat st;
  struct statvfs sv;
  int r;
  time_t t;
  struct tm *tm;

  r = stat(path, &st);
  tm = gmtime_dt(&st.st_ctime);
  t = mktime_dt(&st.st_ctime);
  printf("%s: %luB, mode %s, type %d, aux %u acc %d bu %u\n"
         "pd: %d/%d/%d %d:%d\n"
         "tm: %d/%d/%d %d:%d\n"
         "time: %lu (%d: %s)\n", 
          path,
          st.st_size, (st.st_mode & S_IFMT) == S_IFDIR?"dir":"file", st.st_type, st.st_auxtype, st.st_access, st.st_blocks,
          st.st_ctime.date.day, st.st_ctime.date.mon, st.st_ctime.date.year, st.st_ctime.time.hour, st.st_ctime.time.min,
          tm ? tm->tm_mday:-1, tm ? tm->tm_mon:-1, tm ? tm->tm_year:-1, tm ? tm->tm_hour:-1, tm ? tm->tm_min:-1,
          t, errno, ctime(&t));
}
int main(int argc, char *argv[]) {
  struct timespec tv;
  videomode(VIDEOMODE_80COL);
  clrscr();
  dostat("PRODOS");
  dostat("/RAM");
  dostat("/ADTPRO.2.1.0");
  dostat("/QSDF");
  dostat("TEST.SYSTEM");
  dostat("TEST");
  clock_gettime(0, &tv);
  printf("time %lu\n", tv.tv_sec);
  cgetc();
}
