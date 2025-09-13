#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "malloc0.h"
#include "platform.h"
#include "math.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "compose.h"
#include "common.h"
#include "progress_bar.h"
#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

unsigned int NUM_CHARS = 501;

extern char TRANSLITCMD[];

char *compose_audience_str(char compose_audience) {
  switch(compose_audience) {
    case COMPOSE_PUBLIC:   return "public";
    case COMPOSE_UNLISTED: return "unlisted";
    case COMPOSE_PRIVATE:  return "private";
    case COMPOSE_MENTION:
    default:               return "direct";
  }
}

char *quote_policy_str(char quote_policy) {
  switch(quote_policy) {
    case QUOTE_POLICY_PUBLIC:     return "public";
    case QUOTE_POLICY_FOLLOWERS:  return "followers";
    default:                      return "nobody";
  }
}

#define FILE_ERROR "Can not open file.\r\n"
#define NET_ERROR "Network error.\r\n"

#define SEND_BUF_SIZE 1024
char send_buf[SEND_BUF_SIZE];

void compose_set_num_chars(void) {
  get_surl_for_endpoint(SURL_METHOD_GET, "/api/v2/instance");

  if (!surl_response_ok()) {
    return;
  }
  if (surl_get_json(gen_buf, ".configuration.statuses.max_characters", NULL, SURL_HTMLSTRIP_NONE, BUF_SIZE) < 0) {
    return;
  }
  NUM_CHARS = atoi(gen_buf);
  if (NUM_CHARS > 5000) {
    NUM_CHARS = 5000;
  }
}

char *api_send_hgr_image(char *filename, char *description, char **err, char x, char y, char w) {
  int fd;
  int r;
  int to_send;
  int file_size;
  char *media_id;

  *err = NULL;
  r = 0;

#ifdef __APPLE2__
  _filetype = PRODOS_T_BIN;
#endif

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    *err = FILE_ERROR;
    return NULL;
  }

  file_size = to_send = lseek(fd, 0, SEEK_END);
  /* Will need to reopen, as io buf is shared with surl endpoint buf */
  close(fd);

  if (w > 0)
    progress_bar(x, y, w, 0, file_size);

  get_surl_for_endpoint(SURL_METHOD_POST_DATA, "/api/v2/media");

  /* Send num fields */
  surl_multipart_send_num_fields(1);
  
  /* Send file */
  surl_multipart_send_field_desc("file", (uint32)to_send, 
      monochrome ? "image/hgr" : "image/hgr-color");

  fd = open(filename, O_RDONLY); /* Assume it worked. */
  while ((r = read(fd, send_buf, SEND_BUF_SIZE)) > 0) {
send_again:
    surl_multipart_send_field_data(send_buf, r);
    to_send -= r;
    if (w > 0)
      progress_bar(-1, -1, w, file_size - to_send, file_size);
    if (to_send == 0) {
      break;
    }
  }
  /* Some hgr files don't include the last bytes, that
   * fall into an "HGR-memory-hole" and as such are not
   * indispensable. Fill up with zeroes up to to_send */
  if (to_send > 0) {
    r = min(to_send, SEND_BUF_SIZE);
    bzero(send_buf, r);
    goto send_again;
  }

  close(fd);

  surl_read_response_header();

  media_id = NULL;
  *err = NET_ERROR;
  if (surl_response_ok()) {
    if (surl_get_json(gen_buf, ".id", translit_charset, SURL_HTMLSTRIP_NONE, BUF_SIZE) > 0) {
      if (IS_NOT_NULL(media_id = strchr(gen_buf, '\n'))) {
        *media_id = '\0';
      }
      media_id = strdup(gen_buf);
      *err = NULL;
    }
  }

  if (IS_NOT_NULL(media_id)) {
    /* Set description */
    int len;
    snprintf(send_buf, SEND_BUF_SIZE, "S|description|%s|%s\n%s\n",
                          TRANSLITCMD,
                          translit_charset,
                          description);

    len = strlen(send_buf);

    snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, MEDIA_ENDPOINT"/%s", media_id);
    get_surl_for_endpoint(SURL_METHOD_PUT, endpoint_buf);

    surl_send_data_params((uint32)len, SURL_DATA_APPLICATION_JSON_HELP);
    surl_send_data_chunk(send_buf, len);

    surl_read_response_header();
  }

  return media_id;
}

#define BODY_SIZE 1536
#define EXTRA_BUF_SIZE 768

static char body[BODY_SIZE];
static char extra_buf[EXTRA_BUF_SIZE];
static char ref_status_field[SNOWFLAKE_ID_LEN*2];

