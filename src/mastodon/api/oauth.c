#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "cgets.h"
#include "strsplit.h"
#ifdef __CC65__
#include <conio.h>
#include "dputs.h"
#else
#include "extended_conio.h"
#endif

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

#define BUF_SIZE 255

#define LOGIN_URL "/auth/sign_in"
#define REGISTER_URL "/api/v1/apps"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"
#define OAUTH_URL "/oauth/authorize"

#define CSRF_TOKEN "authenticity_token"
#define CSRF_TOKEN_SCRAPE "name=\""CSRF_TOKEN
#define OAUTH_CODE_SCRAPE "class='oauth-code"

extern char *instance_url;
extern char *client_id;
extern char *client_secret;
extern char *login;
extern char *password;
extern char *oauth_code;
extern char *oauth_token;

static char *get_csrf_token(surl_response *resp, char *body, size_t buf_size) {
  char *w, *token = NULL;
  size_t len;

  if (surl_find_line(resp, body, buf_size, CSRF_TOKEN_SCRAPE) == 0) {
    w = strstr(body, CSRF_TOKEN);
    if (w == NULL) {
      return NULL;
    }
    w = strstr(w, "value=");
    if (w == NULL) {
      return NULL;
    }
    w = strchr(w, '"');
    if (w == NULL) {
      return NULL;
    }
    w++;
    len = strchr(w, '"') - w;
    token = malloc(len + 1);
    strncpy(token, w, len);
    token[len] = '\0';
  } else {
    dputs("Error extracting CSRF token.\r\n");
  }
  return token;
}

static char *get_oauth_code(char *body) {
  char *w, *token = NULL;
  size_t len;
  
  w = strstr(body, OAUTH_CODE_SCRAPE);
  if (w == NULL) {
    return NULL;
  }
  w = strstr(w, "value=");
  if (w == NULL) {
    return NULL;
  }
  w = strchr(w, '\'');
  if (w == NULL) {
    return NULL;
  }
  w++;
  len = strchr(w, '\'') - w;
  token = malloc(len + 1);
  strncpy(token, w, len);
  token[len] = '\0';

  return token;
}

static char *prepare_login_post(char *login, char *password, char *token) {
  char *data = malloc(512);
  snprintf(data, 511, CSRF_TOKEN"\n%s\nuser[email]\n%s\nuser[password]\n%s\nbutton\n\n",
            token, login, password);
  return data;
}

static char *prepare_otp_post(char *otp, char *token) {
  char *data = malloc(512);
  snprintf(data, 511, CSRF_TOKEN"\n%s\nuser[otp_attempt]\n%s\nbutton\n\n",
            token, otp);
  return data;
}

static char *prepare_oauth_post(char *token) {
  char *data = malloc(512);
  snprintf(data, 511, CSRF_TOKEN"\n%s\n"
                      "client_id\n%s\n"
                      "redirect_uri\n%s\n"
                      "state\n%s\n"
                      "response_type\n%s\n"
                      "scope\n%s\n"
                      "button\n\n",
                      token,
                      client_id,
                      REDIRECT_URI,
                      "",
                      "code",
                      "read write");
  return data;
}

