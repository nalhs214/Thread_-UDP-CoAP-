/*
 * 파일명 : sensor.h
 * 기능   : 센서 데이터 구조체 및 함수 선언
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <mosquitto.h>

/*==============================================
 * 센서 데이터 구조체
 * (추후 Thread 노드 바이너리 구조체로 교체)
 *==============================================*/
typedef struct {
    uint8_t node_id;    /* 노드 ID */
    float   temp;       /* 온도 */
    float   humid;      /* 습도 */
} sensor_data_t;

/* 함수 선언 */
void sensor_publish(struct mosquitto *mosq, sensor_data_t *sensor);
void sensor_dummy_update(sensor_data_t *sensor, int count);

#endif /* SENSOR_H */