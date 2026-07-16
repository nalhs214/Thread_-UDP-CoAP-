/*
 * 파일명 : input.h
 * 기능   : 터미널 입력 처리 함수 선언
 */

#ifndef INPUT_H
#define INPUT_H

#include <mosquitto.h>

#define BUF_SIZE    256
#define TOPIC_PUB   "nucode/tx"     /* VM → RPi 명령 토픽 */

/* 함수 선언 */
void input_process(struct mosquitto *mosq, const char *input);

#endif /* INPUT_H */