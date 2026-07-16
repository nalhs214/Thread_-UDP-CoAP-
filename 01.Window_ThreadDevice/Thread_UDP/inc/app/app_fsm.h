#ifndef APP_FSM_H
#define APP_FSM_H

#include <stdint.h>
#include <zephyr/kernel.h>

/* ---------------------------------------------------------------------------
 * FSM 상태 (명세서 04.1)
 *
 *   APP_SYS_INIT        시스템 부팅 직후
 *   APP_THREAD_INIT     Thread 파라미터 설정 후 망 참여 대기
 *   APP_THREAD_ATTACHED 망 참여 완료(역할 확정) → UDP 소켓 열기 트리거
 *   APP_UDP_READY       UDP 준비 완료 → 버튼 송수신 가능
 * ------------------------------------------------------------------------- */
enum app_state {
    APP_SYS_INIT = 0,
    APP_THREAD_INIT,
    APP_THREAD_ATTACHED,
    APP_UDP_READY,
};

/* ---------------------------------------------------------------------------
 * FSM 이벤트 타입 (명세서 04.2)
 *
 *   외부(Thread 콜백 / UDP 수신 / 버튼)에서 발생한 사건을 하나의 큐로 모아
 *   Work Queue 컨텍스트에서 순서대로 처리하기 위한 열거형.
 *
 *   ※ CoAP 단계에서는 FSM_EVT_COAP_* 만 뒤에 추가하면 되고,
 *     기존 처리 구조는 그대로 유지된다. (명세서 08.2)
 * ------------------------------------------------------------------------- */
enum fsm_evt_type {
    FSM_EVT_THREAD_ATTACHED = 0,  /* Thread 망 참여 완료 */
    FSM_EVT_THREAD_DETACHED,      /* Thread 망 이탈     */
    FSM_EVT_UDP_READY,            /* UDP 소켓 준비 완료 */
    FSM_EVT_UDP_RX,               /* UDP 메시지 수신    */
    FSM_EVT_BTN_RELEASED,         /* 버튼에서 손 뗌     */
};

/* ---------------------------------------------------------------------------
 * FSM 이벤트 데이터
 *   type 에 따라 union 중 하나만 유효하다.
 * ------------------------------------------------------------------------- */
struct fsm_evt {
    enum fsm_evt_type type;
    union {
        struct {
            uint8_t  num;      /* 버튼 번호 (0~3)        */
            uint32_t dur_ms;   /* 눌려 있던 시간(ms)     */
        } btn;
    } u;
};

/* FSM 초기화. wq = 상태 전이를 처리할 Work Queue. */
void app_fsm_init(struct k_work_q *wq);

/* 외부에서 FSM 으로 이벤트를 던진다(비동기).
 *   ISR/다른 스레드/OpenThread 콜백 등 어디서 불러도 안전하도록
 *   내부에서 k_msgq_put + k_work_submit 만 수행한다. */
void fsm_evt_post(const struct fsm_evt *evt);

#endif /* APP_FSM_H */
