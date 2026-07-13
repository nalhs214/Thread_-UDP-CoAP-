# Thread_UDP 사용자 가이드

nRF52840 DK 여러 대를 **Thread 메시 네트워크**로 묶고, 버튼을 눌러
**UDP 메시지로 다른 보드의 LED를 제어**하는 예제 애플리케이션입니다.

- **플랫폼**: Zephyr RTOS + nRF Connect SDK (OpenThread FTD)
- **타깃 보드**: Nordic nRF52840 DK (`nrf52840dk/nrf52840`)
- **핵심 동작**: 버튼 → UDP 송신 → 상대 보드 LED 토글 (유니캐스트 / 멀티캐스트)

---

## 1. 무엇을 하는 프로그램인가

여러 개의 보드가 같은 Thread 망에 자동으로 참여합니다. 각 보드는:

- **Button 1**을 누르면 → 지정한 **한 대**(유니캐스트)의 LED4를 토글
- **Button 2**를 누르면 → 망에 있는 **모든 보드**(멀티캐스트)의 LED4를 토글
- **Button 4**를 누르면 → 자기 IPv6 주소를 로그에 출력 (유니캐스트 대상 확인용)

메시지를 **받은** 보드는 명령대로 LED4를 켜거나/끄거나/토글합니다.

---

## 2. 보드 LED / 버튼 매핑

### LED

| LED | 이름 | 의미 |
|-----|------|------|
| LED1 | `LED_THREAD` | Thread 망에 **연결되면 ON**, 이탈하면 OFF |
| LED2 | `LED_ROLE`   | 이 노드가 **Leader이면 ON** (Router/Child는 OFF) |
| LED3 | `LED_TX`     | UDP를 **송신할 때** 토글 (송신 표시) |
| LED4 | `LED_RX`     | UDP를 **수신할 때** 토글 (실제 제어 대상) |

> 정의 위치: [inc/app/app_led.h](inc/app/app_led.h)

### 버튼

| 버튼 | 동작 | 조건 |
|------|------|------|
| Button 1 | 유니캐스트 송신 (특정 노드 LED 토글) | `UDP_READY` 상태에서만 |
| Button 2 | 멀티캐스트 송신 (전체 노드 LED 토글) | `UDP_READY` 상태에서만 |
| Button 3 | (예약, 동작 없음) | — |
| Button 4 | 내 IPv6 주소 로그 출력 | 항상 |

