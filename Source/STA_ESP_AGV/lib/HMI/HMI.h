#ifndef HMI_H
#define HMI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========== Các hàm callback cần được cung cấp bởi người dùng ==========
typedef void (*tjc_send_string_fn)(const char* str);  // gửi chuỗi (không có kết thúc)
typedef void (*tjc_send_byte_fn)(uint8_t b);          // gửi 1 byte
typedef int  (*tjc_byte_available_fn)(void);          // trả về số byte có thể đọc
typedef int  (*tjc_read_byte_fn)(void);               // đọc 1 byte (hoặc -1 nếu không có)

// Callback khi nhận dữ liệu từ màn hình
// type: "text", "number", "unknown"
// value: giá trị dưới dạng chuỗi
typedef void (*tjc_data_callback_fn)(const char* type, const char* value);

// ========== Khởi tạo & cấu hình ==========
void tjc_init(tjc_send_string_fn send_str,
              tjc_send_byte_fn send_byte,
              tjc_byte_available_fn available,
              tjc_read_byte_fn read_byte);

void tjc_set_callback(tjc_data_callback_fn cb);

// ========== Gửi lệnh (tự động thêm 3 byte 0xFF) ==========
void tjc_send_command(const char* cmd);

// ========== Các lệnh tiện ích ==========
void tjc_set_page(const char* page_name);
void tjc_set_text(const char* component, const char* text);
void tjc_set_number(const char* component, int32_t value);
void tjc_beep(uint16_t duration_ms);
void tjc_set_brightness(uint8_t percent);   // 0-100%
void tjc_reset(void);

// ========== Yêu cầu đọc dữ liệu từ màn hình ==========
void tjc_request_text(const char* component);
void tjc_request_number(const char* component);

// ========== Xử lý dữ liệu nhận (gọi trong loop) ==========
void tjc_update(void);



// AGV Position enum
typedef enum {
    AGV_POS_HOME = 0,
    AGV_POS_P1   = 1,
    AGV_POS_P2   = 2,
    AGV_POS_P3   = 3,
    AGV_POS_P4   = 4,
    AGV_POS_P5   = 5,
    AGV_POS_MOVING = 6,
} AGV_Position_t;

// Global AGV position
extern AGV_Position_t g_agv_position;

// HMI control functions - TO BE IMPLEMENTED
uint8_t HMI_is_EMG();   // return 1 if EMG button pressed
uint8_t HMI_is_HOME();  // return 1 if HOME button pressed


#ifdef __cplusplus
}
#endif

#endif




