#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "compose.h"
#include "common.h"
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

char *api_send_hgr_image(char *filename, char *description, char **err) {
  FILE *fp;
  char buf[1024];
  int r = 0;
  int to_send = HGR_LEN;
  static char *hdrs[2] = {NULL, NULL};
  surl_response *resp;
  char **lines;
  char n_lines;
  char *media_id;

  *err = malloc(64);
  *err[0] = '\0';

#ifdef __CC65__
  _filetype = PRODOS_T_TXT;
#endif

  fp = fopen(filename, "r");
  if (fp == NULL) {
    snprintf(*err, 64, "Can not open file %s\r\n", filename);
    return NULL;
  }

  if (hdrs[0] == NULL) {
    hdrs[0] = malloc(BUF_SIZE);
    snprintf(hdrs[0], BUF_SIZE, "Authorization: Bearer %s", oauth_token);
  }
  if (hdrs[1] == NULL) {
    hdrs[1] = "Content-Type: multipart/form-data";
  }

  snprintf(gen_buf, BUF_SIZE, "%s%s", instance_url, "/api/v2/media");
  resp = surl_start_request(SURL_METHOD_POST, gen_buf, hdrs, 2);

  if (resp == NULL) {
    return NULL;
  }

  /* Send num fields */
  surl_multipart_send_num_fields(1);
  
  /* Send file */
  surl_multipart_send_field_desc("file", to_send, "image/hgr");
  while ((r = fread(buf, sizeof(char), sizeof(buf), fp)) > 0) {
    surl_multipart_send_field_data(buf, r);
    to_send -= r;
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
    r = surl_get_json(gen_buf, BUF_SIZE, 0, translit_charset, ".id");
    n_lines = strsplit_in_place(gen_buf, '\n', &lines);
    if (r >= 0 && n_lines == 1) {
      media_id = strdup(lines[0]);
    } else {
      snprintf(*err, 64, "Invalid JSON response\r\n");
    }
    free(lines);
  } else {
    snprintf(*err, 64, "Invalid HTTP response %d\r\n", resp->code);
  }
  surl_response_free(resp);

  if (media_id != NULL) {
    /* Set description */
    int len, o, i;
    char *body = malloc(1536);
    snprintf(body, 1536, "description|TRANSLIT|%s\n",
                          translit_charset);

    /* Escape buffer */
    len = strlen(description);
    o = strlen(body);
    for (i = 0; i < len; i++) {
      if (description[i] != '\n') {
        body[o++] = description[i];
      } else {
        body[o++] = '\\';
        body[o++] = 'r';
        body[o++] = '\\';
        body[o++] = 'n';
      }
    }
    /* End of description */
    body[o++] = '\n';
    len = o - 1;

    snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", "/api/v1/media", media_id);
    resp = get_surl_for_endpoint(SURL_METHOD_PUT, endpoint_buf);

    surl_send_data_params(len, SURL_DATA_APPLICATION_JSON_HELP);
    surl_send_data(body, len);

    free(body);

    surl_read_response_header(resp);

    if (!surl_response_ok(resp)) {
      snprintf(*err, 64, "Description: invalid HTTP response %d\r\n", resp->code);
    }
    surl_response_free(resp);
  }

  if (*err[0] == '\0') {
    free(*err);
    *err = NULL;
  }
  return media_id;
}

char api_send_toot(char *buffer, char *in_reply_to_id, char **media_ids, char n_medias, char compose_audience) {
  surl_response *resp;
  char *body;
  char r;
  int i, o, len;
  char *in_reply_to_buf, *medias_buf;

  r = -1;
  body = malloc(1536);
  if (body == NULL) {
    return -1;
  }

  if (in_reply_to_id) {
    in_reply_to_buf = malloc(48);
    snprintf(in_reply_to_buf, 48, "in_reply_to_id\n%s\n", in_reply_to_id);
  } else {
    in_reply_to_buf = NULL;
  }
  
  if (n_medias > 0) {
    medias_buf = malloc(768);
    snprintf(medias_buf, 768, "media_ids\n[\"%s\""
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

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s", STATUS_ENDPOINT);
  resp = get_surl_for_endpoint(SURL_METHOD_POST, endpoint_buf);

  /* Start of status */
  snprintf(body, 1536, "%s"
                       "%s"
                       "visibility\n%s\n"
                       "status|TRANSLIT|%s\n",
                        in_reply_to_buf ? in_reply_to_buf : "",
                        medias_buf ? medias_buf : "",
                        compose_audience_str(compose_audience),
                        translit_charset);
  free(in_reply_to_buf);
  free(medias_buf);

  /* Escape buffer */
  len = strlen(buffer);
  o = strlen(body);
  for (i = 0; i < len; i++) {
    if (buffer[i] != '\n') {
      body[o++] = buffer[i];
    } else {
      body[o++] = '\\';
      body[o++] = 'r';
      body[o++] = '\\';
      body[o++] = 'n';
    }
  }
  /* End of status */
  body[o++] = '\n';
  len = o - 1;

  surl_send_data_params(len, SURL_DATA_APPLICATION_JSON_HELP);
  surl_send_data(body, len);

  free(body);

  surl_read_response_header(resp);

  if (!surl_response_ok(resp))
    goto err_out;

  r = 0;
err_out:
  surl_response_free(resp);
  return r;
}