int do_login(void) {
  surl_response *resp;
  char *authorize_url;
  char *login_url;
  char *oauth_url;
  char *headers;
  char *body;
  char *token;
  char *post;
  size_t buf_size = 2048;
  size_t post_len;
  int ret = -1;
  char otp_required = 0;
  char login_required = 0;
  char oauth_required = 0;

  resp = NULL;
  headers = NULL;
  body = NULL;

  authorize_url = malloc(BUF_SIZE);
  snprintf(authorize_url, BUF_SIZE,
            "%s" OAUTH_URL
            "?response_type=code"
            "&client_id=%s"
            "&redirect_uri=" REDIRECT_URI
            "&scope=read+write",
            instance_url, client_id);

  login_url = malloc(BUF_SIZE);
  snprintf(login_url, BUF_SIZE, "%s%s", instance_url, LOGIN_URL);

  oauth_url = malloc(BUF_SIZE);
  snprintf(oauth_url, BUF_SIZE, "%s%s", instance_url, OAUTH_URL);

/* First request to get authorization */
  dputs("GET "OAUTH_URL"... ");
  resp = surl_start_request(SURL_METHOD_GET, authorize_url, NULL, 0);
  if (resp == NULL) {
    dputs("Could not start request.\r\n");
    goto err_out;
  }

  body = malloc(buf_size + 1);
  if (body == NULL) {
    dputs("Could not allocate body buffer.\r\n");
    goto err_out;
  }

  if (resp->code == 200) {
    dputs("OK.\r\n");
  } else {
    dputs("Error.\r\n");
    goto err_out;
  }
  if (surl_find_line(resp, body, buf_size, "action=\""LOGIN_URL) == 0) {
    login_required = 1;
    dputs("Login required.\r\n");
    dputs("Enter password: ");
    password = malloc(50);
    
    echo(0);
    cgets(password, 50);
    echo(1);
    *strchr(password, '\n') = '\0';
  } else {
    dputs("Login still valid.\r\n");
  }

  if (login_required) {
    token = get_csrf_token(resp, body, buf_size);
    if (token == NULL)
      goto err_out;
    surl_response_free(resp);

  /* Second request to send login */
    post = prepare_login_post(login, password, token);
    post_len = strlen(post);
    free(token);

    dputs("POST "LOGIN_URL"... ");
    resp = surl_start_request(SURL_METHOD_POST, login_url, NULL, 0);

    if (resp == NULL) {
      dputs("Could not start request.\r\n");
      goto err_out;
    }

    surl_send_data_params(resp, post_len, 0);
    surl_send_data(resp, post, post_len);
    free(post);

    surl_read_response_header(resp);

    if (resp->code != 200) {
      dputs("Invalid response to POST\r\n");
      goto err_out;
    } else {
      dputs("OK\r\n");
    }

    surl_find_line(resp, body, buf_size, "otp-authentication-form");
    if (body[0] != '\0') {
      otp_required = 1;
      dputs("OTP required.\r\n");
      token = get_csrf_token(resp, body, buf_size);
      if (token == NULL)
        goto err_out;
    }
    surl_response_free(resp);

  /* Third request for OTP */
    if (otp_required) {
      char *otp = malloc(10);
      dputs("Enter OTP code: ");
      cgets(otp, 9);
      *strchr(otp, '\n') = '\0';

      post = prepare_otp_post(otp, token);
      post_len = strlen(post);
      free(token);
      free(otp);

      dputs("POST "LOGIN_URL"... ");
      resp = surl_start_request(SURL_METHOD_POST, login_url, NULL, 0);

      if (resp == NULL) {
        dputs("Could not start request.\r\n");
        return -1;
      }

      surl_send_data_params(resp, post_len, 0);
      surl_send_data(resp, post, post_len);
      free(post);

      surl_read_response_header(resp);

      if (resp->code != 200) {
        dputs("Invalid response to POST\r\n");
        goto err_out;
      } else {
        dputs("OK\r\n");
      }

      token = NULL;
    }
  }
  /* End of login */

  if (surl_find_line(resp, body, buf_size, "action=\""OAUTH_URL) == 0) {
    oauth_required = 1;
    dputs("OAuth authorization required.\r\n");
  } else {
    dputs("OAuth authorization valid.\r\n");
  }

  if (oauth_required) {
    token = get_csrf_token(resp, body, buf_size);
    if (token == NULL)
      goto err_out;

    surl_response_free(resp);

  /* Oauth request */
    post = prepare_oauth_post(token);
    post_len = strlen(post);
    free(token);

    dputs("POST "OAUTH_URL"... ");
    resp = surl_start_request(SURL_METHOD_POST, oauth_url, NULL, 0);

    surl_send_data_params(resp, post_len, 0);
    surl_send_data(resp, post, post_len);
    free(post);

    surl_read_response_header(resp);

    if (resp->code != 200) {
      dputs("Invalid response to POST\r\n");
      goto err_out;
    } else {
      dputs("OK.\r\n");
    }

    if (surl_find_line(resp, body, buf_size, "input class='oauth-code") == 0) {
      free(oauth_code);
      oauth_code = get_oauth_code(body);
      dputs("Got OAuth code.\r\n");
    } else {
      dputs("Did not get oauth code.\r\n");
      goto err_out;
    }
  }

  ret = 0;

err_out:
  surl_response_free(resp);
  free(body);
  free(oauth_url);
  free(login_url);
  free(authorize_url);
  return ret;

}

