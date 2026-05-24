# reqirement_AGV_client.md

Yeu cau ngan cho ESP nam tren AGV khi dong vai tro WebSocket client.

## Ket noi

- Ket noi WiFi cung mang voi ESP Controller.
- Ket noi WebSocket den `ws://<CONTROLLER_IP>/agv`.
- Khi ket noi thanh cong se nhan:

```json
{"event":"hello","role":"agv"}
```

- Tu dong reconnect khi mat WiFi hoac mat WebSocket.

## Lenh AGV phai nhan

### Di chuyen den diem giao

```json
{
  "cmd": "agv_goto",
  "pos": "P2",
  "table": 3,
  "s1": "Ten mon slot 1",
  "s2": "Ten mon slot 2",
  "trip": 1
}
```

AGV can:

- Parse `pos` thanh diem dich tren map.
- Luu `table`, `s1`, `s2`, `trip` de hien thi/log va dieu khien khay.
- Chay motor theo encoder den dung vi tri.
- Khi den noi, dung motor va gui feedback ve server.

Trong code AGV, packet nay nen duoc chuyen ve struct toi thieu:

```cpp
typedef struct {
    uint8_t Khay_nao;
    uint8_t ban_nao;
} mess_to_AGV_t;

typedef struct {
    mess_to_AGV_t mes[2];
    uint8_t status_home;
    uint8_t new_req;
} AGV_podcard_t;
```

`mes[0]` la khay 1, `mes[1]` la khay 2. `new_req = 1` khi co goi moi tu controller.

### Quay ve Home

```json
{"cmd":"agv_goto","pos":"Home","table":0}
```

AGV can quay ve vi tri goc va clear thong tin khay neu phu hop.

### Dung khan cap

```json
{"cmd":"agv_estop"}
```

Hien tai kitchen co gui lenh nay, nhung `main.cpp` chua relay/xu ly day du. Khi server duoc bo sung, AGV client phai dung motor ngay, tat dieu khien khay nguy hiem va gui feedback `estop`.

## Feedback AGV phai gui

Gui JSON ve cung socket `/agv`; controller se relay sang kitchen.

```json
{
  "event": "agv_fb",
  "status": "moving",
  "pos": "P2",
  "target": "P3",
  "table": 3,
  "encoder_l": 1234,
  "encoder_r": 1230,
  "s1": "Ten mon slot 1",
  "s2": "Ten mon slot 2"
}
```

`status` nen dung cac gia tri ngan:

- `idle`: dang cho.
- `moving`: dang di chuyen.
- `arrived`: da den diem.
- `delivering`: dang mo/dong khay.
- `returning`: dang ve Home.
- `error`: loi.
- `estop`: dung khan cap.

## Dieu khien phan cung can co

- Motor tien/lui/trai/phai hoac dieu toc 2 banh.
- Doc encoder trai/phai de tinh vi tri.
- Ham di den diem logic: `Home`, `P1`, `P2`, ...
- Dieu khien nap khay slot 1 va slot 2: open/close.
- Bao loi neu encoder/motor/nap khay timeout.
- Vong lap non-blocking, khong dung delay dai lam mat ket noi WebSocket.

## Nguyen tac quan trong

- AGV client khong tao order va khong sua order.
- AGV client chi nhan lenh AGV, chay phan cung, va gui feedback.
- Controller ESP trong `main.cpp` la server trung tam.
- Kitchen/index la web client; remote ESP dieu khien AGV cung nen di qua `/agv`.



```c
┌───────────────┐  Lệnh: agv_goto (Điểm P)   ┌─────────────────┐
      │     IDLE      ├───────────────────────────►│     MOVING      │
      │  (Đang chờ)   │◄──────────────────────────┤ (Đang di chuyển)│
      └──────┬────────┘    Lệnh: agv_goto (Home)   └────────┬────────┘
             │                                              │
             │                                              │ Đến vị trí đích
             │                                              ▼
             │ Lệnh:                               ┌─────────────────┐
             │ agv_estop                           │     ARRIVED     │
             │                                     │  (Đã đến điểm)  │
             │                                     └────────┬────────┘
             │                                              │
             │                                              │ Mở/Đóng khay món ăn
             │                                              ▼
             │                                     ┌─────────────────┐
             │                                     │   DELIVERING    │
             │                                     │  (Đang giao đồ) │
             │                                     └────────┬────────┘
             │                                              │ Giao xong
             ▼                                              ▼
      ┌───────────────┐                            ┌─────────────────┐
      │     ESTOP     │◄───────────────────────────┤    RETURNING    │
      │ (Dừng khẩn)   │     Lệnh: agv_estop        │ (Đang về Home)  │
      └───────────────┘                            └─────────────────┘
             ▲                                              │
             └────────────── Lệnh: agv_estop ───────────────┘
```
