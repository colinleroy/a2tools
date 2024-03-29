#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

#ifdef __APPLE2ENH__
#define TRANSLITCMD "TRANSLIT"
#else
#define TRANSLITCMD "TRANSLITLC"
#endif

char *compose_audience_str(char compose_audience) {
  switch(compose_audience) {
    case COMPOSE_PUBLIC:   return "public";
    case COMPOSE_UNLISTED: return "unlisted";
    case COMPOSE_PRIVATE:  return "private";
    case COMPOSE_MENTION:
    default:               return "direct";
  }
}

#define FILE_ERROR "Can not open file.\r\n"
#define NET_ERROR "Network error.\r\n"
#define SEND_BUF_SIZE 1024

char *api_send_hgr_image(char *filename, char *description, char **err, char x, char y, char w) {
  FILE *fp;
  char buf[SEND_BUF_SIZE];
  int r;
  int to_send;
  char n_lines;
  char *media_id;

  *err = NULL;
  r = 0;
  to_send = HGR_LEN;

#ifdef __APPLE2__
  _filetype = PRODOS_T_BIN;
#endif

  fp = fopen(filename, "r");
  if (fp == NULL) {
    *err = FILE_ERROR;
    return NULL;
  }

  if (w > 0)
    progress_bar(x, y, w, 0, HGR_LEN);

  get_surl_for_endpoint(SURL_METHOD_POST_DATA, "/api/v2/media");

  /* Send num fields */
  surl_multipart_send_num_fields(1);
  
  /* Send file */
  surl_multipart_send_field_desc("file", (uint32)to_send, "image/hgr");
  while ((r = fread(buf, sizeof(char), sizeof(buf), fp)) > 0) {
send_again:
    surl_multipart_send_field_data(buf, r);
    to_send -= r;
    if (w > 0)
      progress_bar(-1, -1, w, HGR_LEN - to_send, HGR_LEN);
    if (to_send == 0) {
      break;
    }
  }
  /* Some hgr files don't include the last bytes, that
   * fall into an "HGR-memory-hole" and as such are not
   * indispensable. Fill up with zeroes up to to_send */
  if (to_send > 0) {
    r = min(to_send, SEND_BUF_SIZE);
    memset(buf, 0, r);
    goto send_again;
  }

  fclose(fp);

  surl_read_response_header();

  media_id = NULL;
  if (surl_response_ok()) {
    if (surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset, ".id") >= 0) {
      n_lines = strnsplit(gen_buf, '\n', lines, 1);
      if (n_lines == 1) {
        media_id = lines[0];
      } else {
        *err = NET_ERROR;
      }
    }
  } else {
    *err = NET_ERROR;
  }

  if (media_id != NULL) {
    /* Set description */
    int len;
    char *body = malloc0(1536);
    snprintf(body, 1536, "S|description|"TRANSLITCMD"|%s\n%s\n",
                          translit_charset,
                          description);

    len = strlen(body);

    snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, MEDIA_ENDPOINT"/%s", media_id);
    get_surl_for_endpoint(SURL_METHOD_PUT, endpoint_buf);

    surl_send_data_params((uint32)len, SURL_DATA_APPLICATION_JSON_HELP);
    surl_send_data(body, len);

    free(body);
    
    surl_read_response_header();
  }

  return media_id;
}

