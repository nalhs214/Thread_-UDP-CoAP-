cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(Thread_UDP)

target_sources(app PRIVATE
    src/main.c
    src/app/app_main.c
    src/app/app_fsm.c
    src/app/app_btn.c
    src/app/app_led.c
    src/thread/thread.c
    src/udp/udp.c
)

# 모듈 헤더(inc/)를 bare-name(#include "xxx.h")으로 include 하기 위한 경로
target_include_directories(app PRIVATE
    inc/app
    inc/thread
    inc/udp
)
