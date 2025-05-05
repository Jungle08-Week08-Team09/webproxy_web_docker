#include "csapp.h" // CS:APP에서 제공하는 래퍼 함수들과 상수, 자료구조가 정의된 헤더 파일 포함

int main(int argc, char **argv)
{
    int clientfd;                  // 서버와의 연결에 사용할 소켓 디스크립터
    char *host, *port, buf[MAXLINE]; // 서버 호스트명, 포트번호, 입출력 버퍼
    rio_t rio;                     // 버퍼링된 입출력용 구조체

    if (argc != 3) // 명령행 인자가 3개가 아닌 경우 (프로그램 이름 + 호스트 + 포트), 사용법 출력
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1]; // 첫 번째 인자는 접속할 서버의 호스트 주소
    port = argv[2]; // 두 번째 인자는 접속할 포트 번호

    clientfd = Open_clientfd(host, port); // 서버에 연결하고, 연결된 소켓의 디스크립터를 clientfd에 저장, 실패 시 −1 반환 (내부적으로 getaddrinfo, socket, connect 사용)
    Rio_readinitb(&rio, clientfd); // clientfd를 기반으로 버퍼링된 읽기 구조체 rio를 초기화, 이후 Rio_readlineb()로 안전한 라인 기반 읽기가 가능함

    while (Fgets(buf, MAXLINE, stdin) != NULL)  // 표준 입력으로부터 한 줄 입력을 받아 buf에 저장 (ex. 사용자 타이핑)
    {

        Rio_writen(clientfd, buf, strlen(buf));  // buf 내용을 서버에 전송 (clientfd는 서버와 연결된 소켓), strlen으로 정확한 바이트 수만큼 전송
        Rio_readlineb(&rio, buf, MAXLINE);  // 서버로부터 한 줄 응답을 읽어 buf에 저장, rio는 내부적으로 버퍼를 사용하여 read 호출을 최적화함
        Fputs(buf, stdout);   // 서버로부터 받은 응답을 표준 출력에 출력 (즉, 사용자에게 보여줌)
    }

    Close(clientfd);  // 모든 작업이 끝난 후 소켓을 닫아 리소스를 정리
    exit(0);          // 프로그램 정상 종료
}