/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);


int main(int argc, char **argv)
{
  // 리스닝 소켓과 연결 소켓, 주소 정보 및 출력용 문자열 버퍼 선언
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  // 포트 인자가 없을 경우 종료
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 주어진 포트에 대해 리스닝 소켓 생성
  listenfd = Open_listenfd(argv[1]);

  while (1) {
    // 클라이언트 요청 수락 대기
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 수락

    // 클라이언트 호스트/포트 정보 출력
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // 클라이언트 요청 처리
    Close(connfd);  // 연결 종료
  }
}


void doit(int fd)
{
  // 정적/동적 콘텐츠 구분, 요청 파싱용 변수 선언
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  int is_head = 0; // HEAD 요청 여부 확인용

  // 요청 라인 읽기
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n%s", buf);

  // 요청 라인 파싱: 메서드, URI, 버전
  sscanf(buf, "%s %s %s", method, uri, version);

  // GET과 HEAD 외의 메서드는 501 오류 반환
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD") != 0) {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

   // HEAD 요청이면 본문 생략
  if (strcasecmp(method, "HEAD") == 0) is_head = 1;

  read_requesthdrs(&rio); // 헤더 읽고 무시

  is_static = parse_uri(uri, filename, cgiargs);  // URI 해석 및 파일 경로/인자 추출

  // 파일 존재 여부 확인
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  // 정적 콘텐츠 처리
  if (is_static) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }

  // 동적 콘텐츠 처리
  else {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}


void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

   // 요청 헤더 한 줄씩 읽어서 출력 (종료 줄: \r\n)
  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}


int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // 정적 콘텐츠인 경우
  if (!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");               // 인자 없음
    strcpy(filename, ".");            // 현재 디렉토리 기준
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')  // 디렉토리일 경우 home.html 추가
      strcat(filename, "home.html");
    return 1;
  }

  // 동적 콘텐츠인 경우
  else {
    ptr = index(uri, '?');
    if (ptr)  // 인자 존재 시 분리
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    } 
    else strcpy(cgiargs, "");

    strcpy(filename, "./");
    strcat(filename, uri);
    return 0;
  }
}



void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype); // 파일 확장자에 따라 MIME 타입 결정

  // 응답 헤더 작성
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf)); // 헤더 전송
  printf("Response headers:\n");
  printf("%s", buf);

  // 파일 내용 전송(response body)
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);    // 본문 전송
  Munmap(srcp, filesize);            // 메모리 해제
}


void get_filetype(char *filename, char *filetype)
{
  // 파일 확장자에 따라 MIME 타입 결정
  if (strstr(filename, ".html")) strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif")) strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg")) strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".png")) strcpy(filetype, "image/png");
  else if (strstr(filename, ".mp4")) strcpy(filetype, "video/mp4");
  else if (strstr(filename, ".css")) strcpy(filetype, "text/css");
  else if (strstr(filename, ".js")) strcpy(filetype, "application/javascript");
  else strcpy(filetype, "text/plain");
}


void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of three-part HTTP response */
  // 응답 헤더 전송
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* The CGI program processes the request and returns the result */
  if (Fork() == 0) // 자식 프로세스 생성
  {
    setenv("QUERY_STRING", cgiargs, 1);    // CGI 인자를 환경변수로 설정
    Dup2(fd, STDOUT_FILENO);               // 표준 출력 리디렉션 → 클라이언트로
    Execve(filename, emptylist, environ);  // CGI 프로그램 실행
  }
  Wait(NULL);  // 부모는 자식 종료 대기
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // 에러 페이지 HTML 생성(response body)
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // 에러 응답 헤더와 본문 전송
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
