#include <conio.h>
#include <errno.h>
#include <fcntl.h>
#include "api.h"
#include "dputs.h"
#include "dget_text.h"
#include "malloc0.h"
#include "progress_bar.h"
#include "strsplit.h"
#include "surl.h"

#pragma code-name(push, "LC")

extern char monochrome;

char *lines[MAX_LINES_NUM];

char small_buf[64];
char login[64];
char oauth_token[64];

#define ROOT_URL "https://640by480.com"

static const surl_response *get_surl_for_endpoint(char method, char *endpoint) {
  static char *hdrs[1] = {NULL};
  char h_num = 0;

  if (IS_NULL(hdrs[0])) {
    hdrs[0] = malloc0(128);
  }
  if (oauth_token[0]) {
    strcpy(hdrs[0], "Authorization: Token ");
    strcat(hdrs[0], oauth_token);
    h_num = 1;
  }

  strcpy(gen_buf, ROOT_URL);
  strcat(gen_buf, endpoint);
  return surl_start_request(hdrs, h_num, gen_buf, method);
}

char api_login(char *saved_creds) {
  unsigned int post_len;
  unsigned char r;
  char *d;

  oauth_token[0] = '\0';

  if (saved_creds) {
    strcpy(login, saved_creds);
    if (d = strchr(login, '\n')) {
      *d = '\0';
      d++;
      /* Temp copy for change comparison */
      strcpy(small_buf, login);

      /* And copy the token */
      strcpy(oauth_token, d);
      if (d = strchr(oauth_token, '\n')) {
        *d = '\0';
      }
    }
    free(saved_creds);
  }

  dputs("Login (empty for read-only): ");
  dget_text_single(login, sizeof(login) - 1, NULL);
  if (login[0]) {
    if (!strcmp(small_buf, login) && oauth_token[0]) {
      /* Same login. check token validity. */
      get_surl_for_endpoint(SURL_METHOD_GET, "/api/");
      if (surl_response_ok()) {
        return 0;
      } else {
        oauth_token[0] = '\0';
        small_buf[0] = '\0';
      }
    }
    dputs("Password: ");
    dgets_echo_on = 0;
    dget_text_single(small_buf, sizeof(small_buf) - 1, NULL);
    dgets_echo_on = 1;

    get_surl_for_endpoint(SURL_METHOD_POST, "/api/auth/login/");
    snprintf(gen_buf, BUF_SIZE, "S|username\n%s\n"
                                "S|password\n%s\n",
                                login, small_buf);
    post_len = strlen(gen_buf);

    surl_send_data_params((uint32)post_len, SURL_DATA_APPLICATION_JSON_HELP);
    surl_send_data_chunk(gen_buf, post_len);

    surl_read_response_header();

    if (surl_response_ok()) {
      r = surl_get_json(gen_buf, ".token",
                    translit_charset, SURL_HTMLSTRIP_NONE, BUF_SIZE);
      if (r > 0) {
        strcpy(oauth_token, gen_buf);
        return 0;
      }
    }
    return -1;
  }
  return 0;
}

char *api_get_creds(void) {
  if (login[0] && oauth_token[0]) {
    char *creds = malloc(strlen(login) + strlen(oauth_token) + 2);
    sprintf(creds, "%s\n%s", login, oauth_token);
    return creds;
  }
  return NULL;
}

#define PAGE_SIZE 20
static signed int current_posts_page = 0;
static signed char current_post_index = PAGE_SIZE-1; /* Init so first Next gets post 0/page 1 */

static char post_json[] = ".results[%d]|"
                          "(.id,.detail,.author.username,.modified,.comment_count,.description//\"\")";

post_t *api_get_post_by_id(unsigned long post_id) {
  char r;
  unsigned char n_lines;
  if (post_id == 0) {
    sprintf(small_buf, "/api/posts/?page=%d", current_posts_page);
    get_surl_for_endpoint(SURL_METHOD_GET, small_buf);
    sprintf(small_buf, post_json, current_post_index);
  } else {
    sprintf(small_buf, "/api/posts/%llu/", post_id);
    get_surl_for_endpoint(SURL_METHOD_GET, small_buf);
    strcpy(small_buf, post_json+13 /* strlen("results[%d]|") */);
  }

  if (surl_response_ok()) {
    r = surl_get_json(gen_buf, small_buf,
                      translit_charset, SURL_HTMLSTRIP_NONE, BUF_SIZE);

    n_lines = strnsplit_in_place(gen_buf, '\n', lines, NUM_POST_FIELDS);
    if (r > 0 && n_lines == NUM_POST_FIELDS) {
      post_t *p;
      char *ret;
      p = malloc0(sizeof(post_t));
      p->id             = strdup(lines[0]);
      p->image_url      = strdup(lines[1]);
      p->author         = strdup(lines[2]);
      p->date           = strdup(lines[3]);
      p->comment_count  = atoi(lines[4]);
      p->description    = strdup(lines[5]);
      /* Fixup date for readability */
      p->date[10]       = ' ';
      p->date[19]       = '\0';
      /* Cut description for layout */
      if ((ret = strchr(p->description, '\n'))) {
        *ret = '\0';
      }
      return p;
    }
  }
  return NULL;
}

#pragma code-name(pop)
#pragma code-name(push, "LOWCODE")

