/*
 * 파일명 : command.h
 * 기능   : 명령 파싱 함수 선언
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <mosquitto.h>

/* 함수 선언 */
void command_parse(struct mosquitto *mosq, const char *cmd);

#endif /* COMMAND_H */