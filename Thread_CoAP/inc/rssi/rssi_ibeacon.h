/*
 * Copyright (c) 2024 Samsung Corp
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rssi_ibeacon.h
 * @brief BLE 광고에서 Ranging Service(RAS, 0x185B) 광고를 매칭하는 인터페이스이다.
 *
 * 스캔한 광고의 16-bit Service UUID 목록에서 RAS UUID
 * (BT_UUID_RANGING_SERVICE_VAL)를 찾아 CS Reflector 광고인지 판정한다.
 * RAS 광고에는 1m 캘리브레이션(measured power) 필드가 없어 다운스트림은
 * RSSI 값만 사용한다.
 */

#ifndef BEACON_IBEACON_H_
#define BEACON_IBEACON_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/bluetooth/bluetooth.h>

/** @brief RAS 광고 매칭 결과를 담는 구조체이다. */
struct ibeacon_match {
	int8_t measured_power;  /**< @brief 1m 캘리브레이션 RSSI이다. RAS 광고엔 없어 0으로 둔다. */
	bool   matched;         /**< @brief RAS UUID(0x185B)가 광고된 경우 true이다. */
};

/**
 * @brief BLE 광고에 Ranging Service(0x185B)가 포함됐는지 판정한다.
 *
 * adv_buf의 AD 요소를 순회하며 16-bit Service UUID 목록(BT_DATA_UUID16_ALL/SOME)에
 * RAS UUID가 있으면 out->matched를 true로 설정한다. RAS 광고엔 캘리브레이션 값이
 * 없으므로 out->measured_power는 0으로 둔다.
 *
 * @param adv_buf 파싱할 BLE 광고 데이터 버퍼이다.
 * @param out 파싱 결과를 받을 구조체이다. 호출 시 내부에서 초기화된다.
 */
void ble_ibeacon_parse(struct net_buf_simple *adv_buf, struct ibeacon_match *out);

#endif /* BEACON_IBEACON_H_ */
