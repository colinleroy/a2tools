/*
 * Copyright (C) 2024 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* Thanks to http://wsxyz.net/tohgr.html */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <cups/cups.h>
#include "printer.h"
#include "imagewriter.h"
#include "charsets.h"

#include "simple_serial.h"
#include "strtrim.h"

static pthread_t printer_thread;
static pthread_mutex_t printer_mutex;

static int printer_thread_started = 0;
static int printer_thread_stop_requested = 0;

extern int aux_ttyfd;
extern char *opt_aux_tty_path;
extern int opt_aux_tty_speed;

static char prints_directory[FILENAME_MAX/2] = "/tmp";
static const char *default_charset = NULL;
int default_charset_num = 0;
static const char *default_papersize = NULL;
int default_papersize_num = 0;
static char *printer_default_dest = NULL;
int job_timeout = 60;
char* g_imagewriter_fixed_font = FONTS_DIR"/"MONO_FONT;
char* g_imagewriter_prop_font = FONTS_DIR"/"PROP_FONT;
int enable_printing = 1;

static const char* papers[N_PAPER_SIZES] = {
	"US_LETTER_8.5x11in",
	"US_LETTER_8.5x14in",
	"ISO_A4_210x297mm",
	"ISO_B5_176x250mm",
	"WIDE_FANFOLD_14x11in",
	"LEDGER_11x17in",
	"ISO_A3_297x420mm"
};

static int printer_start_cups_job(const char *filename, cups_dest_t **dest, cups_dinfo_t **info) {
  int status, job_id;
  const char *printer_name = NULL;

  *dest = NULL;
  *info = NULL;

  if (!printer_default_dest) {
    /* Print to file only */
    return -1;
  } else if (strcmp(printer_default_dest, "default")) {
    printer_name = printer_default_dest;
  }
  printf("Printer: Getting %s printer...\n", printer_name ? printer_name:"default");
  *dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer_name, NULL);
  if (*dest) {
    printf("Printer: Found %s\n", (*dest)->name);
    *info = cupsCopyDestInfo(CUPS_HTTP_DEFAULT, *dest);
    if (*info) {
      printf("Printer: Got printer info\n");
      status = cupsCreateDestJob(CUPS_HTTP_DEFAULT, *dest, *info,
                                 &job_id, filename, 0, NULL);

      printf("Printer: Job %d created with status %d\n", job_id, status);
      if (status == IPP_STATUS_OK) {
        status = cupsStartDestDocument(CUPS_HTTP_DEFAULT, *dest, *info, job_id,
                                       filename, CUPS_FORMAT_POSTSCRIPT, 0, NULL, 1);
        if (status == HTTP_STATUS_CONTINUE) {
          /* We're good! */
          return HTTP_STATUS_CONTINUE;
        } else {
          printf("Printer: Start document: wrong status %d (%s)\n", status,
                 cupsLastErrorString());
        }
      }
      cupsFreeDestInfo(*info);
      *info = NULL;
    } else {
      printf("Printer: Could not get printer info (%s)\n",
             cupsLastErrorString());
    }
    cupsFreeDests(1, *dest);
    *dest = NULL;
  } else {
    printf("Printer: Error: %s\n", cupsLastErrorString());

    if (printer_default_dest == NULL) {
      printf("To fix this, please set a default CUPS printer:\n"
             "- `lpstat -v` to list available printers\n"
             "- `lpadmin -d [NAME]` to set default printer\n"
             "\n"
             "Or set a specific CUPS destination in %s.\n",
             PRINTER_CONF_FILE_PATH);
    }
  }
  return -1;
}

static int printer_finish_cups_job(cups_dest_t *dest, cups_dinfo_t *info) {
  int status;

  status = cupsFinishDestDocument(CUPS_HTTP_DEFAULT, dest, info);
  if (status != IPP_STATUS_OK) {
    printf("Printer: End document: wrong status %d (%s)\n", status,
           cupsLastErrorString());
  }
  return status;
}

int g_imagewriter_dpi = 360;
int g_imagewriter_multipage = 1;
int g_imagewriter_banner = 0;

