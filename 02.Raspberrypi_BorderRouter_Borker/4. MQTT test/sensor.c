/*
 * 파일명 : sensor.c
 * 기능   : 더미 센서 데이터 생성 및 JSON 변환 후 publish
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>
#include "sensor.h"

#define TOPIC_PUB   "nucode/rx"     /* RPi → VM JSON 데이터 */

/*==============================================
 * 함수   : sensor_dummy_update
 * 기능   : 더미 센서 데이터 갱신
 * 인자   : sensor - 센서 데이터 구조체 포인터
 *          count  - 카운터
 *==============================================*/
void sensor_dummy_update(sensor_data_t *sensor, int count)
{
    sensor->node_id = 1;
    sensor->temp    = 20.0f + (count % 10);     /* 20.0 ~ 29.0 */
    sensor->humid   = 50.0f + (count % 20);     /* 50.0 ~ 69.0 */
}

/*==============================================
 * 함수   : sensor_publish
 * 기능   : 센서 데이터를 JSON으로 변환 후 publish
 * 인자   : mosq   - mosquitto 인스턴스
 *          sensor - 센서 데이터 구조체 포인터
 *==============================================*/
void sensor_publish(struct mosquitto *mosq, sensor_data_t *sensor)
{
    /* JSON 객체 생성 */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type",    "sensor");
    cJSON_AddNumberToObject(root, "node_id", sensor->node_id);
    cJSON_AddNumberToObject(root, "temp",    sensor->temp);
    cJSON_AddNumberToObject(root, "humid",   sensor->humid);

    /* JSON 문자열 변환 */
    char *json_str = cJSON_PrintUnformatted(root);

    /* MQTT publish */
    mosquitto_publish(mosq, NULL, TOPIC_PUB,
                     strlen(json_str), json_str, 0, false);

    printf("[송신] 토픽: %s | 데이터: %s\n", TOPIC_PUB, json_str);

    /* 메모리 해제 */
    free(json_str);
    cJSON_Delete(root);
}