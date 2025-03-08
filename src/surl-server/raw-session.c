#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>

#include "simple_serial.h"
#include "raw-session.h"
#include "char-convert.h"
#include "log.h"

static void send_buf(int sockfd, char *c, int nmemb) {
  fd_set fds;
  struct timeval timeout;
  int n;

  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);

  timeout.tv_sec  = 0;
  timeout.tv_usec = 3*1000;

  n = select(sockfd + 1, NULL, &fds, NULL, &timeout);

  if (n > 0 && FD_ISSET(sockfd, &fds)) {
    n = write(sockfd, c, nmemb);
  }
  if (n < 0) {
    LOG("RAW: Write error %s\n", strerror(errno));
  }
}

static int net_recv_char(int sockfd, unsigned char *c) {
  fd_set fds;
  struct timeval timeout;
  int n;

  *c = '\0';

  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);

  timeout.tv_sec  = 0;
  timeout.tv_usec = 3*1000;

  n = select(sockfd + 1, &fds, NULL, NULL, &timeout);

  if (n > 0 && FD_ISSET(sockfd, &fds)) {
    n = read(sockfd, c, 1);
    if (n == 0) {
      LOG("RAW: Read error %s\n", strerror(errno));
      return EOF;
    }
    return 0;
  } else if (n < 0) {
    LOG("RAW: Read error %s\n", strerror(errno));
    return EOF;
  }
  return 0;
}

static int ser_recv_char(unsigned char *c) {
  int i = simple_serial_getc_immediate();
  if (i == EOF)
    return EOF;
  
  *c = i;
  return 0;
}

static int socket_connect(int sock, char *remote_url) {
  int port, r;
  struct addrinfo hints, *cur, *result;
  char addr_str[255];
  struct sockaddr_in *ipv4 = NULL;
  struct sockaddr_in6 *ipv6 = NULL;

  memset (&hints, 0, sizeof (hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;
  
  if (!strchr(remote_url, ':')) {
    port = 23;
  } else {
    port = atoi(strchr(remote_url, ':') + 1);
    *strchr(remote_url, ':') = '\0';
  }

  r = getaddrinfo(remote_url, NULL, &hints, &result);
  if (r < 0) {
    simple_serial_printf("RAW: Cannot resolve: %s\n", strerror(errno));
    return -1;
  }
  cur = result;
  while (cur) {
    inet_ntop(cur->ai_family, cur->ai_addr->sa_data, addr_str, 255);
    if (cur->ai_family == AF_INET) {
      ipv4 = (struct sockaddr_in *) cur->ai_addr;
      break;
    }
    if (cur->ai_family == AF_INET6) {
      ipv6 = (struct sockaddr_in6 *) cur->ai_addr;
    }
    cur = cur->ai_next;
  }

  if (ipv4 != NULL) {
    ipv4->sin_port = htons(port);
    
    r = connect(sock, (struct sockaddr *)ipv4, sizeof(struct sockaddr_in));
  } else if (ipv6 != NULL) {
    ipv6->sin6_port = htons(port);
    
    r = connect(sock, (struct sockaddr *)ipv6, sizeof(struct sockaddr_in6));
  } else {
    r = -1;
  }

  freeaddrinfo(result);
  return r;
}

static void set_non_blocking(int sockfd) {
  /* coverity[check_return] */
  fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
}

extern int serial_delay;
#define RAW_BUFSIZE 1024
void surl_server_raw_session(char *remote_url) {
  unsigned char i, last_i, o;
  char *in_buf = malloc(RAW_BUFSIZE);
  char *out_buf = malloc(RAW_BUFSIZE);
  int n_in = 0, n_out = 0;
  int sockfd;
  time_t last_traffic;

  LOG("RAW: starting raw session.\n");

  serial_delay = 4800;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    simple_serial_printf("RAW: Could not create socket.\n%c", 0x04);
    goto cleanup;
  }

  if (socket_connect(sockfd, remote_url) < 0) {
    simple_serial_printf("RAW: Could not connect.\n%c", 0x04);
    goto cleanup;
  }
  
  set_non_blocking(sockfd);

  LOG("RAW: Connected to %s: %d\n", remote_url, sockfd);
  simple_serial_puts("Connected.\r\n");
  last_traffic = time(NULL);
  do {
    int read_res, has_escape_code = 0;
    int rem_utf8 = 0;
    last_i = '\0';

    n_in = 0;
maybe_finish_ctrl_ser:
    has_escape_code = 0;
    while (n_in < RAW_BUFSIZE - 1 && ser_recv_char(&i) != EOF) {
      last_i = i;

      if (i == ('d'|0x80)) {
        LOG("RAW: Client closed connection.\n");
        goto cleanup;
      }
      in_buf[n_in++] = i;
      if (i == '\33') {
        has_escape_code = 1;
      }
      last_traffic = time(NULL);
    }
    if (has_escape_code && n_in < RAW_BUFSIZE - 1) {
      usleep(10*1000);
      goto maybe_finish_ctrl_ser;
    }

    if (n_in > 0) {
      send_buf(sockfd, in_buf, n_in);
    }
    n_out = 0;

    rem_utf8 = 0;
maybe_finish_ctrl_net:
maybe_finish_utf8_net:
    has_escape_code = 0;
    while (n_out < RAW_BUFSIZE - 1 
           && (read_res = net_recv_char(sockfd, &o)) != EOF 
           && o != '\0') {
      out_buf[n_out++] = o;
      if (o == '\33') {
        has_escape_code = 1;
      } else if ((o & 0xC0) == 0xC0 && rem_utf8 == 0) {
        rem_utf8 = 1;
      } else if ((o & 0xE0) == 0xE0 && rem_utf8 == 0) {
        rem_utf8 = 2;
      }
      last_traffic = time(NULL);
    }
    if (has_escape_code && n_out < RAW_BUFSIZE - 1) {
      usleep(10*1000);
      goto maybe_finish_ctrl_net;
    } else if (rem_utf8 > 0 && n_out < RAW_BUFSIZE - 1) {
      usleep(10*1000);
      rem_utf8--;
      goto maybe_finish_utf8_net;
    }

    if (n_out > 0) {
      simple_serial_write(out_buf, n_out);
    }
    if (read_res == EOF) {
      simple_serial_printf("Remote host closed connection.\r\n%c", 0x04);
      goto cleanup;
    }
    if (time(NULL) - last_traffic > 600) {
      simple_serial_printf("Inactivity timeout.\r\n%c", 0x04);
      goto cleanup;
    }
  } while(last_i != ('d'|0x80));

cleanup:
  free(out_buf);
  free(in_buf);
  if (sockfd >= 0) {
    tcflush(sockfd, TCIOFLUSH);
    close(sockfd);
  }
  serial_delay = 0;
  simple_serial_flush();
}
