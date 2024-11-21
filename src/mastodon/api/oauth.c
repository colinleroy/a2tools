#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "dgets.h"
#include "strsplit.h"
#include "dputs.h"
#include "extended_conio.h"
#include "common.h"

/* This is heavily based on screen-scraping and not very solid.
 * Maybe using https://github.com/lexbor/lexbor proxy-side would be
 * better.
 */

#define LOGIN_URL "/auth/sign_in"
#define REGISTER_URL "/api/v1/apps"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"
#define OAUTH_URL "/oauth/authorize"

#define CSRF_TOKEN "authenticity_token"
#define CSRF_TOKEN_SCRAPE "name=['\"]"CSRF_TOKEN

extern char *instance_url;
extern char *client_id;
extern char *client_secret;
extern char *login;
extern char *password;
extern char *oauth_code;
extern char *oauth_token;

static char *get_csrf_token(char *body, size_t buf_size) {
  char *w, *token = NULL;
  size_t len;

  if (surl_find_line(body, CSRF_TOKEN_SCRAPE, buf_size, SURL_REGEXP_CASE_SENSITIVE) == 0) {
    w = strstr(body, CSRF_TOKEN);
    if (IS_NULL(w)) {
      goto out;
    }
    w = strstr(w, "value=");
    if (IS_NULL(w)) {
      goto out;
    }
    w = strchr(w, '"');
    if (IS_NULL(w)) {
      goto out;
    }
    w++;
    len = strchr(w, '"') - w;
    token = malloc0(len + 1);
    strncpy(token, w, len);
    token[len] = '\0';
  }
out:
  if (IS_NULL(token)) {
    dputs("Error extracting CSRF token.\r\n");
  }
  return token;
}

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

static char *get_oauth_code(char *body) {
  char *w, *token = NULL;
  size_t len;
  
  w = strstr(body, "code=");
  if (IS_NULL(w)) {
    return NULL;
  }
  w+=5;
  len = strchr(w, '\r') - w;
  token = malloc0(len + 1);
  strncpy(token, w, len);
  token[len] = '\0';

  return token;
}

static char *prepare_login_post(char *login, char *password, char *token) {
  char *data = malloc0(512);
  snprintf(data, 511, CSRF_TOKEN"\n%s\nuser[email]\n%s\nuser[password]\n%s\nbutton\n\n",
            token, login, password);
  return data;
}

static char *prepare_otp_post(char *otp, char *token) {
  char *data = malloc0(512);
  snprintf(data, 511, CSRF_TOKEN"\n%s\nuser[otp_attempt]\n%s\nbutton\n\n",
            token, otp);
  return data;
}

