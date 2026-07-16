/**
 * @file udp_tx.h
 * @brief Thread leader로의 UDP 송신 인터페이스이다.
 *
 * node가 Thread leader의 RLOC 주소로 UDP 메시지를 전송하기 위한 함수를 제공한다.
 * 문자열 메시지 송신과, ble_scan 등과 공유되는 raw 바이트 페이로드 송신을 지원한다.
 */
#ifndef UDP_TX_H_
#define UDP_TX_H_

#include <stdint.h>

/**
 * @brief 널 종료 문자열 메시지를 Thread leader로 UDP 전송한다.
 *
 * leader RLOC을 조회하여 해당 주소로 메시지를 보낸다. leader RLOC 조회나
 * 메시지 할당/전송에 실패하면 로그만 남기고 반환한다.
 *
 * @param msg 전송할 널 종료 문자열. 길이는 strlen으로 계산한다.
 */
void udp_send_to_leader(const char *msg);

/**
 * @brief raw 40바이트 CS 페이로드를 Thread leader로 UDP 전송한다.
 *
 * ble_scan과 공유되는 raw 바이트 송신 경로이다. leader RLOC을 조회하여
 * 해당 주소로 데이터를 보내며, 실패 시 로그만 남기고 반환한다.
 *
 * @param data 전송할 바이트 버퍼.
 * @param len 전송할 바이트 수.
 */
/* Raw 40-byte CS payload send (shared with ble_scan) */
void udp_send_raw_to_leader(const uint8_t *data, uint16_t len);

#endif
