#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "compose.h"
#include "common.h"
#include "progress_bar.h"

#ifdef __APPLE2__
#include <apple2enh.h>
#endif

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

static char *compose_audience_str(char compose_audience) {
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

char *api_send_hgr_image(char *filename, char *description, char **err, char x, char y, char w) {
  FILE *fp;
  char buf[1024];
  int r;
  int to_send;
  surl_response *resp;
  char **lines;
  char n_lines;
  char *media_id;

  *err = NULL;
  r = 0;
  to_send = HGR_LEN;

#ifdef __CC65__
  _filetype = PRODOS_T_BIN;
#endif

  fp = fopen(filename, "r");
  if (fp == NULL) {
    *err = FILE_ERROR;
    return NULL;
  }

  if (w > 0)
    progress_bar(x, y, w, 0, HGR_LEN);
  resp = get_surl_for_endpoint(SURL_METHOD_POST_DATA, "/api/v2/media");

  if (resp == NULL) {
    fclose(fp);
    *err = NET_ERROR;
    return NULL;
  }

  /* Send num fields */
  surl_multipart_send_num_fields(1);
  
  /* Send file */
  surl_multipart_send_field_desc("file", to_send, "image/hgr");
  while ((r = fread(buf, sizeof(char), sizeof(buf), fp)) > 0) {
    surl_multipart_send_field_data(buf, r);
    to_send -= r;
    if (w > 0)
      progress_bar(x, y, w, HGR_LEN - to_send, HGR_LEN);
    if (to_send == 0) {
      break;
    }
  }
  while (to_send > 0) {
    /* Some hgr files don't include the last bytes, that
     * fall into an "HGR-memory-hole" and as such are not
     * indispensable */
    surl_multipart_send_field_data(0x00, 1);
    to_send--;
  }

  fclose(fp);

  surl_read_response_header(resp);

  media_id = NULL;
  if (surl_response_ok(resp)) {
    if (surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset, ".id") >= 0) {
      n_lines = strsplit(gen_buf, '\n', &lines);
      if (n_lines == 1) {
        media_id = lines[0];
      } else {
        *err = NET_ERROR;
      }
      free(lines);
    }
  } else {
    *err = NET_ERROR;
  }
  surl_response_free(resp);

  if (media_id != NULL) {
    /* Set description */
    int len;
    char *body = malloc(1536);
    snprintf(body, 1536, "S|description|TRANSLIT|%s\n%s\n",
                          translit_charset,
                          description);

    len = strlen(body);

    snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", "/api/v1/media", media_id);
    resp = get_surl_for_endpoint(SURL_METHOD_PUT, endpoint_buf);

    surl_send_data_params(len, SURL_DATA_APPLICATION_JSON_HELP);
    surl_send_data(body, len);

    free(body);

    surl_read_response_header(resp);
    surl_response_free(resp);
  }

  return media_id;
}

signed char api_send_toot(char mode, char *buffer, char *cw, char sensitive_medias,
                          char *ref_toot_id, char **media_ids, char n_medias,
                          char compose_audience) {
  surl_response *resp;
  char *body;
  int i, o, len;
  char *medias_buf;

  body = malloc(1536);
  if (body == NULL) {
    return -1;
  }

  if (n_medias > 0) {
    medias_buf = malloc(768);
    snprintf(medias_buf, 768, "A|media_ids\n[\"%s\""
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
  } else {
    medias_buf = NULL;
  }

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s%s%s", STATUS_ENDPOINT,
           mode == 'e' ? "/" : "",
           mode == 'e' ? ref_toot_id : "");
  resp = get_surl_for_endpoint(mode == 'e' ? SURL_METHOD_PUT : SURL_METHOD_POST, endpoint_buf);

  /* Start of status */
  snprintf(body, 1536, "%c|in_reply_to_id\n"
                       "%s\n"
                       "%s"
                       "S|visibility\n%s\n"
                       "B|sensitive\n%s\n"
                       "S|spoiler_text|TRANSLIT|%s\n%s\n"
                       "S|status|TRANSLIT|%s\n",
                        (ref_toot_id && mode == 'r') ? 'S' : 'B',
                        (ref_toot_id && mode == 'r') ? ref_toot_id : "null",
                        medias_buf ? medias_buf : "",
                        compose_audience_str(compose_audience),
                        sensitive_medias ? "true":"false",
                        translit_charset,
                        cw,
                        translit_charset);

  free(medias_buf);

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

  surl_send_data_params(len, SURL_DATA_APPLICATION_JSON_HELP);
  surl_send_data(body, len);

  free(body);

  surl_read_response_header(resp);

  if (surl_response_ok(resp)) {
    surl_response_free(resp);
    return 0;
  } else {
    surl_response_free(resp);
    return -1;
  }
}

char *compose_get_status_text(char *status_id) {
  surl_response *resp;
  char *content = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s/source", STATUS_ENDPOINT, status_id);
  resp = get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (resp && surl_response_ok(resp)) {
    int r;
    
    content = malloc(NUM_CHARS);
    if (content == NULL)
      goto err_out;

    r = surl_get_json(content, NUM_CHARS, SURL_HTMLSTRIP_NONE, translit_charset, ".text");

    if (r < 0) {
      free(content);
      content = NULL;
    }
  }

err_out:
  surl_response_free(resp);
  return content;
}
