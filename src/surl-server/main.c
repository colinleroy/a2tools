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
#include "platform.h"
#include "surl_protocol.h"
#include "simple_serial.h"
#include "strtrim.h"
#include "strsplit.h"
#include "math.h"
#include "raw-session.h"
#include "char-convert.h"
#include "jq-get.h"
#include "html2txt.h"
#include "hgr-convert.h"
#include "stream.h"

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

static curl_buffer *surl_handle_request(char method, char *url, char **headers, int n_headers);
static void curl_buffer_free(curl_buffer *curlbuf);

static const char *surl_method_str(unsigned char method) {
  switch (method) {
    case SURL_METHOD_ABORT:        return "ABORT";
    case SURL_METHOD_RAW:          return "RAW";
    case SURL_METHOD_GET:          return "GET";
    case SURL_METHOD_POST:         return "POST";
    case SURL_METHOD_PUT:          return "PUT";
    case SURL_METHOD_DELETE:       return "DELETE";
    case SURL_METHOD_POST_DATA:    return "POST_DATA";
    case SURL_METHOD_GETTIME:      return "GETTIME";
    case SURL_METHOD_PING:         return "PING";
    case SURL_METHOD_DEBUG:        return "DEBUG";
    case SURL_METHOD_STREAM_AUDIO: return "STREAM_AUDIO";
    case SURL_METHOD_STREAM_VIDEO: return "STREAM_VIDEO";
    default:                    return "[UNKNOWN]";
  }
}
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
    printf("Could not open file: %s.\n", strerror(errno));
    return NULL;
  }
  if (fwrite(buffer, 1, size, fp) < size) {
    printf("Could not write to file: %s.\n", strerror(errno));
    fclose(fp);
    return NULL;
  }
  fclose(fp);
  return filename;
}

static void do_debug(char *file_line) {
  static char debug_buf[SIMPLE_SERIAL_BUF_SIZE + 1];
  static unsigned short len;

  if (strchr(file_line, '\n'))
    *strchr(file_line, '\n') = '\0';
  printf("DBG: %s: ", file_line);
  simple_serial_read((char *)&len, 2);
  len = ntohs(len);
  simple_serial_read(debug_buf, len);
  printf("%s\n", debug_buf);
  fflush(stdout);
}

#define IO_BARRIER(msg) do {                            \
    int r;                                              \
    printf("IO Barrier (%s)\n", msg);                   \
    do {                                                \
      r = simple_serial_getc();                         \
    } while (r != SURL_CLIENT_READY                     \
          && r != SURL_METHOD_ABORT);                   \
} while (0)

static void send_response_headers(curl_buffer *response) {
  uint16 code, hdr_size, ct_size;
  uint32 size;

  size = htonl(response->size);
  code = htons(response->response_code);
  hdr_size = htons(response->headers_size);
  ct_size = htons(strlen(response->content_type) + 1);

  IO_BARRIER("response headers");
  simple_serial_write_fast((char *)&size, 4);
  simple_serial_write_fast((char *)&code, 2);
  simple_serial_write_fast((char *)&hdr_size, 2);
  simple_serial_write_fast((char *)&ct_size, 2);
  IO_BARRIER("response content-type");
  simple_serial_puts(response->content_type);
  simple_serial_putc('\0');
}

char reqbuf[BUFSIZE];

