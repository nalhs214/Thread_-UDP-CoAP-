/*
 * 파일명 : command.c
 * 기능   : VM으로부터 수신한 문자열 명령 파싱 및 처리
 *          - "led_on"  → LED ON
 *          - "led_off" → LED OFF
 *          - 그 외     → 에코
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>
#include "command.h"

#define TOPIC_PUB   "nucode/rx"     /* RPi → VM 응답 토픽 */

/*==============================================
 * 함수   : led_on
 * 기능   : LED ON 제어
 * (추후 GPIO 제어로 교체)
 *==============================================*/
static void led_on(void)
{
    printf("[LED] LED ON\n");
}

/*==============================================
 * 함수   : led_off
 * 기능   : LED OFF 제어
 * (추후 GPIO 제어로 교체)
 *==============================================*/
static void led_off(void)
{
    printf("[LED] LED OFF\n");
}

/*==============================================
 * 함수   : command_parse
 * 기능   : 수신 문자열 파싱 후 동작 처리 및 응답 publish
 * 인자   : mosq - mosquitto 인스턴스
 *          cmd  - 수신된 명령 문자열
 *==============================================*/
void command_parse(struct mosquitto *mosq, const char *cmd)
{
    char response[256];

    if(strcmp(cmd, "led_on") == 0) {
        led_on();
        snprintf(response, sizeof(response), "LED ON 완료");
    }
    else if(strcmp(cmd, "led_off") == 0) {
        led_off();
        snprintf(response, sizeof(response), "LED OFF 완료");
    }
    else {
        /* 명령 아닌 문자열 → 에코 */
        snprintf(response, sizeof(response), "echo:%s", cmd);
    }

    /* 응답 JSON 생성 */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "response");
    cJSON_AddStringToObject(root, "msg",  response);

    char *json_str = cJSON_PrintUnformatted(root);

    /* 응답 publish */
    mosquitto_publish(mosq, NULL, TOPIC_PUB,
                     strlen(json_str), json_str, 0, false);

    printf("[응답] 토픽: %s | 데이터: %s\n", TOPIC_PUB, json_str);

    /* 메모리 해제 */
    free(json_str);
    cJSON_Delete(root);
}