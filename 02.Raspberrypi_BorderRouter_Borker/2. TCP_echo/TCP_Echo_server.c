/* 
* 기능 : 서버에서 하나의 클라이언트와 연결되어 에코 서비스를 제공
* - 클라이언트는 프로그램 사용자로부터 문자열을 입력받아 서버에 전송
* - 서버는 받은 문자열 데이터를 클라이언트로 재전송, 즉 에코 시킨다
* - Q 입력 시 종료
* - 한번에 하나의 client만 연결 가능하기 때문에 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 1024
void error_handling(char *message);

int main(int argc, char *argv[])        // argv[] = ip, addr, port
{
    int serv_sock, clnt_sock;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    char message[BUF_SIZE];
    int str_len;

    if(argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock=socket(AF_INET, SOCK_STREAM, 0);           // TCP 소켓 생성
    if(serv_sock == -1) {
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);       // 모든 IP로부터 접속 허용
    serv_addr.sin_port=htons(atoi(argv[1]));           // 포트


    // bind
    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1) {  // bind() 호출
        error_handling("bind() error");
    }

    // listen
    if(listen(serv_sock, 5)==-1) {                     // listen() 호출
        error_handling("listen() error");
    } 

    clnt_addr_size = sizeof(clnt_addr);

    // accept
    for(int i = 0; i<5; i++) {
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);  // accept() 호출
        if(clnt_sock == -1){
            error_handling("accept() error");
        } else {
            printf("Connected client %d \n", i+1);
        }

        while((str_len=read(clnt_sock, message, BUF_SIZE))!=0) {  // read() 호출
            write(clnt_sock, message, str_len);                    // write() 호출
        }
        
        close(clnt_sock);                                    // 클라이언트 소켓 종료
    }
    close(serv_sock);                                    // 서버 소켓 종료
    return 0;
}


void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}