/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "../csapp.h"

int main(void)
{
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    /* Extract the two arguments */
    if ((buf = getenv("QUERY_STRING")) != NULL)
    {
        p = strchr(buf, '&');
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p + 1);
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }
    // if ((buf = getenv("QUERY_STRING")) != NULL)
    // {
    //  sscanf(buf, "num1=%d&num2=%d", &n1, &n2);
    // }p

    /* Make the response body */
    sprintf(content, "<html><head><title>Adder Result</title></head><body>");

    sprintf(content + strlen(content), "<h1>Welcome to add.com</h1>");
    sprintf(content + strlen(content), "<p>QUERY_STRING=%s</p>", buf);
    sprintf(content + strlen(content), "<p>THE Internet addition portal.</p>");
    sprintf(content + strlen(content), "<p>The answer is: %d + %d = %d</p>", n1, n2, n1 + n2);
    sprintf(content + strlen(content), "<p>Thanks for visiting!</p>");
    sprintf(content + strlen(content), "</body></html>");

    /* Generate the HTTP response */
    printf("Content-type: text/html\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}
/* $end adder */