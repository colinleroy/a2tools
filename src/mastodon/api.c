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
#define STATUS_ENDPOINT   "/api/v1/statuses/"

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

  *handle = NULL;
  *public_name = NULL;

  endpoint = malloc(BUF_SIZE);
  if (endpoint == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return -1;
  }
  snprintf(endpoint, BUF_SIZE, "%s/verify_credentials", ACCOUNTS_ENDPOINT);
  resp = get_surl_for_endpoint("GET", endpoint);
  free(endpoint);

  if (resp == NULL || resp->code < 200 || resp->code >= 300)
    goto err_out;

  if (surl_get_json(resp, gen_buf, BUF_SIZE, 0, 1, ".display_name,.username") == 0) {
    if (strchr(gen_buf,'\n')) {
      *handle = strdup(strchr(gen_buf,'\n') + 1);
      *(strchr(gen_buf, '\n')) = '\0';
      *public_name = strdup(gen_buf);
    }
  }

  r = 0;
err_out:
  surl_response_free(resp);
  return r;
}

int api_get_timeline_posts(char *tlid, char to_load, char *last_to_load, char *first_to_load, char **post_ids) {
  surl_response *resp;
  char *endpoint;
  int n_status;
  char *raw;

  endpoint = malloc(BUF_SIZE);
  if (endpoint == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return 0;
  }

  n_status = 0;
  snprintf(endpoint, BUF_SIZE, "%s/%s?limit=%d%s%s%s%s", TIMELINE_ENDPOINT, tlid, to_load,
            first_to_load ? "&max_id=" : "",
            first_to_load ? first_to_load : "",
            last_to_load ? "&min_id=" : "",
            last_to_load ? last_to_load : ""
          );
  resp = get_surl_for_endpoint("GET", endpoint);
  free(endpoint);
  
  if (resp == NULL || resp->code < 200 || resp->code >= 300)
    goto err_out;

  raw = malloc(512);
  if (surl_get_json(resp, raw, 512, 0, 0, ".[].id") == 0) {
    char **tmp;
    int i;
    n_status = strsplit(raw, '\n', &tmp);
    for (i = 0; i < n_status; i++) {
      post_ids[i] = tmp[i];
    }
    free(tmp);
  }
  free(raw);

err_out:
  surl_response_free(resp);
  return n_status;
}

int api_get_status_and_replies(char to_load, status *root, char **post_ids) {
  surl_response *resp;
  char *endpoint;
  int n_status;
  char n_before, n_after;
  char *raw;

  endpoint = malloc(BUF_SIZE);
  if (endpoint == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return 0;
  }

  n_status = 0;
  snprintf(endpoint, BUF_SIZE, "%s/%s/context", STATUS_ENDPOINT, root->reblog ? root->reblog->id : root->id);
  resp = get_surl_for_endpoint("GET", endpoint);
  free(endpoint);
  
  if (resp == NULL || resp->code < 200 || resp->code >= 300)
    goto err_out;

  raw = malloc(512);
  n_before = to_load/3;
  n_after  = (2 * to_load) / 3;
  if (n_before + n_after == to_load) {
    n_before--;
  }
  snprintf(selector, SELECTOR_SIZE, "(.ancestors|.[-%d:]|.[].id),\"-\",(.descendants|.[0:%d]|.[].id)", n_before, n_after);
  if (surl_get_json(resp, raw, 512, 0, 0, selector) == 0) {
    char **tmp;
    int i;
    n_status = strsplit(raw, '\n', &tmp);
    for (i = 0; i < n_status; i++) {
      if (tmp[i][0] != '-') {
        post_ids[i] = tmp[i];
      } else {
        post_ids[i] = strdup(root->id);
        free(tmp[i]);
      }
    }
    free(tmp);
  }
  free(raw);

err_out:
  surl_response_free(resp);
  return n_status;
}

status *api_get_status(char *status_id, char full) {
  surl_response *resp;
  status *s;
  char *endpoint;

  s = NULL;

  endpoint = malloc(BUF_SIZE);
  if (endpoint == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }

  snprintf(endpoint, BUF_SIZE, "%s/%s", STATUS_ENDPOINT, status_id);
  resp = get_surl_for_endpoint("GET", endpoint);
  free(endpoint);
  
  if (resp == NULL || resp->code < 200 || resp->code >= 300)
    goto err_out;

  s = status_new_from_json(resp, status_id, full, 0);

err_out:
  surl_response_free(resp);
  return s;
}
