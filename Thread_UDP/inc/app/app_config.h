#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ---------------------------------------------------------------------------
 * Thread 네트워크 파라미터
 *   4대의 보드가 모두 동일한 값을 사용해야 같은 Thread 망에 참여한다.
 * ------------------------------------------------------------------------- */
#define THREAD_CHANNEL      15          /* 2.4GHz 15번 채널 (11~26) */
#define THREAD_PAN_ID       0xABCD      /* 16-bit PAN ID            */

/* Network Key (16 byte) — otNetworkKey.m8 초기화용 */
#define THREAD_NETWORK_KEY                                  \
    { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,       \
      0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff }

/* ---------------------------------------------------------------------------
 * UDP 파라미터
 * ------------------------------------------------------------------------- */
#define UDP_PORT            1212        /* 송수신 공통 포트 (NFR-04)  */

/* Realm-Local All-Nodes 멀티캐스트: 망 전체에 브로드캐스트 */
#define UDP_MULTICAST_ADDR  "ff03::1"

/* 
 * 유니캐스트(Button 1) 대상 주소.
 * 실제로는 상대 보드가 부팅 시 로그에 찍는 Mesh-Local EID(fd..)로 교체한다.
 * 초기값은 링크-로컬 All-Nodes 로 두어 인접 노드에 도달하게 한다.
 */
#define UDP_PEER_ADDR       "ff02::1"

/* ---------------------------------------------------------------------------
 * Work Queue
 * ------------------------------------------------------------------------- */
#define APP_WQ_STACK_SIZE   4096
#define APP_WQ_PRIORITY     5

#endif /* APP_CONFIG_H */
