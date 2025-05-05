#include "csapp.h"
//입력 -> 서버 전송 -> 응답 받음 -> 출력
//stdin에서 한 줄 입력 -> 서버에 보냄 -> 응답 받아서 stdout에 출력
int main(int argc, char **argv) //argc(argument count): 인자의 개수, argv(argument vector): 인자 값 배열
// 예시 - ./client localhost 12345 -> argc: 3, argv[0]: ./client(실행파일이름), argv[1]: localhost(서버주소), argv[2]: 12345(서버포트)
{
    int clientfd; //클라이언트 소켓 디스크립터, Open_clientfd 함수로 초기화됨
    char *host, *port, buf[MAXLINE]; //MAXLINE 크기의 버퍼 선언
    rio_t rio; //rio_t: 읽기 전용 구조체, 쓰기는 버퍼 없이 직접 시스템 호출을 반복해서 처리
               //내부 버퍼(rio_buf)을 사용해 read를 효율적으로 반복처리
/*쓰기: ssize_t rio_writen(int fd, void *usrbuf, size_t n)
버퍼가 따로 필요 없음, 시스템 콜을 반복해서 끝까지 씀*/ 

    if (argc != 3){ //인자 개수가 3이 아닐 때 오류처리
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1]; //접속할 서버의 호스트 이름 또는 IP주소
    port = argv[2]; //접속할 서버의 포트 번호

    /*Open_clientfd: TCP 클라이언트 소켓을 여는 에러 처리용 래퍼
    open_clientfd: 호스트 이름과 포트 번호를 기반으로 TCP 클라이언트 소켓을 만들고 서버에 연결
    getaddrinfo를 통해 hostname+portnum -> 주소목록(listp), listp를 돌면서 socket생성 후 connect시도
    연결된 소켓 디스크립터 반환*/
    clientfd = Open_clientfd(host, port); //host, port로 TCP 서버에 접속 시도. 성공하면 연결된 소켓 디스크립터 번호를 clientfd에 저장
    Rio_readinitb(&rio, clientfd); //clientfd에 저장한 소켓을 통해 robust I/O 읽기 준비

    //사용자로부터 한줄 입력(stdin)을 받음
    while (Fgets(buf, MAXLINE, stdin) != NULL) { //fgets(): 파일/입력 스트림에서 한줄 읽기 Fgets(): 에러처리 포함
        /*입력받은 문자열을 서버에 전송, strlen으로 현재 입력 길이만큼 전송,
        저장한 바이트를 다 쓸 때까지 write 반복 호출*/
        Rio_writen(clientfd, buf, strlen(buf));
        /*서버로부터 응답을 한 줄 읽기(버퍼링된 robust 방식
        MAXLINE만큼 도달할 때까지 내부에서 읽기 수행*/
        Rio_readlineb(&rio, buf, MAXLINE);
        /*받은 응답을 화면(stdout)에 출력*/
        Fputs(buf, stdout);
    }
    Close(clientfd);//서버와의 연결을 닫음
    exit(0);
}