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

#include "simple_serial.h"

static pthread_t printer_thread;
static pthread_mutex_t printer_mutex;

static int printer_thread_started = 0;
static int printer_thread_stop_requested = 0;

extern int aux_ttyfd;
extern char *opt_aux_tty_path;
extern int opt_aux_tty_speed;

static void handle_document(unsigned char first_byte) {
  static int filenum = 0;
  int i;
  unsigned char c;
  size_t n_bytes = 0;
  char filename[FILENAME_MAX];
  char buffer[SIMPLE_SERIAL_BUF_SIZE];
  FILE *fp = NULL, *filter_fp = NULL;
  char timestamp[64];
  time_t now = time(NULL);

  strftime(timestamp, sizeof timestamp, "%Y-%m-%d-%H-%M-%S", localtime(&now));
  snprintf(filename, FILENAME_MAX, "%s/iwprint-%s-%d.ps", "/tmp/", timestamp, filenum++);
  fp = fopen(filename, "wb");

  if (fp == NULL) {
    printf("Printer: could not open %s: %s\n", filename, strerror(errno));
  } else {
    filter_fp = fopen(printer_get_iwem(), "rb");
    if (filter_fp) {
      size_t r;
      while ((r = fread(buffer, 1, SIMPLE_SERIAL_BUF_SIZE, filter_fp)) > 0) {
        fwrite(buffer, 1, r, fp);
      }
      fclose(filter_fp);
    }
    printf("Printer: printing to %s (%02x)\n", filename, first_byte);
    fputc(first_byte, fp);
  }

  while ((i = __simple_serial_getc_with_tv_timeout(aux_ttyfd, 1, 5, 0)) != EOF) {
    c = (unsigned char)i;
    if (fp != NULL) {
      fputc(c, fp);
    }
    n_bytes++;
  }
  if (fp) {
    fclose(fp);
  }
  printf("Document done after %zu bytes.\n", n_bytes);
}

static void *printer_thread_listener(void *arg) {
  /* open port */
  if (aux_ttyfd > 0) {
    printf("Error: printer serial port already opened.\n");
    exit(1);
  }
  simple_serial_open_printer();

  if (aux_ttyfd < 0) {
    printf("Could not open printer port %s.\n", opt_aux_tty_path ? opt_aux_tty_path : "(NULL)");
    return (void *)0;
  }

  /* Listen for data */
  while (1) {
    int i;
    pthread_mutex_lock(&printer_mutex);
    if (printer_thread_stop_requested) {
      pthread_mutex_unlock(&printer_mutex);
      /* close port */
      printf("Stopping printer thread.\n");
      simple_serial_close_printer();
      break;
    } else {
      pthread_mutex_unlock(&printer_mutex);
    }

    /* Handle printer input here */
    i = __simple_serial_getc_with_tv_timeout(aux_ttyfd, 1, 5, 0);
    if (i != EOF) {
      handle_document((unsigned char)i);
    }
  }
  return (void *)0;
}

int install_printer_thread(void) {
  pthread_mutex_init(&printer_mutex, NULL);
  return 0;
}

int start_printer_thread(void) {
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
