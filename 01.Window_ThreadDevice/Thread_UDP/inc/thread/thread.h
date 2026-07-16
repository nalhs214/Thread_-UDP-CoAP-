#ifndef THREAD_H
#define THREAD_H

#include <stdbool.h>

/* ---------------------------------------------------------------------------
 * thread.h  —  Thread 네트워크 계층 인터페이스
 *
 *   app_config.h 의 파라미터(채널/PAN ID/네트워크 키)로 Thread 망을
 *   코드에서 직접 구성하고 시작한다. (OpenThread CLI 미사용)
 *
 *   ★ OpenThread(ot*) 타입/함수는 thread.c 안에만 있다. 이 인터페이스는
 *     Zephyr 세계의 타입(bool 등)만 노출한다.
 * ------------------------------------------------------------------------- */

/* Thread 초기화:
 *   1) 상태 변화 콜백 등록
 *   2) 채널 / PAN ID / 네트워크 키 설정
 *   3) IPv6 인터페이스 up → Thread 프로토콜 시작
 * 반환: 0 성공, 음수 실패 */
int thread_init(void);

/* 이 노드가 현재 Thread 망에서 Leader 역할인지 반환한다.
 *   내부에서 OpenThread 역할 조회를 뮤텍스로 감싸 처리한다. */
bool thread_role_is_leader(void);

/* 현재 이 노드가 가진 IPv6 주소들을 로그로 출력한다.
 *   망 참여(attach) 직후 호출하면, 다른 보드에 유니캐스트를 보낼 때 쓸
 *   Mesh-Local EID(fd..) 를 확인할 수 있다. */
void thread_log_addresses(void);

#endif /* THREAD_H */
