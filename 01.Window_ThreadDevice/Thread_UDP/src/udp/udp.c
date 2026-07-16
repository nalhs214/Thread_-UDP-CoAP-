/* ===========================================================================
 * udp.c  —  UDP 송수신 모듈 (Zephyr BSD 소켓)
 *
 *   OpenThread 네이티브 UDP API 대신 Zephyr 표준 소켓(zsock_*)을 사용한다.
 *   덕분에 이 파일에는 OpenThread(ot*) 코드가 전혀 없다.
 *     - Thread 스택 위에 올라간 IPv6/UDP 를 일반 소켓처럼 다룰 수 있다.
 *     - 뮤텍스로 감싸야 하는 ot* 호출도, 수동 메시지 버퍼 관리도 사라진다.
 *
 * 송신 경로:
 *   udp_send_unicast/multicast()   [FSM(app_wq) 컨텍스트]
 *     → 주소 문자열 파싱 → zsock_sendto()   (소켓은 thread-safe)
 *
 * 수신 경로:
 *   udp_rx_thread()                [전용 수신 스레드]
 *     → zsock_recvfrom() 블로킹 → RX 메시지 큐(k_msgq)에 넣고
 *     → FSM_EVT_UDP_RX 이벤트 post → FSM 이 udp_rx_get() 으로 꺼내 처리
 * ========================================================================= */

#include "udp.h"
#include "app_config.h"
#include "app_fsm.h"

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(udp, LOG_LEVEL_INF);

/* 수신 큐 슬롯 개수 / 수신 스레드 파라미터 */
#define UDP_RX_QUEUE_LEN    16
#define UDP_RX_STACK_SIZE   2048
#define UDP_RX_PRIORITY     5

/* 수신 프레임 대기 큐 (수신 스레드 → FSM) */
K_MSGQ_DEFINE(g_rx_q, sizeof(udp_frame_t), UDP_RX_QUEUE_LEN, 4);

/* UDP 소켓 fd (열리기 전엔 -1) */
static int g_sock = -1;

/* 수신 스레드 */
static K_THREAD_STACK_DEFINE(g_rx_stack, UDP_RX_STACK_SIZE);
static struct k_thread g_rx_thread;

/* ---------------------------------------------------------------------------
 * make_addr - "ff03::1" 같은 문자열 IPv6 주소 → sockaddr_in6
 * ------------------------------------------------------------------------- */
static int make_addr(const char *ip, struct sockaddr_in6 *out)
{
    memset(out, 0, sizeof(*out));
    out->sin6_family = AF_INET6;
    out->sin6_port   = htons(UDP_PORT);

    if (zsock_inet_pton(AF_INET6, ip, &out->sin6_addr) != 1) {
        LOG_ERR("bad ipv6 addr: %s", ip);
        return -EINVAL;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * udp_rx_thread - 소켓에서 블로킹으로 수신 → 큐 적재 → FSM 알림
 * ------------------------------------------------------------------------- */
static void udp_rx_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

    while (1) {
        udp_frame_t frame;
        memset(&frame, 0, sizeof(frame));

        struct sockaddr_in6 src;
        socklen_t           slen = sizeof(src);

        /* 페이로드를 udp_msg_t 크기만큼 수신, 송신자 주소는 src 로 받는다. */
        int n = zsock_recvfrom(g_sock, &frame.msg, sizeof(frame.msg), 0,
                               (struct sockaddr *)&src, &slen);
        if (n < 0) {
            LOG_ERR("recvfrom failed: %d", errno);
            k_msleep(100);      /* 폭주 방지 후 재시도 */
            continue;
        }

        frame.addr = src;            /* 누가 보냈는지 (FR-02-5) */
        frame.len  = (uint16_t)n;
        /* RX 큐에 넣고 FSM 에 알림. 큐가 가득 차면 이 프레임은 버린다. */
        if (k_msgq_put(&g_rx_q, &frame, K_NO_WAIT) != 0) {
            LOG_WRN("rx queue full, dropped");
            continue;
        }

        struct fsm_evt evt = { .type = FSM_EVT_UDP_RX };
        fsm_evt_post(&evt);
    }
}

/* ---------------------------------------------------------------------------
 * udp_init - 모듈 준비 (소켓은 아직 열지 않음)
 * ------------------------------------------------------------------------- */
int udp_init(struct k_work_q *wq)
{
    ARG_UNUSED(wq);   /* 소켓 방식에선 TX 용 Work Queue 가 필요 없다 */
    LOG_INF("udp module ready (socket-based)");
    return 0;
}

/* ---------------------------------------------------------------------------
 * udp_open - 소켓 열기 + 포트 바인딩 + 수신 스레드 시작 (attach 후 호출)
 * ------------------------------------------------------------------------- */
int udp_open(void)
{
    g_sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (g_sock < 0) {
        LOG_ERR("socket() failed: %d", errno);
        return -EIO;
    }

    /* 모든 로컬 주소에서 UDP_PORT 로 수신 */
    struct sockaddr_in6 bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin6_family = AF_INET6;
    bind_addr.sin6_addr   = in6addr_any;
    bind_addr.sin6_port   = htons(UDP_PORT);

    if (zsock_bind(g_sock, (struct sockaddr *)&bind_addr,
                   sizeof(bind_addr)) < 0) {
        LOG_ERR("bind() failed: %d", errno);
        zsock_close(g_sock);
        g_sock = -1;
        return -EIO;
    }

    /* 수신 전용 스레드 시작 (블로킹 recvfrom 루프) */
    k_thread_create(&g_rx_thread, g_rx_stack,
                    K_THREAD_STACK_SIZEOF(g_rx_stack),
                    udp_rx_thread, NULL, NULL, NULL,
                    UDP_RX_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&g_rx_thread, "udp_rx");

    LOG_INF("udp socket bound on port %d", UDP_PORT);

    /* 준비 완료 → FSM 에 알림 */
    struct fsm_evt evt = { .type = FSM_EVT_UDP_READY };
    fsm_evt_post(&evt);
    return 0;
}

/* ---------------------------------------------------------------------------
 * udp_send_unicast - 유니캐스트 전송
 * ------------------------------------------------------------------------- */
int udp_send_unicast(const char *dst_ipv6, const udp_msg_t *msg)
{
    struct sockaddr_in6 dst;
    int err = make_addr(dst_ipv6, &dst);
    if (err) {
        return err;
    }

    int n = zsock_sendto(g_sock, msg, sizeof(*msg), 0,
                         (struct sockaddr *)&dst, sizeof(dst));
    if (n < 0) {
        LOG_ERR("sendto() failed: %d", errno);
        return -EIO;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * udp_send_multicast - 멀티캐스트 전송(전체 노드)
 * ------------------------------------------------------------------------- */
int udp_send_multicast(const udp_msg_t *msg)
{
    return udp_send_unicast(UDP_MULTICAST_ADDR, msg);
}

/* ---------------------------------------------------------------------------
 * udp_rx_get - 수신 큐에서 프레임 하나 꺼내기
 * ------------------------------------------------------------------------- */
bool udp_rx_get(udp_frame_t *out)
{
    return k_msgq_get(&g_rx_q, out, K_NO_WAIT) == 0;
}
