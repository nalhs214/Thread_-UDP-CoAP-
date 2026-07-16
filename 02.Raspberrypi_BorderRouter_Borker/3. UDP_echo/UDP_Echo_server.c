#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 30
void error_handling(char *message);

int main(int argc, char *argv[])        // argv[] = ip, addr, port
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t adr_size;

    struct sockaddr_in serv_addr, clnt_addr;

    if(argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    sock=socket(AF_INET, SOCK_DGRAM, 0);           // TCP 소켓 생성
    if(sock == -1) {
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(sock));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);       // 모든 IP로부터 접속 허용
    serv_addr.sin_port=htons(atoi(argv[1]));           // 포트

    // bind
    if(bind(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1) {  // bind() 호출
        error_handling("bind() error");
    }

    while(1){
        adr_size = sizeof(clnt_addr);
        str_len = recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr*)&clnt_addr, &adr_size);  // recvfrom() 호출
        sendto(sock, message, str_len, 0, (struct sockaddr*)&clnt_addr, adr_size);  // sendto() 호출
    }

    
    close(sock);                                    // 서버 소켓 종료
    return 0;
}


void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}