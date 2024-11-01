#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "../surl_protocol.h"
#include "../log.h"
#include "simple_serial.h"
#include "strtrim.h"
#include "vsdrive.h"

static void get_datetime_bytes(VSDriveValues prodos_version, unsigned char *bytes) {
  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  int word1 = 0, word2 = 0;

  if (prodos_version == VSDRIVE_PRODOS_24) {
    word1 = now->tm_min + (now->tm_hour << 8);
    word2 = now->tm_mday + ((now->tm_mon + 1) << 5) + ((now->tm_year % 100) << 9);
  } else if (prodos_version == VSDRIVE_PRODOS_25) {
    word1 = (now->tm_mday << 11) + (now->tm_hour << 6) + (now->tm_min);
    word2 = ((now->tm_mon + 1) << 12) + (now->tm_year + 1900);
  }
  bytes[0] = word1 & 0xff;
  bytes[1] = word1 >> 8;
  bytes[2] = word2 & 0xff;
  bytes[3] = word2 >> 8;
}

static char *vdrive[] = {NULL, NULL};

static void vsdrive_cmd(VSDriveValues cmd, unsigned char drive, unsigned int blknum) {
  FILE *fp;
  int i, error = 0;
  unsigned char rcv_chksum, chksum = 0;
  unsigned char header[8];
  unsigned char data[VSDRIVE_BLOCK_SIZE];

  if (drive > 1) {
    printf("VSdrive: Wrong drive number %d\n", drive+1);
    return;
  }
  
  if (vdrive[drive] == NULL) {
    printf("VSdrive: drive %d is not configured.\n", drive+1);
    return;
  }

  fp = fopen(vdrive[drive], "r+b");
  if (fp == NULL) {
    printf("VSdrive: Can not open %s: %s\n", vdrive[drive], strerror(errno));
    return;
  }

  if (fseek(fp, blknum * VSDRIVE_BLOCK_SIZE, SEEK_SET) < 0) {
    printf("VSdrive: Can not seek %s: %s\n", vdrive[drive], strerror(errno));
    goto vsdrive_done;
  }

  /* Prepare response header */
  header[0] = SURL_METHOD_VSDRIVE;
  header[1] = 2 + (drive << 1) + cmd;
  header[2] = blknum & 0xff;
  header[3] = blknum >> 8;

  if (cmd == VSDRIVE_READ) {
    /* Finish header with date */
    get_datetime_bytes(VSDRIVE_PRODOS_24, &header[4]);
    for (i = 0; i < 8; i++) {
      chksum ^= header[i];
    }

    /* Read data from file */
    if (fread(data, 1, VSDRIVE_BLOCK_SIZE, fp) < VSDRIVE_BLOCK_SIZE) {
      printf("VSdrive: read error %s\n", strerror(errno));
      /* break checksum if error. */
      chksum++;
    }

    /* Write response header */
    simple_serial_write((char *)header, 8);
    simple_serial_putc(chksum);

    if (!error) {
      /* Compute data checksum */
      chksum = 0;
      for (i = 0; i < VSDRIVE_BLOCK_SIZE; i++) {
        chksum ^= data[i];
      }
      /* send data */
      simple_serial_write((char *)data, VSDRIVE_BLOCK_SIZE);
      simple_serial_putc(chksum);
    }

  } else if (cmd == VSDRIVE_WRITE) {
    /* Read data from serial */
    simple_serial_read((char *)data, VSDRIVE_BLOCK_SIZE);
    rcv_chksum = simple_serial_getc();

    /* Check the checksum */
    chksum = 0;
    for (i = 0; i < VSDRIVE_BLOCK_SIZE; i++) {
      chksum ^= data[i];
    }

    /* Write data to file if checksum is good */
    if (chksum != rcv_chksum) {
      printf("VSdrive: wrong checksum for writing.\n");
      error = 1;
    } else {
      if (fwrite(data, 1, VSDRIVE_BLOCK_SIZE, fp) < VSDRIVE_BLOCK_SIZE) {
        printf("VSdrive: write error %s\n", strerror(errno));
        /* break checksum if error. */
        chksum++;
      }
    }

    /* Send response */
    simple_serial_write((char *)header, 4);
    simple_serial_putc(chksum);
  }

vsdrive_done:
  fclose(fp);
}

void handle_vsdrive_request(void) {
  unsigned char cmd, drive, rcv_chksum, my_chksum, c;
  unsigned int blknum = 0;

  cmd = simple_serial_getc();

  c = simple_serial_getc();
  blknum |= c;
  c = simple_serial_getc();
  blknum |= c << 8;
  rcv_chksum = simple_serial_getc();

  my_chksum = SURL_METHOD_VSDRIVE ^ cmd ^ (blknum & 0xff) ^ (blknum >> 8);

  if (my_chksum != rcv_chksum) {
    printf("VSdrive: checksum error. Received %02x %02x %02x %02x %02x, chksum %02x\n",
           SURL_METHOD_VSDRIVE, cmd, (blknum & 0xff), (blknum >> 8), rcv_chksum, my_chksum);
  }

  drive = ((cmd - 2) >> 1);
  cmd = cmd % 2;

  switch (cmd) {
    case VSDRIVE_READ:
    case VSDRIVE_WRITE:
      printf("VSdrive: request to %s drive %d block %d\n", cmd == VSDRIVE_WRITE ? "write":"read", drive, blknum);
      vsdrive_cmd(cmd, drive, blknum);
      break;
    default:
      printf("VSdrive: Unknown command %02x\n", cmd);
  }
}

static void vsdrive_write_defaults(void) {
  FILE *fp = NULL;

  fp = fopen(VSDRIVE_CONF_FILE_PATH, "w");
  if (fp == NULL) {
    LOG("Cannot open %s for writing: %s\n", VSDRIVE_CONF_FILE_PATH,
           strerror(errno));
    LOG("Please create this configuration in the following format:\n\n");
    fp = stdout;
  }
  fprintf(fp, "# Point drive1 and/or drive2 to PO images to use as virtual drives\n"
              "# with ADTPro Virtual Drive system.\n"
              "# https://adtpro.com/vdrive.html\n"
              "\n"
              "drive1:\n"
              "drive2:\n");

  if (fp != stdout) {
    fclose(fp);
    LOG("A default vsdrive configuration file has been generated to %s.\n"
           "Please review it.\n", VSDRIVE_CONF_FILE_PATH);
  } else {
    LOG("\n\n");
  }
}

void vsdrive_read_config(void) {
  FILE *fp = fopen(VSDRIVE_CONF_FILE_PATH, "r");
  if (fp) {
    char buf[512];
    while (fgets(buf, 512-1, fp) > 0) {
      char *path = NULL;
      int num = 0;
      if (!strncmp(buf,"drive1:", 7)) {
        path = trim(buf + 7);
        num = 0;
      }
      if (!strncmp(buf,"drive2:", 7)) {
        path = trim(buf + 7);
        num = 1;
      }
      if (path) {
        if (vdrive[num]) {
          free(vdrive[num]);
          vdrive[num] = NULL;
        }
        if (path[0]) {
          vdrive[num] = path;
        } else {
          free(path);
        }
      }
    }
    fclose(fp);
  } else {
    vsdrive_write_defaults();
  }
}
