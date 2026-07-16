/* ===========================================================================
 * app_main.c  —  애플리케이션 글루(초기화 오케스트레이션)
 *
 * 역할:
 *   1) 애플리케이션 전용 Work Queue(app_wq) 를 만든다.
 *      → 모든 상태 전이/UDP 송신이 이 큐에서만 실행된다(단일 컨텍스트).
 *   2) 각 모듈을 의존성 순서대로 초기화한다.
 *      LED → 버튼 → UDP(큐 준비) → FSM(Thread 시작)
 *   3) 버튼 콜백을 FSM 이벤트로 연결한다.
 * ========================================================================= */

#include "app_main.h"
#include "app_config.h"
#include "app_fsm.h"
#include "app_btn.h"
#include "app_led.h"
#include "udp.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_main, LOG_LEVEL_DBG);

/* 애플리케이션 전용 Work Queue 와 그 스택 */
K_THREAD_STACK_DEFINE(g_app_wq_stack, APP_WQ_STACK_SIZE);
static struct k_work_q g_app_wq;

/* ---------------------------------------------------------------------------
 * on_btn_released_cb - 버튼 뗌 콜백 → FSM 이벤트로 변환
 *   app_btn 모듈이 (DK 라이브러리의 Work 컨텍스트에서) 호출한다.
 * ------------------------------------------------------------------------- */
static void on_btn_released_cb(uint8_t num, uint32_t dur_ms, void *user)
{
    ARG_UNUSED(user);

    struct fsm_evt evt = {
        .type         = FSM_EVT_BTN_RELEASED,
        .u.btn.num    = num,
        .u.btn.dur_ms = dur_ms,
    };
    fsm_evt_post(&evt);
}

/* ---------------------------------------------------------------------------
 * app_init - 초기화 순서 지휘
 * ------------------------------------------------------------------------- */
int app_init(void)
{
    LOG_INF("app init start");

    /* 1) 애플리케이션 Work Queue 시작 */
    k_work_queue_start(&g_app_wq,
                       g_app_wq_stack,
                       K_THREAD_STACK_SIZEOF(g_app_wq_stack),
                       APP_WQ_PRIORITY, NULL);

    /* 2) 출력(LED) → 입력(버튼) 초기화 */
    app_led_init();

    struct app_btn_callbacks btn_cbs = {
        .released = on_btn_released_cb,   /* 뗌 이벤트만 사용 */
    };
    app_btn_init(&g_app_wq, &btn_cbs, NULL);

    /* 3) UDP 모듈 준비(큐/Work). 소켓은 attach 후 FSM 이 연다. */
    udp_init(&g_app_wq);

    /* 4) FSM 시작 → 내부에서 thread_init() 호출로 Thread 구동 */
    app_fsm_init(&g_app_wq);
    
    LOG_INF("app init done");
    return 0;
}
