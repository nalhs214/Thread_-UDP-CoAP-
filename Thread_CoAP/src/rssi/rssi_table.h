/*
 * Copyright (c) 2024 Samsung Corp
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rssi_table.h
 * @brief 스캔된 비콘별 RSSI 측정값을 관리하는 테이블 인터페이스이다.
 *
 * 비콘 MAC 주소를 키로 RSSI(최신값), 샘플 수, 추정 거리를 고정 크기
 * 배열에 보관한다. 테이블이 가득 차면 가장 오래 보고된 항목을 evict한다.
 * 갱신/스냅샷/clear는 mutex로 보호되어 런타임 호출이 안전하다.
 */

#ifndef BEACON_TABLE_H_
#define BEACON_TABLE_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief 비콘 테이블이 보관하는 최대 항목 수이다(동시 청취 ~10 + 전이 여유). */
#define BEACON_TABLE_MAX_ENTRIES  16

/** @brief 비콘 1개의 측정 상태를 담는 테이블 항목이다. */
struct beacon_entry {
	uint8_t  addr[6];       /**< @brief BLE MAC address(little-endian)이다. */
	int8_t   rssi;          /**< @brief 최신 측정 RSSI(정수 dBm)이다. */
	uint8_t  sample_count;  /**< @brief 지금까지 수집한 샘플 수이다. */
	int16_t  distance_cm;   /**< @brief RSSI 기반 추정 거리(cm)이다. */
	bool     valid;         /**< @brief 항목이 사용 중이면 true이다. */
	bool     seen;          /**< @brief 현재 보고 주기에 광고가 수신됐으면 true이다. */
	uint32_t last_report;   /**< @brief 마지막으로 보고에 포함된 report_tick이다(신규=0, LRU 로테이션용). */
};

/**
 * @brief 비콘 테이블을 초기화한다.
 *
 * 테이블 배열을 0으로 비운다. 부팅 시 1회만 호출되므로 mutex를 사용하지 않는다.
 */
void beacon_table_init(void);

/**
 * @brief 비콘 테이블 전체를 비운다.
 *
 * 모든 항목을 0으로 지운다. mutex로 보호되어 런타임 호출이 안전하다.
 */
void beacon_table_clear(void);

/**
 * @brief 비콘 RSSI 샘플로 테이블을 갱신한다.
 *
 * addr가 이미 존재하면 최신 RSSI로 덮어쓰고 sample_count를 증가시키며 거리를
 * 재계산한다. 신규 비콘이면 빈 슬롯을 쓰거나, 없으면 가장 오래 보고된(last_report가
 * 가장 작은) 항목을 evict한다. mutex로 보호된다.
 *
 * @param addr 비콘 BLE MAC 주소(6 bytes, little-endian)이다.
 * @param rssi 이번에 측정된 RSSI(dBm)이다.
 * @param measured_power 비콘의 1m 캘리브레이션 RSSI(dBm)이다. 거리 계산에 쓰인다.
 */
void beacon_table_update(const uint8_t *addr, int8_t rssi, int8_t measured_power);

/**
 * @brief 현재 valid 비콘들을 out[]에 복사한다.
 *
 * mutex로 보호된 스냅샷을 호출자에게 제공한다. 최대 max개까지 복사한다.
 *
 * @param out 항목을 복사받을 배열이다.
 * @param max 복사할 최대 개수이다.
 * @return 실제로 복사된 항목 수이다.
 */
int beacon_table_snapshot(struct beacon_entry *out, int max);

/**
 * @brief 이번 주기 present 비콘 중 보고할 항목을 LRU 로테이션으로 선택한다.
 *
 * 한 번의 lock 안에서: (1) 이번 주기에 광고가 수신된(seen=true) 비콘만 present로
 * 모으고 seen을 리셋하며, 광고가 없던(seen=false) 항목은 valid=false로 제거한다.
 * (2) present가 max_send 이하이면 전부, 초과하면 last_report가 작은(가장 오래 안 보낸)
 * 순서로 max_send개를 고른다. 동률이면 RSSI가 강한 쪽을 우선한다. (3) 선택된 항목의
 * last_report를 이번 tick으로 갱신해 다음 주기엔 미선택분이 우선되도록 한다.
 *
 * 보고 주기마다 1회 호출하는 것을 전제로 하며, "보고 주기 = presence 판단 창"이 된다.
 *
 * @param out           선택된 비콘을 복사받을 배열이며 최소 max_send개를 보장해야 한다.
 * @param max_send      한 번에 보낼 최대 비콘 수이다.
 * @param total_present 이번 주기 present 총수를 받을 포인터이다(로깅용, NULL 허용).
 * @return out[]에 채워진(선택된) 비콘 수이다(<= max_send).
 */
int beacon_table_collect_for_report(struct beacon_entry *out, int max_send,
				    int *total_present);

#endif /* BEACON_TABLE_H_ */
