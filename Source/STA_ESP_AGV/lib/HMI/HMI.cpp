#include "HMI.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

// Global AGV position state
AGV_Position_t g_agv_position = AGV_POS_HOME;

// ========== HMI State ==========
static struct {
    hmi_send_string_fn send_str;
    hmi_available_fn available;
    hmi_read_byte_fn read_byte;

    uint8_t flag_emg;
    uint8_t flag_home;
    uint8_t flag_cnt;

    uint8_t rx_buffer[64];
    uint8_t rx_idx;
    uint8_t term_count; // count consecutive 0xFF bytes
} hmi_state = {0};

// ========== Initialization ==========
void HMI_init(hmi_send_string_fn send_str,
              hmi_available_fn available,
              hmi_read_byte_fn read_byte)
{
    hmi_state.send_str = send_str;
    hmi_state.available = available;
    hmi_state.read_byte = read_byte;

    hmi_state.flag_emg = 0;
    hmi_state.flag_home = 0;
    hmi_state.flag_cnt = 0;
    hmi_state.rx_idx = 0;
    hmi_state.term_count = 0;

    Serial.println("[HMI] initialized");
}

// ========== Send position value to HMI ==========
void HMI_send_position(uint8_t value)
{
    char cmd[32];
    // Nextion requires three 0xFF terminators
    snprintf(cmd, sizeof(cmd), "h0.val=%u\xFF\xFF\xFF", value);
    if (hmi_state.send_str) {
        hmi_state.send_str(cmd);
    }
    Serial.printf("[HMI TX] %s\n", cmd);
}

// ========== Parse received data from HMI ==========
static void hmi_parse_command(const char* cmd)
{
    if (!cmd) return;
    Serial.printf("[HMI RX] '%s'\n", cmd);

    if (strcmp(cmd, "C_EMG") == 0) {
        hmi_state.flag_emg = 1;
    }
    else if (strcmp(cmd, "C_HOM") == 0) {
        hmi_state.flag_home = 1;
    }
    else if (strcmp(cmd, "C_CNT") == 0) {
        hmi_state.flag_cnt = 1;
    }
}

// ========== Update HMI (process incoming data) ==========
void HMI_update(void)
{
    if (!hmi_state.available || !hmi_state.read_byte) {
        return;
    }

    int available_count = hmi_state.available();

    while (available_count > 0) {
        int byte = hmi_state.read_byte();

        if (byte == -1) {
            break;
        }

        // HMI sends commands like: C_EMG.C_HOM.C_CNT.
        // Every '.' marks the end of a command from HMI.
        if (byte == '.') {
            hmi_state.rx_buffer[hmi_state.rx_idx] = 0;
            if (hmi_state.rx_idx > 0) {
                hmi_parse_command((const char*)hmi_state.rx_buffer);
            }
            hmi_state.rx_idx = 0;
        }
        else if (byte == '\r' || byte == '\n') {
            // ignore line endings if they exist
        }
        else if (hmi_state.rx_idx < sizeof(hmi_state.rx_buffer) - 1) {
            hmi_state.rx_buffer[hmi_state.rx_idx++] = (uint8_t)byte;
        }
        else {
            // overflow — reset frame
            hmi_state.rx_idx = 0;
        }

        available_count--;
    }
}

// ========== Button Status Functions ==========
uint8_t HMI_is_EMG(void)
{
    return hmi_state.flag_emg;
}

uint8_t HMI_is_HOME(void)
{
    return hmi_state.flag_home;
}

uint8_t HMI_is_CNT(void)
{
    return hmi_state.flag_cnt;
}

// ========== Clear button flags ==========
void HMI_clear_EMG(void)
{
    hmi_state.flag_emg = 0;
}

void HMI_clear_HOME(void)
{
    hmi_state.flag_home = 0;
}

void HMI_clear_CNT(void)
{
    hmi_state.flag_cnt = 0;
}
