#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"

static volatile uint32_t remote_buttons = 0;
static bool udp_initialized = false;

// UDP パケット受信コールバック
void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        if (p->len >= 2) {
            remote_buttons = *((uint16_t*)p->payload);
        }
        pbuf_free(p);
    }
}

// UDP サーバーの立ち上げ処理
static void setup_udp_server() {
    struct udp_pcb *pcb = udp_new();
    if (pcb != NULL) {
        err_t err_bind = udp_bind(pcb, IP_ADDR_ANY, 5000);
        if (err_bind == ERR_OK) {
            udp_recv(pcb, udp_recv_callback, NULL);
            printf("[Wi-Fi] UDP server listening on port 5000\n");
            udp_initialized = true;
        } else {
            printf("[Wi-Fi] ERROR: Failed to bind UDP port 5000\n");
        }
    } else {
        printf("[Wi-Fi] ERROR: Failed to create UDP PCB\n");
    }
}

// 初期化関数（USBを待たせない非同期処理）
extern "C" void init_wifi_udp_input() {
    printf("[Wi-Fi] Initializing CYW43 architecture...\n");
    
    if (cyw43_arch_init()) {
        printf("[Wi-Fi] ERROR: Failed to initialize CYW43!\n");
        return;
    }

    cyw43_arch_enable_sta_mode();
    printf("[Wi-Fi] Starting async Wi-Fi connection...\n");

    const char* wifi_ssid = "Buffalo-2G-FDA0";
    const char* wifi_pass = "nuu5a4v8anx33";

    // 接続処理をバックグラウンドで開始（待たずにすぐ次に進む）
    cyw43_arch_wifi_connect_async(wifi_ssid, wifi_pass, CYW43_AUTH_WPA2_AES_PSK);
}

// 毎ループ実行される関数
extern "C" void poll_wifi_udp_input() {
    // Wi-Fi イベントのポーリング
    cyw43_arch_poll();

    // 接続状態の監視（接続完了したら 1 度だけ UDP サーバーを初期化）
    if (!udp_initialized) {
        int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        if (status == CYW43_LINK_JOIN) {
            printf("[Wi-Fi] Connected successfully!\n");
            uint8_t *ip = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
            printf("[Wi-Fi] IP Address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
            
            setup_udp_server();
        }
    }
}

// Core0 からボタン状態を取得する関数
extern "C" uint32_t get_remote_buttons() {
    return remote_buttons;
}