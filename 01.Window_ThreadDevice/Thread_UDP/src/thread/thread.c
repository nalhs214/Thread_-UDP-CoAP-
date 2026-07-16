/* ===========================================================================
 * thread.c  —  Thread 네트워크 초기화 / 상태 감지
 *
 * 역할:
 *   OpenThread 네이티브 API 를 코드에서 직접 호출하여
 *     - 네트워크 파라미터(채널/PAN ID/네트워크 키)를 설정하고
 *     - IPv6 인터페이스를 올리고 Thread 프로토콜을 시작한다.
 *   또 노드의 역할 변화(Leader/Router/Child/Detached)를 콜백으로 받아
 *   FSM 이벤트로 변환해 던진다.
 *
 * ★ 계층 원칙:
 *   이 프로젝트에서 OpenThread(ot*) API 는 "오직 이 파일에서만" 쓴다.
 *   UDP(소켓)·FSM·app 계층은 Zephyr 타입/함수만 다룬다.
 *   Thread 고유 정보(예: 역할이 Leader인가)가 필요하면, 이 파일이
 *   thread_role_is_leader() 처럼 Zephyr 세계의 값(bool)으로 감싸 노출한다.
 *
 * 중요 - OpenThread API 뮤텍스:
 *   Zephyr 에서 OpenThread 스택은 별도 스레드에서 동작한다.
 *   따라서 다른 스레드(여기서는 app_wq)에서 ot* API 를 호출할 때는
 *   반드시 openthread_api_mutex_lock/unlock 으로 감싸야 한다.
 *   단, 상태변화 콜백(on_thread_state_changed)은 이미 OpenThread
 *   컨텍스트에서 불리므로 그 안에서는 다시 lock 하면 안 된다(데드락).
 * ========================================================================= */

#include "thread.h"
#include "app_config.h"
#include "app_fsm.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>      /* openthread_get_default_* / api_mutex */

#include <openthread/thread.h>
#include <openthread/link.h>
#include <openthread/ip6.h>
#include <openthread/instance.h>

LOG_MODULE_REGISTER(ot_thread, LOG_LEVEL_INF);

/* ---------------------------------------------------------------------------
 * OpenThread 핸들 캐시
 *   Zephyr 가 소유·생성한 OT 인스턴스/컨텍스트를 thread_init() 에서 한 번만
 *   받아 보관한다. 이후 ot* 호출부에서 매번 get_default_*() 를 부르지 않아
 *   "Zephyr 접근자 + OT API" 가 뒤섞여 보이는 노이즈를 줄인다.
 * ------------------------------------------------------------------------- */
static otInstance                *s_inst;   /* OT 인스턴스 핸들      */
static struct openthread_context *s_ctx;    /* 뮤텍스 등 Zephyr 문맥 */

/* 역할(enum)을 사람이 읽을 수 있는 문자열로 변환 (로그용) */
static const char *role_str(otDeviceRole role)
{
    switch (role) {
    case OT_DEVICE_ROLE_DISABLED: return "DISABLED";
    case OT_DEVICE_ROLE_DETACHED: return "DETACHED";
    case OT_DEVICE_ROLE_CHILD:    return "CHILD";
    case OT_DEVICE_ROLE_ROUTER:   return "ROUTER";
    case OT_DEVICE_ROLE_LEADER:   return "LEADER";
    default:                      return "UNKNOWN";
    }
}

/* ---------------------------------------------------------------------------
 * on_thread_state_changed - OpenThread 상태 변화 콜백
 *
 *   flags 에 어떤 항목이 바뀌었는지 비트로 들어온다.
 *   여기서는 "역할(Role) 변화"만 관심 있으므로 그 외에는 무시한다.
 *
 *   ★ 이 함수는 OpenThread 스레드 컨텍스트에서 호출된다.
 *      → ot* API(otThreadGetDeviceRole)는 락 없이 바로 호출 가능.
 *      → FSM 으로는 fsm_evt_post 로 비동기 전달(직접 처리 X).
 *      → 역할값은 FSM 으로 넘기지 않는다. 필요 시 FSM 이
 *        thread_role_is_leader() 로 조회한다(OT 타입을 FSM 에 노출 안 함).
 * ------------------------------------------------------------------------- */
