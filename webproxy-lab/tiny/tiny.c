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
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);


int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}


void doit(int fd)
{
  int is_static; /* static or dynamic? */
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n%s", buf);
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  if (is_static) /* Serve static content */
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size); // line:netp:tiny:static
  }
  else /* Serve dynamic content */
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs); // line:netp:tiny:dynamic
  }
}


void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  /* Read and discard headers */
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

  if (!strstr(uri, "cgi-bin")) /* Static content */
  {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/') strcat(filename, "home.html");
    return 1;
  }
  else /* Dynamic content */
  {
    ptr = index(uri, '?');
    if (ptr) /* Dynamic content */
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
    {
      strcpy(cgiargs, "");
    }
    strcpy(filename, "./");
    strcat(filename, uri);
    return 0;
  }
}


void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // line:netp:tiny:static1
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf)); // line:netp:tiny:static2
  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); // line:netp:tiny:static3
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // line:netp:tiny:static4
  Close(srcfd); // line:netp:tiny:static5
  Rio_writen(fd, srcp, filesize); // line:netp:tiny:static6
  Munmap(srcp, filesize); // line:netp:tiny:static7
}


void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html")) strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif")) strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg")) strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".png")) strcpy(filetype, "image/png");
  // else if (strstr(filename, ".css")) strcpy(filetype, "text/css");
  // else if (strstr(filename, ".js")) strcpy(filetype, "application/javascript");
  else strcpy(filetype, "text/plain");
}


void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of three-part HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // line:netp:tiny:dynamic1
  Rio_writen(fd, buf, strlen(buf)); // line:netp:tiny:dynamic2
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* The CGI program processes the request and returns the result */
  if (Fork() == 0) /* Child process */
  {
    setenv("QUERY_STRING", cgiargs, 1); // Set environment variable
    Dup2(fd, STDOUT_FILENO); // Redirect stdout to client
    Execve(filename, emptylist, environ); // Execute CGI program
  }
  Wait(NULL); // Parent waits for and reaps child
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response header and the response body */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // line:netp:tiny:error1
  Rio_writen(fd, buf, strlen(buf)); // line:netp:tiny:error2
  sprintf(buf, "Content-type: text/html\r\n"); // line:netp:tiny:error3
  Rio_writen(fd, buf, strlen(buf)); // line:netp:tiny:error4
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // line:netp:tiny:error5
  Rio_writen(fd, buf, strlen(buf)); // line:netp:tiny:error6
  Rio_writen(fd, body, strlen(body)); // line:netp:tiny:error7
}