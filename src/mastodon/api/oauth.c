#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "dget_text.h"
#include "strsplit.h"
#include "dputs.h"
#include "extended_conio.h"
#include "common.h"

/* This is heavily based on screen-scraping and not very solid.
 * Maybe using https://github.com/lexbor/lexbor proxy-side would be
 * better.
 */

#define LOGIN_ENDPOINT "/auth/sign_in"
#define LOGOUT_ENDPOINT "/auth/sign_out"
#define REGISTER_ENDPOINT "/api/v1/apps"
#define OAUTH_AUTH_ENDPOINT "/oauth/authorize"
#define OAUTH_TOKEN_ENDPOINT "/oauth/token"

#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"

#define CSRF_TOKEN "authenticity_token"
#define CSRF_TOKEN_SCRAPE "name=['\"]"CSRF_TOKEN

extern char *instance_url;
extern char *client_id;
extern char *client_secret;
extern char *login;
extern char *oauth_code;
extern char *oauth_token;

char *password = NULL;

#define LOG_URL(method, url) do { \
  dputs(method);                  \
  dputs(url);                     \
  dputs("... ");                   \
} while(0)

static char *get_csrf_token(void) {
  char *w, *token = NULL;
  size_t len;

  if (surl_find_line(gen_buf, CSRF_TOKEN_SCRAPE, BUF_SIZE, SURL_REGEXP_CASE_SENSITIVE) == 0) {
    w = strstr(gen_buf, CSRF_TOKEN);
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

static char *get_oauth_code(void) {
  char *w, *token = NULL;
  size_t len;

  w = strstr(gen_buf, "code=");
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

int do_login(void) {
  char *authorize_endpoint;
  char *token;
  size_t buf_size = 256;
  size_t post_len;
  int ret = -1;
  char otp_required = 0;
  char oauth_required = 0;

/* Force logout */
  LOG_URL("DELETE ", LOGOUT_ENDPOINT);
  get_surl_for_endpoint(SURL_METHOD_DELETE, LOGOUT_ENDPOINT);
  dputs("OK.\r\n");

  /* Cannot use gen_buf here. */
  authorize_endpoint = malloc0(BUF_SIZE);
  snprintf(authorize_endpoint, BUF_SIZE,
            OAUTH_AUTH_ENDPOINT
            "?response_type=code"
            "&client_id=%s"
            "&redirect_uri=" REDIRECT_URI
            "&scope=read+write",
            client_id);

/* Get authorization */
  LOG_URL("GET ", OAUTH_AUTH_ENDPOINT);
  get_surl_for_endpoint(SURL_METHOD_GET, authorize_endpoint);

  if (surl_response_ok()) {
    dputs("OK.\r\n");
  } else {
    dputs("Error.\r\n");
    goto err_out;
  }

/* Get authorization done, password if needed */
  password = malloc0(50);

password_again:
  token = get_csrf_token();
  if (IS_NULL(token)) {
    goto err_out;
  }

reenter_password:
  dputs("Your password: ");

  dgets_echo_on = 0;
  password[0] = '\0';
  dget_text_single(password, 50, NULL);
  if (password[0] == '\0') {
    goto reenter_password;
  }

  dgets_echo_on = 1;

/* Second request to send login */
  LOG_URL("POST ", LOGIN_ENDPOINT);
  get_surl_for_endpoint(SURL_METHOD_POST, LOGIN_ENDPOINT);

  snprintf(gen_buf, BUF_SIZE, CSRF_TOKEN"\n%s\nuser[email]\n%s\nuser[password]\n%s\nbutton\n\n",
                              token, login, password);
  post_len = strlen(gen_buf);
  free(token);

  surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
  surl_send_data_chunk(gen_buf, post_len);

  surl_read_response_header();

  if (!surl_response_ok()) {
    dputs("Invalid response to POST.\r\n");
    goto err_out;
  }

  surl_find_line(gen_buf, "flash-message alert", BUF_SIZE, SURL_MATCH_CASE_SENSITIVE);
  if (gen_buf[0] != '\0') {
    dputs("Authentication error.\r\n");
    goto password_again;
  }

  dputs("OK.\r\n");

  surl_find_line(gen_buf, "otp-authentication-form", BUF_SIZE, SURL_MATCH_CASE_SENSITIVE);
  if (gen_buf[0] != '\0') {
    otp_required = 1;
  }

/* Third request for OTP */
  if (otp_required) {
    char *otp = malloc0(10);

otp_again:
    token = get_csrf_token();
    if (IS_NULL(token)) {
      goto err_out;
    }
    dputs("Enter OTP code: ");
    otp[0] = '\0';
    dget_text_single(otp, 9, NULL);

    LOG_URL("POST ", LOGIN_ENDPOINT);
    get_surl_for_endpoint(SURL_METHOD_POST, LOGIN_ENDPOINT);

    snprintf(gen_buf, BUF_SIZE, CSRF_TOKEN"\n%s\nuser[otp_attempt]\n%s\nbutton\n\n",
              token, otp);
    post_len = strlen(gen_buf);
    free(token);

    surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
    surl_send_data_chunk(gen_buf, post_len);

    surl_read_response_header();

    if (!surl_response_ok()) {
      dputs("Invalid response to POST.\r\n");
      goto err_out;
    } else {

      surl_find_line(gen_buf, "flash-message alert", BUF_SIZE, SURL_MATCH_CASE_SENSITIVE);
      if (gen_buf[0] != '\0') {
        dputs("OTP error.\r\n");
        goto otp_again;
      }

      dputs("OK.\r\n");
    }
    free(otp);
  }
  /* End of login */

  if (surl_find_line(gen_buf, "action=['\"]"OAUTH_AUTH_ENDPOINT, BUF_SIZE, SURL_REGEXP_CASE_SENSITIVE) == 0) {
    oauth_required = 1;
  }

  if (oauth_required) {
    /* This only works because Authorize is before Deny */
    token = get_csrf_token();
    if (IS_NULL(token)) {
      goto err_out;
    }

    /* Oauth request */
    LOG_URL("POST ", OAUTH_AUTH_ENDPOINT);
    get_surl_for_endpoint(SURL_METHOD_POST, OAUTH_AUTH_ENDPOINT);

    snprintf(gen_buf, BUF_SIZE, CSRF_TOKEN"\n%s\n"
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
    post_len = strlen(gen_buf);
    free(token);

    surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
    surl_send_data_chunk(gen_buf, post_len);

    surl_read_response_header();

    if (!surl_response_ok()) {
      dputs("Invalid response to POST.\r\n");
      goto err_out;
    }

    if (surl_find_header(gen_buf, "Location: ", BUF_SIZE, SURL_MATCH_CASE_INSENSITIVE) == 0) {
      free(oauth_code);
      oauth_code = get_oauth_code();
      dputs("OK.\r\n");
    } else {
      dputs("No OAuth code.\r\n");
      goto err_out;
    }
  }

  ret = 0;

err_out:
  free(authorize_endpoint);
  return ret;

}

#ifdef __CC65__
#pragma code-name (pop)
#endif

int register_app(void) {
  char *post;
  size_t post_len;
  int res;

  res = -1;

  LOG_URL("POST ", REGISTER_ENDPOINT);
  get_surl_for_endpoint(SURL_METHOD_POST, REGISTER_ENDPOINT);

  snprintf(gen_buf, BUF_SIZE, "client_name\n%s\n"
                              "redirect_uris\n%s\n"
                              "scopes\nread write\n"
                              "website\n%s\n",

                              "Mastodon for Apple II",
                              REDIRECT_URI,
                              "https://www.colino.net/wordpress/en/mastodon-for-apple-II/");
  post_len = strlen(gen_buf);

  surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
  surl_send_data_chunk(gen_buf, post_len);

  surl_read_response_header();

  if (!surl_response_ok()) {
    dputs("Invalid response to POST.\r\n");
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

  dputs("OK.\r\n");
  res = 0;

err_out:
  return res;
}

int get_oauth_token(void) {
  size_t post_len;

/* First request to get authorization */
  LOG_URL("POST ", OAUTH_TOKEN_ENDPOINT);
  get_surl_for_endpoint(SURL_METHOD_POST, OAUTH_TOKEN_ENDPOINT);

  if (IS_NULL(oauth_token)) {
    oauth_token = malloc(50);
  }

  snprintf(gen_buf, BUF_SIZE, "grant_type\n%s\n"
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
  post_len = strlen(gen_buf);

  surl_send_data_params((uint32)post_len, SURL_DATA_X_WWW_FORM_URLENCODED_HELP);
  surl_send_data_chunk(gen_buf, post_len);

  surl_read_response_header();

  if (surl_get_json(oauth_token, ".access_token", NULL, SURL_HTMLSTRIP_NONE, BUF_SIZE) < 0) {
    dputs("No OAuth token.\r\n");
    free(oauth_token);
    oauth_token = NULL;
    return -1;
  } else {
    char *w;
    if (IS_NOT_NULL(w = strchr(oauth_token, '\n'))) {
      *w = '\0';
    }
    dputs("OK.\r\n");
    return 0;
  }
}
