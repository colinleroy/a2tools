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

#include "simple_serial.h"
#include "raw-session.h"

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
    write(sockfd, c, nmemb);
  } else if (n < 0) {
    printf("Write error %s\n", strerror(errno));
  }
}

static char recv_char(int sockfd) {
  fd_set fds;
  struct timeval timeout;
  int n;

  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);

  timeout.tv_sec  = 0;
  timeout.tv_usec = 3*1000;

  n = select(sockfd + 1, &fds, NULL, NULL, &timeout);

  if (n > 0 && FD_ISSET(sockfd, &fds)) {
    char c;
    n = read(sockfd, &c, 1);
    if (n == 0) {
      printf("Read error %s\n", strerror(errno));
      return EOF;
    }
    return c;
  } else if (n < 0) {
    printf("Read error %s\n", strerror(errno));
    return EOF;
  }
  return '\0';
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
    simple_serial_printf("Cannot resolve: %s\n", strerror(errno));
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
  fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
}

void surl_server_raw_session(char *remote_url) {
  char i, last_i, o;
  char *in_buf = malloc(1024);
  char *out_buf = malloc(1024);
  int n_in = 0, n_out = 0;
  int sockfd;

  printf("starting raw session.\n");

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    simple_serial_printf("Could not create socket.\n%c", 0x04);
    goto cleanup;
  }

  if (socket_connect(sockfd, remote_url) < 0) {
    simple_serial_printf("Could not connect.\n%c", 0x04);
    goto cleanup;
  }
  
  set_non_blocking(sockfd);

  printf("Connected to %s: %d\n", remote_url, sockfd);
  do {
    last_i = '\0';

    n_in = 0;
    while ((i = simple_serial_getc_with_timeout()) != (char)EOF && n_in < 1023) {
      last_i = i;
      if (i == 0x04) {
        break;
      }
      in_buf[n_in++] = i;
    }
    if (n_in > 0)
      send_buf(sockfd, in_buf, n_in);
    
    n_out = 0;
    while ((o = recv_char(sockfd)) != (char)EOF && o != '\0') {
      out_buf[n_out++] = o;
    }
    if (n_out > 0) {
      simple_serial_write(out_buf, 1, n_out);
    }
    if (o == (char)EOF) {
      simple_serial_printf("Remote host closed connection.\n%c", 0x04);
      goto cleanup;
    }
  } while(last_i != 0x04);

cleanup:
  close(sockfd);
}
