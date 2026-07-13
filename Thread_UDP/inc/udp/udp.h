#ifndef UDP_H
#define UDP_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>   /* struct sockaddr_in6 (Zephyr 네이티브) */

/* ---------------------------------------------------------------------------
 * UDP 메시지 포맷 (명세서 07.1)
 *   실제 무선으로 나가는 페이로드(고정 64바이트).
 *   __packed 로 패딩 없이 그대로 직렬화되게 한다.
 * ------------------------------------------------------------------------- */
#define UDP_PAYLOAD_MAX  60

/* 메시지 종류: 수신 측이 payload 해석 없이 type 만으로 동작하게 함 */
typedef enum {
    MSG_TYPE_LED_TOGGLE = 0x01,   /* LED 토글 */
    MSG_TYPE_LED_ON     = 0x02,   /* LED 켜기 */
    MSG_TYPE_LED_OFF    = 0x03,   /* LED 끄기 */
} msg_type_t;

typedef struct {
    uint8_t  type;                    /* msg_type_t          */
    uint8_t  seq;                     /* 시퀀스 번호         */
    uint16_t node_id;                 /* 송신 노드 ID        */
    uint8_t  payload[UDP_PAYLOAD_MAX];/* 여분 데이터         */
} __attribute__((packed)) udp_msg_t;  /* = 64 byte           */

/* -------------------------------------------------------------- ----------
 * 수신 프레임 한 칸
 *   무선 페이로드(udp_msg_t) + "누가 보냈는지"(송신자 주소)를 함 보관한다.
 *   주소 타입은 Zephyr 표준 sockaddr_in6 (OpenThread 타입에 의존 지 않음).
 * -------------------------------------------------------------- -------- */
typedef struct {
    struct sockaddr_in6 addr;        /* RX 송신자 IPv6 주소           */
    bool                multicast;   /* 멀티캐스트 수신 여부(예약)    */
    uint16_t            len;          /* 실제 수신 바이트 수           */
    udp_msg_t           msg;          /* 무선 페이로드                 */
} udp_frame_t;

/* ---------------------------------------------------------------------------
 * API  (모두 Zephyr BSD 소켓 기반 — OpenThread 타입이 인터페이스에 없다)
 * ------------------------------------------------------------------------- */

/* UDP 모듈 준비(부팅 시 1회). 소켓은 udp_open() 에서 연다.
 *   wq = 인터페이스 통일용(현재 미사용). */
int udp_init(struct k_work_q *wq);

/* UDP 소켓 열기/바인딩 + 수신 스레드 시작. Thread 망에 attach 된 뒤 호출.
 *   성공 시 FSM_EVT_UDP_READY 이벤트를 post 한다. */
int udp_open(void);

/* 유니캐스트 전송: dst_ipv6 문자열 주소로 msg 전송(zsock_sendto). */
int udp_send_unicast(const char *dst_ipv6, const udp_msg_t *msg);

/* 멀티캐스트 전송: UDP_MULTICAST_ADDR 로 msg 전송(전체 노드). */
int udp_send_multicast(const udp_msg_t *msg);

/* 수신 큐에서 프레임 하나 꺼내기(FSM 이 RX 이벤트 처리 시 호출).
 *   반환: true = 꺼냄, false = 비어 있음 */
bool udp_rx_get(udp_frame_t *out);

#endif /* UDP_H */
