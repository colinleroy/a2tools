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
#include <arpa/inet.h>
#include "char_convert.h"
#include "simple_serial.h"
#include "extended_string.h"
#include "strsplit.h"
#include "math.h"
#include "raw-session.h"
#include "jq.h"
#include "html2txt.h"
#include "hgr.h"
#include "surl_protocol.h"

#define BUFSIZE 1024

static int VERBOSE = 0;
static int VERY_VERBOSE = 0;

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

  /* do not free */
  unsigned char *hgr_buf;
  size_t hgr_len;

  time_t start_secs;
  long start_msecs;

  jv json_data;
};

static curl_buffer *curl_request(char method, char *url, char **headers, int n_headers);
static void curl_buffer_free(curl_buffer *curlbuf);

static void handle_signal(int signal) {
  if (signal == SIGTERM) {
    simple_serial_close();
    exit(0);
  } else {
    printf("SIG: Ignoring signal %d\n", signal);
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

static const char *dump_response_to_file(char *buffer, size_t size) {
  char *filename = "/tmp/imgdata";
  FILE *fp = fopen(filename, "w+b");
  if (!fp) {
    return NULL;
  }
  if (fwrite(buffer, 1, size, fp) < size) {
    fclose(fp);
    return NULL;
  }
  rewind(fp);
  return filename;
}

int main(int argc, char **argv)
{
  char reqbuf[BUFSIZE];
  char method = SURL_METHOD_ABORT, *url = NULL;
  char cmd;
  char **headers = NULL;
  int n_headers = 0;
  size_t bufsize = 0, sent = 0;
  int sending_headers = 0;
  int sending_body = 0;
  curl_buffer *response = NULL;
  time_t secs;
  long msecs;
  struct timespec cur_time;
  unsigned short r_hdrs[3];

  if (argc > 1) {
    if (!strcmp(argv[1], "--help")) {
      printf("Usage: %s [--verbose|--very-verbose]\n", argv[0]);
      exit(0);
    } else if (!strcmp(argv[1], "--verbose")) {
      VERBOSE = 1;
    } else if (!strcmp(argv[1], "--very-verbose")) {
      VERBOSE = 1;
      VERY_VERBOSE = 1;
    }
  }

  install_sig_handler();

  curl_global_init(CURL_GLOBAL_ALL);

reopen:
  simple_serial_close();
  while (simple_serial_open() != 0) {
    sleep(1);
  }

  fflush(stdout);

  while(1) {

    /* read request */
    if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
      int i;
new_req:
      printf("\n");
      fflush(stdout);

      free(url);
      for (i = 0; i < n_headers; i++) {
        free(headers[i]);
      }
      free(headers);

      method = SURL_METHOD_ABORT;
      url = NULL;
      n_headers = 0;
      headers = NULL;
      curl_buffer_free(response);
      response = NULL;

      method = reqbuf[0];
      url = strdup(reqbuf + 1);
      if (strchr(url, '\n'))
        *strchr(url, '\n') = '\0';

      /* The Apple //c sends a NULL byte on boot */
      if (reqbuf[0] == '\0' && reqbuf[1] == SURL_METHOD_ABORT && reqbuf[2] == '\n') {
        printf("REQ: Apple 2 reboot?\n");
        continue;
      }
      if (method == SURL_METHOD_ABORT) {
        continue;
      } else if (url[0] == '\0') {
        printf("REQ: Could not parse request.\n");
        continue;
      }
    } else {
      printf("REQ: Fatal read error: %s\n", strerror(errno));
      goto reopen;
    }

    /* read headers */
    do {
      reqbuf[0] = '\0';
      if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
        if (reqbuf[0] == SURL_METHOD_ABORT) {
          /* It's a reset */
          goto reopen;
        } else if (strcmp(reqbuf, "\n")) {
          headers = realloc(headers, (n_headers + 1) * sizeof(char *));
          headers[n_headers] = trim(reqbuf);
          n_headers++;
        }
      } else {
        printf("R_HDRS: Fatal read error: %s\n", strerror(errno));
        goto reopen;
      }
    } while (strcmp(reqbuf, "\n"));

    response = curl_request(method, url, headers, n_headers);
    if (response == NULL) {
      printf("REQ: 0x%02x %s - done with no answer\n", method, url);
      continue;
    }

    r_hdrs[0] = htons(response->response_code);
    r_hdrs[1] = htons(response->size);
    r_hdrs[2] = htons(response->headers_size);

    simple_serial_write((char *)r_hdrs, 6);
    simple_serial_puts(response->content_type);
    simple_serial_putc('\n');

    printf("RESP: code %d, %zd response bytes, %zd header bytes, content-type: %s",
           response->response_code, response->size, response->headers_size,
           response->content_type);
    if (!strncasecmp(response->content_type, "application/json", 16)) {
      response->json_data = jv_parse(response->buffer);
      printf(" [built json data]");
    }
    printf("\n");

    if (VERBOSE && VERY_VERBOSE) {
      printf("RESP: headers (%zu bytes):\n%s\n", response->headers_size, response->headers);
      printf("RESP: body (%zu bytes):\n%s\n", response->size,
                (!strncasecmp(response->content_type, "application/json", 16)
                 || !strncasecmp(response->content_type, "text/", 5)) ? response->buffer:"[binary]");
    }

    sent = 0;
    sending_headers = 0;
    sending_body = 0;
    while (1) {
      size_t to_send;
      char *param;
      char striphtml = 0;
      char *translit = NULL;
      unsigned short size;
      size_t l;
      long start_secs = 0;
      long start_msecs = 0;

      /* read command */
      cmd = simple_serial_getc();
      clock_gettime(CLOCK_REALTIME, &cur_time);
      start_secs = cur_time.tv_sec;
      start_msecs = cur_time.tv_nsec / 1000000;

      if (SURL_IS_CMD(cmd)) {
        if(cmd == SURL_CMD_SEND || cmd == SURL_CMD_HEADERS) {
          simple_serial_read((char *)&size, 2);
          bufsize = ntohs(size);
        } else if (cmd == SURL_CMD_FIND) {
          simple_serial_read((char *)&size, 2);
          bufsize = ntohs(size);

          simple_serial_gets(reqbuf, BUFSIZE);
          param = reqbuf;
          *strchr(param, '\n') = '\0';
        } else if (cmd == SURL_CMD_JSON) {
          simple_serial_read((char *)&size, 2);
          bufsize = ntohs(size);

          striphtml = simple_serial_getc() == 1;
          simple_serial_gets(reqbuf, BUFSIZE);
          translit = reqbuf;
          param = strchr(translit, ' ') + 1;
          *strchr(translit, ' ') = '\0';
          *strchr(param, '\n') = '\0';
        } else if (cmd == SURL_CMD_HGR) {
          char *format = strrchr(url, '.');
          char monochrome = simple_serial_getc();
          if (format) {
            format++;
          }
          response->hgr_buf = sdl_to_hgr(
              dump_response_to_file(response->buffer, response->size),
              monochrome, &(response->hgr_len));
        }
      } else {
        printf("RESP: finished\n");
        /* Put that back as a REQUEST */
        reqbuf[0] = cmd;
        simple_serial_gets(reqbuf + 1, BUFSIZE - 1);
        goto new_req;
      }

      if (cmd == SURL_CMD_SEND) {
        if (!sending_body) {
          sent = 0;
          sending_headers = 0;
        }
        sending_body = 1;
        to_send = min(bufsize, response->size - sent);
        printf("RESP: SEND %zu body bytes from %zu\n", to_send, sent);
        simple_serial_write(response->buffer + sent, to_send);
        sent += to_send;
      } else if (cmd == SURL_CMD_HEADERS) {
        if (!sending_headers) {
          sent = 0;
          sending_body = 0;
        }
        sending_headers = 1;
        to_send = min(bufsize, response->headers_size - sent);
        printf("RESP: HEADERS %zu header bytes from %zu\n", to_send, sent);
        simple_serial_write(response->headers + sent, to_send);
        sent += to_send;
      } else if (cmd == SURL_CMD_FIND) {
        char *found = NULL;
        sending_headers = 0;
        sending_body = 0;

        found = strstr(response->buffer, param);
        printf("RESP: FIND '%s' into %zu bytes: %s\n", param, bufsize, found != NULL ? "found":"not found");
        if (found) {
          found = strdup(found);
          simple_serial_putc(SURL_ERROR_OK);
          if (strlen(found) >= bufsize) {
            found[bufsize - 1] = '\n';
            found[bufsize] = '\0';

            l = htons(strlen(found));
            simple_serial_write((char *)&l, 2);

            simple_serial_puts(found);
          } else {

            l = htons(strlen(found) + 1);
            simple_serial_write((char *)&l, 2);

            simple_serial_puts(found);
            simple_serial_putc('\n');
          }
          free(found);
        } else {
          simple_serial_putc(SURL_ERROR_NOT_FOUND);
        }
      } else if (cmd == SURL_CMD_JSON) {
        if (strncasecmp(response->content_type, "application/json", 16)) {
          printf("RESP: JSON '%s' into %zu bytes: response is not json\n", param, bufsize);
          simple_serial_putc(SURL_ERROR_NOT_JSON);
        } else {
          char *result = jq_get(jv_copy(response->json_data), param);
          printf("RESP: JSON '%s' into %zu bytes%s, translit: %s: %zu bytes %s\n", param, bufsize,
                  striphtml ? ", striphtml":"",
                  translit,
                  result != NULL ? min(strlen(result),bufsize) : 0,
                  result != NULL ? "" :"not found");

          if (result) {
            simple_serial_putc(SURL_ERROR_OK);
            if (striphtml) {
              char *text = html2text(result);
              free(result);
              result = text;
            }
            if (translit[0] != '0') {
              char *text = do_apple_convert(result, OUTGOING, translit, &l);
              free(result);
              result = text;
            }

            char *trimmed = trim(result);
            free(result);
            result = trimmed;

            if (result && strlen(result) >= bufsize) {
              result[bufsize - 1] = '\0';
            }

            l = htons(strlen(result));
            simple_serial_write((char *)&l, 2);
            simple_serial_puts(result);
            free(result);
          } else {
            simple_serial_putc(SURL_ERROR_NOT_FOUND);
          }
        }
      } else if (cmd == SURL_CMD_HGR) {
        if (response->hgr_buf && response->hgr_len) {
            printf("RESP: HGR %zu bytes\n", response->hgr_len);
            simple_serial_putc(SURL_ERROR_OK);
            l = htons(response->hgr_len);
            simple_serial_write((char *)&l, 2);
            simple_serial_write((char *)response->hgr_buf, response->hgr_len);
        } else {
          printf("RESP: HGR: No HGR data\n");
          simple_serial_putc(SURL_ERROR_CONV_FAILED);
        }
      }

      clock_gettime(CLOCK_REALTIME, &cur_time);
      secs = cur_time.tv_sec - 1;
      msecs = 1000 + (cur_time.tv_nsec / 1000000);

      printf("RESP: handled in %lums\n",
          (1000*(secs - start_secs))+(msecs - start_msecs));
    }

    fflush(stdout);

  }
}

