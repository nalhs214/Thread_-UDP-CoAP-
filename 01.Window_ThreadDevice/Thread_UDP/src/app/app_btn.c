/* ===========================================================================
 * app_btn.c  —  버튼 입력 모듈
 *
 * 역할:
 *   nRF52840 DK 의 버튼 4개(sw0~sw3) 눌림/뗌을 감지하여
 *   상위 계층에 "pressed / released(눌린 시간 포함)" 콜백으로 전달한다.
 *
 * 의존:
 *   - dk_buttons_and_leds (CONFIG_DK_LIBRARY=y)
 *       dk_buttons_init(handler) : 버튼 초기화 + 상태변화 핸들러 등록
 *       핸들러는 (button_state, has_changed) 비트마스크를 넘겨준다.
 *
 * 디바운스:
 *   DK 라이브러리가 내부적으로 디바운스(짧은 채터링 제거)를 처리한 뒤
 *   콜백을 호출하므로, 이 모듈에서 별도 디바운스는 하지 않는다.
 *   (명세서의 "디바운스 후 처리" 요구를 DK 라이브러리가 충족)
 *
 * 실행 컨텍스트:
 *   DK 라이브러리의 버튼 핸들러는 시스템 Work Queue 컨텍스트에서 불린다.
 *   따라서 여기서 콜백을 호출해도 ISR 이 아니며, 그 안에서
 *   k_msgq_put / k_work_submit 같은 커널 API 를 안전하게 쓸 수 있다.
 * ========================================================================= */

#include "app_btn.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(app_btn, LOG_LEVEL_DBG);

/* 상위에서 등록한 콜백/사용자 포인터 보관 */
static struct app_btn_callbacks s_cbs;
static void                    *s_user;

/* 각 버튼이 "눌린 시각"(ms). release 시점에 빼서 눌린 시간을 계산한다. */
static int64_t s_press_time[4];

/* 버튼 번호 → DK 비트마스크 (DK_BTN1_MSK = bit0 ... DK_BTN4_MSK = bit3) */
static const uint32_t s_btn_mask[4] = {
    DK_BTN1_MSK, DK_BTN2_MSK, DK_BTN3_MSK, DK_BTN4_MSK,
};

/* ---------------------------------------------------------------------------
 * button_handler - DK 라이브러리가 버튼 상태 변화 시 호출하는 핸들러
 *
 *   button_state : 현재 눌림 상태 비트마스크 (1=눌림)
 *   has_changed  : 이번에 상태가 바뀐 버튼 비트마스크
 *
 *   네 개 버튼을 순회하며, 바뀐 버튼에 대해 눌림/뗌을 구분해 콜백을 부른다.
 * ------------------------------------------------------------------------- */
static void button_handler(uint32_t button_state, uint32_t has_changed)
{
    for (uint8_t i = 0; i < 4; i++) {
        uint32_t mask = s_btn_mask[i];

        /* 이 버튼은 이번에 바뀌지 않았으면 건너뛴다 */
        if (!(has_changed & mask)) {
            continue;
        }

        if (button_state & mask) {
            /* ── 눌림(press) ── */
            s_press_time[i] = k_uptime_get();      /* 눌린 시각 기록 */
            LOG_DBG("btn%u pressed", i);
            if (s_cbs.pressed) {
                s_cbs.pressed(i, s_user);
            }
        } else {
            /* ── 뗌(release) ── 눌린 시각과의 차이 = 눌린 시간(ms) */
            int64_t dur = k_uptime_get() - s_press_time[i];
            LOG_DBG("btn%u released (%lld ms)", i, dur);
            if (s_cbs.released) {
                s_cbs.released(i, (uint32_t)dur, s_user);
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * app_btn_init - 버튼 모듈 초기화
 * ------------------------------------------------------------------------- */
int app_btn_init(struct k_work_q *wq,
                 const struct app_btn_callbacks *cbs,
                 void *user)
{
    ARG_UNUSED(wq);   /* 현재는 DK 라이브러리가 자체 컨텍스트를 쓰므로 미사용 */

    if (cbs == NULL) {
        return -EINVAL;
    }

    /* 콜백/사용자 포인터 저장 */
    s_cbs  = *cbs;
    s_user = user;

    /* DK 라이브러리로 버튼 초기화 + 핸들러 등록 */
    int err = dk_buttons_init(button_handler);
    if (err) {
        LOG_ERR("dk_buttons_init failed: %d", err);
        return err;
    }

    return 0;
}