> 버튼 처리 위치: [src/app/app_fsm.c:76](src/app/app_fsm.c#L76)

`UDP_READY` 상태가 되기 전(= LED1이 켜지기 전)에 Button 1/2를 누르면
`btn ignored (not UDP_READY)` 경고만 뜨고 무시됩니다.

---

## 3. 빌드 & 플래시

nRF Connect SDK 환경(`west`)이 설정되어 있어야 합니다.

```bash
# 프로젝트 루트에서
west build -b nrf52840dk/nrf52840 .

# 보드를 USB로 연결한 뒤
west flash
```

로그를 보려면 보드의 VCOM 시리얼 포트(기본 115200 8N1)를 터미널로 엽니다.

```bash
# 예: nRF Connect Serial Terminal, 또는
minicom -D /dev/ttyACM0 -b 115200
```

**같은 펌웨어를 사용할 보드 모두에 플래시**합니다. 네트워크 파라미터가
동일하므로 별도 설정 없이 같은 망에 붙습니다.

---

## 4. 사용 순서 (2대 이상 기준)

1. **모든 보드에 플래시 후 전원 인가.**
2. 잠시 뒤 각 보드의 **LED1이 켜지면** Thread 망 참여 완료입니다.
   - 한 대는 **LED2도 켜집니다** → 그 보드가 Leader입니다.
   - 로그에 `state -> UDP_READY (버튼 송수신 가능)` 이 뜨면 준비 끝.
3. **멀티캐스트 테스트 (가장 간단)**
   - 아무 보드에서 **Button 2**를 누릅니다.
   - → 망의 **모든 보드**(자신 포함 여부는 주소 설정에 따라 다름)의
     LED4가 토글됩니다. 누른 보드는 LED3(TX)도 토글됩니다.
4. **유니캐스트 테스트**
   - 대상 보드에서 **Button 4**를 눌러 로그에 찍힌 IPv6 주소 중
     **`fd`로 시작하는 Mesh-Local EID**를 확인합니다.
   - 그 주소를 [inc/app/app_config.h](inc/app/app_config.h)의
     `UDP_PEER_ADDR`에 넣고 **송신할 보드**를 다시 빌드/플래시합니다.
   - 송신 보드에서 **Button 1**을 누르면 → 그 한 대의 LED4만 토글됩니다.

---

## 5. 설정 값 (config)

네트워크·주소·포트는 [inc/app/app_config.h](inc/app/app_config.h)에 모여 있습니다.
**같은 망에 붙이려면 모든 보드의 아래 3개 값이 동일해야 합니다.**

| 매크로 | 기본값 | 설명 |
|--------|--------|------|
| `THREAD_CHANNEL` | `15` | 2.4GHz 채널 (11~26) |
| `THREAD_PAN_ID` | `0xABCD` | 16-bit PAN ID |
| `THREAD_NETWORK_KEY` | `00 11 22 … ff` | 16-byte 네트워크 키 |
| `UDP_PORT` | `1212` | 송·수신 공통 UDP 포트 |
| `UDP_MULTICAST_ADDR` | `ff03::1` | 멀티캐스트(전체 노드) 대상 |
| `UDP_PEER_ADDR` | `ff02::1` | 유니캐스트(Button 1) 대상 — **실제 보드의 `fd..` 주소로 교체** |

> `UDP_PEER_ADDR`의 기본값 `ff02::1`은 링크-로컬 전체 노드라서, 교체 전에는
> Button 1도 인접 노드 전체에 도달합니다. 특정 한 대만 제어하려면 위 4번
> 절차대로 대상 보드의 Mesh-Local EID로 바꾸세요.

메시지 종류는 [inc/udp/udp.h](inc/udp/udp.h)에 정의되어 있습니다
(`MSG_TYPE_LED_TOGGLE / _ON / _OFF`). 현재 버튼은 항상 `TOGGLE`을 보냅니다.

---

## 6. 동작 흐름 (상태 머신)

애플리케이션의 모든 사건(Thread 상태 변화, UDP 수신, 버튼)은 하나의 이벤트
큐로 모여 **단일 Work Queue(app_wq) 컨텍스트**에서 순서대로 처리됩니다
(동시성 문제 원천 차단). 상태 전이는 다음과 같습니다.

```
SYS_INIT
   │  app_init() → 각 모듈 초기화, thread_init()
   ▼
THREAD_INIT ──────────── Thread 파라미터 설정 후 망 참여 대기
   │  (역할이 Child/Router/Leader로 잡히면 콜백)
   ▼
THREAD_ATTACHED ──────── LED1 ON, Leader면 LED2 ON, udp_open()
   │  (소켓 bind 성공)
   ▼
UDP_READY ────────────── 버튼 송·수신 가능 ★
```

망에서 이탈하면(`DETACHED`) LED1/LED2가 꺼지고 `THREAD_INIT`으로 되돌아갑니다.

> 상태 머신 구현: [src/app/app_fsm.c](src/app/app_fsm.c)

---

## 7. 코드 구조

```
src/
  main.c              진입점 — app_init() 호출 후 잠듦
  app/
    app_main.c        초기화 오케스트레이션 (LED→버튼→UDP→FSM)
    app_fsm.c         상태 머신 (이 프로젝트의 "두뇌")
    app_btn.c         버튼 입력 (DK 라이브러리 래핑)
    app_led.c         LED 출력 (DK 라이브러리 래핑)
  thread/
    thread.c          OpenThread 초기화 / 역할 변화 감지 (ot* 는 여기에만)
  udp/
    udp.c             UDP 송수신 (Zephyr BSD 소켓 zsock_*)
inc/                  위 각 모듈의 헤더 (bare-name include)
```

**계층 원칙 (중요)**
- **`ot*`(OpenThread) API 는 오직 thread.c 안에서만** 사용합니다.
  Thread 고유 정보가 필요하면 `thread_role_is_leader()`처럼 Zephyr 값(bool)으로
  감싸 노출하므로, UDP·FSM·app 계층은 OpenThread 타입을 보지 않습니다.
- **UDP 는 Zephyr BSD 소켓**으로 구현됩니다(`zsock_socket/bind/sendto/recvfrom`).
  송신은 `zsock_sendto` 직접 호출, 수신은 전용 스레드가 `zsock_recvfrom`으로
  받아 `k_msgq`에 넣고 FSM에 알립니다.

**컨텍스트 규칙 (중요)**
- 상태 전이·UDP 송신은 **app_wq**에서만 실행됩니다.
- thread.c가 app_wq 등 외부 스레드에서 `ot*` API를 호출할 때는 반드시
  `openthread_api_mutex_lock/unlock`으로 감쌉니다.
- 단, OpenThread 콜백(상태변화) 안에서는 이미 락이 잡혀 있으므로
  다시 락하지 않습니다(데드락 방지).

---

## 8. 문제 해결 (Troubleshooting)

| 증상 | 확인할 것 |
|------|-----------|
| LED1이 안 켜짐 (망 참여 안 됨) | 모든 보드의 `CHANNEL / PAN_ID / NETWORK_KEY`가 동일한지 확인 |
| Button 1/2가 무시됨 | `UDP_READY` 도달 전. LED1이 켜지고 로그에 `UDP_READY`가 뜰 때까지 대기 |
| 멀티캐스트는 되는데 유니캐스트만 안 됨 | `UDP_PEER_ADDR`를 대상 보드의 `fd..` Mesh-Local EID로 교체했는지 확인 |
| 수신은 되는데 LED4 반응 없음 | 수신 보드 로그의 `UDP RX: type=... seq=...` 출력 여부 확인 |
| 송신 로그는 뜨는데 상대가 못 받음 | 두 보드가 같은 채널/PAN에 있고 무선 도달 거리 내인지 확인 |

로그 태그별 의미: `ot_thread`(Thread), `ot_udp`(UDP 송수신),
`app_fsm`(상태 전이), `app_led`/`app_btn`(입출력).
