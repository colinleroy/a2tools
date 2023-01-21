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
#include "char_convert.h"
#include "simple_serial.h"
#include "extended_string.h"
#include "math.h"
#include "raw-session.h"
#include "jq.h"
#include "html2txt.h"

#define BUFSIZE 1024

typedef struct _curl_buffer curl_buffer;
struct _curl_buffer {
  char *buffer;
  int response_code;
  size_t size;

  char *headers;
  size_t headers_size;

  size_t upload_size;
  size_t orig_upload_size;
  char *upload_buffer;
  char *cur_upload_ptr;

  char *content_type;
  
  time_t start_secs;
  long start_msecs;
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
  int sending_headers = 0;
  int sending_body = 0;
  curl_buffer *response = NULL;
  time_t secs;
  long msecs;
  struct timespec cur_time;

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
        for (i = 0; i < num_parts; i++) {
          free(parts[i]);
        }
        free(parts);
        if (!strcmp(reqbuf, "\4\n")) {
          /* it's a reset */
          continue;
        }
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
        if (!strcmp(reqbuf, "\4\n")) {
          /* It's a reset */
          goto reopen;
        } else if (strcmp(reqbuf, "\n")) {
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
      if (!strcmp(reqbuf, "\4\n")) {
        /* it's a reset */
        continue;
      }
      printf("could not parse request '%s'\n", reqbuf);
      continue;
    }

    response = curl_request(method, url, headers, n_headers);
    if (response == NULL) {
      printf("%s %s - done\n", method, url);
      continue;
    }
    simple_serial_printf("%d,%d,%d,%s\n", response->response_code, response->size, 
                                    response->headers_size, response->content_type);
    sent = 0;
    sending_headers = 0;
    sending_body = 0;
    while (1) {
      size_t to_send;
      char *param;
      char striphtml = 0, translit = 0;

      if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
        if(!strncmp("SEND ", reqbuf, 5) 
        || !strncmp("HDRS ", reqbuf, 5)) {
          bufsize = atoi(reqbuf + 5);
        } else if (!strncmp("FIND ", reqbuf, 5)) {
          bufsize = atoi(reqbuf + 5);
          param = strchr(reqbuf + 5, ' ') + 1;
          *strchr(param, '\n') = '\0';
        } else if (!strncmp("JSON ", reqbuf, 5)) {
          bufsize = atoi(reqbuf + 5);
          striphtml = *(strchr(reqbuf + 5, ' ') + 1) == '1';
          translit = *(strchr(reqbuf + 5, ' ') + 3) == '1';
          param = strchr(reqbuf + 5, ' ') + 5;
          *strchr(param, '\n') = '\0';
        } else {
          printf("Aborted request\n");
          goto new_req;
        }
      } else {
        printf("Aborted request\n");
        simple_serial_flush();
        goto new_req;
      }

