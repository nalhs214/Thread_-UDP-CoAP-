/**
 * @file thread_cfg.h
 * @brief OpenThread 네트워크 설정 적용 인터페이스이다.
 *
 * Thread 네트워크의 운영 데이터셋(network key, PAN ID, channel 등)을 코드에
 * 하드코딩된 값으로 구성하고 활성화하는 진입점을 제공한다. 이를 통해 별도의
 * 커미셔닝 절차 없이 고정된 파라미터로 동일한 Thread 네트워크에 참여한다.
 */
#ifndef THREAD_CFG_H_
#define THREAD_CFG_H_

/**
 * @brief 하드코딩된 운영 데이터셋을 OpenThread 인스턴스에 적용하고 Thread를 활성화한다.
 *
 * 고정된 network key, extended PAN ID, mesh-local prefix, channel, PAN ID,
 * network name으로 active dataset을 구성한 뒤 IPv6와 Thread를 켠다.
 * OpenThread 스택이 초기화된 이후에 호출되어야 한다.
 */
void thread_cfg_apply(void);

#endif
