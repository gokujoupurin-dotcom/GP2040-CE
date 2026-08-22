#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "gp2040.h"
#include "gamepad.h" // Gamepadクラスやボタンマスクの定義に必要
#include <string.h>

// 接続するWi-Fiの情報（ハードコーディング）
#define WIFI_SSID "Buffalo-2G-FDA0"
#define WIFI_PASSWORD "nuu5a4v8anx33"

static struct udp_pcb* udp_server_pcb = NULL;

// リモート（UDP）から受信したボタンの状態を保持するマスク変数
static uint32_t remote_buttons_state = 0;

// 外部（main.cppなど）からグローバルな gp2040Core0 にアクセスするための宣言
extern GP2040 * gp2040Core0;

// gamepad.cpp から呼ばれる外部関数（ここでリモートのボタン状態を返す）
extern "C" uint32_t get_remote_buttons() {
    return remote_buttons_state;
}

// ボタン名文字列を GP2040 のボタンマスクに変換するヘルパー関数
static uint32_t get_button_mask_by_name(const char* name) {
    if (strcmp(name, "A") == 0) return GAMEPAD_MASK_B2; // 一般的な配置に合わせて適宜調整
    if (strcmp(name, "B") == 0) return GAMEPAD_MASK_B1;
    if (strcmp(name, "X") == 0) return GAMEPAD_MASK_B4;
    if (strcmp(name, "Y") == 0) return GAMEPAD_MASK_B3;
    if (strcmp(name, "L") == 0) return GAMEPAD_MASK_L1;
    if (strcmp(name, "R") == 0) return GAMEPAD_MASK_R1;
    if (strcmp(name, "MINUS") == 0) return GAMEPAD_MASK_S1;
    if (strcmp(name, "PLUS") == 0) return GAMEPAD_MASK_S2;
    if (strcmp(name, "HOME") == 0) return GAMEPAD_MASK_A1;
    if (strcmp(name, "CAPTURE") == 0) return GAMEPAD_MASK_A2;
    return 0;
}

// UDPパケットを受信したときの処理
static void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        char buffer[64];
        int len = p->tot_len;
        if (len >= sizeof(buffer)) {
            len = sizeof(buffer) - 1;
        }
        
        pbuf_copy_partial(p, buffer, len, 0);
        buffer[len] = '\0';

        // --- パケットの解析 (例: "BTN_A_ON", "BTN_A_OFF" など) ---
        // プレフィックス "BTN_" で始まる場合の処理
        if (strncmp(buffer, "BTN_", 4) == 0) {
            // 例: "BTN_A_ON" からボタン名とアクションを切り出す
            // 形式: BTN_[ボタン名]_[ON/OFF]
            char btn_name[32];
            char action[8];
            
            // "BTN_" の後ろの文字列を解析
            char *p_underscore = strrchr(buffer, '_');
            if (p_underscore != NULL) {
                // アクション部分 (ON または OFF)
                strcpy(action, p_underscore + 1);
                
                // ボタン名部分を抽出
                int name_len = p_underscore - (buffer + 4);
                if (name_len > 0 && name_len < sizeof(btn_name)) {
                    strncpy(btn_name, buffer + 4, name_len);
                    btn_name[name_len] = '\0';

                    uint32_t mask = get_button_mask_by_name(btn_name);
                    if (mask != 0) {
                        if (strcmp(action, "ON") == 0) {
                            remote_buttons_state |= mask;  // ビットを立てる（ON）
                        } else if (strcmp(action, "OFF") == 0) {
                            remote_buttons_state &= ~mask; // ビットを落とす（OFF）
                        }
                    }
                }
            }
        }

        pbuf_free(p);
    }
}

extern "C" void init_wifi_udp_input() {
    if (cyw43_arch_init()) {
        return;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000) != 0) {
        return;
    }

    udp_server_pcb = udp_new();
    if (udp_server_pcb != NULL) {
        udp_bind(udp_server_pcb, IP_ADDR_ANY, 4444);
        udp_recv(udp_server_pcb, udp_recv_callback, NULL);
    }
}

extern "C" void poll_wifi_udp_input() {
    cyw43_arch_poll();
}