      if (!strncmp("SEND ", reqbuf, 5)) {
        if (!sending_body) {
          sent = 0;
          sending_headers = 0;
        }
        sending_body = 1;
        to_send = min(bufsize, response->size - sent);
        printf("SEND %zu body bytes from %zu\n", to_send, sent);
        sent += simple_serial_write(response->buffer + sent, sizeof(char), to_send);
      } else if (!strncmp("HDRS ", reqbuf, 5)) {
        if (!sending_headers) {
          sent = 0;
          sending_body = 0;
        }
        sending_headers = 1;
        to_send = min(bufsize, response->headers_size - sent);
        printf("SEND %zu header bytes from %zu\n", to_send, sent);
        sent += simple_serial_write(response->headers + sent, sizeof(char), to_send);
      } else if (!strncmp("FIND ", reqbuf, 5)) {
        char *found = NULL;
        sending_headers = 0;
        sending_body = 0;

        found = strstr(response->buffer, param);
        printf("FIND '%s' into %zu bytes: %s\n", param, bufsize, found != NULL ? "found":"not found");
        if (found) {
          found = strdup(found);
          
          if (strlen(found) >= bufsize) {
            found[bufsize - 1] = '\n';
            found[bufsize] = '\0';
            //printf("cut %zu bytes with '%s'\n", strlen(found), found);
            simple_serial_printf("%d\n", strlen(found));
            simple_serial_puts(found);
          } else {
            //printf("send %zu bytes with '%s'\n", strlen(found) + 1, found);
            simple_serial_printf("%d\n", strlen(found) + 1);
            simple_serial_puts(found);
            simple_serial_putc('\n');
          }
          free(found);
        } else {
          simple_serial_write("<NOT_FOUND>\n", sizeof(char), strlen("<NOT_FOUND>\n"));
        }
      } else if (!strncmp("JSON ", reqbuf, 5)) {
        if (strncasecmp(response->content_type, "application/json", 16)) {
          printf("JSON '%s' into %zu bytes: response is not json\n", param, bufsize);
          simple_serial_write("<NOT_JSON>\n", sizeof(char), strlen("<NOT_JSON>\n"));
        } else {
          char *result = jq_get(response->buffer, param);
          printf("JSON '%s' into %zu bytes%s%s: %s\n", param, bufsize, 
                  striphtml ? ", striphtml":"",
                  translit ? ", translit":"",
                  result != NULL ? result:"not found");
          /* DEBUG */

          if (result) {
            if (striphtml) {
              char *text = html2text(result);
              free(result);
              result = text;
            }
            if (translit) {
              size_t l;
              char *text = do_apple_convert(result, OUTGOING, &l);
              free(result);
              result = text;
            }
            if (result && strlen(result) >= bufsize) {
              result[bufsize - 1] = '\0';
            }
            printf(" now %s\n", result);
            simple_serial_printf("%d\n", strlen(result) + 1);
            simple_serial_puts(result);
            simple_serial_putc('\n');
            free(result);
          } else {
            simple_serial_write("<NOT_FOUND>\n", sizeof(char), strlen("<NOT_FOUND>\n"));
          }
        }
      }
    }

    clock_gettime(CLOCK_REALTIME, &cur_time);
    secs = cur_time.tv_sec - 1;
    msecs = 1000 + (cur_time.tv_nsec / 1000000);
    
    printf("%s %s - %d (%zub, %lums)\n", method, url, response->response_code, response->size,
        (1000*(secs - response->start_secs))+(msecs - response->start_msecs));
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
  free(curlbuf->upload_buffer);
  free(curlbuf);
}

static void proxy_set_curl_opts(CURL *curl) {
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "surl-server/1.0");
  curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookies.txt");
  curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt");
}

static int data_seek_cb(void *cbdata, curl_off_t offset, int origin) {
  curl_buffer *curlbuf = (curl_buffer *)cbdata;
  switch (origin) {
    case SEEK_SET:
      curlbuf->upload_size = curlbuf->orig_upload_size - offset;
      curlbuf->cur_upload_ptr = curlbuf->upload_buffer + offset;
      break;
    case SEEK_CUR:
      curlbuf->upload_size += offset;
      curlbuf->cur_upload_ptr += offset;
      break;
    case SEEK_END:
      curlbuf->upload_size = curlbuf->orig_upload_size - offset;
      curlbuf->cur_upload_ptr = curlbuf->upload_buffer - offset;
      break;
  }
  return 0;
}

static size_t data_send_cb(char *ptr, size_t size, size_t nmemb, void *cbdata) {
  curl_buffer *curlbuf = (curl_buffer *)cbdata;
  size_t xfr_size = min(nmemb, curlbuf->upload_size);

  if (xfr_size == 0) {
    return 0;
  }

  memcpy(ptr, curlbuf->cur_upload_ptr, xfr_size);
  curlbuf->cur_upload_ptr += xfr_size;
  curlbuf->upload_size -= xfr_size;

  return xfr_size;
}

/* we're going to url_encode and concat each line, expecting
 * an alternance of param, value
 */
