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
};

static curl_buffer *curl_get(const char *url);
static void send_response(curl_buffer *curlbuf);
static void curl_buffer_free(curl_buffer *curlbuf);

int main(int argc, char **argv)
{
  char reqbuf[BUFSIZE];
  char *method = NULL, *url = NULL;


  if (argc < 2) {
    printf("Usage: %s [serial tty]\n", argv[0]);
    exit(1);
  }

  simple_serial_open(argv[1], B9600);

  curl_global_init(CURL_GLOBAL_ALL);

  while(1) {
    free(method);
    free(url);
    method = NULL;
    url = NULL;

    if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
      char **parts;
      int num_parts, i;
      
      num_parts = strsplit(reqbuf, ' ', &parts);
      
      method = trim(parts[0]);
      url = trim(parts[1]);
      
      for (i = 0; i < num_parts; i++) {
        free(parts[i]);
      }
      free(parts);
    }
    
    if (method == NULL || url == NULL) {
      printf("could not parse request '%s'\n", reqbuf);
      continue;
    }
    
    if (!strcmp(method, "GET")) {
      curl_buffer *response = curl_get(url);
      send_response(response);
      printf("sent response to %s %s:\n", method, url);

      curl_buffer_free(response);
      response = NULL;
    } else {
      printf("%s method: not implemented\n", method);
    }
    reqbuf[0] = '\0';
  }
}

static size_t curl_write_cb(void *contents, size_t size, size_t nmemb, void *data)
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

static void curl_buffer_free(curl_buffer *curlbuf) {
  free(curlbuf->buffer);
  free(curlbuf);
}

static void proxy_set_curl_opts(CURL *curl) {
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "http-serial-proxy/1.0");
}

static curl_buffer *curl_get(const char *url) {
  CURL *curl;
  CURLcode res;
  curl_buffer *curlbuf = malloc(sizeof(curl_buffer));
  
  curlbuf->buffer = NULL;
  curlbuf->size = 0;
  curlbuf->response_code = 500;

  curl = curl_easy_init();
  proxy_set_curl_opts(curl);

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)curlbuf);

    res = curl_easy_perform(curl);

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
  simple_serial_write(curlbuf->buffer, sizeof(char), curlbuf->size);
}
