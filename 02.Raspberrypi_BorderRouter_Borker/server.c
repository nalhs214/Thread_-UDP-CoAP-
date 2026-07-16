#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main(int argc, char *argv[])        // argv[] = ip, addr, port
{
    int server_fd, client_fd[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    fd_set read_fds;
    char buffer[BUFFER_SIZE];
    int max_fd, activity, i;

    /* 클라이언트 소켓 초기화 */
    for (i = 0; i < MAX_CLIENTS; i++)
        client_fd[i] = 0;

    /* 소켓 생성 */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);    // domain(IPv4), type(연결지향), protocol
    if (server_fd < 0) {
        perror("socket() 실패");        // 소켓 못열면 fd가 -1이라 fail
        exit(EXIT_FAILURE);
    }

    /* 소켓 옵션 설정 */ 
    // setsockopt(sockfd, level,optname, optval, optlen)
    // level에는 SOL_SOCKET, IPPROTO_IP, IPPROTO_TCP
    // Soptname 에는 SO_REUSEADDR, SO_KEEPALIVE, TCP_NODELAY
    // optval에는 1(사용), 0(사용안함) 넣어주면 됨
    // optlen에는 optval의 크기 넣어주면 됨 sizeof(optval)
    int opt = 1;        
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 서버 주소 설정 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;               // argv[1]
    server_addr.sin_port        = htons(PORT);              // argv[2]

    /* 바인딩 */
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() 실패");
        exit(EXIT_FAILURE);
    }

    /* 리슨 */
    if (listen(server_fd, 5) < 0) {     // backlog가 5개라 5개의 연결 요청까지 대기
        perror("listen() 실패");
        exit(EXIT_FAILURE);
    }

    printf("[서버] 시작 - PORT: %d\n", PORT);   
    printf("[서버] 클라이언트 대기 중...\n");

    while (1) {
        FD_ZERO(&read_fds);                 // 감시 목록 초기화
        FD_SET(server_fd, &read_fds);       // 감시할 소켓 등록
        max_fd = server_fd;                 // 감시범위 설정

        /* 클라이언트 소켓 등록 */
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fd[i] > 0)
                FD_SET(client_fd[i], &read_fds);
            if (client_fd[i] > max_fd)
                max_fd = client_fd[i];
        }

        /* select() 대기 */
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select() 실패");
            break;
        }

        /* 새 클라이언트 연결 */
        if (FD_ISSET(server_fd, &read_fds)) {
            int new_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (new_fd < 0) {
                perror("accept() 실패");
                continue;
            }
            printf("[서버] 새 클라이언트 연결: %s:%d\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_fd[i] == 0) {
                    client_fd[i] = new_fd;
                    break;
                }
            }
        }

        /* 클라이언트 데이터 처리 */
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fd[i] > 0 && FD_ISSET(client_fd[i], &read_fds)) {
                int bytes = recv(client_fd[i], buffer, BUFFER_SIZE - 1, 0);
                if (bytes <= 0) {
                    printf("[서버] 클라이언트 연결 종료: fd=%d\n", client_fd[i]);
                    close(client_fd[i]);
                    client_fd[i] = 0;
                } else {
                    buffer[bytes] = '\0';
                    printf("[서버] 수신: %s\n", buffer);
                    send(client_fd[i], buffer, bytes, 0);
                    printf("[서버] 에코 전송 완료\n");
                }
            }
        }
    }

    close(server_fd);
    return 0;
}