static size_t curl_write_data_cb(void *contents, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  curl_buffer *curlbuf = (curl_buffer *)data;

  char *ptr = realloc(curlbuf->buffer, curlbuf->size + realsize + 1);

  if(!ptr) {
    printf("ERR: not enough memory\n");
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
    printf("ERR: not enough memory\n");
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
  jv_free(curlbuf->json_data);
  free(curlbuf);
}

static void proxy_set_curl_opts(CURL *curl) {
  CURLcode r = 0;
  r |= curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  r |= curl_easy_setopt(curl, CURLOPT_USERAGENT, "surl-server/1.0");
  r |= curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookies.txt");
  r |= curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt");
  r |= curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  r |= curl_easy_setopt(curl, CURLOPT_POST, 0L);
  r |= curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);
  r |= curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_ALL);
  r |= curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
  r |= curl_easy_setopt(curl, CURLOPT_QUOTE, NULL);
  r |= curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 0L);
  if (r) {
    printf("CURL: Couldn't set standard options\n");
  }
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

static char *replace_new_lines(char *in) {
  int len = strlen(in);
  char *out = malloc(len + 1);
  int i, o;
  for (i = 0, o = 0; i < len; i++) {
    if (in[i] == '\\' && i + 3 < len
     && in[i + 1] == 'r'
     && in[i + 2] == '\\'
     && in[i + 3] == 'n') {
      out[o++] = '\n';
      i += 3;
    } else {
      out[o++] = in[i];
    }
  }
  out[o++] = '\0';
  return out;
}

