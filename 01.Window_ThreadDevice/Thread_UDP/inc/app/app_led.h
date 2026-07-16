#ifndef APP_LED_H
#define APP_LED_H

#include <stdbool.h>

/* nRF52840 DK LED4개 (0-based, dk_set_led 인덱스와 일치) */
typedef enum {
    LED_THREAD = 0,   /* LED1: Thread 네트워크 연결 상태 */
    LED_ROLE   = 1,   /* LED2: 노드 역할(Leader 등)      */
    LED_TX     = 2,   /* LED3: UDP 송신 표시             */
    LED_RX     = 3,   /* LED4: UDP 수신 시 토글          */
} app_led_t;

void app_led_init(void);
void app_led_set(app_led_t led, bool on);
void app_led_toggle(app_led_t led);

#endif /* APP_LED_H */
