/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

// Pi Pico includes
#include "pico/multicore.h"

// GP2040 includes
#include "gp2040.h"
#include "gp2040aux.h"

#include <cstdlib>

extern "C" void init_wifi_udp_input();
extern "C" void poll_wifi_udp_input();

// Custom implementation of __gnu_cxx::__verbose_terminate_handler() to reduce binary size
namespace __gnu_cxx {
void __verbose_terminate_handler()
{
    abort();
}
}

static GP2040 * gp2040Core0 = nullptr;
static GP2040Aux * gp2040Core1 = nullptr;

void core1() {
    gp2040Core1->setup();

    while (1) {
        gp2040Core1->run();
    }
}

int main() {
    // Create GP2040 Main Core (core0), Core1 is dependent on Core0
    gp2040Core0 = new GP2040();
    gp2040Core1 = new GP2040Aux();

    // ★ Core 0 で Wi-Fi を初期化（Core 1 を起動する前に呼ぶのが最善）
    init_wifi_udp_input();

    // Create GP2040 Main Core - Setup Core0
    gp2040Core0->setup();

    // Create GP2040 Thread for Core1
    multicore_launch_core1(core1);

    // Sync Core0 and Core1
    while(gp2040Core1->ready() == false ) {
        __asm volatile ("nop\n");
    }

    // GP2040 Main Loop
    while (1) {
        // ★ Core 0 のメインループで Wi-Fi のポーリング処理を実行
        poll_wifi_udp_input();

        // GP2040-CE の通常のメイン処理を実行
        // （gp2040Core0->run() は内部でループを持たない設計のため、明示的にループさせます）
        gp2040Core0->run();
    }

    return 0;
}