/* we're going to url_encode and concat each line, expecting
 * an alternance of param, value
 */
static char *prepare_post(char *buffer, size_t *len) {
  char *tmp, *nl;
  char *out = malloc((*len * 3) + 1);
  char *out_ptr = out;
  char **lines;
  int n_lines = strsplit_in_place(buffer, '\n', &lines);
  int i;
  for (i = 0; i < n_lines; i += 2) {
    char *translit = NULL;
    if (strstr(lines[i], "|TRANSLIT")) {
      translit = strstr(lines[i], "|TRANSLIT") + 1;
      translit = strchr(translit, '|') + 1;

      *strstr(lines[i], "|TRANSLIT") = '\0';
    }
    nl = replace_new_lines(lines[i]);
    tmp = curl_easy_escape(NULL, nl, 0);
    free(nl);
    sprintf(out_ptr, "%s=", tmp);
    out_ptr += strlen(tmp) + 1;
    curl_free(tmp);
    if (strlen(lines[i + 1]) > 0) {
      char *translit_data;
      nl = replace_new_lines(lines[i + 1]);
      if (translit) {
        size_t tmp_len;
        translit_data = do_apple_convert(nl, INCOMING, translit, &tmp_len);
      } else {
        translit_data = strdup(nl);
      }
      free(nl);
      tmp = curl_easy_escape(NULL, translit_data, 0);
      free(translit_data);
    } else
      tmp = curl_easy_escape(NULL, "", 0);
    sprintf(out_ptr, "%s&", tmp);
    out_ptr += strlen(tmp) + 1;
    curl_free(tmp);
  }
  free(lines);
  free(buffer);
  *len = strlen(out);
  return out;
}

