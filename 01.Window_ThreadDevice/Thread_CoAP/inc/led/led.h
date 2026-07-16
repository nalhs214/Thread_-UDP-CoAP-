/*
 * Copyright (c) 2024 Samsung Corp
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file led.h
 * @brief 상태 표시용 LED(빨강/초록/파랑) 제어 API이다.
 *
 * 보드 overlay의 statusled-red/green/blue alias에 연결된 active-low LED를 제어한다.
 *
 * - 빨강: 전원 입력 시 점등(led_init)되어 계속 유지된다.
 * - 초록: Thread 네트워크 attach 여부를 표시한다.
 * - 파랑: BLE 상태(미연결·미광고=OFF / 광고중=1초 깜빡 / 연결=ON)를 표시한다.
 */

#ifndef LED_H_
#define LED_H_

#include <stdbool.h>

/** @brief 파랑 LED가 표시하는 BLE 상태이다. */
enum led_ble_state {
	LED_BLE_OFF,         /**< 미광고·미연결: 꺼짐 */
	LED_BLE_ADVERTISING, /**< 광고중: 1초 주기 깜빡 */
	LED_BLE_CONNECTED,   /**< 연결됨: 계속 ON */
};

/**
 * @brief LED GPIO를 초기화하고 빨강 LED를 점등한다.
 *
 * 세 LED를 출력으로 설정하고 빨강은 ON, 초록·파랑은 OFF 상태로 둔다.
 * 시스템 부팅 직후 한 번 호출해야 한다.
 */
void led_init(void);

/**
 * @brief Thread attach 상태에 따라 초록 LED를 켜거나 끈다.
 *
 * @param attached attach되었으면 true(ON), 아니면 false(OFF)이다.
 */
void led_set_thread_attached(bool attached);

/**
 * @brief BLE 상태에 따라 파랑 LED를 제어한다.
 *
 * @param state 표시할 BLE 상태이다.
 */
void led_set_ble_state(enum led_ble_state state);

/**
 * @brief CS 측위 데이터를 UDP로 전송했음을 초록 LED로 짧게 표시한다.
 *
 * 초록 LED를 현재 상태(Thread attach 기준)의 반대로 잠깐 반전시켰다가 복원해
 * "이 노드가 CS 측위 데이터를 보내는 중"임을 깜빡임으로 나타낸다. RSSI 전송에는
 * 호출하지 않으므로, 초록이 깜빡이지 않는 노드는 CS 측위를 못 하는(이니시에이터가
 * 죽은) 노드로 식별할 수 있다. 비차단(non-blocking)이며 복원은 delayable work로
 * 처리한다. 스레드 컨텍스트에서 호출해야 한다.
 */
void led_blink_cs_tx(void);

#endif /* LED_H_ */