static void on_thread_state_changed(otChangedFlags flags, void *context)
{
    ARG_UNUSED(context);    // 사용하지 않는 변수나 파라미터 경고 x

    /* 역할이 바뀐 경우가 아니면 관심 없음 */
    if (!(flags & OT_CHANGED_THREAD_ROLE)) {
        return;
    }

    otDeviceRole role = otThreadGetDeviceRole(s_inst);
    LOG_INF("Thread role changed -> %s", role_str(role));

    struct fsm_evt evt;

    switch (role) {
    case OT_DEVICE_ROLE_LEADER:
    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_CHILD:
        /* 망에 붙었다 → ATTACHED 이벤트 (역할은 FSM 이 필요할 때 조회) */
        evt.type = FSM_EVT_THREAD_ATTACHED;
        fsm_evt_post(&evt);
        break;

    case OT_DEVICE_ROLE_DETACHED:
    case OT_DEVICE_ROLE_DISABLED:
        /* 망에서 떨어졌다 → DETACHED 이벤트 */
        evt.type = FSM_EVT_THREAD_DETACHED;
        fsm_evt_post(&evt);
        break;

    default:
        break;
    }
}

/* ---------------------------------------------------------------------------
 * thread_init - Thread 파라미터 설정 + 네트워크 시작
 *   app_wq 컨텍스트에서 호출된다 → OT API 는 뮤텍스로 보호.
 * ------------------------------------------------------------------------- */
int thread_init(void)
{
    /* Zephyr 가 만든 OT 핸들을 한 번만 받아 캐싱 */
    s_inst = openthread_get_default_instance();
    s_ctx  = openthread_get_default_context();

    if (s_inst == NULL || s_ctx == NULL) {
        LOG_ERR("OpenThread instance/context not ready");
        return -ENODEV;
    }

    /* 아래 ot* 호출들을 하나의 임계구역으로 묶는다 */
    openthread_api_mutex_lock(s_ctx);

    /* 1) 상태 변화 콜백 등록 (역할 변화를 FSM 으로 전달하기 위함) */
    otSetStateChangedCallback(s_inst, on_thread_state_changed, NULL);

    /* 2) 네트워크 파라미터 설정 (4대가 동일해야 같은 망) */
    otLinkSetChannel(s_inst, THREAD_CHANNEL);
    otLinkSetPanId(s_inst, THREAD_PAN_ID);

    otNetworkKey key = { .m8 = THREAD_NETWORK_KEY };
    otThreadSetNetworkKey(s_inst, &key);

    /* 3) IPv6 인터페이스 up → Thread 시작
     *    반드시 IP6 먼저, 그 다음 Thread 순서. */
    otError err = otIp6SetEnabled(s_inst, true);
    if (err == OT_ERROR_NONE) {
        err = otThreadSetEnabled(s_inst, true);
    }

    openthread_api_mutex_unlock(s_ctx);

    if (err != OT_ERROR_NONE) {
        LOG_ERR("thread start failed: %d", err);
        return -EIO;
    }

    LOG_INF("Thread started (ch=%d, panid=0x%04x)",
            THREAD_CHANNEL, THREAD_PAN_ID);
    return 0;
}

/* ---------------------------------------------------------------------------
 * thread_role_is_leader - 이 노드가 현재 Leader 인가?
 *
 *   "역할이 Leader인가"는 Thread 고유 개념이라 Zephyr 에 대응 API 가 없다.
 *   그래서 OT 조회를 이 함수 하나에 가두고, 바깥(FSM)에는 bool 만 돌려준다.
 *   app_wq 등 OT 외부 스레드에서 불리므로 뮤텍스로 보호한다.
 * ------------------------------------------------------------------------- */
bool thread_role_is_leader(void)
{
    openthread_api_mutex_lock(s_ctx);
    otDeviceRole role = otThreadGetDeviceRole(s_inst);
    openthread_api_mutex_unlock(s_ctx);

    return role == OT_DEVICE_ROLE_LEADER;
}

/* ---------------------------------------------------------------------------
 * thread_log_addresses - 이 노드의 IPv6 유니캐스트 주소들을 로그로 출력
 *   다른 보드에 유니캐스트를 보낼 때 쓸 Mesh-Local EID(fd..)를 확인하는 용도.
 * ------------------------------------------------------------------------- */
void thread_log_addresses(void)
{
    openthread_api_mutex_lock(s_ctx);

    /* 유니캐스트 주소 연결 리스트를 순회하며 문자열로 출력 */
    for (const otNetifAddress *addr = otIp6GetUnicastAddresses(s_inst);
         addr != NULL; addr = addr->mNext) {

        char buf[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&addr->mAddress, buf, sizeof(buf));
        LOG_INF("  ipv6 addr: %s", buf);
    }

    openthread_api_mutex_unlock(s_ctx);
}
