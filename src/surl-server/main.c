/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <curl/curl.h>
#include "simple_serial.h"
#include "extended_string.h"
#include "math.h"
#include "raw-session.h"

#define BUFSIZE 255

typedef struct _curl_buffer curl_buffer;
struct _curl_buffer {
  char *buffer;
  int response_code;
  size_t size;

  char *headers;
  size_t headers_size;

  size_t upload_size;
  char *content_type;
};

static curl_buffer *curl_request(char *method, char *url, char **headers, int n_headers);
static void curl_buffer_free(curl_buffer *curlbuf);

static void handle_signal(int signal) {
  if (signal == SIGTERM) {
    simple_serial_close();
    exit(0);
  } else {
    printf("Ignoring signal %d\n", signal);
  }
}

static void install_sig_handler(void) {
	sigset_t    mask;
	struct sigaction act;

	sigemptyset(&mask);

	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGHUP);

	act.sa_handler = handle_signal;
	act.sa_mask    = mask;
	act.sa_flags   = 0;

	sigaction(SIGTERM, &act, 0);
	sigaction(SIGHUP, &act, 0);

	sigprocmask(SIG_UNBLOCK, &mask, 0);
}

int main(int argc, char **argv)
{
  char reqbuf[BUFSIZE];
  char *method = NULL, *url = NULL;
  char **headers = NULL;
  int n_headers = 0;
  size_t bufsize = 0, sent = 0;
  curl_buffer *response = NULL;

  install_sig_handler();

  curl_global_init(CURL_GLOBAL_ALL);

reopen:
  simple_serial_close();
  while (simple_serial_open() != 0) {
    sleep(1);
  }

  fflush(stdout);

  while(1) {

    if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
      char **parts;
      int num_parts, i;

new_req:
      fflush(stdout);

      free(method);
      free(url);
      for (i = 0; i < n_headers; i++) {
        free(headers[i]);
      }
      free(headers);

      method = NULL;
      url = NULL;
      n_headers = 0;
      headers = NULL;
      curl_buffer_free(response);
      response = NULL;

      num_parts = strsplit(reqbuf, ' ', &parts);
      if (num_parts < 2) {
        printf("Could not parse request '%s'\n", reqbuf);
        continue;
      }
      method = trim(parts[0]);
      url = trim(parts[1]);
      
      for (i = 0; i < num_parts; i++) {
        free(parts[i]);
      }
      free(parts);
    } else {
      printf("Fatal read error: %s\n", strerror(errno));
      goto reopen;
    }
    
    do {
      reqbuf[0] = '\0';
      if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
        if (strcmp(reqbuf, "\n")) {
          headers = realloc(headers, (n_headers + 1) * sizeof(char *));
          headers[n_headers] = trim(reqbuf);
          n_headers++;
        }
      } else {
        printf("Fatal read error: %s\n", strerror(errno));
        goto reopen;
      }
    } while (strcmp(reqbuf, "\n"));

    if (method == NULL || url == NULL) {
      printf("could not parse request '%s'\n", reqbuf);
      continue;
    }

    response = curl_request(method, url, headers, n_headers);
    if (response == NULL) {
      printf("%s %s - done\n", method, url);
      continue;
    }
    simple_serial_printf("%d,%d,%s\n", response->response_code, response->size, response->content_type);
    sent = 0;
    while (sent < response->size) {
      size_t to_send;

      if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
        if(!strncmp("SEND ", reqbuf, 5)) {
          bufsize = atoi(reqbuf + 5);
        } else {
          printf("Aborted request\n");
          simple_serial_flush();
          goto new_req;
        }
      } else {
        printf("Aborted request\n");
        simple_serial_flush();
        goto new_req;
      }

      to_send = min(bufsize, response->size - sent);
      sent += simple_serial_write(response->buffer + sent, sizeof(char), to_send);
    }

    printf("%s %s - %d (%zub)\n", method, url, response->response_code, response->size);
    fflush(stdout);

  }
}

static size_t curl_write_data_cb(void *contents, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  curl_buffer *curlbuf = (curl_buffer *)data;

  char *ptr = realloc(curlbuf->buffer, curlbuf->size + realsize + 1);

  if(!ptr) {
    printf("not enough memory\n");
    return 0;
  }

  curlbuf->buffer = ptr;
  memcpy(&(curlbuf->buffer[curlbuf->size]), contents, realsize);
  curlbuf->size += realsize;
  curlbuf->buffer[curlbuf->size] = '\0';

  return realsize;
}

static size_t curl_write_header_cb(void *contents, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  curl_buffer *curlbuf = (curl_buffer *)data;

  char *ptr = realloc(curlbuf->headers, curlbuf->headers_size + realsize + 1);

  if(!ptr) {
    printf("not enough memory\n");
    return 0;
  }

  curlbuf->headers = ptr;
  memcpy(&(curlbuf->headers[curlbuf->headers_size]), contents, realsize);
  curlbuf->headers_size += realsize;
  curlbuf->headers[curlbuf->headers_size] = '\0';

  return realsize;
}

static void curl_buffer_free(curl_buffer *curlbuf) {
  if (curlbuf == NULL) {
    return;
  }
  free(curlbuf->buffer);
  free(curlbuf->headers);
  free(curlbuf->content_type);
  free(curlbuf);
}

static void proxy_set_curl_opts(CURL *curl) {
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "surl-server/1.0");

}