static char *prepare_post(char *buffer, size_t *len) {
  char *tmp;
  char *out = malloc(*len * 3);
  char *out_ptr = out;
  char **lines;
  int n_lines = strsplit_in_place(buffer, '\n', &lines);
  int i;
  for (i = 0; i < n_lines; i += 2) {
    tmp = curl_easy_escape(NULL, lines[i], strlen(lines[i]));
    sprintf(out_ptr, "%s=", tmp);
    out_ptr += strlen(tmp) + 1;
    curl_free(tmp);
    if (strlen(lines[i + 1]) > 0)
      tmp = curl_easy_escape(NULL, lines[i + 1], strlen(lines[i + 1]));
    else
      tmp = strdup("");
    sprintf(out_ptr, "%s&", tmp);
    out_ptr += strlen(tmp) + 1;
    curl_free(tmp);
  }
  free(lines);
  free(buffer);
  *len = strlen(out);
  return out;
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
  struct timespec cur_time;


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
  curlbuf->orig_upload_size = 0;
  curlbuf->upload_buffer = NULL;
  curlbuf->content_type = NULL;

  clock_gettime(CLOCK_REALTIME, &cur_time);

  curlbuf->start_secs = cur_time.tv_sec;
  curlbuf->start_msecs = cur_time.tv_nsec / 1000000;

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
      curlbuf->orig_upload_size = atol(upload_buf);
      curlbuf->upload_buffer = malloc(curlbuf->upload_size);
      curlbuf->cur_upload_ptr = curlbuf->upload_buffer;

      simple_serial_puts("UPLOAD\n");
      simple_serial_read(curlbuf->upload_buffer, sizeof(char), curlbuf->upload_size);

      if (!strchr(upload_buf, ',')) {
        printf("Unexpected reply\n");
        return NULL;
      }
      /* Massage an x-www-urlencoded form */
      if (strcmp(strchr(upload_buf, ','), ",1\n")) {
        curlbuf->upload_buffer = realloc(curlbuf->upload_buffer, curlbuf->upload_size + 1);
        curlbuf->upload_buffer[curlbuf->upload_size] = '\0';
        curlbuf->upload_buffer = prepare_post(curlbuf->upload_buffer, &(curlbuf->upload_size));
        curlbuf->cur_upload_ptr = curlbuf->upload_buffer;
        printf("POST upload now %zu bytes\n", curlbuf->upload_size);
      } else {
        printf("POST raw upload: %zu bytes\n", curlbuf->upload_size);
      }

      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, data_send_cb);
      curl_easy_setopt(curl, CURLOPT_SEEKFUNCTION, data_seek_cb);
      curl_easy_setopt(curl, CURLOPT_READDATA, curlbuf);
      curl_easy_setopt(curl, CURLOPT_SEEKDATA, curlbuf);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, curlbuf->upload_size);
  } else if (!strcmp(method, "PUT")) {
      simple_serial_puts("SEND_SIZE_AND_DATA\n");
      simple_serial_gets(upload_buf, 255);
      curlbuf->upload_size = atol(upload_buf);
      curlbuf->orig_upload_size = atol(upload_buf);

      curlbuf->upload_buffer = malloc(curlbuf->upload_size);
      curlbuf->cur_upload_ptr = curlbuf->upload_buffer;

      if (!strchr(upload_buf, ',')) {
        printf("Unexpected reply\n");
        return NULL;
      }

      if (strcmp(strchr(upload_buf, ','), ",1\n")) {
        simple_serial_puts("ERROR\n");
        printf("Expected raw upload\n");
        return NULL;
      }
      simple_serial_puts("UPLOAD\n");
      simple_serial_read(curlbuf->upload_buffer, sizeof(char), curlbuf->upload_size);
      printf("PUT upload: %zu bytes\n", curlbuf->upload_size);

      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, data_send_cb);
      curl_easy_setopt(curl, CURLOPT_SEEKFUNCTION, data_seek_cb);
      curl_easy_setopt(curl, CURLOPT_READDATA, curlbuf);
      curl_easy_setopt(curl, CURLOPT_SEEKDATA, curlbuf);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_INFILESIZE, curlbuf->upload_size);
      if (is_sftp) {
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_SFTP);
      } else if (is_ftp) {
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_FTP);
      }
  } else if (!strcmp(method, "DELETE")) {
      if (is_ftp) {
        char *path = strdup(url);
        char *o_path = path;
        char *cmd;

        path = strstr(path, "://");
        if (path) {
          path += 3;
          path = strchr(path, '/');
        }
        
        if(strrchr(url, '/')) {
          *(strrchr(url, '/') + 1) = '\0';
        }
        
        cmd = malloc(strlen("DELE ") + strlen(path) + 1);
        sprintf(cmd, "DELE %s", path);
        printf("will delete %s in %s\n", path, url);
        free(o_path);
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_FTP);
        curl_easy_setopt(curl, CURLOPT_QUOTE, curl_slist_append(NULL,cmd));
        simple_serial_puts("WAIT\n");
      } else {
        printf("Unsupported method %s\n", method);
        simple_serial_puts("ERROR\n");
        return curlbuf;
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
  if (!strncmp(curlbuf->content_type, "text/", 5)) {
    char *tmp;
    size_t tmp_len;
    tmp = do_apple_convert(curlbuf->buffer, OUTGOING, &tmp_len);
    if (tmp) {
      free(curlbuf->buffer);
      curlbuf->buffer = tmp;
      curlbuf->size = tmp_len;
    }
  }
  return curlbuf;
}
