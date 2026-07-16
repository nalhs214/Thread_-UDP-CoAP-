/*
 * 파일명 : mqtt.c
 * 기능   : MQTT 브로커 연결, 콜백 등록 및 처리
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include "mqtt.h"
#include "command.h"

/*==============================================
 * 브로커 설정
 *==============================================*/
#define BROKER_HOST  "localhost"    /* 브로커 주소 (RPi 자신) */
#define BROKER_PORT  1883           /* MQTT 기본 포트 */
#define KEEP_ALIVE   60             /* 연결 유지 시간 (초) */
#define USERNAME     "raspberrypi"  /* 브로커 인증 유저명 */
#define PASSWORD     "1234"   /* 브로커 인증 비밀번호 */
#define TOPIC_SUB    "nucode/tx"    /* VM → RPi 명령 토픽 */

/*==============================================
 * 콜백 함수 : 브로커 연결 완료 시 호출
 *==============================================*/
static void on_connect(struct mosquitto *mosq, void *userdata, int rc)
{
    if(rc == 0) {
        printf("[연결] 브로커 연결 성공\n");

        /* 연결 성공 시 VM 명령 토픽 구독 */
        mosquitto_subscribe(mosq, NULL, TOPIC_SUB, 0);
        printf("[구독] 토픽 구독 시작: %s\n", TOPIC_SUB);
    } else {
        printf("[연결] 브로커 연결 실패 (rc: %d)\n", rc);
    }
}

/*==============================================
 * 콜백 함수 : 메시지 수신 시 호출
 *==============================================*/
static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *msg)
{
    char *cmd = (char*)msg->payload;
    printf("[수신] 토픽: %s | 명령: %s\n", msg->topic, cmd);

    /* command 모듈로 전달 */
    command_parse(mosq, cmd);
}

/*==============================================
 * 함수   : mqtt_init
 * 기능   : mosquitto 초기화 및 브로커 연결
 * 반환   : mosquitto 인스턴스 포인터
 *==============================================*/
struct mosquitto *mqtt_init(void)
{
    struct mosquitto *mosq = NULL;

    /* 1. 라이브러리 초기화 */
    mosquitto_lib_init();

    /* 2. 클라이언트 인스턴스 생성 */
    mosq = mosquitto_new("rpi_client", true, NULL);
    if(mosq == NULL) {
        printf("[오류] mosquitto 인스턴스 생성 실패\n");
        return NULL;
    }

    /* 3. 인증 설정 */
    mosquitto_username_pw_set(mosq, USERNAME, PASSWORD);

    /* 4. 콜백 등록 */
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    /* 5. 브로커 연결 */
    if(mosquitto_connect(mosq, BROKER_HOST, BROKER_PORT, KEEP_ALIVE)
       != MOSQ_ERR_SUCCESS) {
        printf("[오류] 브로커 연결 실패\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    /* 6. 네트워크 루프 시작 (논블로킹) */
    mosquitto_loop_start(mosq);

    return mosq;
}

/*==============================================
 * 함수   : mqtt_cleanup
 * 기능   : mosquitto 인스턴스 정리
 * 인자   : mosq - mosquitto 인스턴스
 *==============================================*/
void mqtt_cleanup(struct mosquitto *mosq)
{
    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}