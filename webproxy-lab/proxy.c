#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// /* You won't lose style points for including this long line in your code */
// static const char *user_agent_hdr =
//     "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
//     "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    handle_client(connfd);

    Close(connfd);
  }
}

void handle_client(int client_fd) {
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char host[MAXLINE], path[MAXLINE], port[MAXLINE];
  rio_t rio_client;

  Rio_readinitb(&rio_client, client_fd);
  Rio_readlineb(&rio_client, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);

  printf("Received request: %s", buf);

  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {
    // 단순화를 위해 GET, HEAD만 지원
    char *err_msg = "Proxy only supports GET and HEAD\r\n";
    Rio_writen(client_fd, err_msg, strlen(err_msg));
    return;
  }

  parse_url(uri, host, path, port);

  int server_fd = Open_clientfd(host, port);
  if (server_fd < 0) {
    char *err_msg = "Proxy couldn't connect to server\r\n";
    Rio_writen(client_fd, err_msg, strlen(err_msg));
    return;
  }

  forward_request(server_fd, host, port, buf); // 요청 전송
  forward_response(client_fd, server_fd);      // 응답 전송

  Close(server_fd);
}


void forward_request(int server_fd, char *host, char *port, char *request) {
  char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char new_request[MAXLINE];

  sscanf(request, "%s %s %s", method, uri, version);

  // path만 추출
  char path[MAXLINE];
  sscanf(uri, "http://%*[^/]%s", path);
  if (strlen(path) == 0) strcpy(path, "/");

  // 요청 라인 수정
  sprintf(new_request, "%s %s HTTP/1.0\r\n", method, path);
  sprintf(new_request + strlen(new_request), "Host: %s\r\n", host);
  sprintf(new_request + strlen(new_request), "Connection: close\r\n");
  sprintf(new_request + strlen(new_request), "Proxy-Connection: close\r\n");
  sprintf(new_request + strlen(new_request), "\r\n");

  Rio_writen(server_fd, new_request, strlen(new_request));
}


void forward_response(int client_fd, int server_fd) {
  char buf[MAXBUF];
  ssize_t n;

  rio_t rio_server;
  Rio_readinitb(&rio_server, server_fd);

  while ((n = Rio_readlineb(&rio_server, buf, MAXBUF)) > 0) {
    Rio_writen(client_fd, buf, n);
  }
}

void parse_url(char *url, char *host, char *path, char *port) {
  *path = '\0';
  *port = '\0';

  strcpy(port, "80");  // 기본 포트

  // 포트 포함된 경우
  if (sscanf(url, "http://%[^:/]:%[^/]%s", host, port, path) == 3) return;
  // 포트 없는 경우
  if (sscanf(url, "http://%[^/]%s", host, path) == 2) return;
  // path가 없는 경우
  if (sscanf(url, "http://%[^/]/", host) == 1) {
      strcpy(path, "/");
      return;
  }
}