static void handle_document(unsigned char first_byte) {
  static int filenum = 0;
  int i;
  unsigned char c;
  size_t n_bytes = 0;
  char filename[FILENAME_MAX];
  char buffer[SIMPLE_SERIAL_BUF_SIZE];
  char timestamp[64];
  time_t now = time(NULL);
  int cups_status = -1;
  cups_dest_t *dest = NULL;
  cups_dinfo_t *info = NULL;
  FILE *fp;
  size_t r;

  strftime(timestamp, sizeof timestamp, "%Y-%m-%d-%H-%M-%S", localtime(&now));
  snprintf(filename, FILENAME_MAX, "%s/iwprint-%s-%d.ps", prints_directory, timestamp, filenum++);

  printf("Printer: Receiving data!\n");

  imagewriter_init(g_imagewriter_dpi, default_papersize_num, g_imagewriter_banner, filename,
                   g_imagewriter_multipage, default_charset_num);
  imagewriter_loop(first_byte);

  while ((i = __simple_serial_getc_with_tv_timeout(aux_ttyfd, 1, 10, 0)) != EOF) {
    c = (unsigned char)i;
    if (n_bytes && (n_bytes % 2048) == 0) {
      printf("Printer: got %zu bytes...\n", n_bytes);
    }
    imagewriter_loop(c);
    n_bytes++;
  }

  printf("Printer: End of data after %zu bytes.\n", n_bytes);
  imagewriter_feed();
  imagewriter_close();

  if (n_bytes < 4) {
    printf("Printer: ignoring data, probably Apple II reboot.\n");
    unlink(filename);
    return;
  }
  cups_status = printer_start_cups_job(filename, &dest, &info);
  if (cups_status == HTTP_STATUS_CONTINUE) {
    fp = fopen(filename, "rb");
    if (fp) {
      while ((r = fread(buffer, 1, SIMPLE_SERIAL_BUF_SIZE, fp)) > 0) {
        if (cups_status == HTTP_STATUS_CONTINUE) {
          cups_status = cupsWriteRequestData(CUPS_HTTP_DEFAULT, buffer, r);
        }
      }
      if (cups_status == HTTP_STATUS_CONTINUE) {
        cups_status = printer_finish_cups_job(dest, info);
        cupsFreeDestInfo(info);
        cupsFreeDests(1, dest);
      }
      fclose(fp);
      /* We now expect cups_status to be IPP_STATUS_OK for printing to have
       * succeeded */
      if (cups_status == IPP_STATUS_OK) {
        printf("Printer: Removing file %s after successful printing\n", filename);
        unlink(filename);
      }
    } else {
      printf("Printer: can't reopen %s (%s)\n", filename, strerror(errno));
    }
  }
}

static void *printer_thread_listener(void *arg) {
  int err_warned = 0;
  /* open port */
  if (aux_ttyfd > 0) {
    printf("Error: printer serial port already opened.\n");
    exit(1);
  }

  /* Listen for data */
  while (1) {
    int i;

    pthread_mutex_lock(&printer_mutex);

    if (aux_ttyfd < 0)
      simple_serial_open_printer();

    if (aux_ttyfd < 0 && !err_warned) {
      printf("Could not open printer port %s.\n", opt_aux_tty_path ? opt_aux_tty_path : "(NULL)");
      err_warned = 1;
    }

    if (printer_thread_stop_requested) {
      pthread_mutex_unlock(&printer_mutex);
      /* close port */
      printf("Stopping printer thread.\n");
      simple_serial_close_printer();
      break;
    } else {
      pthread_mutex_unlock(&printer_mutex);
    }

    if (aux_ttyfd < 0) {
      sleep(1);
      continue;
    }

    /* Handle printer input here */
    i = __simple_serial_getc_with_tv_timeout(aux_ttyfd, 1, 5, 0);
    if (i != EOF) {
      handle_document((unsigned char)i);
    }
  }
  return (void *)0;
}

static void printer_write_defaults(void) {
  FILE *fp = NULL;
  int i;

  fp = fopen(PRINTER_CONF_FILE_PATH, "w");
  if (fp == NULL) {
    printf("Cannot open %s for writing: %s\n", PRINTER_CONF_FILE_PATH,
           strerror(errno));
    printf("Please create this configuration in the following format:\n\n");
    fp = stdout;
  }
  fprintf(fp, "#Enable printing\n"
              "enable_printing: true\n"
              "\n"
              "#Leave cups_printer_name empty to print to file only. Use 'default'\n"
              "#to use the system's default CUPS printer.\n"
              "#Otherwise, set it to a CUPS printer name as shown by `lpstat -v`.\n"
              "cups_printer_name:\n"
              "\n"
              "#How many seconds to wait for end of input before finishing the job.\n"
              "#PrintShop needs 60 seconds.\n"
              "job_timeout: %d\n"
              "\n"
              "#The directory where to put iwprint-*.ps files.\n"
              "prints_directory: %s\n"
              "\n"
              "#Fixed-width font to use.\n"
              "fixed_font: %s\n"
              "\n"
              "#Proportional-width font to use.\n"
              "proportional_font: %s\n"
              "\n"
              "#Default printer charset:\n"
              "default_charset: %s\n"
              "\n"
              "#Default printer charset:\n"
              "default_paper_size: %s\n",
              job_timeout,
              prints_directory,
              g_imagewriter_fixed_font,
              g_imagewriter_prop_font,
              default_charset,
              default_papersize);

  fprintf(fp, "\n#Available charsets:\n");
  for (i = 0; i < N_CHARSETS; i++) {
    fprintf(fp, "# %s\n", charsets[i]);
  }

  fprintf(fp, "\n#Available paper sizes:\n");
  for (i = 0; i < N_PAPER_SIZES; i++) {
    fprintf(fp, "# %s\n", papers[i]);
  }

  if (fp != stdout) {
    fclose(fp);
    printf("A default printer configuration file has been generated to %s.\n"
           "Please review it.\n", PRINTER_CONF_FILE_PATH);
  } else {
    printf("\n\n");
  }
}

