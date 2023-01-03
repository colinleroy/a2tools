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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include <curl/curl.h>

#define BUFSIZE 255

#ifdef __CC65__
#error
#else

void http_connect_proxy(void) {
  curl_global_init(CURL_GLOBAL_ALL);
}

static size_t curl_write_data_cb(void *contents, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  http_response *resp = (http_response *)data;

  char *ptr = realloc(resp->body, resp->size + realsize + 1);

  if(!ptr) {
    printf("not enough memory\n");
    return 0;
  }

  resp->body = ptr;
  memcpy(&(resp->body[resp->size]), contents, realsize);
  resp->size += realsize;
  resp->body[resp->size] = '\0';

  return realsize;
}

http_response *http_request(const char *method, const char *url, const char **headers, int n_headers) {
  http_response *resp = malloc(sizeof(http_response));
  int i;
  CURL *curl;
  CURLcode res;

  resp->body = NULL;
  resp->size = 0;
  resp->code = 500;

  if (strcmp(method, "GET")) {
    printf("Unsupported method %s\n", method);
    return resp;
  }

  curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "http-serial-proxy/1.0");

  if (curl) {
    struct curl_slist *curl_headers = NULL;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp);
  
    for (i = 0; i < n_headers; i++) {
      curl_headers = curl_slist_append(curl_headers, headers[i]);
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    res = curl_easy_perform(curl);
    curl_slist_free_all(curl_headers);

    if(res != CURLE_OK) {
      printf("curl error %s\n", curl_easy_strerror(res));
    } else {
      curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &(resp->code));
    }
  }

  curl_easy_cleanup(curl);
  return resp;
}

void http_response_free(http_response *resp) {
  free(resp->body);
  free(resp);
}
#endif
