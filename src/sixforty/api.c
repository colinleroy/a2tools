#include <conio.h>
#include <errno.h>
#include <fcntl.h>
#include "api.h"
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

  cputs("Login (empty for read-only): ");
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
    cputs("Password: ");
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

static post_t *fetch_post(void) {
  char r;
  unsigned char n_lines;
  sprintf(small_buf, "/api/posts/?page=%d", current_posts_page);
  
  get_surl_for_endpoint(SURL_METHOD_GET, small_buf);
  if (surl_response_ok()) {
    sprintf(small_buf, ".results[%d]|(.id,.detail,.author.username,.modified,.description//\"\")", current_post_index);
    r = surl_get_json(gen_buf, small_buf,
                      translit_charset, SURL_HTMLSTRIP_NONE, BUF_SIZE);

    n_lines = strnsplit_in_place(gen_buf, '\n', lines, NUM_POST_FIELDS);
    if (r > 0 && n_lines == 5) {
      post_t *p;
      p = malloc0(sizeof(post_t));
      p->id          = strdup(lines[0]);
      p->image_url   = strdup(lines[1]);
      p->description = strdup(lines[4]);
      p->author      = strdup(lines[2]);
      p->date        = strdup(lines[3]);
      /* Fixup date for readability */
      p->date[10]    = ' ';
      p->date[19]    = '\0';
      return p;
    }
  }
  return NULL;
}

#pragma code-name(pop)
/* Get a post. Init with index_offset = 0, then
 * navigate with -1/+1 */
post_t *api_get_post(signed char index_offset) {
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

  return fetch_post();
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

char api_post_hgr_image(char *filename, char *description, char x, char y, char w) {
  int fd;
  int r;
  size_t file_size;
  size_t d_len, to_send;

  r = 0;

#ifdef __APPLE2__
  _filetype = PRODOS_T_BIN;
#endif

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    return EIO;
  }

  file_size = to_send = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  if (w > 0)
    progress_bar(x, y, w, 0, file_size);

  get_surl_for_endpoint(SURL_METHOD_POST_DATA, "/api/posts/");

  /* Send num fields */
  surl_multipart_send_num_fields(2);
  
  /* Send file */
  d_len = strlen(description);
  surl_multipart_send_field_desc("description", d_len, "text/plain");
  surl_multipart_send_field_data(description, d_len);

  surl_multipart_send_field_desc("image", (uint32)to_send, 
      monochrome ? "image/hgr" : "image/hgr-color");

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
