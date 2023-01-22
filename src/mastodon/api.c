#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "api.h"

#define BUF_SIZE 255

#define ACCOUNTS_ENDPOINT "/api/v1/accounts"
#define TIMELINE_ENDPOINT "/api/v1/timelines"

static unsigned char scrw, scrh;

static char gen_buf[BUF_SIZE];

extern char *instance_url;
extern char *oauth_token;

char selector[SELECTOR_SIZE];

static surl_response *get_surl_for_endpoint(char *method, char *endpoint) {
  static char **token_hdr = NULL;
  surl_response *resp;

  if (token_hdr == NULL) {
    token_hdr = malloc(sizeof(char *));
    token_hdr[0] = malloc(BUF_SIZE);
    snprintf(token_hdr[0], BUF_SIZE, "Authorization: Bearer %s", oauth_token);
  }

  snprintf(gen_buf, BUF_SIZE, "%s%s", instance_url, endpoint);
  resp = surl_start_request(method, gen_buf, token_hdr, 1);
  
  return resp;
}

int api_get_profile(char **public_name, char **handle) {
  surl_response *resp;
  int r = -1;
  char *endpoint;

  endpoint = malloc(BUF_SIZE);
  if (endpoint == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return -1;
  }
  snprintf(endpoint, BUF_SIZE, "%s/verify_credentials", ACCOUNTS_ENDPOINT);
  resp = get_surl_for_endpoint("GET", endpoint);
  free(endpoint);

  if (resp == NULL || resp->code < 200)
    goto err_out;

  if (surl_get_json(resp, gen_buf, BUF_SIZE, 1, 1, ".display_name") == 0) {
    *public_name = strdup(gen_buf);
  }
  if (surl_get_json(resp, gen_buf, BUF_SIZE, 1, 1, ".username") == 0) {
    *handle = strdup(gen_buf);
  }

err_out:
  surl_response_free(resp);
  return r;
}

int api_get_timeline_posts(char *tlid, status ***posts) {
  surl_response *resp;
  status **r = NULL;
  char *endpoint;
  int n_status, i;

  endpoint = malloc(BUF_SIZE);
  if (endpoint == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return -1;
  }

  snprintf(endpoint, BUF_SIZE, "%s/%s?limit=5", TIMELINE_ENDPOINT, tlid);
  resp = get_surl_for_endpoint("GET", endpoint);
  free(endpoint);
  
  *posts = NULL;

  if (resp == NULL || resp->code < 200)
    goto err_out;

  if (surl_get_json(resp, gen_buf, BUF_SIZE, 0, 0, ".[].id") == 0) {
    char **status_ids = NULL;
    n_status = strsplit_in_place(gen_buf, '\n', &status_ids);
    r = malloc(n_status * sizeof(status *));
    if (r == NULL) {
      printf("No more memory at %s:%d\n",__FILE__, __LINE__);
      return -1;
    }
    for (i = 0; i < n_status; i++) {
      r[i] = status_new_from_json(resp, status_ids[i], 0);
    }
    free(status_ids);
  }
  *posts = r;

err_out:
  surl_response_free(resp);
  return n_status;
}
