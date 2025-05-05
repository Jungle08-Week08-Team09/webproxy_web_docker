#include "csapp.h"
/*Robust I/O: 신뢰성있는 입출력 래퍼 함수 모음
특징 - 버퍼링(데이터를 바로 처리하지 않고, 버퍼에 모아놨다가 한번에 처리), 요청한 바이트만큼 다 읽고/쓸 때까지 반복 시도
인터럽트 자동 처리, 줄 단위/블록 단위 지원(rio_readnb())
*/

void echo(int connfd) //connfd : 클라이언트와 연결된 소켓의 파일 디스크립터, accept 함수의 반환값
{
    size_t n; //읽어들인 바이트 수 저장
    char buf[MAXLINE]; //읽은 데이터를 임시로 저장할 버퍼
    rio_t rio; //rio_readinitb와 함께 쓰이는 구조체, 버퍼링을 제공

    Rio_readinitb(&rio, connfd); //connfd를 robust I/O 버퍼 rio에 연결, Rio_readlineb로 줄 단위 안전한 입력 가능
    /*Rio_readlineb: 소켓이나 파일 디스크립터로부터 줄단위로 안전하게 읽는 함수(에러처리 포함)
    실제로 줄 단위 robust 읽기를 하는 것은 rio_readlineb
    Rio_readinitb: 읽기 버퍼 초기화(소켓 연결)
    */
    while((n=Rio_readlineb(&rio, buf, MAXLINE)) != 0){ //클라이언트가 데이터를 줄 단위로 보낼 때마다 버퍼에 저장, n!=0이면 반복
        printf("server received %d bytes\n", (int)n); //서버 측 로그 출력, 몇 바이트를 받았는지 콘솔에 출력
        Rio_writen(connfd, buf, n); //클라이언트에게 받은 내용을 그대로 다시 보냄
    }/*Rio_writen: rio_writen()이라는 진짜 데이트를 n바이트만큼 안전하게 write하는 함수를 호출,
        쓰기 실패or짧게 쓰였을 경우 에러메세지 출력 후 종료
        rio_writen(): n바이트가 다 써질 때까지 반복해서 write() 호출

ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;             // 남은 바이트 수
    ssize_t nwritten;            // 이번에 쓴 바이트 수
    char *bufp = usrbuf;         // 현재 write할 위치

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR)  // 시그널 인터럽트라면
                nwritten = 0;    // 다시 시도
            else
                return -1;       // 진짜 오류면 종료
        }
        nleft -= nwritten;       // 남은 바이트 수 감소
        bufp += nwritten;        // 다음 write 위치로 이동
    }
    return n;                    // 전체 바이트 다 썼으니 성공
}
*/
}