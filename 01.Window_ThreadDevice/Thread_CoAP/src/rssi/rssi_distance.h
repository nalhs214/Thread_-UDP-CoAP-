/*
 * Copyright (c) 2024 Samsung Corp
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rssi_distance.h
 * @brief RSSI를 경로손실 모델 기반 거리(cm)로 변환하는 인터페이스이다.
 *
 * iBeacon이 보낸 measured_power(1m 캘리브레이션 RSSI)와 측정 RSSI의 차이를
 * 경로손실(path-loss)로 보고 거리를 추정한다. 강한 신호 영역과 RX 감도
 * 한계 영역은 신뢰도가 낮아 각각 클램프/saturate 처리한다.
 *
 * - path_loss <= 0 (1m 이내, 강한 신호): 100cm로 클램프한다.
 * - path_loss >= floor 임계: -96 dBm 등가 거리로 saturate한다.
 * - 그 외: 경로손실 모델로 계산한다.
 *
 * 캘리브레이션은 rssi_distance.c 안의 PATH_LOSS_EXPONENT_N 값만 조정한다.
 */

#ifndef BEACON_DISTANCE_H_
#define BEACON_DISTANCE_H_

#include <stdint.h>

/**
 * @brief RSSI를 경로손실 모델로 거리(cm)로 변환한다.
 *
 * path_loss = measured_power - rssi(정수 dBm)를 구한 뒤 영역별로 처리한다.
 * path_loss가 0 이하이면 100cm로 클램프하고, RX 감도 한계 이상이면 floor
 * 값으로 cap한 뒤 distance_m = 10^(path_loss / (10 * N)) 공식으로 계산한다.
 *
 * @param measured_power 비콘이 보낸 1m 캘리브레이션 RSSI(dBm).
 * @param rssi 측정된 RSSI(dBm).
 * @return 추정 거리(cm). INT16_MAX로 상한이 제한된다.
 */
int16_t rssi_to_distance_cm(int8_t measured_power, int8_t rssi);

#endif /* BEACON_DISTANCE_H_ */
