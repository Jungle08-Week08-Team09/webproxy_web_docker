#include "csapp.h" // csapp 라이브러리에는 에러 처리 및 입출력 관련 유틸 함수가 정의되어 있음

void echo(int connfd); // 클라이언트와 연결된 소켓을 통해 echo 동작을 수행하는 함수 (정의는 echo.c에 있음)

int main(int argc, char **argv) // argc : argument count(인자의 개수), argv : argument vector(입력된 인자 문자열 배열)
{
    int listenfd, connfd;
    // listenfd: 리스닝 소켓의 파일 디스크립터(File Descriptor) - 서버에서 단 하나, 클라이언트의 연결 요청을 기다림
    //           서버가 socket() → bind() → listen()을 통해 생성, 클라이언트의 연결 요청을 기다리는 수동 소켓(passive socket), 단 하나만 만들어서 accept()에 계속 넘겨줌
    // connfd: accept() 호출 결과로 반환되는 클라이언트와 연결된 소켓 디스크립터 - 클라이언트마다 하나씩, 각각의 연결을 위해 따로 생성됨
    //         이 소켓은 특정 클라이언트와의 통신 전용으로 사용됨, read(), write() 또는 echo() 함수 내부에서 메시지를 주고받을 때 사용
    // 이 두 개는 둘 다 소켓이며, 내부적으로는 유닉스 커널이 관리하는 파일 디스크립터임. 네트워크 연결도 마치 "파일"처럼 다룰 수 있다는 UNIX 철학
    socklen_t clientlen; // clientaddr의 구조체 크기를 담는 변수, accept()에서 클라이언트 주소를 받기 위해 주소 크기를 미리 알려주는 용도
    struct sockaddr_storage clientaddr; // 클라이언트의 주소 정보를 담을 범용 구조체(sockaddr_in, sockaddr_in6 모두 수용), 나중에 getnameinfo()로 IP 주소나 도메인명을 얻는 데 사용됨
    char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트의 호스트 이름(또는 IP 문자열)과 클라이언트가 접속한 포트 번호 문자열을 저장하는 버퍼, getnameinfo()를 통해 clientaddr에서 추출됨

    if (argc != 2)  // 포트 번호를 명령행 인자로 받지 않으면 사용법 출력 후 종료
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    // ./echoserveri 8080라는 명령을 받으면, 인자가 2개임("./echoserveri"(argv[0]), "8080"(argv[1]))

    listenfd = Open_listenfd(argv[1]);  // 해당 포트에 바인딩된 리스닝 소켓을 생성하고, 그 디스크립터를 반환함(클라이언트 연결 요청을 받는 데 사용)
    while (1) 
    {
        clientlen = sizeof(struct sockaddr_storage);  // 클라이언트 주소 정보를 받을 구조체 크기 설정
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // 클라이언트 연결 수락. connfd는 클라이언트와 통신할 소켓
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);  // 클라이언트의 IP 주소와 포트 번호를 문자열로 변환
        printf("Connected to (%s, %s)\n", client_hostname, client_port);  // 클라이언트 정보 출력
        echo(connfd); // echo 서비스 수행 (받은 내용을 그대로 돌려줌)
        Close(connfd); // 클라이언트 연결 종료
    }
    exit(0);
}

// 단일 프로세스 기반의 TCP 에코 서버
// 클라이언트와 연결을 수락한 후, 해당 연결에 대해 echo() 함수를 실행하고, 종료되면 연결을 닫고 다음 연결을 수락
// Accept() → echo() → Close()의 순환을 무한 반복하는 구조