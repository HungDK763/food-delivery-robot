# Hướng dẫn sử dụng thư viện HMI (C) cho màn hình TJC T1 USART-HMI

Thư viện **HMI.h** / **HMI.c** viết bằng C thuần, dùng cho ESP32/Arduino/STM32, hỗ trợ giao tiếp với màn hình TJC dòng T1 qua USART.

---

## 📁 Cấu trúc thư viện

- `HMI.h` – khai báo các hàm, kiểu dữ liệu và callback.
- `HMI.c` – định nghĩa chi tiết các hàm.

---

## 🔌 Kết nối phần cứng

| Màn hình TJC T1 | ESP32 / Arduino |
|----------------|-----------------|
| TX             | RX (ví dụ GPIO16 nếu dùng Serial2) |
| RX             | TX (ví dụ GPIO17 nếu dùng Serial2) |
| GND            | GND             |
| VCC            | 5V hoặc 3.3V (tuỳ model) |

---

## ⚙️ Cấu hình baudrate

- **Tốc độ khuyến nghị:** `115200 bps`
- Phải khớp giữa **code** và **phần mềm USART HMI**:
  - Trong code: `Serial2.begin(115200);`
  - Trong USART HMI: mở file `Program.s`, sửa dòng `bauds=115200`

---

## 🧩 Các hàm callback cần cung cấp (wrapper)

Thư viện không gọi trực tiếp `Serial` mà dùng 4 con trỏ hàm. Bạn cần viết 4 hàm nhỏ để kết nối với cổng Serial thực tế (ví dụ `Serial2`):

```c
void my_send_string(const char* str);   // gửi chuỗi (không thêm 0xFF)
void my_send_byte(uint8_t b);           // gửi 1 byte
int  my_available(void);                // trả về số byte có thể đọc
int  my_read_byte(void);                // đọc 1 byte (-1 nếu không có)