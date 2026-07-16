/*
 * 파일명 : input.c
 * 기능   : 터미널 입력 처리 및 MQTT publish
 *          - "led_on"  → RPi LED ON 명령 전송
 *          - "led_off" → RPi LED OFF 명령 전송
 *          - 그 외     → 문자열 그대로 전송
 */

#include <stdio.h>
#include <string.h>
#include <mosquitto.h>
#include "input.h"

/*==============================================
 * 함수   : input_process
 * 기능   : 입력 문자열 처리 후 publish
 * 인자   : mosq  - mosquitto 인스턴스
 *          input - 입력 문자열
 *==============================================*/
void input_process(struct mosquitto *mosq, const char *input)
{
    /* 입력 문자열 그대로 publish */
    mosquitto_publish(mosq, NULL, TOPIC_PUB,
                     strlen(input), input, 0, false);

    printf("[송신] 토픽: %s | 데이터: %s\n", TOPIC_PUB, input);
}