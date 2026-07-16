/*
 * 파일명 : main.c
 * 기능   : MQTT VM 클라이언트 메인
 *          - 터미널 입력 문자열 publish → RPi
 *          - RPi JSON 데이터 수신 출력
 * 빌드   : make
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>
#include <mosquitto.h>
#include "mqtt.h"
#include "input.h"

int main(void)
{
    struct mosquitto *mosq = NULL;
    char input[BUF_SIZE];

    fd_set read_fds;
    struct timeval timeout;

    /*------------------------------------------
     * 1. MQTT 초기화 및 브로커 연결
     *------------------------------------------*/
    mosq = mqtt_init();
    if(mosq == NULL) {
        return -1;
    }

    printf("[시작] MQTT VM 클라이언트 실행\n");
    printf("[안내] 문자열 입력 후 Enter → RPi로 전송\n");
    printf("[안내] 'led_on' / 'led_off' → RPi LED 제어\n");
    printf("[안내] 'q' 입력 시 종료\n\n");

    /*------------------------------------------
     * 2. 메인 루프
     *    - select()로 터미널 입력 감지
     *    - RPi 수신은 on_message 콜백에서 처리
     *------------------------------------------*/
    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        int ret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);

        if(ret > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
            if(fgets(input, BUF_SIZE, stdin) == NULL)
                break;

            /* 개행 문자 제거 */
            input[strcspn(input, "\n")] = 0;

            /* 종료 명령 */
            if(strcmp(input, "q") == 0)
                break;

            /* 빈 문자열 무시 */
            if(strlen(input) == 0)
                continue;

            /* 입력 처리 및 publish */
            input_process(mosq, input);
        }
    }

    /*------------------------------------------
     * 3. 정리
     *------------------------------------------*/
    printf("[종료] MQTT VM 클라이언트 종료\n");
    mqtt_cleanup(mosq);

    return 0;
}