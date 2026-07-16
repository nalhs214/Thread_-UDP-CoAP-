/*
 * 파일명 : mqtt.h
 * 기능   : MQTT 초기화 및 콜백 함수 선언
 */

#ifndef MQTT_H
#define MQTT_H

#include <mosquitto.h>

/* 함수 선언 */
struct mosquitto *mqtt_init(void);
void mqtt_cleanup(struct mosquitto *mosq);

#endif /* MQTT_H */