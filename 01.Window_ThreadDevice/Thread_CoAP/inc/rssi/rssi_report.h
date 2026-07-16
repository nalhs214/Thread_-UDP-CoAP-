/*
 * Copyright (c) 2024 Samsung Corp
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rssi_report.h
 * @brief 비콘 RSSI 측정값을 leader로 주기적으로 UDP 송신하는 인터페이스이다.
 *
 * delayable work를 통해 일정 주기로 비콘 테이블 스냅샷을 떠 leader에
 * RSSI_SCAN_RESULT_IND 프레임을 보낸다. OT 메시지 풀 고갈을 막기 위해 한
 * tick당 최대 4개 태그만 라운드로빈으로 보내고, 다중 노드 동시 TX를 막기
 * 위해 EUI-64 기반 시작 오프셋으로 송신 위상을 스태거링한다.
 */

#ifndef BEACON_REPORT_H_
#define BEACON_REPORT_H_

#include <stdint.h>

/**
 * @brief 주기적 비콘 RSSI UDP 보고를 시작한다.
 *
 * 진행 중인 work를 취소한 뒤 interval_ms를 보고 주기로 설정한다. EUI-64
 * 하위 16비트로 [0, interval_ms) 범위의 노드별 초기 오프셋을 계산해 첫
 * work를 스태거링한다. 이후 주기는 변경되지 않고 위상만 분산된다.
 *
 * @param interval_ms 보고 주기(ms)이다. 0이면 재스케줄하지 않는다.
 */
void beacon_report_start(uint32_t interval_ms);

/**
 * @brief 주기적 비콘 RSSI UDP 보고를 중지한다.
 *
 * delayable work를 취소하고 보고 주기를 0으로 설정한다.
 */
void beacon_report_stop(void);

#endif /* BEACON_REPORT_H_ */
