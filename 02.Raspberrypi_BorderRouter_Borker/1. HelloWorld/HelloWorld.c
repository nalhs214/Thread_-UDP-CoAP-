/* Hello World 프로그램 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

int main(int argc, char *argv[])        // argv[] = ip, addr, port
{
    int serv_sock;
    int clnt_sock;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    char message[] = "Hello World!";

    if(argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock=socket(AF_INET, SOCK_STREAM, 0);           // TCP 소켓 생성
    if(serv_sock == -1) {
        printf("socket() error\n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);       // 모든 IP로부터 접속 허용
    serv_addr.sin_port=htons(atoi(argv[1]));           // 포트


    // bind
    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1) {  // bind() 호출
        printf("bind() error\n");
        exit(1);
    }

    // listen
    if(listen(serv_sock, 5)==-1) {                     // listen() 호출
        printf("listen() error\n");
        exit(1);
    }

    // accept
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);  // accept() 호출
    if(clnt_sock == -1){
        printf("accept() error\n");
        exit(1);
    }

    write(clnt_sock, message, sizeof(message));        // 클라이언트로 메시지 전송
    close(clnt_sock);                                    // 클라이언트 소켓 종료
    close(serv_sock);                                    // 서버 소켓 종료
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}