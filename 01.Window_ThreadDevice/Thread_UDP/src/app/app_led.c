/* ===========================================================================
 * app_led.c  —  LED 출력 모듈
 *
 * 역할:
 *   nRF52840 DK 보드의 LED 4개를 제어한다.
 *   상위 계층(app_fsm 등)이 "LED_THREAD 켜라", "LED_RX 토글해라" 처럼
 *   의미 있는 이름으로 호출하면, 내부에서 DK 라이브러리 API로 변환한다.
 *
 * 의존:
 *   - dk_buttons_and_leds (CONFIG_DK_LIBRARY=y)
 *       dk_leds_init()          : LED GPIO 초기화
 *       dk_set_led(idx, val)    : idx 번(0~3) LED 를 val(0/1) 로 설정
 *
 * 설계 메모:
 *   DK 라이브러리에는 "토글" API 가 없다.
 *   그래서 각 LED 의 현재 on/off 상태를 이 모듈이 직접 배열로 기억해 두고,
 *   토글 요청이 오면 반대 값으로 다시 set 한다.
 * ========================================================================= */

#include "app_led.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

/* 이 파일의 로그를 "app_led" 태그로 출력하도록 등록 */
LOG_MODULE_REGISTER(app_led, LOG_LEVEL_INF);

/* 각 LED(0~3)의 현재 on/off 상태.
 * dk_set_led 는 상태를 읽어오는 기능이 없으므로 여기서 직접 추적한다. */
static bool s_state[4];

/* ---------------------------------------------------------------------------
 * app_led_init - LED 하드웨어 초기화
 *   부팅 시 1회 호출. 모든 LED 를 꺼진 상태로 맞춘다.
 * ------------------------------------------------------------------------- */
void app_led_init(void)
{
    /* DK 라이브러리로 LED GPIO 를 초기화한다. */
    int err = dk_leds_init();
    if (err) {
        LOG_ERR("dk_leds_init failed: %d", err);
        return;
    }

    /* 내부 상태 배열과 실제 LED 를 모두 OFF 로 동기화 */
    for (int i = 0; i < 4; i++) {
        s_state[i] = false;
        dk_set_led(i, 0);
    }
}

/* ---------------------------------------------------------------------------
 * app_led_set - 지정한 LED 를 켜거나 끈다
 *   led : LED_THREAD / LED_ROLE / LED_TX / LED_RX (0~3)
 *   on  : true = 켜기, false = 끄기
 * ------------------------------------------------------------------------- */
void app_led_set(app_led_t led, bool on)
{
    /* 현재 상태를 기억해 둔다 (다음 토글 계산에 사용) */
    s_state[led] = on;

    /* 실제 하드웨어 LED 반영 */
    dk_set_led((uint8_t)led, on ? 1 : 0);
}

/* ---------------------------------------------------------------------------
 * app_led_toggle - 지정한 LED 의 상태를 반전시킨다
 *   현재 켜져 있으면 끄고, 꺼져 있으면 켠다.
 *   주로 UDP 수신 표시(LED_RX)에 사용한다.
 * ------------------------------------------------------------------------- */
void app_led_toggle(app_led_t led)
{
    app_led_set(led, !s_state[led]);
}
