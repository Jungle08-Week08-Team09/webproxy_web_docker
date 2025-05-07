#include "csapp.h" // 표준 라이브러리와 에러 처리, IO 처리를 포함하는 사용자 정의 헤더

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE]; // 클라이언트로부터 읽어올 데이터를 저장할 버퍼 (최대 MAXLINE 바이트)
    rio_t rio; // Robust I/O 처리를 위한 구조체 변수 (연결된 파일 디스크립터, 내부적으로 사용할 읽기 버퍼, 현재 버퍼 내 위치, 남은 바이트 수 등 저장)

    Rio_readinitb(&rio, connfd);
    // connfd에 연결된 소켓을 rio 객체에 연결해서, 그걸로 안전하게 읽겠다는 준비 작업임
    // rio 구조체를 초기화, 어떤 소켓 디스크립터(connfd)에서 읽을지 설정
    // 이 함수를 호출한 이후, rio를 통해 connfd에서 데이터를 읽을 수 있음. Rio_readlineb() 같은 함수들이 이 rio 구조체를 활용해서 읽기를 수행

    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        // 클라이언트로부터 한 줄씩 입력을 읽음
        // 0을 반환하면 EOF (연결 종료)

        printf("server received %zu bytes\n", n);
        // 몇 바이트를 수신했는지 출력 (디버깅용)

        Rio_writen(connfd, buf, n);
        // 받은 내용을 클라이언트에게 그대로 다시 전송 (에코)
    }
}

// rio를 쓰는 이유
// 일반적인 read() 호출은 버퍼링이 없고 부분 읽기(partial read)가 발생할 수 있어 번거로움
// RIO는 내부 버퍼를 활용해 라인 단위 또는 지정된 바이트 수만큼 안정적으로 읽을 수 있도록 도와줌
// 줄 단위로 데이터를 주고받는 TCP 서버를 만들 때 매우 유용