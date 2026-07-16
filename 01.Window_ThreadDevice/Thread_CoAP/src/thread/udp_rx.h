/**
 * @file udp_rx.h
 * @brief UDP socket 수신 인터페이스이다.
 *
 * Thread 네트워크에서 leader가 node들로부터 CS/RSSI 페이로드를 수신하기 위한
 * UDP socket을 초기화하고, 송신 모듈이 공유할 수 있도록 socket 핸들을 노출한다.
 * 수신된 페이로드는 로그로 출력되고 CS queue로 전달된다.
 */
#ifndef UDP_RX_H_
#define UDP_RX_H_

#include <openthread/udp.h>

/** @brief UDP 송수신에 사용하는 포트 번호이다. 송신측과 동일한 값을 사용한다. */
#define UDP_PORT 1234

/**
 * @brief UDP socket을 열고 수신 포트에 bind하여 수신을 시작한다.
 *
 * OpenThread 인스턴스에 UDP socket을 열어 수신 callback을 등록하고
 * @ref UDP_PORT 에 bind한다. OpenThread 스택 초기화 이후에 호출되어야 한다.
 */
void udp_init(void);

/**
 * @brief 모듈 내부의 UDP socket 핸들을 반환한다.
 *
 * 송신 모듈(udp_tx)이 같은 socket으로 메시지를 보내도록 공유 핸들을 제공한다.
 *
 * @return 내부 정적 otUdpSocket의 포인터.
 */
otUdpSocket *udp_get_socket(void);

#endif