/* The same, as JSON
 */
static char *json_escape(char *in) {
  int len = strlen(in);
  char *out = malloc(len * 2 + 1);
  int i, o;
  for (i = 0, o = 0; i < len; i++) {
    if (in[i] == '"') {
      out[o++] = '\\';
      out[o++] = '"';
    } else if (in[i] == '\n') {
      out[o++] = '\\';
      out[o++] = 'n';
    } else {
      out[o++] = in[i];
    }
  }
  out[o++] = '\0';
  return out;
}

/* format is an alternance of param/value lines where param format
 * is
 * T|name|TRANSLIT|charset
 * T = type (S string, B boolean, A array, O object)
 * TRANSLIT and charset optional
 * strings will be json-encoded
 * the other will be untouched
 */
static char *prepare_json_post(char *buffer, size_t *len) {
  char *tmp, *nl, *json_esc;
  char *out = malloc((*len * 3) + 1);
  char *out_ptr = out;
  char **lines;
  int n_lines = strsplit_in_place(buffer, '\n', &lines);
  int i;
  sprintf(out_ptr, "{");
  out_ptr++;
  for (i = 0; i < n_lines; i += 2) {
    char *translit = NULL;
    char type = 'S';

    /* get type */
    type = lines[i][0];
    if (lines[i][1] != '|' || !strchr("SBAO", type)) {
      printf("ERR: JSON: malformed input data\n");
      break;
    }
    lines[i] += 2;

    if (strstr(lines[i], "|TRANSLIT")) {
      translit = strstr(lines[i], "|TRANSLIT") + 1;
      translit = strchr(translit, '|') + 1;

      *strstr(lines[i], "|TRANSLIT") = '\0';
    }
    nl = replace_new_lines(lines[i]);

    if (i > 0) {
      sprintf(out_ptr, ",");
      out_ptr++;
    }
    sprintf(out_ptr, "\"%s\": ", nl);
    out_ptr += strlen(nl) + 4;
    free(nl);

    if (i + 1 < n_lines && strlen(lines[i + 1]) > 0) {
      char *translit_data;

      nl = replace_new_lines(lines[i + 1]);

      if (translit) {
        size_t tmp_len;
        translit_data = do_apple_convert(nl, INCOMING, translit, &tmp_len);
      } else {
        translit_data = strdup(nl);
      }
      free(nl);
      tmp = strdup(translit_data);
      free(translit_data);
    } else if (i + 1 < n_lines){
      tmp = strdup(lines[i + 1]);
    } else {
      printf("ERR: JSON: Missing parameter value\n");
      tmp = strdup("");
    }

    if (type != 'S') {
      sprintf(out_ptr, "%s", tmp);
      out_ptr += strlen(tmp);
    } else {
      json_esc = json_escape(tmp);
      free(tmp);
      tmp = json_esc;
      sprintf(out_ptr, "\"%s\"", tmp);
      out_ptr += strlen(tmp) + 2;
    }
    free(tmp);
  }
  sprintf(out_ptr, "}");
  free(lines);
  free(buffer);
  *len = strlen(out);
  return out;
}