signed char api_send_toot(char mode, char *buffer, char *cw, char sensitive_medias,
                          char *ref_toot_id, char **media_ids, char n_medias,
                          poll *toot_poll, char compose_audience,
                          char quote_policy, char **err) {
  int i, o, len;
  char c;

  *err = NULL;

  if (n_medias > 0) {
    snprintf(extra_buf, 768, "A|media_ids\n[\"%s\"", media_ids[0]);
    for (c = 1; c < n_medias; c++) {
      strcat(extra_buf, ",\"");
      strcat(extra_buf, media_ids[c]);
      strcat(extra_buf, "\"");
    }
    strcat(extra_buf, "]\n");
  } else if (IS_NOT_NULL(toot_poll)) {
    char duration, n_options;
    for (duration = 0; duration < NUM_POLL_DURATIONS; duration++) {
      if (compose_poll_durations_hours[duration] == toot_poll->expires_in_hours)
        break;
    }
    n_options = toot_poll->options_count;

    /* Doing real JSON here because the object is too complicated to
     * transform. This means no translit :(
     */
    snprintf(extra_buf, 768, "O|poll\n{"
                             "\"expires_in\": %s,"
                             "\"multiple\": %s,"
                             "\"options\":[\"%s\"",
                             compose_poll_durations_seconds[duration],
                             toot_poll->multiple ? "true":"false",
                             toot_poll->options[0].title);
    for (c = 1; c < n_options; c++) {
      strcat(extra_buf, ",\"");
      strcat(extra_buf, toot_poll->options[c].title);
      strcat(extra_buf, "\"");
    }
    strcat(extra_buf, "]}\n");
  } else {
    extra_buf[0] = '\0';
  }

  /* Init ref status field to nothing */
  ref_status_field[0] = '\0';

  switch(mode) {
    case 'e':
      /* Edit endpoint is different - PUT to /status/{status_id} */
      snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, STATUS_ENDPOINT"/%s",
              ref_toot_id);
      get_surl_for_endpoint(SURL_METHOD_PUT, endpoint_buf);
      /* Skip default endpoint */
      goto continue_building;
    case 'r':
      /* Reply - set in_reply_to_id field */
      sprintf(ref_status_field, "S|in_reply_to_id\n%s\n", ref_toot_id);
      break;
    case 'q':
      /* Quote - set quoted_status_id field */
      sprintf(ref_status_field, "S|quoted_status_id\n%s\n", ref_toot_id);
      break;
  }

  /* Compose, Reply, Quote all use the same endpoint */
  get_surl_for_endpoint(SURL_METHOD_POST, STATUS_ENDPOINT);

continue_building:
  /* Start of status */
  snprintf(body, 1536, "%s"
                       "%s"
                       "S|visibility\n%s\n"
                       "S|quote_approval_policy\n%s\n"
                       "B|sensitive\n%s\n"
                       "S|spoiler_text|%s|%s\n%s\n"
                       "S|status|%s|%s\n",
                        ref_status_field,
                        extra_buf,
                        compose_audience_str(compose_audience),
                        quote_policy_str(quote_policy),
                        sensitive_medias ? "true":"false",
                        TRANSLITCMD, translit_charset, cw,
                        TRANSLITCMD, translit_charset);

  /* Escape buffer */
  len = strlen(buffer);
  o = strlen(body);
  for (i = 0; i < len; i++) {
    if (buffer[i] != '\n') {
      body[o++] = buffer[i];
    } else {
      strcpy(body + o, "\\r\\n");
      o += 4;
    }
  }

  /* End of status */
  body[o] = '\n';
  len = o;

  surl_send_data_params((uint32)len, SURL_DATA_APPLICATION_JSON_HELP);
  surl_send_data_chunk(body, len);

  surl_read_response_header();

  if (surl_response_ok())
    return 0;

  *err = malloc(512);
  if (IS_NOT_NULL(*err)) {
    surl_get_json(*err, ".error", translit_charset, SURL_HTMLSTRIP_NONE, 511);
  }
  return -1;
}

char *compose_get_status_text(char *status_id) {
  char *content = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, STATUS_ENDPOINT"/%s/source", status_id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (surl_response_ok()) {
    int r;
    
    content = malloc0(512);

    r = surl_get_json(content, ".text", translit_charset, SURL_HTMLSTRIP_NONE, 511);

    if (r < 0) {
      free(content);
      return NULL;
    }
  }

  return content;
}

const char compose_poll_durations_hours[] = {1, 6, 12, 24, 48, 72, 168};
const char *compose_poll_durations_seconds[] = {"3600", "21600", "43200", "86400", "172800", "259200", "604800"};

static unsigned char ORG_FRENCH_CHARS[] = { ']','|','@','\\','}','{' };
static unsigned char SUB_FRENCH_CHARS[] = { '@','u','a','c' ,'e','e' };

void compose_sanitize_str(char *s) {
  char *r;
  unsigned char i;
  while ((r = strchr(s, '"')))
    *r = '\'';
  if(!strcmp(translit_charset, "ISO646-FR1")) {
    i = sizeof(ORG_FRENCH_CHARS);
    do {
      i--;
      while ((r = strchr(s, ORG_FRENCH_CHARS[i]))) {
        *r = SUB_FRENCH_CHARS[i];
      }
    } while (i);
  }
}