char api_patch_post(post_t *post, char type, char *field, char *value) {
  size_t len;

  if (IS_NULL(post)) {
    return -1;
  }
  sprintf(small_buf, "/api/posts/%s/", post->id);
  get_surl_for_endpoint(SURL_METHOD_PATCH, small_buf);

  snprintf(gen_buf, BUF_SIZE, "%c|%s\n%s\n",
           type, field, value);

  len = strlen(gen_buf);
  surl_send_data_params((uint32)len, SURL_DATA_APPLICATION_JSON_HELP);
  surl_send_data_chunk(gen_buf, len);

  surl_read_response_header();
  return surl_response_ok() ? 0 : -1;
}

comment_t *api_get_comment(post_t *post, unsigned char index) {
  char r;
  unsigned char n_lines;

  if (IS_NULL(post)) {
    return NULL;
  }

  sprintf(small_buf, "/api/posts/%s/", post->id);

  get_surl_for_endpoint(SURL_METHOD_GET, small_buf);
  if (surl_response_ok()) {
    sprintf(small_buf, ".comments[%d]|"
                       "(.author.username,.modified,.text)",
                       index);
    r = surl_get_json(gen_buf, small_buf,
                      translit_charset, SURL_HTMLSTRIP_NONE, BUF_SIZE);

    n_lines = strnsplit_in_place(gen_buf, '\n', lines, NUM_COMMENT_FIELDS);
    if (r > 0 && n_lines == NUM_COMMENT_FIELDS) {
      comment_t *c;
      c = malloc0(sizeof(comment_t));
      c->author         = strdup(lines[0]);
      c->date           = strdup(lines[1]);
      c->text           = strdup(lines[2]);
      /* Fixup date for readability */
      c->date[10]       = ' ';
      c->date[19]       = '\0';
      return c;
    }
  }
  return NULL;
}

char api_post_comment(post_t *post, char *comment) {
  size_t len, o, i;

  if (IS_NULL(post)) {
    return -1;
  }

  sprintf(small_buf, "/api/posts/%s/add_comment/", post->id);

  get_surl_for_endpoint(SURL_METHOD_POST, small_buf);

  strcpy(gen_buf, "S|text\n");

  /* Escape buffer */
  len = strlen(comment);
  o = strlen(gen_buf);
  for (i = 0; i < len; i++) {
    if (comment[i] != '\n') {
      gen_buf[o++] = comment[i];
    } else {
      strcpy(gen_buf + o, "\\r\\n");
      o += 4;
    }
  }

  /* End of comment */
  gen_buf[o] = '\n';
  len = o;

  surl_send_data_params((uint32)len, SURL_DATA_APPLICATION_JSON_HELP);
  surl_send_data_chunk(gen_buf, len);

  surl_read_response_header();

  return surl_response_ok() ? 0 : -1;
}

/* Get a post. Init with index_offset = 0, then
 * navigate with -1/+1 */
post_t *api_get_next_post(signed char index_offset) {
  current_post_index += index_offset;

  if (current_post_index < 0) {
    current_posts_page--;
    current_post_index = PAGE_SIZE-1;
  }
  if (current_post_index == PAGE_SIZE) {
    current_posts_page++;
    current_post_index = 0;
  }

  if (current_posts_page <= 0) {
    current_posts_page = 1;
    current_post_index = 0;
  }

  return api_get_post_by_id(0);
}

/* Delete a post. */
char api_delete_post(post_t *post) {
  if (!post) {
    return -1;
  }

  sprintf(small_buf, "/api/posts/%s/", post->id);

  get_surl_for_endpoint(SURL_METHOD_DELETE, small_buf);
  return surl_response_ok() ? 0 : -1;
}

#pragma code-name(pop)

char jpeg_magic[] = { 0xFF, 0xD8, 0xFF};
char qtk_magic[]  = "qkt";
char mime_type[]  = "application/octet-stream";

char api_post_image(char *filename, char *description, char x, char y, char w) {
  int fd;
  int r;
  uint32 file_size, d_len, to_send;

  r = 0;

#ifdef __APPLE2__
  _filetype = PRODOS_T_BIN;
#endif

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    return EIO;
  }

  get_surl_for_endpoint(SURL_METHOD_POST_DATA, "/api/posts/");

  read(fd, gen_buf, 0x79);
  file_size = to_send = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  if (w > 0)
    progress_bar(x, y, w, 0, file_size);

  /* Send num fields */
  surl_multipart_send_num_fields(2);

  /* Send file */
  d_len = strlen(description);
  surl_multipart_send_field_desc("description", d_len, "text/plain");
  surl_multipart_send_field_data(description, d_len);

  if (!memcmp(gen_buf, jpeg_magic, 3)) {
    strcpy(mime_type, "image/jpg");
  } else if (!memcmp(gen_buf, qtk_magic, 3)) {
    strcpy(mime_type, "image/qtk");
  } else {
    strcpy(mime_type, (gen_buf[0x78] % 2) == 0 ? "image/hgr" : "image/hgr-color");
  }

  surl_multipart_send_field_desc("image", (uint32)to_send, mime_type);

  while ((r = read(fd, gen_buf, BUF_SIZE)) > 0) {
    surl_multipart_send_field_data(gen_buf, r);
    to_send -= r;
    if (w > 0) {
      progress_bar(-1, -1, w, file_size - to_send, file_size);
    }
  }

  close(fd);

  surl_read_response_header();

  if (surl_response_ok()) {
    current_posts_page = 1;
    current_post_index = 0;
    return 0;
  }

  return ENOENT;
}