static size_t data_send_cb(char *ptr, size_t size, size_t nmemb, void *cbdata) {
  unsigned long nread;
  curl_buffer *curlbuf = (curl_buffer *)cbdata;
  size_t retcode;
  
  if (curlbuf->upload_size == 0) {
    return 0;
  }

  retcode = simple_serial_read(ptr, size, min(nmemb, curlbuf->upload_size));

  if(retcode > 0) {
    nread = (unsigned long)retcode;
    curlbuf->upload_size -= nread;
    printf("read %lu bytes from serial\n", nread);
  }

  return retcode;
}

static curl_buffer *curl_request(char *method, char *url, char **headers, int n_headers) {
  CURL *curl;
  CURLcode res;
  int i;
  curl_buffer *curlbuf;
  int is_sftp = !strncmp("sftp", url, 4);
  int is_ftp = !strncmp("ftp", url, 3) || is_sftp;
  int ftp_is_maybe_dir = (is_ftp && url[strlen(url)-1] != '/' && !strcmp(method, "GET"));
  int ftp_try_dir = (is_ftp && url[strlen(url)-1] == '/' && !strcmp(method, "GET"));
  static char *upload_buf = NULL;

  if (upload_buf == NULL) {
    upload_buf = malloc(4096);
  }

  if (ftp_is_maybe_dir) {
    /* try dir first */
    char *dir_url = malloc(strlen(url) + 2);
    memcpy(dir_url, url, strlen(url));
    dir_url[strlen(url)] = '/';
    dir_url[strlen(url) + 1] = '\0';
    curlbuf = curl_request(method, dir_url, headers, n_headers);
    free(dir_url);
    if (curlbuf->response_code >= 200 && curlbuf->response_code < 300) {
      return curlbuf;
    } else {
      /* get as file */
      curl_buffer_free(curlbuf);
    }
  }
  
  curlbuf = malloc(sizeof(curl_buffer));
  curlbuf->buffer = NULL;
  curlbuf->size = 0;
  curlbuf->response_code = 500;
  curlbuf->headers = NULL;
  curlbuf->headers_size = 0;
  curlbuf->upload_size = 0;
  curlbuf->content_type = NULL;

  curl = curl_easy_init();
  proxy_set_curl_opts(curl);

  if (!strcmp(method, "POST")) {
      if (is_ftp) {
        printf("Unsupported ftp method POST\n");
        return curlbuf;
      }
      simple_serial_puts("SEND_SIZE_AND_DATA\n");
      simple_serial_gets(upload_buf, 255);
      curlbuf->upload_size = atol(upload_buf);

      printf("POST upload: %zu bytes\n", curlbuf->upload_size);
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, data_send_cb);
      curl_easy_setopt(curl, CURLOPT_READDATA, curlbuf);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, curlbuf->upload_size);
  } else if (!strcmp(method, "PUT")) {
      simple_serial_puts("SEND_SIZE_AND_DATA\n");
      simple_serial_gets(upload_buf, 255);
      curlbuf->upload_size = atol(upload_buf);

      printf("PUT upload: %zu bytes\n", curlbuf->upload_size);
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, data_send_cb);
      curl_easy_setopt(curl, CURLOPT_READDATA, curlbuf);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_INFILESIZE, curlbuf->upload_size);
      if (is_sftp) {
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_SFTP);
      } else if (is_ftp) {
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_FTP);
      }
  } else if (!strcmp(method, "GET")) {
    /* Don't send WAIT twice */
    if (ftp_try_dir || !ftp_is_maybe_dir) {
      simple_serial_puts("WAIT\n");
    }
  } else if (!strcmp(method, "RAW")) {
    simple_serial_puts("RAW_SESSION_START\n");
    curl_easy_cleanup(curl);
    curl = NULL;

    surl_server_raw_session(url);

    return NULL;
  } else {
    printf("Unsupported method %s\n", method);
    return curlbuf;
  }


  printf("%s %s - start\n", method, url);
  if (curl) {
    struct curl_slist *curl_headers = NULL;
    char *tmp;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_write_header_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)curlbuf);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)curlbuf);
    if (ftp_try_dir) {
      curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1L);
    }
    for (i = 0; i < n_headers; i++) {
      curl_headers = curl_slist_append(curl_headers, headers[i]);
    }
    
    printf("\n");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    res = curl_easy_perform(curl);
    curl_slist_free_all(curl_headers);

    if(res != CURLE_OK) {
      printf("Curl error %d: %s\n", res, curl_easy_strerror(res));
      /* Empty read buffer if needed */
      while (curlbuf->upload_size > 0) {
        data_send_cb(upload_buf, 1, 4096, curlbuf);
      }
      if (res == CURLE_REMOTE_ACCESS_DENIED) {
        curlbuf->response_code = 401;
      }
    } else {
      curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &(curlbuf->response_code));
      if (curlbuf->response_code == 0) {
        curlbuf->response_code = 200;
      }
    }
    curl_easy_getinfo (curl, CURLINFO_CONTENT_TYPE, &tmp);
    if (tmp != NULL) {
      curlbuf->content_type = strdup(tmp);
    } else {
      if (ftp_try_dir) {
        curlbuf->content_type = strdup("directory");
      } else {
        curlbuf->content_type = strdup("application/octet-stream");
      }
    }
  }
  fflush(stdout);
  curl_easy_cleanup(curl);
  return curlbuf;
}