int main(int argc, char **argv)
{
  unsigned char method = SURL_METHOD_ABORT;
  char *url = NULL;
  char cmd;
  char **headers = NULL;
  int n_headers = 0;
  size_t bufsize = 0, sent = 0;
  int sending_headers = 0;
  int sending_body = 0;
  curl_buffer *response = NULL;
  long start_secs = 0;
  long start_msecs = 0;
  time_t secs;
  long msecs;
  struct timespec cur_time;

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

  printf("surl-server Protocol %d\n", SURL_PROTOCOL_VERSION);

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

      if (method == SURL_METHOD_DEBUG) {
        do_debug(reqbuf + 1);
        continue;
      }

      /* The Apple //c sends a NULL byte on boot */
      if (reqbuf[0] == '\0' && (unsigned char)reqbuf[1] == SURL_METHOD_ABORT && reqbuf[2] == '\n') {
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
        if ((unsigned char)reqbuf[0] == SURL_METHOD_ABORT) {
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

    clock_gettime(CLOCK_REALTIME, &cur_time);
    start_secs = cur_time.tv_sec;
    start_msecs = cur_time.tv_nsec / 1000000;

    /* Perform the request */
    response = surl_handle_request(method, url, headers, n_headers);
    if (response == NULL) {
      printf("REQ: %s %s - done\n", surl_method_str(method), url);
      continue;
    }

    /* Send short response headers */
    send_response_headers(response);

    printf("RESP: code %d, %zd response bytes, %zd header bytes, content-type: %s",
           response->response_code, response->size, response->headers_size,
           response->content_type);

    /* Parse JSON if it is */
    if (!strncasecmp(response->content_type, "application/json", 16)) {
      clock_gettime(CLOCK_REALTIME, &cur_time);
      start_secs = cur_time.tv_sec;
      start_msecs = cur_time.tv_nsec / 1000000;

      response->json_data = jv_parse(response->buffer);

      clock_gettime(CLOCK_REALTIME, &cur_time);
      secs = cur_time.tv_sec - 1;
      msecs = 1000 + (cur_time.tv_nsec / 1000000);

      printf(" [built json data in %lums]",
          (1000*(secs - start_secs))+(msecs - start_msecs));
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

    /* Enter sub-request loop until client is done with it */
    while (1) {
      size_t to_send;
      char *param;
      char striphtml = 0;
      char *translit = NULL;
      unsigned short size;
      size_t l;

      /* read command */
      cmd = simple_serial_getc();
      clock_gettime(CLOCK_REALTIME, &cur_time);
      start_secs = cur_time.tv_sec;
      start_msecs = cur_time.tv_nsec / 1000000;

      if (SURL_IS_CMD(cmd)) {

        if(cmd == SURL_CMD_SEND || cmd == SURL_CMD_HEADERS) {
          /* client wants body or headers data. 
           * Input: 16-bit word: maximum number of bytes to return
           */
          simple_serial_read((char *)&size, 2);
          bufsize = ntohs(size);

        } else if (cmd == SURL_CMD_FIND) {
          /* client wants to body data at a substring match. 
           * Input: 16-bit word: maximum number of bytes to return
           *        string, \n terminated: what is looked for
           */
          simple_serial_read((char *)&size, 2);
          bufsize = ntohs(size);

          simple_serial_gets(reqbuf, BUFSIZE);
          param = reqbuf;
          *strchr(param, '\n') = '\0';

        } else if (cmd == SURL_CMD_JSON) {
          /* client wants parsed json. 
           * Input: 16-bit word: maximum number of bytes to return
           *        char (SURL_HTMLSTRIP_*): how to strip HTML in parsed result
           *        string, space (' ') terminated: Charset transliteration, "0" for none
           *        string, \n terminated: JSON selector
           *
           * Warning: The htmlstrip and charset transliteration apply to every
           *          matched field. Do multiple commands if you need different
           *          settings for different fields.
           */

          /* Size */
          simple_serial_read((char *)&size, 2);
          bufsize = ntohs(size);
          /* Striphtml */
          striphtml = simple_serial_getc();

          /* Charset transliteration and JSON selector */
          simple_serial_gets(reqbuf, BUFSIZE);
          translit = reqbuf;
          if (strchr(translit,' ')) {
            param = strchr(translit, ' ') + 1;
            *strchr(translit, ' ') = '\0';
            *strchr(param, '\n') = '\0';
          } else if ((unsigned char)translit[0] == SURL_METHOD_ABORT) {
            goto abort;
          } else {
            printf("Unknown error\n");
            goto new_req;
          }

        } else if (cmd == SURL_CMD_HGR) {
          /* Client wants an image
           * Input: char: Monochrome (\1) or color (\0)
           */
          char monochrome = simple_serial_getc();
          printf("RESP: converting to %s HGR\n", monochrome?"monochrome":"color");
          response->hgr_buf = sdl_to_hgr(
              dump_response_to_file(response->buffer, response->size),
              monochrome, 0, &(response->hgr_len), 0, 0);
        } else if (cmd == SURL_CMD_STRIPHTML) {
          /* Strip the HTML in a response, and update size
           * Input: char: Strip level
           */
           striphtml = simple_serial_getc();
        } else if (cmd == SURL_CMD_TRANSLIT) {
          /* Translit a response, and update size
           * Input: string: charset
           */
          simple_serial_gets(reqbuf, BUFSIZE);
          translit = reqbuf;
        }
      } else {
        /* special case for debugs in the middle of a request */
        if (cmd == SURL_METHOD_DEBUG) {
          reqbuf[0] = cmd;
          simple_serial_gets(reqbuf + 1, BUFSIZE - 1);
          do_debug(reqbuf + 1);
          continue;
        }

abort:
        printf("RESP: finished\n");
        /* Put that back as a REQUEST */
        reqbuf[0] = cmd;
        simple_serial_gets(reqbuf + 1, BUFSIZE - 1);
        goto new_req;
      }

      /* Parameters are set, now answer */
      if (cmd == SURL_CMD_SEND) {
        /* SEND response format:
         * array of bytes (up to the maximum specified)
         *
         * Warning: It is up to the client to make sure they don't
         * read more bytes than available, using surl_response->size.
         */

        IO_BARRIER("Client command");

        if (!sending_body) {
          /* Switching from HEADERS resets the cursor */
          sent = 0;
          sending_headers = 0;
        }
        sending_body = 1;
        to_send = min(bufsize, response->size - sent);
        printf("RESP: SEND %zu body bytes from %zu\n", to_send, sent);
        simple_serial_write_fast(response->buffer + sent, to_send);
        sent += to_send;

      } else if (cmd == SURL_CMD_HEADERS) {
        /* HEADERS response format:
         * array of bytes (up to the maximum specified)
         *
         * Warning: It is up to the client to make sure they don't
         * read more bytes than available, using surl_response->headers_size.
         */

        IO_BARRIER("Client command");

        if (!sending_headers) {
          /* Switching from BODY resets the cursor */
          sent = 0;
          sending_body = 0;
        }
        sending_headers = 1;
        to_send = min(bufsize, response->headers_size - sent);
        printf("RESP: HEADERS %zu header bytes from %zu\n", to_send, sent);
        simple_serial_write_fast(response->headers + sent, to_send);
        sent += to_send;

      } else if (cmd == SURL_CMD_FIND) {
        /* FIND response format:
         * char:           search status (SURL_ERROR_OK or SURL_ERROR_NOT_FOUND)
         * 16-bit word:    length of the result
         * array of bytes: search result (substring of maximum length: bufsize - 1),
         *                 starting at first search pattern match,
         *                 followed by a \n
         */

        char *found = NULL;
        sending_headers = 0;
        sending_body = 0;

        found = strstr(response->buffer, param);
        printf("RESP: FIND '%s' into %zu bytes: ", param, bufsize);
        if (found) {
          size_t len = strlen(found);
          found = strdup(found);
          if (len > bufsize - 1) {
            found[bufsize - 1] = '\0';
            len = strlen(found);
          }

          /* We'll add a \n */
          len++;
          printf("found (%zu bytes)\n", len);
          l = htons(len);

          simple_serial_putc(SURL_ERROR_OK);
          IO_BARRIER("FIND, pre-len");

          simple_serial_write_fast((char *)&l, 2);
          IO_BARRIER("FIND, pre-content");

          simple_serial_puts(found);
          simple_serial_putc('\n');

          free(found);
        } else {
          printf("not found\n");

          simple_serial_putc(SURL_ERROR_NOT_FOUND);
        }

      } else if (cmd == SURL_CMD_JSON) {
        /* JSON response format:
         * char:             status (SURL_ERROR_OK, SURL_ERROR_NOT_FOUND or 
                             SURL_ERROR_NOT_JSON)
         * 16-bit word:      length of the result
         * array of strings: JSON result, each field matched by the selector
         *                   separated on its own line.
         *
         * Warning: unmatched fields in the selector get NO line. Make sure to put
         * only one field whose existence is uncertain, put it last in the selector,
         * or do one command per uncertain field,
         * or use a default in the selector (like .optional_id//"-")
         *
         * Warning 2: a field containing text with \n will be represented on multiple
         * lines. Make sure to put only one field of this type per command, and put it
         * last, or do multiple commands.
         */

        if (strncasecmp(response->content_type, "application/json", 16)) {
          /* HTTP response was not JSON */
          printf("RESP: JSON '%s' into %zu bytes: response is not json\n", param, bufsize);
          simple_serial_putc(SURL_ERROR_NOT_JSON);
        } else {
          /* Extract result */
          char *result;

          result = jq_get(jv_copy(response->json_data), param);

          clock_gettime(CLOCK_REALTIME, &cur_time);
          secs = cur_time.tv_sec - 1;
          msecs = 1000 + (cur_time.tv_nsec / 1000000);

          printf("RESP: JSON '%s' into %zu bytes%s, translit: %s: %zu bytes %s (%lums)\n", param, bufsize,
                  striphtml ? ", striphtml":"",
                  translit,
                  result != NULL ? min(strlen(result),bufsize) : 0,
                  result != NULL ? "" :"not found",
                  (1000*(secs - start_secs))+(msecs - start_msecs));

          if (result) {
            /* We have a result */
            simple_serial_putc(SURL_ERROR_OK);

            /* Strip HTML at the required level */
            if (striphtml) {
              char *text = html2text(result,
                            striphtml == SURL_HTMLSTRIP_FULL ? 
                              HTML2TEXT_EXCLUDE_LINKS : HTML2TEXT_DISPLAY_LINKS);
              free(result);
              result = text;
            }

            /* Convert the charset */
            if (translit[0] != '0') {
              char *text = do_charset_convert(result, OUTGOING, translit, 0, &l);
              free(result);
              result = text;
            }

            /* Remove whitespace */
            char *trimmed = trim(result);
            free(result);
            result = trimmed;

            if (result && strlen(result) >= bufsize) {
              result[bufsize - 1] = '\0';
            }

            /* And send the result */
            l = htons(strlen(result));

            IO_BARRIER("JSON, pre-len");
            simple_serial_write_fast((char *)&l, 2);

            IO_BARRIER("JSON, pre-content");
            simple_serial_puts(result);
            free(result);
          } else {
            simple_serial_putc(SURL_ERROR_NOT_FOUND);
          }
        }

      } else if (cmd == SURL_CMD_HGR) {
        /* HGR response format:
         * char:           status (SURL_ERROR_OK or SURL_ERROR_CONV_FAILED)
         * 16-bit word:    length of the result
         * array of bytes: Binary HGR data (eligible to be directly written
         *                 to the Apple2 HGR page 1 or 2).
         */
        if (response->hgr_buf && response->hgr_len) {
            printf("RESP: HGR %zu bytes\n", response->hgr_len);
            simple_serial_putc(SURL_ERROR_OK);
            l = htons(response->hgr_len);

            IO_BARRIER("HGR, pre-len");
            simple_serial_write_fast((char *)&l, 2);

            IO_BARRIER("HGR, pre-content");
            simple_serial_write_fast((char *)response->hgr_buf, response->hgr_len);
        } else {
          printf("RESP: HGR: No HGR data\n");
          simple_serial_putc(SURL_ERROR_CONV_FAILED);
        }
      } else if (cmd == SURL_CMD_STRIPHTML) {
        char *tmp = html2text(response->buffer,
                      striphtml == SURL_HTMLSTRIP_FULL ? 
                        HTML2TEXT_EXCLUDE_LINKS : HTML2TEXT_DISPLAY_LINKS);

        if (tmp) {
          free(response->buffer);
          response->buffer = tmp;
          response->size = strlen(response->buffer) + 1;
          printf("RESP: HTML_STRIP to %zd response bytes\n", response->size);
        }
        send_response_headers(response);
      } else if (cmd == SURL_CMD_TRANSLIT) {
        char *tmp = NULL;
        if (translit[0] != '\0') {
          tmp = do_charset_convert(response->buffer, OUTGOING, translit, 0, &l);
        }

        if (tmp) {
          free(response->buffer);
          response->buffer = tmp;
          response->size = strlen(response->buffer) + 1;
          printf("RESP: TRANSLIT to %zd response bytes\n", response->size);
        }
        send_response_headers(response);
        sending_body = 1;
        sent = 0; /* Reset cursor */
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
  int lowercase = 0;

  for (i = 0; i < n_lines; i += 2) {
    char *translit = NULL;

    lowercase = (strstr(lines[i], "|TRANSLITLC")) != NULL;

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
        translit_data = do_charset_convert(nl, INCOMING, translit, lowercase, &tmp_len);
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
 * the other types will be untouched
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
    int lowercase;

    /* get type */
    type = lines[i][0];
    if (lines[i][1] != '|' || !strchr("SBAO", type)) {
      printf("ERR: JSON: malformed input data\n");
      break;
    }
    lines[i] += 2;

    lowercase = (strstr(lines[i], "|TRANSLITLC")) != NULL;

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
        translit_data = do_charset_convert(nl, INCOMING, translit, lowercase, &tmp_len);
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

static int setup_simple_upload_request(char method, CURL *curl,
                                       struct curl_slist **curl_headers,
                                       curl_buffer *curlbuf) {
  int r = 0;
  uint32 size;
  unsigned short mode;

  simple_serial_putc(SURL_ANSWER_SEND_SIZE);
  simple_serial_read((char *)&size, 4);
  simple_serial_read((char *)&mode, 2);
  size = ntohl(size);
  mode = ntohs(mode);

  if (mode > 2) {
    simple_serial_putc(SURL_UPLOAD_PARAM_ERROR);
    printf("REQ: Unexpected serial reply\n");
    return -1;
  }

  curlbuf->upload_size = size;
  curlbuf->orig_upload_size = size;
  curlbuf->upload_buffer = malloc(curlbuf->upload_size);
  curlbuf->cur_upload_ptr = curlbuf->upload_buffer;

  simple_serial_putc(SURL_UPLOAD_GO);
  simple_serial_read(curlbuf->upload_buffer, curlbuf->upload_size);

  if (mode == SURL_DATA_X_WWW_FORM_URLENCODED_HELP) {
    /* Massage an x-www-urlencoded form */
    massage_upload_urlencoded(curlbuf);
    if (VERBOSE) {
      printf("REQ: %s x-www-urlencoded [%zu bytes], body:\n", surl_method_str(method), curlbuf->upload_size);
      printf("%s\n", curlbuf->upload_buffer);
    }
  } else if (mode == SURL_DATA_APPLICATION_JSON_HELP) {
    /* Massage an simple application/json form (no sub-entities handled )*/
    *curl_headers = curl_slist_append(*curl_headers, "Content-Type: application/json");
    massage_upload_json(curlbuf);
    if (VERBOSE) {
      printf("REQ: %s application/json [%zu bytes], body:\n", surl_method_str(method), curlbuf->upload_size);
      printf("%s\n", curlbuf->upload_buffer);
    }
  } else { /* assume SURL_DATA_X_WWW_FORM_URLENCODED_RAW */
    if (VERBOSE) {
      printf("REQ: %s raw [%zu bytes]\n", surl_method_str(method), curlbuf->upload_size);
    }
  }

  r |= curl_easy_setopt(curl, CURLOPT_READFUNCTION, data_send_cb);
  r |= curl_easy_setopt(curl, CURLOPT_SEEKFUNCTION, data_seek_cb);
  r |= curl_easy_setopt(curl, CURLOPT_READDATA, curlbuf);
  r |= curl_easy_setopt(curl, CURLOPT_SEEKDATA, curlbuf);

  if (method == SURL_METHOD_POST) {
    r |= curl_easy_setopt(curl, CURLOPT_POST, 1L);
    r |= curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, curlbuf->upload_size);
  } else {
    r |= curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    r |= curl_easy_setopt(curl, CURLOPT_INFILESIZE, curlbuf->upload_size);
  }
  if (r) {
    printf("CURL: Could not set %s option(s)\n", surl_method_str(method));
  }
  return 0;
}

static curl_mime *setup_multipart_upload_request(char method, CURL *curl,
                                          struct curl_slist **curl_headers,
                                          curl_buffer *curlbuf) {
  int i;
  char n_fields;
  char field_name[255];
  char field_type[255];
  curl_mimepart *field = NULL;
  curl_mime *form = NULL;

  printf("REQ: POST multipart/form-data\n");

  *curl_headers = curl_slist_append(*curl_headers, "Content-Type: multipart/form-data");

  simple_serial_putc(SURL_ANSWER_SEND_NUM_FIELDS);
  n_fields = simple_serial_getc();

  form = curl_mime_init(curl);
  if (form) {
    for (i = 0; i < n_fields; i++) {
      char *field_contents;
      uint32 h_len;
      size_t f_len;
      simple_serial_gets(field_name, 255);
      if (strchr(field_name, '\n'))
        *strchr(field_name, '\n') = '\0';

      simple_serial_gets(field_type, 255);
      if (strchr(field_type, '\n'))
        *strchr(field_type, '\n') = '\0';

      simple_serial_read((char *)&h_len, 4);
      f_len = ntohl(h_len);

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
    if (curl_easy_setopt(curl, CURLOPT_MIMEPOST, form) != CURLE_OK) {
      printf("REQ: POST: could not add form\n");
      curl_mime_free(form);
      form = NULL;
    }
  } else {
    printf("REQ: POST: could not setup mime form\n");
  }
  return form;
}

static struct curl_slist *curl_ftp_opts = NULL;
static int setup_ftp_delete(CURL *curl, const char *url) {
  char r = 0;
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
  curl_ftp_opts = curl_slist_append(curl_ftp_opts, cmd);
  r |= curl_easy_setopt(curl, CURLOPT_QUOTE, curl_ftp_opts);

  return r;
}

static curl_buffer *surl_handle_request(char method, char *url, char **headers, int n_headers) {
  static CURL *curl = NULL;
  static CURLU *c_url = NULL;
  CURLcode res;
  int i;
  CURLcode r = 0;
  curl_buffer *curlbuf;
  int is_sftp = 0;
  int is_ftp  = 0;
  int ftp_is_maybe_dir = 0;
  int ftp_try_dir = 0;
  static char *upload_buf = NULL;
  struct timespec cur_time;
  long secs, msecs;
  struct curl_slist *curl_headers = NULL;
  char *tmp = NULL;
  curl_mime *form = NULL;

  if (upload_buf == NULL) {
    upload_buf = malloc(4096);
  }

  if (!strncasecmp("sftp", url, 4)) {
    is_ftp = is_sftp = 1;
  } else if (!strncasecmp("ftp", url, 3)) {
    is_ftp = 1;
  }

  /* CURL short ftp list doesn't tell us if something is
   * a directory. Test by appending a / if there isn't one
   */
  if (is_ftp && method == SURL_METHOD_GET) {
    ftp_is_maybe_dir = url[strlen(url)-1] != '/';
    ftp_try_dir      = !ftp_is_maybe_dir;
  }

  /* Handle methods not needing a curlbuf first */
  if (method == SURL_METHOD_GETTIME) {
    uint32_t now = htonl((uint32_t)time(NULL));
    simple_serial_putc(SURL_ANSWER_TIME);
    IO_BARRIER("GETTIME");
    simple_serial_write_fast((char *)&now, 4);
    return NULL;
  } else if (method == SURL_METHOD_PING) {
    simple_serial_putc(SURL_ANSWER_PONG);
    IO_BARRIER("PONG");
    simple_serial_putc(SURL_PROTOCOL_VERSION);
    return NULL;
  } else if (method == SURL_METHOD_RAW) {
    simple_serial_putc(SURL_ANSWER_RAW_START);
    surl_server_raw_session(url);
    return NULL;
  } else if (method == SURL_METHOD_STREAM_VIDEO) {
    simple_serial_putc(SURL_ANSWER_WAIT);
    surl_stream_video(url);
    return NULL;
  } else if (method == SURL_METHOD_STREAM_AUDIO) {
    char *translit;
    char monochrome;
    simple_serial_putc(SURL_ANSWER_WAIT);
    simple_serial_gets(reqbuf, BUFSIZE);
    translit = reqbuf;
    monochrome = simple_serial_getc();

    surl_stream_audio(url, translit, monochrome);
    return NULL;
  }

  if (ftp_is_maybe_dir) {
    /* try dir first */
    char *dir_url = malloc(strlen(url) + 2);
    memcpy(dir_url, url, strlen(url));
    dir_url[strlen(url)] = '/';
    dir_url[strlen(url) + 1] = '\0';
    curlbuf = surl_handle_request(method, dir_url, headers, n_headers);
    free(dir_url);

    /* Did we get a good response ? */
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

  /* Init curl */
  if (curl == NULL) {
    curl = curl_easy_init();
    
  } else {
    curl_easy_reset(curl);
  }
  proxy_set_curl_opts(curl);

  if (is_ftp) {
    curl_ftp_opts = curl_slist_append(curl_ftp_opts, "*OPTS UTF8 ON");
    r |= curl_easy_setopt(curl, CURLOPT_QUOTE, curl_ftp_opts);
  }

  if (method == SURL_METHOD_POST || method == SURL_METHOD_POST_DATA) {
      if (is_ftp) {
        printf("REQ: Unsupported FTP method POST\n");
        curl_buffer_free(curlbuf);
        return NULL;
      }

      if (method == SURL_METHOD_POST) {
        if (setup_simple_upload_request(method, curl, &curl_headers, curlbuf) < 0) {
          curl_buffer_free(curlbuf);
          return NULL;
        }
      } else if (method == SURL_METHOD_POST_DATA) {
        form = setup_multipart_upload_request(method, curl, &curl_headers, curlbuf);
        if (form == NULL) {
          curl_buffer_free(curlbuf);
          return NULL;
        }
      }
  } else if (method == SURL_METHOD_PUT) {
    if (setup_simple_upload_request(SURL_METHOD_PUT, curl, &curl_headers, curlbuf) < 0) {
      curl_buffer_free(curlbuf);
      return NULL;
    }
    if (is_sftp) {
      r |= curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_SFTP);
    } else if (is_ftp) {
      r |= curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_FTP);
    }
  } else if (method == SURL_METHOD_DELETE) {
      if (is_ftp) {
        r |= setup_ftp_delete(curl, url);
      } else {
        r |= curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
      }

      simple_serial_putc(SURL_ANSWER_WAIT);

      if (r) {
        printf("CURL: Could not set DELETE option(s)\n");
      }
  } else if (method == SURL_METHOD_GET) {
    /* Don't send WAIT twice */
    if (ftp_try_dir || !ftp_is_maybe_dir) {
      simple_serial_putc(SURL_ANSWER_WAIT);
    }
  } else {
    printf("Unsupported method 0x%02x\n", method);
    curlbuf->response_code = 500;
    curlbuf->size = 0;
    curlbuf->content_type = strdup("application/octet-stream");
    return curlbuf;
  }

  printf("%s %s - start\n", surl_method_str(method), url);

  if (c_url == NULL) {
    c_url = curl_url();
  }
  r |= curl_url_set(c_url, CURLUPART_URL, url, CURLU_URLENCODE);
  if (r != 0) {
    printf("Can't set URL %s (%d)\n", url, r);
  }
  /* Setup standards options */
  r |= curl_easy_setopt(curl, CURLOPT_CURLU, c_url);
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
  /* Add custom headers */
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
  if (curl_ftp_opts) {
    curl_slist_free_all(curl_ftp_opts);
    curl_ftp_opts = NULL;
  }
  if (form) {
    curl_mime_free(form);
    form = NULL;
  }

  if(res != CURLE_OK) {
    printf("CURL: error %d: %s\n", res, curl_easy_strerror(res));
    if (res == CURLE_REMOTE_ACCESS_DENIED) {
      curlbuf->response_code = 401;
    } else if (res == CURLE_OPERATION_TIMEDOUT) {
      curlbuf->response_code = 504;
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