static char *prepare_app_register_post(void) {
  char *data;

  data = malloc(512);

  snprintf(data, 511, "client_name\n%s\n"
                      "redirect_uris\n%s\n"
                      "scopes\nread write\n"
                      "website\n%s\n",

                      "MastApple//c",
                      REDIRECT_URI,
                      "https://www.colino.net/");
  return data;
}

int register_app(void) {
  surl_response *resp;
  char *post;
  char *reg_url;
  size_t post_len;
  int res;
  
  res = -1;

  post = prepare_app_register_post();
  post_len = strlen(post);

  reg_url = malloc(strlen(instance_url) + strlen(REGISTER_URL) + 1);
  sprintf(reg_url, "%s%s", instance_url, REGISTER_URL);

  dputs("POST "REGISTER_URL"... ");
  resp = surl_start_request(SURL_METHOD_POST, reg_url, NULL, 0);
  free(reg_url);

  if (resp == NULL) {
    dputs("Could not start request.\r\n");
    free(post);
    return -1;
  }

  surl_send_data_params(resp, post_len, 0);
  surl_send_data(resp, post, post_len);
  free(post);

  surl_read_response_header(resp);

  if (resp->code != 200) {
    dputs("App registration: Invalid response to POST\r\n");
    goto err_out;
  }

  if (surl_get_json(resp, client_id, BUF_SIZE, 0, NULL, ".client_id") < 0) {
    dputs("App registration: no client_id.\r\n");
    goto err_out;
  }
  if (surl_get_json(resp, client_secret, BUF_SIZE, 0, NULL, ".client_secret") < 0) {
    dputs("App registration: no client_secret.\r\n");
    goto err_out;
  }

  if (strchr(client_id, '\n'))
    *strchr(client_id, '\n') = '\0';
  if (strchr(client_secret, '\n'))
    *strchr(client_secret, '\n') = '\0';

  dputs("Done.\r\n");
  res = 0;

err_out:
  if (resp) {
    surl_response_free(resp);
  }
  return res;
}


static char *prepare_oauth_token_post(void) {
  char *data;

  data = malloc(512);

  snprintf(data, 511, "grant_type\n%s\n"
                      "code\n%s\n"
                      "client_id\n%s\n"
                      "client_secret\n%s\n"
                      "redirect_uri\n%s\n"
                      "scope\nread write\n",

                      "authorization_code",
                      oauth_code,
                      client_id,
                      client_secret,
                      REDIRECT_URI);
  return data;
}

int get_oauth_token(void) {
  surl_response *resp;
  char *oauth_url;
  char *post;
  size_t buf_size = 2048;
  size_t post_len;
  int ret = -1;

  resp = NULL;

  if (!oauth_token) {
    oauth_token = malloc(50);
  }

  oauth_url = malloc(BUF_SIZE);
  snprintf(oauth_url, BUF_SIZE, "%s/oauth/token", instance_url);

/* First request to get authorization */
  dputs("POST "OAUTH_URL"... ");
  resp = surl_start_request(SURL_METHOD_POST, oauth_url, NULL, 0);
  if (resp == NULL) {
    dputs("Could not start request.\r\n");
    free(oauth_url);
    return -1;
  }

  post = prepare_oauth_token_post();
  post_len = strlen(post);
  surl_send_data_params(resp, post_len, 0);
  surl_send_data(resp, post, post_len);
  free(post);

  surl_read_response_header(resp);

  if (surl_get_json(resp, oauth_token, BUF_SIZE, 0, NULL, ".access_token") < 0) {
    dputs("OAuth token not found.\r\n");
    goto err_out;
  } else {
    if (strchr(oauth_token, '\n')) {
      *strchr(oauth_token, '\n') = '\0';
    }
    dputs("Got OAuth token.\r\n");
  }
  ret = 0;

err_out:
  if (ret != 0) {
    cgetc();
  }
  free(oauth_url);
  surl_response_free(resp);
  return ret;
}