static void printer_read_opts(void) {
  FILE *fp;

  default_charset = charsets[default_charset_num];
  default_papersize = papers[default_papersize_num];

  fp = fopen(PRINTER_CONF_FILE_PATH, "r");
  if (fp) {
    char buf[512];
    while (fgets(buf, 512-1, fp) > 0) {
      if (!strncmp(buf,"prints_directory:", 17)) {
        char *tmp = trim(buf + 17);
        snprintf(prints_directory, sizeof(prints_directory), "%s", tmp);
        free(tmp);
      }

      if (!strncmp(buf,"enable_printing:", 16)) {
        char *tmp = trim(buf + 16);
        enable_printing = !strcasecmp(tmp, "yes") ||
                          !strcasecmp(tmp, "true") ||
                          !strcasecmp(tmp, "1");
        free(tmp);
      }

      if (!strncmp(buf,"cups_printer_name:", 18)) {
        char *tmp = trim(buf + 18);
        if (strlen(tmp))
          printer_default_dest = tmp;
        else
          free(tmp);
      }

      if (!strncmp(buf,"job_timeout:", 12)) {
        char *tmp = trim(buf + 12);
        job_timeout = atoi(tmp);
        free(tmp);
      }

      if (!strncmp(buf,"fixed_font:", 11)) {
        char *tmp = trim(buf + 11);
        g_imagewriter_fixed_font = tmp;
      }

      if (!strncmp(buf,"proportional_font:", 18)) {
        char *tmp = trim(buf + 18);
        g_imagewriter_prop_font = tmp;
      }

      if (!strncmp(buf,"default_charset:", 16)) {
        char *tmp = trim(buf + 16);
        int i, found = 0;
        for (i = 0; i < N_CHARSETS; i++) {
          if (!strcmp(charsets[i], tmp)) {
            default_charset = charsets[i];
            default_charset_num = i;
            found = 1;
            printf("Printer: charset %s\n", default_charset);
            break;
          }
        }
        if (!found) {
          printf("Printer: ignoring unknown charset \"%s\".\n", tmp);
        }
        free(tmp);
      }

      if (!strncmp(buf,"default_paper_size:", 19)) {
        char *tmp = trim(buf + 19);
        int i, found = 0;
        for (i = 0; i < N_PAPER_SIZES; i++) {
          if (!strcmp(papers[i], tmp)) {
            default_papersize = papers[i];
            default_papersize_num = i;
            found = 1;
            printf("Printer: paper %s\n", default_papersize);
            break;
          }
        }
        if (!found) {
          printf("Printer: ignoring unknown paper size \"%s\".\n", tmp);
        }
        free(tmp);
      }
    }
    fclose(fp);
  } else {
    printer_write_defaults();
  }
}

int install_printer_thread(void) {
  printer_read_opts();
  pthread_mutex_init(&printer_mutex, NULL);
  return 0;
}

int start_printer_thread(void) {
  if (!enable_printing) {
    printf("Printer: disabled\n");
    return 0;
  }

  if (!printer_thread_started) {
    pthread_mutex_lock(&printer_mutex);
    printer_thread_stop_requested = 0;
    pthread_mutex_unlock(&printer_mutex);

    pthread_create(&printer_thread, NULL, *printer_thread_listener, NULL);
    printer_thread_started = 1;
  }
  return 0;
}

int stop_printer_thread(void) {
  if (printer_thread_started) {
    pthread_mutex_lock(&printer_mutex);
    printer_thread_stop_requested = 1;
    pthread_mutex_unlock(&printer_mutex);

    pthread_join(printer_thread, NULL);
    printer_thread_started = 0;
  }
  return 0;
}
