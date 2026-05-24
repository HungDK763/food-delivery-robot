# STA_ESP_AGV Project Notes

## Muc tieu

Firmware ESP32 cho he thong nha hang co dat mon qua web, man hinh bep va ket noi AGV qua WebSocket.

## Phan cung / nen tang

- Board PlatformIO: `esp32dev`
- Framework: Arduino ESP32
- Luu file web/anh: SPIFFS

## Thu vien chinh

- `ESPAsyncWebServer`: web server bat dong bo, REST API, WebSocket
- `AsyncTCP`: TCP async dung cho ESP32
- `ArduinoJson`: parse/tao JSON
- `WiFi`, `SPIFFS`: thu vien Arduino ESP32

## Chuc nang trong `src/main.cpp`

- Web khach: route `/1` den `/10`
- Web bep: route `/100`
- REST API:
  - `GET /api/menu`
  - `GET /api/order?table=N`
  - `GET /api/status`
  - `POST /api/order`
  - `POST /api/payment`
- WebSocket:
  - `/ws`: bep/khach realtime
  - `/agv`: giao tiep voi AGV
- Quan ly 10 ban, toi da 16 mon an.

## Loi da gap

Build bi loi vi PlatformIO keo them `RPAsyncTCP`, day la thu vien cho Raspberry Pi Pico/RP2040, khong phu hop voi `esp32dev`. Ngoai ra `ESPAsyncWebServer` 3.x goi `AsyncServer::status()` tren object `const`, nen `AsyncTCP` local can co `status() const`.

## Fix da ap dung

- Them `lib_ignore = ESPAsyncTCP, RPAsyncTCP` vao `platformio.ini`.
- Sua `AsyncServer::status()` trong `lib/AsyncTCP-master/src/AsyncTCP.cpp` thanh ham `const`.

## Luu y

Nen chi dung `AsyncTCP` cho ESP32. Khong dung `RPAsyncTCP` trong du an nay.
