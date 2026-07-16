/*
 * 파일명 : main.c
 * 기능   : MQTT RPi 클라이언트 메인
 *          - 2초마다 더미 센서 데이터 publish
 *          - VM 명령 수신 및 처리
 * 빌드   : make
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>     /* select() - 터미널 입력 감지 */
#include "mqtt.h"
#include "sensor.h"

#define BUF_SIZE 256
#define TOPIC_PUB  "nucode/rx"     /* RPi → VM 응답 토픽 */

int main(void)
{
    struct mosquitto *mosq = NULL;
    sensor_data_t sensor   = {0};
    char input[BUF_SIZE];
    int count              = 0;

    fd_set read_fds;
    struct timeval timeout;

    /*------------------------------------------
     * 1. MQTT 초기화 및 브로커 연결
     *------------------------------------------*/
    mosq = mqtt_init();
    if(mosq == NULL) {
        return -1;
    }
    
    printf("[시작] MQTT RPi 클라이언트 실행\n");
    printf("[안내] 문자열 입력 후 Enter → VM으로 전송\n");
    printf("[안내] 'q' 입력 시 종료\n\n");


    /*------------------------------------------
     * 2. 메인 루프
     *    - select()로 터미널 입력 감지 (논블로킹)
     *    - 2초마다 센서 데이터 publish
     *------------------------------------------*/
    while(1) {
        /* select() 타임아웃 2초 설정 */
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); /* 표준 입력 감시 */
        timeout.tv_sec  = 2;
        timeout.tv_usec = 0;

        int ret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);

        if(ret > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
            /*--------------------------------------
             * 터미널 입력 감지
             *--------------------------------------*/
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

            /* 입력 문자열 publish */
            mosquitto_publish(mosq, NULL, TOPIC_PUB,
                             strlen(input), input, 0, false);
            printf("[송신] 토픽: %s | 데이터: %s\n", TOPIC_PUB, input);
        }
        else {
            /*--------------------------------------
             * 타임아웃 → 2초마다 센서 데이터 publish
             *--------------------------------------*/
            sensor_dummy_update(&sensor, count);
            sensor_publish(mosq, &sensor);
            count++;
        }
    }

    /*------------------------------------------
     * 3. 정리
     *------------------------------------------*/
    printf("[종료] MQTT RPi 클라이언트 종료\n");
    mqtt_cleanup(mosq);

    return 0;
}