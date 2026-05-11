#include <conio.h>
#include "dget_text.h"
#include "api.h"
#include "malloc0.h"
#include "strsplit.h"
#include "surl.h"

#pragma code-name(push, "API640")
#pragma rodata-name(push, "API640")
#pragma data-name(push, "API640") 

/* Forward declarations */
static unsigned char do_login_640(void);
static post_t *get_post_640(signed char index_offset);

/* HEADER for the segment ====================================== */
unsigned int features = 0b1111111111111111;
/* Implementation callbacks ==================================== */
void *sixforty_callbacks[] = {
  /* LOGIN */           do_login_640,
  /* GET_POST */        get_post_640,
};

/* Internal code =============================================== */

char small_buf[64];
char login[64];
char oauth_token[64];

#define ROOT_URL "https://640by480.com"

static const surl_response *get_surl_for_endpoint(char method, char *endpoint) {
  static char *hdrs[1] = {NULL};
  static char h_num = 0;

  if (IS_NULL(hdrs[1]) && oauth_token[0]) {
    hdrs[0] = malloc0(128);
    strcpy(hdrs[0], "Authorization: Token ");
    strcat(hdrs[0], oauth_token);
    h_num = 1;
  }

  strcpy(gen_buf, ROOT_URL);
  strcat(gen_buf, endpoint);
  return surl_start_request(hdrs, h_num, gen_buf, method);
}

static unsigned char do_login_640(void) {
  unsigned int post_len;
  oauth_token[0] = '\0';
  cputs("Login (empty for read-only): ");
  dget_text_single(login, sizeof(login) - 1, NULL);
  if (login[0]) {
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

    return surl_response_ok();
  }
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

    n_lines = strnsplit_in_place(gen_buf, '\n', lines, PAGE_SIZE*NUM_POST_FIELDS);
    if (r > 0 && n_lines == 5) {
      post_t *p;
      p = malloc0(sizeof(post_t));
      p->id          = strdup(lines[0]);
      p->image_url   = strdup(lines[1]);
      p->description = strdup(lines[4]);
      p->author      = strdup(lines[2]);
      p->date        = strdup(lines[3]);
      return p;
    }
  }
  return NULL;
}

/* Get a post. Init with index_offset = 0, then
 * navigate with -1/+1 */
static post_t *get_post_640(signed char index_offset) {
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
