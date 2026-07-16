#ifndef APP_BTN_H
#define APP_BTN_H

#include <stdint.h>
#include <zephyr/kernel.h>

/* ---------------------------------------------------------------------------
 * 버튼 콜백 묶음
 *   상위 계층(app_main)이 원하는 콜백만 채워서 등록한다.
 *   NULL 로 두면 해당 이벤트는 무시된다.
 *
 *   pressed  : 버튼이 눌린 순간
 *   released : 버튼에서 손을 뗀 순간 (dur_ms = 눌려 있던 시간)
 *
 *   num      : 버튼 번호 (0=Button1/sw0 ... 3=Button4/sw3)
 *   user     : app_btn_init 에 넘긴 사용자 포인터 (컨텍스트 전달용)
 * ------------------------------------------------------------------------- */
struct app_btn_callbacks {
    void (*pressed)(uint8_t num, void *user);
    void (*released)(uint8_t num, uint32_t dur_ms, void *user);
};

/* 버튼 모듈 초기화.
 *   wch  : 상위에서 쓰는 Work Queue (인터페이스 통일용, 현재는 참조만)
 *   cbs  : 콜백 묶음
 *   user : 콜백에 그대로 전달될 사용자 포인터
 * 반환: 0 성공, 음수 실패 */
int app_btn_init(struct k_work_q *wq,
                 const struct app_btn_callbacks *cbs,
                 void *user);

#endif /* APP_BTN_H */