signed char api_send_toot(char mode, char *buffer, char *cw, char sensitive_medias,
                          char *ref_toot_id, char **media_ids, char n_medias,
                          poll *toot_poll, char compose_audience) {
  char *body;
  int i, o, len;
  char *extra_buf;

  body = malloc0(1536);

  if (n_medias > 0) {
    extra_buf = malloc0(768);
    snprintf(extra_buf, 768, "A|media_ids\n[\"%s\""
                                "%s%s%s"
                                "%s%s%s"
                                "%s%s%s"
                                "]\n",
                                media_ids[0],
                                n_medias > 1 ? ",\"":"",
                                n_medias > 1 ? media_ids[1]:"",
                                n_medias > 1 ? "\"":"",
                                n_medias > 2 ? ",\"":"",
                                n_medias > 2 ? media_ids[2]:"",
                                n_medias > 2 ? "\"":"",
                                n_medias > 3 ? ",\"":"",
                                n_medias > 3 ? media_ids[3]:"",
                                n_medias > 3 ? "\"":""
                              );
  } else if (toot_poll != NULL) {
    char duration, n_options;
    for (duration = 0; duration < NUM_POLL_DURATIONS; duration++) {
      if (compose_poll_durations_hours[duration] == toot_poll->expires_in_hours)
        break;
    }
    n_options = toot_poll->options_count;

    extra_buf = malloc0(768);
    snprintf(extra_buf, 768, "O|poll\n{"
                             "\"expires_in\": %s,"
                             "\"multiple\": %s,"
                             "\"options\":[\"%s\""
                                           "%s%s%s"
                                           "%s%s%s"
                                           "%s%s%s"
                                           "]"
                             "}\n",
                             compose_poll_durations_seconds[duration],
                             toot_poll->multiple ? "true":"false",
                             toot_poll->options[0].title,
                             n_options > 1 ? ",\"":"",
                             n_options > 1 ? toot_poll->options[1].title:"",
                             n_options > 1 ? "\"":"",
                             n_options > 2 ? ",\"":"",
                             n_options > 2 ? toot_poll->options[2].title:"",
                             n_options > 2 ? "\"":"",
                             n_options > 3 ? ",\"":"",
                             n_options > 3 ? toot_poll->options[3].title:"",
                             n_options > 3 ? "\"":"");
  } else {
    extra_buf = NULL;
  }

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, STATUS_ENDPOINT"%s%s",
           mode == 'e' ? "/" : "",
           mode == 'e' ? ref_toot_id : "");
  get_surl_for_endpoint(mode == 'e' ? SURL_METHOD_PUT : SURL_METHOD_POST, endpoint_buf);

  /* Start of status */
  snprintf(body, 1536, "%c|in_reply_to_id\n"
                       "%s\n"
                       "%s"
                       "S|visibility\n%s\n"
                       "B|sensitive\n%s\n"
                       "S|spoiler_text|"TRANSLITCMD"|%s\n%s\n"
                       "S|status|"TRANSLITCMD"|%s\n",
                        (ref_toot_id && mode == 'r') ? 'S' : 'B',
                        (ref_toot_id && mode == 'r') ? ref_toot_id : "null",
                        extra_buf ? extra_buf : "",
                        compose_audience_str(compose_audience),
                        sensitive_medias ? "true":"false",
                        translit_charset,
                        cw,
                        translit_charset);

  free(extra_buf);

  /* Escape buffer */
  len = strlen(buffer);
  o = strlen(body);
  for (i = 0; i < len; i++) {
    if (buffer[i] != '\n') {
      body[o++] = buffer[i];
    } else {
      strcpy(body + o, "\\r\\n");
      o+=4;
    }
  }
  /* End of status */
  body[o++] = '\n';
  len = o - 1;

  surl_send_data_params((uint32)len, SURL_DATA_APPLICATION_JSON_HELP);
  surl_send_data(body, len);

  free(body);

  surl_read_response_header();

  return surl_response_ok() ? 0 : -1;
}

char *compose_get_status_text(char *status_id) {
  char *content = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, STATUS_ENDPOINT"/%s/source", status_id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (surl_response_ok()) {
    int r;
    
    content = malloc0(NUM_CHARS);

    r = surl_get_json(content, NUM_CHARS, SURL_HTMLSTRIP_NONE, translit_charset, ".text");

    if (r < 0) {
      free(content);
      return NULL;
    }
  }

  return content;
}

const char compose_poll_durations_hours[] = {1, 6, 12, 24, 48, 72, 168};
const char *compose_poll_durations_seconds[] = {"3600", "21600", "43200", "86400", "172800", "259200", "604800"};

void compose_sanitize_str(char *s) {
  char *r;
  while ((r = strchr(s, '"')))
    *r = '\'';
  if(!strcmp(translit_charset, "ISO646-FR1")) {
    while ((r = strchr(s, '{')))
      *r = 'e';
    while ((r = strchr(s, '}')))
      *r = 'e';
    while ((r = strchr(s, '\\')))
      *r = 'c';
    while ((r = strchr(s, '@')))
      *r = 'a';
    while ((r = strchr(s, '|')))
      *r = 'u';
    while ((r = strchr(s, ']')))
      *r = '@';
  }
}
