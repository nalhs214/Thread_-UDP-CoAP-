/* ===========================================================================
 * app_fsm.c  —  애플리케이션 상태 머신(FSM)
 *
 * 이 프로젝트의 "두뇌".
 *   외부에서 벌어진 모든 사건(Thread 상태 변화, UDP 수신, 버튼)을
 *   하나의 이벤트 큐로 모아, 단일 Work Queue 컨텍스트에서 순서대로 처리한다.
 *   → 여러 컨텍스트가 상태를 동시에 건드리는 문제를 원천 차단한다.
 *     (명세서 FR-05-1: 모든 상태 전이는 app_wq 에서만)
 *
 * 상태 흐름:
 *   SYS_INIT → THREAD_INIT →(attach)→ THREAD_ATTACHED →(udp open)→ UDP_READY
 * ========================================================================= */

#include "app_fsm.h"
#include "app_led.h"
#include "app_config.h"
#include "thread.h"
#include "udp.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_fsm, LOG_LEVEL_INF);

/* ---- 이벤트 큐 / Work (명세서 04.3) ------------------------------------ */
/* 이벤트 8개까지 버퍼링, 4바이트 정렬 */
K_MSGQ_DEFINE(g_fsm_evt_q, sizeof(struct fsm_evt), 8, 4);
static struct k_work    g_fsm_evt_work;   /* 이벤트를 꺼내 처리하는 Work   */
static struct k_work_q *g_wq;             /* 처리에 사용할 Work Queue      */

/* ---- FSM 상태 --------------------------------------------------------- */
static enum app_state g_state = APP_SYS_INIT;

/* ---- 송신 메시지 시퀀스 번호 ------------------------------------------ */
static uint8_t g_seq;

/* ---------------------------------------------------------------------------
 * make_led_cmd - LED 제어용 UDP 메시지 하나 만들기
 * ------------------------------------------------------------------------- */
static udp_msg_t make_led_cmd(uint8_t type)
{
    udp_msg_t m;
    memset(&m, 0, sizeof(m));
    m.type    = type;
    m.seq     = g_seq++;
    m.node_id = 0;            /* (확장) 노드 고유 ID 부여 지점 */
    return m;
}

/* ---------------------------------------------------------------------------
 * on_udp_rx - 수신 큐에 쌓인 모든 프레임 처리
 *   - LED_RX 를 명령대로 제어(다른 기기의 LED 제어 = 이 프로젝트의 목표)
 * ------------------------------------------------------------------------- */
static void on_udp_rx(void)
{
    udp_frame_t f;
    while (udp_rx_get(&f)) {
        LOG_INF("UDP RX: type=0x%02x seq=%u len=%u",
                f.msg.type, f.msg.seq, f.len);

        /* 수신한 명령대로 LED_RX 제어 */
        switch (f.msg.type) {
        case MSG_TYPE_LED_ON:     app_led_set(LED_RX, true);  break;
        case MSG_TYPE_LED_OFF:    app_led_set(LED_RX, false); break;
        case MSG_TYPE_LED_TOGGLE:
        default:                  app_led_toggle(LED_RX);     break;
        }
    }
}

/* ---------------------------------------------------------------------------
 * on_btn_released - 버튼 뗌 처리 (UDP_READY 상태에서만 동작)
 *   Button1(0): 유니캐스트 / Button2(1): 멀티캐스트 / Button4(3): 상태 로그
 * ------------------------------------------------------------------------- */
