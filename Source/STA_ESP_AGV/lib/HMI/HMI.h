#ifndef HMI_H
#define HMI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========== Callback functions for UART communication ==========
typedef void (*hmi_send_string_fn)(const char* str);  // gửi chuỗi
typedef int  (*hmi_available_fn)(void);               // số byte có thể đọc
typedef int  (*hmi_read_byte_fn)(void);               // đọc 1 byte (-1 nếu không có)

// ========== HMI Initialization ==========
void HMI_init(hmi_send_string_fn send_str,
              hmi_available_fn available,
              hmi_read_byte_fn read_byte);

// ========== Update HMI (call in main loop) ==========
void HMI_update(void);

// ========== Send position value to HMI display ==========
void HMI_send_position(uint8_t value);

// ========== HMI Button Status Functions ==========
uint8_t HMI_is_EMG(void);   // return 1 if EMG button pressed
uint8_t HMI_is_HOME(void);  // return 1 if HOME button pressed
uint8_t HMI_is_CNT(void);   // return 1 if CONTINUE button pressed

// ========== Clear button flags ==========
void HMI_clear_EMG(void);
void HMI_clear_HOME(void);
void HMI_clear_CNT(void);

// ========== AGV Position enum ==========
typedef enum {
    AGV_POS_HOME = 0,
    AGV_POS_P1 = 1,     // B1/B2
    AGV_POS_P2 = 2,     // B3/B4
    AGV_POS_P3 = 3,     // B5/B6
    AGV_POS_P4 = 4,     // B7/B8
    AGV_POS_P5 = 5,     // B9/B10
    AGV_POS_MOVING = 6,
} AGV_Position_t;

// Global AGV position
extern AGV_Position_t g_agv_position;

#ifdef __cplusplus
}
#endif

#endif




