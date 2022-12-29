#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "simple_serial.h" //Mostly for timings */
#include "extended_string.h"

#define BUFSIZE 255

typedef struct _curl_buffer curl_buffer;
struct _curl_buffer {
  char *buffer;
  int response_code;
  size_t size;

  char *headers;
  size_t headers_size;
};

static curl_buffer *curl_request(char *method, char *url, char **headers, int n_headers);
static void send_response(curl_buffer *curlbuf);
static void curl_buffer_free(curl_buffer *curlbuf);

int main(int argc, char **argv)
{
  char reqbuf[BUFSIZE];
  char *method = NULL, *url = NULL;
  char **headers = NULL;
  int i, n_headers = 0;

  if (argc < 2) {
    printf("Usage: %s [serial tty]\n", argv[0]);
    exit(1);
  }

  simple_serial_open(argv[1], B9600);

  curl_global_init(CURL_GLOBAL_ALL);

  while(1) {
    curl_buffer *response;

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

    if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
      char **parts;
      int num_parts, i;
      
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
    }
    
    do {
      reqbuf[0] = '\0';
      if (simple_serial_gets(reqbuf, BUFSIZE) != NULL && strcmp(reqbuf, "\n")) {
        headers = realloc(headers, (n_headers + 1) * sizeof(char *));
        headers[n_headers] = trim(reqbuf);
        n_headers++;
      }
    } while (strcmp(reqbuf, "\n"));

    if (method == NULL || url == NULL) {
      printf("could not parse request '%s'\n", reqbuf);
      continue;
    }
    
    response = curl_request(method, url, headers, n_headers);
    
    send_response(response);
    printf("sent %d response to %s %s (%ld bytes)\n", response->response_code, method, url, response->size);

    curl_buffer_free(response);
    response = NULL;
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
  free(curlbuf->buffer);
  free(curlbuf->headers);
  free(curlbuf);
}

static void proxy_set_curl_opts(CURL *curl) {
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "http-serial-proxy/1.0");
}

static curl_buffer *curl_request(char *method, char *url, char **headers, int n_headers) {
  CURL *curl;
  CURLcode res;
  int i;
  curl_buffer *curlbuf = malloc(sizeof(curl_buffer));

  curlbuf->buffer = NULL;
  curlbuf->size = 0;
  curlbuf->response_code = 500;
  curlbuf->headers = NULL;
  curlbuf->headers_size = 0;

  if (strcmp(method, "GET")) {
    printf("Unsupported method %s\n", method);
    return curlbuf;
  }

  curl = curl_easy_init();
  proxy_set_curl_opts(curl);

  printf("%s %s\n", method, url);
  if (curl) {
    struct curl_slist *curl_headers = NULL;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_write_header_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)curlbuf);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)curlbuf);
  
    for (i = 0; i < n_headers; i++) {
      curl_headers = curl_slist_append(curl_headers, headers[i]);
      printf("%s\n", headers[i]);
    }
    
    printf("\n");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    res = curl_easy_perform(curl);
    curl_slist_free_all(curl_headers);

    if(res != CURLE_OK) {
      printf("curl error %s\n", curl_easy_strerror(res));
    }
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &(curlbuf->response_code));
  }

  curl_easy_cleanup(curl);
  return curlbuf;
}

static void send_response(curl_buffer *curlbuf) {
  simple_serial_printf("%d,%d\n", curlbuf->response_code, curlbuf->size);
  printf("%d,%ld\n", curlbuf->response_code, curlbuf->size);

  simple_serial_write(curlbuf->buffer, sizeof(char), curlbuf->size);
  printf("%s\n", curlbuf->buffer);
}