static char *prepare_oauth_post(char *token) {
  char *data = malloc0(512);
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
  char *authorize_url;
  char *login_url;
  char *oauth_url;
  char *body;
  char *token;
  char *post;
  size_t buf_size = 256;
  size_t post_len;
  int ret = -1;
  char otp_required = 0;
  char login_required = 0;
  char oauth_required = 0;

  body = NULL;

  authorize_url = malloc0(BUF_SIZE);
  snprintf(authorize_url, BUF_SIZE,
            "%s" OAUTH_URL
            "?response_type=code"
            "&client_id=%s"
            "&redirect_uri=" REDIRECT_URI
            "&scope=read+write",
            instance_url, client_id);

  login_url = malloc0(BUF_SIZE);
  snprintf(login_url, BUF_SIZE, "%s%s", instance_url, LOGIN_URL);

  oauth_url = malloc0(BUF_SIZE);
  snprintf(oauth_url, BUF_SIZE, "%s%s", instance_url, OAUTH_URL);

/* First request to get authorization */
  dputs("GET "OAUTH_URL"... ");
  surl_start_request(NULL, 0, authorize_url, SURL_METHOD_GET);

  body = malloc0(buf_size + 1);

  if (surl_response_ok()) {
    dputs("OK.\r\n");
  } else {
    dputs("Error.\r\n");
    goto err_out;
  }

  if (surl_find_line(body, "action=['\"]"LOGIN_URL, buf_size, SURL_REGEXP_CASE_SENSITIVE) == 0) {
    login_required = 1;
    dputs("Login required.\r\n");
  } else {
    dputs("Login still valid.\r\n");
  }

/* Get authorization done, password if needed */

  if (login_required) {
    password = malloc0(50);

password_again:
    token = get_csrf_token(body, buf_size);
    if (IS_NULL(token)) {
      goto err_out;
    }
    dputs("Enter password: ");
    
    dgets_echo_on = 0;
    password[0] = '\0';
    dget_text(password, 50, NULL, 0);
    dgets_echo_on = 1;

  /* Second request to send login */
    post = prepare_login_post(login, password, token);
    post_len = strlen(post);
    free(token);

    dputs("POST "LOGIN_URL"... ");
    surl_start_request(NULL, 0, login_url, SURL_METHOD_POST);

    surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
    surl_send_data_chunk(post, post_len);
    free(post);

    surl_read_response_header();

    if (!surl_response_ok()) {
      dputs("Invalid response to POST\r\n");
      goto err_out;
    } else {
      dputs("OK\r\n");
    }

    surl_find_line(body, "flash-message alert", buf_size, SURL_MATCH_CASE_SENSITIVE);
    if (body[0] != '\0') {
      dputs("Authentication error.\r\n");
      goto password_again;
    }

    surl_find_line(body, "otp-authentication-form", buf_size, SURL_MATCH_CASE_SENSITIVE);
    if (body[0] != '\0') {
      otp_required = 1;
      dputs("OTP required.\r\n");
    }

  /* Third request for OTP */
    if (otp_required) {
      char *otp = malloc0(10);

otp_again:
      token = get_csrf_token(body, buf_size);
      if (IS_NULL(token)) {
        goto err_out;
      }
      dputs("Enter OTP code: ");
      otp[0] = '\0';
      dget_text(otp, 9, NULL, 0);

      post = prepare_otp_post(otp, token);
      post_len = strlen(post);
      free(token);

      dputs("POST "LOGIN_URL"... ");
      surl_start_request(NULL, 0, login_url, SURL_METHOD_POST);

      surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
      surl_send_data_chunk(post, post_len);
      free(post);

      surl_read_response_header();

      if (!surl_response_ok()) {
        dputs("Invalid response to POST\r\n");
        goto err_out;
      } else {
        
        surl_find_line(body, "flash-message alert", buf_size, SURL_MATCH_CASE_SENSITIVE);
        if (body[0] != '\0') {
          dputs("OTP error.\r\n");
          goto otp_again;
        }

        dputs("OK\r\n");
      }
      free(otp);
    }
  }
  /* End of login */

  if (surl_find_line(body, "action=['\"]"OAUTH_URL, buf_size, SURL_REGEXP_CASE_SENSITIVE) == 0) {
    oauth_required = 1;
    dputs("OAuth authorization required.\r\n");
  } else {
    dputs("OAuth authorization valid.\r\n");
  }

  if (oauth_required) {
    /* This only works because Authorize is before Deny */
    token = get_csrf_token(body, buf_size);
    if (IS_NULL(token)) {
      goto err_out;
    }
    /* Oauth request */
    post = prepare_oauth_post(token);
    post_len = strlen(post);
    free(token);

    dputs("POST "OAUTH_URL"... ");
    surl_start_request(NULL, 0, oauth_url, SURL_METHOD_POST);

    surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
    surl_send_data_chunk(post, post_len);
    free(post);

    surl_read_response_header();

    if (!surl_response_ok()) {
      dputs("Invalid response to POST\r\n");
      goto err_out;
    } else {
      dputs("OK.\r\n");
    }

    if (surl_find_header(body, "Location: ", buf_size, SURL_MATCH_CASE_INSENSITIVE) == 0) {
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
  free(body);
  free(oauth_url);
  free(login_url);
  free(authorize_url);
  return ret;

}

#ifdef __CC65__
#pragma code-name (pop)
#endif

static char *prepare_app_register_post(void) {
  char *data;

  data = malloc(512);

  snprintf(data, 511, "client_name\n%s\n"
                      "redirect_uris\n%s\n"
                      "scopes\nread write\n"
                      "website\n%s\n",

                      "Mastodon for Apple II",
                      REDIRECT_URI,
                      "https://www.colino.net/wordpress/en/mastodon-for-apple-II/");
  return data;
}

int register_app(void) {
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
  surl_start_request(NULL, 0, reg_url, SURL_METHOD_POST);
  free(reg_url);

  surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
  surl_send_data_chunk(post, post_len);
  free(post);

  surl_read_response_header();

  if (!surl_response_ok()) {
    cprintf("Invalid response %d to POST", surl_response_code());
    dputs("\r\n"); /* Scroll if needed */
    goto err_out;
  }

  if (surl_get_json(client_id, ".client_id", NULL, SURL_HTMLSTRIP_NONE, BUF_SIZE) < 0) {
    dputs("No client_id.\r\n");
    goto err_out;
  }
  if (surl_get_json(client_secret, ".client_secret", NULL, SURL_HTMLSTRIP_NONE, BUF_SIZE) < 0) {
    dputs("No client_secret.\r\n");
    goto err_out;
  }

  if (IS_NOT_NULL(post = strchr(client_id, '\n')))
    *post = '\0';
  if (IS_NOT_NULL(post = strchr(client_secret, '\n')))
    *post = '\0';

  dputs("Done.\r\n");
  res = 0;

err_out:
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
  char *oauth_url;
  char *post;
  size_t post_len;
  int ret = -1;

  if (IS_NULL(oauth_token)) {
    oauth_token = malloc(50);
  }

  oauth_url = malloc(BUF_SIZE);
  snprintf(oauth_url, BUF_SIZE, "%s/oauth/token", instance_url);

/* First request to get authorization */
  dputs("POST "OAUTH_URL"... ");
  surl_start_request(NULL, 0, oauth_url, SURL_METHOD_POST);

  post = prepare_oauth_token_post();
  post_len = strlen(post);
  surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
  surl_send_data_chunk(post, post_len);
  free(post);

  surl_read_response_header();

  if (surl_get_json(oauth_token, ".access_token", NULL, SURL_HTMLSTRIP_NONE, BUF_SIZE) < 0) {
    dputs("OAuth token not found.\r\n");
    goto err_out;
  } else {
    if (IS_NOT_NULL(post = strchr(oauth_token, '\n'))) {
      *post = '\0';
    }
    dputs("Got OAuth token.\r\n");
  }
  ret = 0;

err_out:
  free(oauth_url);
  return ret;
}