static void massage_upload_urlencoded(curl_buffer *curlbuf) {
  curlbuf->upload_buffer = realloc(curlbuf->upload_buffer, curlbuf->upload_size + 1);
  curlbuf->upload_buffer[curlbuf->upload_size] = '\0';
  curlbuf->upload_buffer = prepare_post(curlbuf->upload_buffer, &(curlbuf->upload_size));
  curlbuf->cur_upload_ptr = curlbuf->upload_buffer;
}

static void massage_upload_json(curl_buffer *curlbuf) {
  curlbuf->upload_buffer = realloc(curlbuf->upload_buffer, curlbuf->upload_size + 1);
  curlbuf->upload_buffer[curlbuf->upload_size] = '\0';
  curlbuf->upload_buffer = prepare_json_post(curlbuf->upload_buffer, &(curlbuf->upload_size));
  curlbuf->cur_upload_ptr = curlbuf->upload_buffer;
}

static curl_buffer *curl_request(char method, char *url, char **headers, int n_headers) {
  static CURL *curl = NULL;
  CURLcode res;
  int i;
  CURLcode r = 0;
  curl_buffer *curlbuf;
  int is_sftp = !strncmp("sftp", url, 4);
  int is_ftp = !strncmp("ftp", url, 3) || is_sftp;
  int ftp_is_maybe_dir = (is_ftp && url[strlen(url)-1] != '/' && method == SURL_METHOD_GET);
  int ftp_try_dir = (is_ftp && url[strlen(url)-1] == '/' && method == SURL_METHOD_GET);
  static char *upload_buf = NULL;
  struct timespec cur_time;
  long secs, msecs;
  struct curl_slist *curl_headers = NULL;
  char *tmp;
  curl_mime *form = NULL;

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
  memset(curlbuf, 0, sizeof(curl_buffer));
  curlbuf->response_code = 500;

  if (curl == NULL) {
    curl = curl_easy_init();
  } else {
    curl_easy_reset(curl);
  }
  proxy_set_curl_opts(curl);

  if (method == SURL_METHOD_POST || method == SURL_METHOD_POST_DATA) {
      int is_multipart = (method == SURL_METHOD_POST_DATA);

      if (is_ftp) {
        printf("REQ: Unsupported ftp method POST\n");
        curl_buffer_free(curlbuf);
        return NULL;
      }

      if (!is_multipart) {
        simple_serial_putc(SURL_ANSWER_SEND_SIZE);
        simple_serial_gets(upload_buf, 255);
        curlbuf->upload_size = atol(upload_buf);
        curlbuf->orig_upload_size = atol(upload_buf);
        curlbuf->upload_buffer = malloc(curlbuf->upload_size);
        curlbuf->cur_upload_ptr = curlbuf->upload_buffer;

        simple_serial_puts("UPLOAD\n");
        simple_serial_read(curlbuf->upload_buffer, curlbuf->upload_size);

        if (!strchr(upload_buf, ',')) {
          printf("REQ: Unexpected serial reply\n");
          curl_buffer_free(curlbuf);
          return NULL;
        }

        if (!strcmp(strchr(upload_buf, ','), ",0\n")) {
          /* Massage an x-www-urlencoded form */
          massage_upload_urlencoded(curlbuf);
          if (VERBOSE) {
            printf("REQ: POST x-www-urlencoded [%zu bytes], body:\n", curlbuf->upload_size);
            printf("%s\n", curlbuf->upload_buffer);
          }
        } else if (!strcmp(strchr(upload_buf, ','), ",2\n")) {
          /* Massage an simple application/json form (no sub-entities handled )*/
          curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
          massage_upload_json(curlbuf);
          if (VERBOSE) {
            printf("REQ: POST application/json [%zu bytes], body:\n", curlbuf->upload_size);
            printf("%s\n", curlbuf->upload_buffer);
          }
        } else {
          if (VERBOSE) {
            printf("REQ: POST raw [%zu bytes]\n", curlbuf->upload_size);
          }
        }

        r |= curl_easy_setopt(curl, CURLOPT_POST, 1L);
        r |= curl_easy_setopt(curl, CURLOPT_READFUNCTION, data_send_cb);
        r |= curl_easy_setopt(curl, CURLOPT_SEEKFUNCTION, data_seek_cb);
        r |= curl_easy_setopt(curl, CURLOPT_READDATA, curlbuf);
        r |= curl_easy_setopt(curl, CURLOPT_SEEKDATA, curlbuf);
        r |= curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, curlbuf->upload_size);
        if (r) {
          printf("CURL: Couldn't set POST option(s)\n");
        }
      } else {
        char n_fields;
        char field_name[255];
        char field_len[255];
        char field_type[255];
        curl_mimepart *field = NULL;

        printf("REQ: POST multipart/form-data\n");

        curl_headers = curl_slist_append(curl_headers, "Content-Type: multipart/form-data");

        simple_serial_putc(SURL_ANSWER_SEND_NUM_FIELDS);
        n_fields = simple_serial_getc();

        form = curl_mime_init(curl);
        if (form) {
          for (i = 0; i < n_fields; i++) {
            char *field_contents;
            size_t f_len;
            simple_serial_gets(field_name, 255);
            if (strchr(field_name, '\n'))
              *strchr(field_name, '\n') = '\0';

            simple_serial_gets(field_len, 255);
            f_len = atoi(field_len);

            simple_serial_gets(field_type, 255);
            if (strchr(field_type, '\n'))
              *strchr(field_type, '\n') = '\0';

            if (!strncasecmp(field_type, "text/", 5)) {
              field_contents = malloc(f_len + 1);
              simple_serial_gets(field_contents, 255);
              if (strchr(field_contents, '\n'))
                *strchr(field_contents, '\n') = '\0';
            } else {
              field_contents = malloc(f_len);
              simple_serial_read(field_contents, f_len);
            }
            if (!strcasecmp(field_type, "image/hgr")) {
              size_t png_len;
              char *png_data = hgr_to_png(field_contents, f_len, 1, &png_len);
              free(field_contents);
              field_contents = png_data;
              strcpy(field_type, "image/png");
              f_len = png_len;
            }

            field = curl_mime_addpart(form);
            if (field) {
              if (curl_mime_name(field, field_name) != CURLE_OK)
                printf("REQ: POST: could not set field name %s\n", field_name);
              if (curl_mime_data(field, field_contents, f_len) != CURLE_OK)
                printf("REQ: POST: could not set field contents\n");

              if (VERBOSE) {
                printf("%s (%s): %s (%zu bytes)\n", field_name, field_type,
                        !strncasecmp(field_type, "text/", 5) ? field_contents:"[binary]",
                        f_len);
              }

              if (strncasecmp(field_type, "text/", 5)) {
                char *field_filename = malloc(512);

                if (curl_mime_type(field, field_type) != CURLE_OK)
                  printf("REQ: POST: could not set field type %s\n", field_type);

                snprintf(field_filename, 512, "file-%lu-%s", time(NULL), field_type);
                if (strchr(field_type, '/'))
                  *strchr(field_type, '/') = '.';
                if (curl_mime_filename(field, field_filename) != CURLE_OK)
                  printf("REQ: POST: could not set field filename %s\n", field_filename);
                free(field_filename);
              }
              free(field_contents);
            } else {
              printf("REQ: POST: could not add field\n");
            }
          }
          curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        } else {
          printf("REQ: POST: could not setup mime form\n");
        }
      }
  } else if (method == SURL_METHOD_PUT) {
      simple_serial_putc(SURL_ANSWER_SEND_SIZE);
      simple_serial_gets(upload_buf, 255);
      curlbuf->upload_size = atol(upload_buf);
      curlbuf->orig_upload_size = atol(upload_buf);

      curlbuf->upload_buffer = malloc(curlbuf->upload_size);
      curlbuf->cur_upload_ptr = curlbuf->upload_buffer;

      if (!strchr(upload_buf, ',')) {
        simple_serial_puts("ERROR\n");
        printf("Unexpected reply\n");
        curl_buffer_free(curlbuf);
        return NULL;
      }

      simple_serial_puts("UPLOAD\n");
      simple_serial_read(curlbuf->upload_buffer, curlbuf->upload_size);

      if (!strcmp(strchr(upload_buf, ','), ",0\n")) {
        /* Massage an x-www-urlencoded form */
        massage_upload_urlencoded(curlbuf);
        if (VERBOSE) {
          printf("REQ: PUT x-www-urlencoded [%zu bytes], body:\n", curlbuf->upload_size);
          printf("%s\n", curlbuf->upload_buffer);
        }
      } else if (!strcmp(strchr(upload_buf, ','), ",2\n")) {
        /* Massage an simple application/json form (no sub-entities handled )*/
        curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
        massage_upload_json(curlbuf);
        if (VERBOSE) {
          printf("REQ: PUT application/json [%zu bytes], body:\n", curlbuf->upload_size);
          printf("%s\n", curlbuf->upload_buffer);
        }
      } else {
        if (VERBOSE) {
          printf("REQ: PUT raw [%zu bytes]\n", curlbuf->upload_size);
        }
      }

      r |= curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
      r |= curl_easy_setopt(curl, CURLOPT_READFUNCTION, data_send_cb);
      r |= curl_easy_setopt(curl, CURLOPT_SEEKFUNCTION, data_seek_cb);
      r |= curl_easy_setopt(curl, CURLOPT_READDATA, curlbuf);
      r |= curl_easy_setopt(curl, CURLOPT_SEEKDATA, curlbuf);
      r |= curl_easy_setopt(curl, CURLOPT_INFILESIZE, curlbuf->upload_size);
      if (is_sftp) {
        r |= curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_SFTP);
      } else if (is_ftp) {
        r |= curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_FTP);
      }
      if (r) {
        printf("CURL: Could not set PUT option(s)\n");
      }
  } else if (method == SURL_METHOD_DELETE) {
      if (is_ftp) {
        char *path = strdup(url);
        char *o_path = path;
        char *cmd;

        path = strstr(path, "://");
        if (path) {
          path += 3;
          path = strchr(path, '/');
        } else {
          path = o_path;
        }

        if(strrchr(url, '/')) {
          *(strrchr(url, '/') + 1) = '\0';
        }

        cmd = malloc(strlen("DELE ") + strlen(path) + 1);
        sprintf(cmd, "DELE %s", path);
        printf("REQ: DELE %s in %s:\n", path, url);
        free(o_path);
        r |= curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_FTP);
        r |= curl_easy_setopt(curl, CURLOPT_QUOTE, curl_slist_append(NULL,cmd));
        simple_serial_putc(SURL_ANSWER_WAIT);
      } else {
        r |= curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        simple_serial_putc(SURL_ANSWER_WAIT);
      }
      if (r) {
        printf("CURL: Could not set DELETE option(s)\n");
      }
  } else if (method == SURL_METHOD_GET) {
    /* Don't send WAIT twice */
    if (ftp_try_dir || !ftp_is_maybe_dir) {
      simple_serial_putc(SURL_ANSWER_WAIT);
    }
  } else if (method == SURL_METHOD_RAW) {
    simple_serial_putc(SURL_ANSWER_RAW_START);
    curl_buffer_free(curlbuf);

    surl_server_raw_session(url);
    return NULL;
  } else {
    printf("Unsupported method 0x%02x\n", method);
    curlbuf->response_code = 500;
    curlbuf->size = 0;
    curlbuf->content_type = strdup("application/octet-stream");
    return curlbuf;
  }

  printf("0x%02x %s - start\n", method, url);

  r |= curl_easy_setopt(curl, CURLOPT_URL, url);
  r |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);
  r |= curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_write_header_cb);
  r |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)curlbuf);
  r |= curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)curlbuf);

  if (ftp_try_dir) {
    r |= curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1L);
  }
  if (VERBOSE) {
    printf("REQ: headers:\n");
  }
  for (i = 0; i < n_headers; i++) {
    curl_headers = curl_slist_append(curl_headers, headers[i]);
  }
  if (VERBOSE) {
    struct curl_slist *h;
    for (h = curl_headers; h; h = h->next)
      printf("%s\n", h->data);
  }

  r |= curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
  if (r) {
    printf("CURL: Could not set general option(s)\n");
  }

  clock_gettime(CLOCK_REALTIME, &cur_time);

  curlbuf->start_secs = cur_time.tv_sec;
  curlbuf->start_msecs = cur_time.tv_nsec / 1000000;

  res = curl_easy_perform(curl);

  clock_gettime(CLOCK_REALTIME, &cur_time);
  secs = cur_time.tv_sec - 1;
  msecs = 1000 + (cur_time.tv_nsec / 1000000);

  printf("CURL: request took %lums\n",
      (1000*(secs - curlbuf->start_secs))+(msecs - curlbuf->start_msecs));

  curl_slist_free_all(curl_headers);
  if (form) {
    curl_mime_free(form);
    form = NULL;
  }

  if(res != CURLE_OK) {
    printf("CURL: error %d: %s\n", res, curl_easy_strerror(res));
    if (res == CURLE_REMOTE_ACCESS_DENIED) {
      curlbuf->response_code = 401;
    } else {
      curlbuf->response_code = 599;
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
  fflush(stdout);

  return curlbuf;
}
