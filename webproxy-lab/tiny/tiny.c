/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd); //하나의 http트랜잭션을 처리
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) // 프로그램 이름, 포트번호를 인자로 받음 ex) ./tiny 8000
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  //인자(입력)이 옳지 않은 경우
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); //listen 소켓 생성
  //listen 소켓이 정상적으로 만들어졌을 때 클라이언트 요청에 대해 대기
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // 클라이언트와 연결이 성립된 소켓 디스크립터
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  //트랜잭션 수행
    Close(connfd); //연결 끊기
  }
}

int parse_uri(char *uri, char *filename, char *cgiargs) //cgiargs -> CGI 파라미터 문자열을 분리해서 저장할 버퍼
{
  char *ptr;

  if(!strstr(uri, "cgi-bin")) { //url에 cgi-bin이 없을 때
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri); //filename을 상대경로로 변환 -> . + /index.html
    if (uri[strlen(uri)-1] == '/') //URI가 /로 끝나면 기본 파일 이름을 붙임
      strcat(filename, "home.html");
    return 1; //정적 요청일 때 1반환
  }
  else { //동적일 때
    ptr = index(uri, '?'); //? 기준으로 인자와 경로를 분리, ptr은 ?를 가리키는 포인터
    if (ptr){
      strcpy(cgiargs, ptr+1); //?이후를 cgiargs에 저장
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, ""); //없으면 CGI인자는 없는거
    //URI를 로컬경로로 변환
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0; //동적 요청일 때 0반환
  }
}

//파일 확장자를 보고 HTTP 헤더의 Content-Type을 결정
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");

}
//정적 콘텐츠를 클라이언트에게 전송
void serve_static(int fd, char *filename, int filesize)//클라이언트와의 연결소켓 파일 디스크립터
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));//완성된 헤더 문자열을 클라이언트 소켓으로 전송
  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0); //파일 열기(읽기 전용)
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); //파일 내용을 메모리 주소 공간에 매핑
  Close(srcfd); //파일 드스크립터를 닫음
  Rio_writen(fd, srcp, filesize); //파일 내용을 그대로 클라이언트에 전송
  Munmap(srcp, filesize); //메모리 매핑 해제
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork()==0){
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);

  }
  Wait(NULL);
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); //요청 헤더를 한줄씩 읽음
  while(strcmp(buf, "\r\n")) { //빈 줄을 만나면 종료
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf); //요청 헤더 출력(디버깅용)
  }
  return;
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE); //buf에 HTTP의 요청 메세지 중 첫 줄을 저장
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); //요청라인을 파싱, buf에 저장된 요청을 method, URI, VERSION 순으로 파싱
  if (strcasecmp(method, "GET")) { 
    if (strcasecmp(method, "HEAD")){
      clienterror(fd, method, "501", "Not implemented", 
        "Tiny does not implement this method");
    return;
    }
    clienterror(fd, method, "501", "Not implemented", 
        "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio); //클라이언트가 GET요청을 보낼 때, 요청 다음 라인의 여러개의 HTTP 헤더줄들을 제거(이건 다음 명령에서)

  /*요청이 정적인지 동적인지를 확인*/
  is_static = parse_uri(uri, filename, cgiargs);
  /*ex) /index.html → is_static = 1, filename = ./index.html
        /cgi-bin/add?x=1&y=2 → is_static = 0, filename = ./cgi-bin/add, cgiargs = x=1&y=2
  */

  //파일 존재 여부 확인
  if(stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found",
        "Tiny couldn't find this file");
    return;
  }

  //정적 요청일 때
  if (is_static) {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //일반파일인가? -> S_ISREG, 읽을 수 있는 파일인가? -> S_IRUSR(읽기권한)
      clienterror(fd, filename, "403", "Forbidden",
          "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  //동적 요청일 때
  else {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
          "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum, 
                  char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}