static void on_btn_released(uint8_t num)
{
    if (g_state != APP_UDP_READY) {
        LOG_WRN("btn%u ignored (not UDP_READY)", num);
        return;
    }

    switch (num) {
    case 0: {   /* Button 1 → 유니캐스트 (특정 노드 LED 토글) */
        udp_msg_t m = make_led_cmd(MSG_TYPE_LED_TOGGLE);
        app_led_toggle(LED_TX);                       /* 송신 표시 */
        udp_send_unicast(UDP_PEER_ADDR, &m);
        LOG_INF("TX unicast -> %s", UDP_PEER_ADDR);
        break;
    }
    case 1: {   /* Button 2 → 멀티캐스트 (전체 노드 LED 토글) */
        udp_msg_t m = make_led_cmd(MSG_TYPE_LED_TOGGLE);
        app_led_toggle(LED_TX);                       /* 송신 표시 */
        udp_send_multicast(&m);
        LOG_INF("TX multicast -> %s", UDP_MULTICAST_ADDR);
        break;
    }
    case 3:     /* Button 4 → 내 IPv6 주소 로그 출력(유니캐스트 대상 확인용) */
        thread_log_addresses();
        break;

    default:    /* Button 3(2): 확장 예약 */
        break;
    }
}

/* ---------------------------------------------------------------------------
 * fsm_evt_work_handler - 이벤트 큐를 비우며 상태 전이 수행
 *   app_wq 컨텍스트에서만 실행된다.
 * ------------------------------------------------------------------------- */
static void fsm_evt_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    struct fsm_evt evt;
    while (k_msgq_get(&g_fsm_evt_q, &evt, K_NO_WAIT) == 0) {
        switch (evt.type) {

        case FSM_EVT_THREAD_ATTACHED:
            /* 망 참여 완료 → 연결 LED ON, 역할 표시, UDP 소켓 열기 */
            if (g_state == APP_THREAD_INIT) {
                app_led_set(LED_THREAD, true);
                app_led_set(LED_ROLE, thread_role_is_leader());
                thread_log_addresses();
                udp_open();                 /* → 성공 시 UDP_READY 이벤트 */
                g_state = APP_THREAD_ATTACHED;
                LOG_INF("state -> THREAD_ATTACHED");
            }
            break;

        case FSM_EVT_THREAD_DETACHED:
            /* 망 이탈 → 연결 LED OFF, 다시 초기화 상태로 */
            app_led_set(LED_THREAD, false);
            app_led_set(LED_ROLE, false);
            g_state = APP_THREAD_INIT;
            LOG_INF("state -> THREAD_INIT (detached)");
            break;

        case FSM_EVT_UDP_READY:
            if (g_state == APP_THREAD_ATTACHED) {
                g_state = APP_UDP_READY;
                LOG_INF("state -> UDP_READY (버튼 송수신 가능)");
            }
            break;

        case FSM_EVT_UDP_RX:
            on_udp_rx();
            break;

        case FSM_EVT_BTN_RELEASED:
            on_btn_released(evt.u.btn.num);
            break;

        default:
            break;
        }
    }
}

/* ---------------------------------------------------------------------------
 * fsm_evt_post - 외부 → FSM 이벤트 전달(비동기)
 *   어느 컨텍스트에서 불려도 안전(큐에 넣고 Work 만 깨움).
 * ------------------------------------------------------------------------- */
void fsm_evt_post(const struct fsm_evt *evt)
{
    /* 큐가 가득 차면(K_NO_WAIT) 이 이벤트는 유실된다. 경고만 남긴다. */
    if (k_msgq_put(&g_fsm_evt_q, evt, K_NO_WAIT) != 0) {
        LOG_WRN("fsm evt queue full, dropped type=%d", evt->type);
        return;
    }
    k_work_submit_to_queue(g_wq, &g_fsm_evt_work);
}

/* ---------------------------------------------------------------------------
 * app_fsm_init - FSM 초기화 + Thread 시작 트리거
 * ------------------------------------------------------------------------- */
void app_fsm_init(struct k_work_q *wq)
{
    g_wq    = wq;
    g_state = APP_SYS_INIT;
    k_work_init(&g_fsm_evt_work, fsm_evt_work_handler);

    /* THREAD_INIT 진입: Thread 네트워크 시작.
     * 이후 역할이 잡히면 콜백 → FSM_EVT_THREAD_ATTACHED 로 이어진다. */
    g_state = APP_THREAD_INIT;
    LOG_INF("state -> THREAD_INIT");
    thread_init();
}
