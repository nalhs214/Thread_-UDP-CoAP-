/* ===========================================================================
 * main.c  —  진입점
 *
 *   초기화는 전부 app_init() 에 위임한다.
 *   이후 모든 동작은 Work Queue / OpenThread 스레드에서 이벤트 기반으로
 *   진행되므로, main 스레드는 할 일이 없어 영원히 잠든다.
 * ========================================================================= */

#include "app_main.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    int err = app_init();
    if (err) {
        LOG_ERR("app_init failed: %d", err);
        return err;
    }

    /* main 스레드는 더 이상 할 일이 없다.
     * (실제 로직은 app_wq / OpenThread 스레드에서 이벤트로 처리) */
    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
