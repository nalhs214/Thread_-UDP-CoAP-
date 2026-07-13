/**
 * @file thread_role.h
 * @brief Thread device role 변화 감지 및 처리 인터페이스이다.
 *
 * OpenThread의 device role(leader, router, child 등)이 바뀔 때마다 그에 맞춰
 * BLE advertising/scanning 동작을 전환하는 모듈의 초기화 진입점을 제공한다.
 * leader는 gateway 역할을, 그 외 role은 RSSI 보고용 node 역할을 수행한다.
 */
#ifndef THREAD_ROLE_H_
#define THREAD_ROLE_H_

/**
 * @brief Thread role 변화 콜백을 등록하여 role 기반 동작 전환을 활성화한다.
 *
 * role 처리를 수행할 k_work를 초기화하고 OpenThread state-changed callback을
 * 등록한다. OpenThread 스택 초기화 이후에 한 번 호출되어야 한다.
 */
void thread_role_init(void);

#endif
