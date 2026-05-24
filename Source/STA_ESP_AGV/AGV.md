# AGV.md

Tai lieu ngan ve cac truong du lieu va giao tiep hien co trong `src/main.cpp`.

## Vai tro cac node

- `ESP32 Controller / WebServer`: chay `main.cpp`, giu web server, REST API, WebSocket, quan ly order va relay lenh AGV.
- `AGV ESP / AGV Client`: ket noi vao `ws://<ESP_IP>/agv`, nhan lenh tu controller, dieu khien motor/encoder/nap khay, gui feedback ve controller.
- `index.html`: giao dien khach theo ban, truy cap `/1` den `/10`, chi dat mon va thanh toan.
- `kitchen.html`: giao dien bep, truy cap `/100`, xem don, doi trang thai mon/don, gui lenh AGV.
- `Remote ESP khac`: bo dieu khien tu xa cho AGV, nen ket noi `/agv`, khong can thiep order.

Mo hinh hien tai la `Controller ESP = server`, `AGV ESP = client`. AGV khong host web order; AGV chi nhan command va gui telemetry/feedback.

## Node truy cap server

- `GET /1` den `GET /10`: giao dien khach tung ban, tra `index.html`.
- `GET /100`: giao dien bep, tra `kitchen.html`.
- `GET /api/menu`: tra danh sach mon.
- `GET /api/order?table=N`: tra don cua ban `N`.
- `GET /api/order`: tra tat ca don dang hoat dong.
- `GET /api/status`: trang thai ESP, gom `fw`, `ip`, `uptime_ms`, `ws_clients`, `agv_clients`, `free_heap`, `active_tables`.
- `GET /debug/files`: debug HTML/SPIFFS.
- `WS /ws`: realtime cho `index.html` va `kitchen.html`.
- `WS /agv`: kenh rieng cho AGV/remote controller.
- `GET /img/...`: anh mon an trong SPIFFS neu co upload filesystem.

## Struct / enum quan trong

### `MonAn_t`

ID mon an tu `0x01` den `0x10`. Gia tri ID duoc dung de map anh `img<ID>.jpg`.

- `MON_NONE = 0x00`
- `xui_cao_hap = 0x01`
- `mi_van_than = 0x02`
- `vit_quay_bac_kinh = 0x03`
- `ha_cao_tom = 0x04`
- `com_chien_duong_chau = 0x05`
- `dau_phu_tu_xuyen = 0x06`
- `mi_xao_hai_san = 0x07`
- `canh_chua_ca = 0x08`
- `banh_bao_nhan_dau = 0x09`
- `che_thai = 0x0A`
- `tra_dao = 0x0B`
- `nuoc_cam_ep = 0x0C`
- `tra_sua_tran_chau = 0x0D`
- `bia_333 = 0x0E`
- `nuoc_suoi = 0x0F`
- `ca_phe_sua_da = 0x10`

### `MonAnInfo`

```cpp
struct MonAnInfo {
    MonAn_t  id;
    char     ten[32];
    uint32_t gia;
    char     loai[12];
};
```

Dung cho menu tinh: ten mon, gia, loai mon.

### `MonStatus_t`

- `MON_CHUA_NAU = 0`
- `MON_DANG_NAU = 1`
- `MON_XONG = 2`
- `MON_DA_GIAO = 3`

### `MonAn_Status_t`

```cpp
typedef struct {
    MonAn_t  type;
    uint8_t  Status;
    uint32_t Gia;
    uint8_t  Soluong;
} MonAn_Status_t;
```

Dung cho tung mon trong mot don hang.

### `TrangThaiDon_t`

- `DON_TRONG = 0x00`
- `DON_DANG_XU_LY = 0x01`
- `DON_DA_GIAO = 0x02`
- `DON_THANH_TOAN = 0x03`
- `DON_CHUA_TT = 0x04`

Khi xuat JSON, cac trang thai la: `trong`, `dang_xu_ly`, `da_giao`, `thanh_toan`, `chua_thanh_toan`.

### `DonHang_Status_t`

```cpp
typedef struct {
    uint8_t  Trang_thai;
    uint16_t ID;
    MonAn_Status_t mon_an[MAX_MON_AN];
    uint8_t  so_mon;
    uint32_t tong_gia_tri;
    uint32_t timestamp;
} DonHang_Status_t;
```

Day la struct chinh de AGV doc don, biet ban nao can giao, co may mon, tong tien va trang thai.

## index -> ESP

### Lay menu

```http
GET /api/menu
```

Response:

```json
{
  "menu": [
    {"id":1,"ten":"...","gia":45,"loai":"Mon_chinh","img":"img1.jpg"}
  ]
}
```

### Gui don moi

```http
POST /api/order
Content-Type: application/json
```

Body:

```json
{
  "table_id": 3,
  "mon_an": [
    {"type": 1, "soluong": 2},
    {"type": 7, "soluong": 1}
  ]
}
```

Response:

```json
{"ok":true,"table_id":3,"tong_gia":155}
```

ESP se tao `DonHang_Status_t`, set `Trang_thai = DON_DANG_XU_LY`, tinh tong tien, va broadcast `don_moi` qua `/ws`.

### Thanh toan

```http
POST /api/payment
```

Body:

```json
{"table_id":3}
```

ESP set `Trang_thai = DON_THANH_TOAN` va broadcast `don_update`.

### Realtime khach

`index.html` ket noi:

```text
ws://<ESP_IP>/ws
```

Khach hien tai chu yeu nhan:

```json
{"event":"don_update","table_id":3,"trang_thai":"da_giao","tong_gia":155}
```

## kitchen -> ESP

`kitchen.html` ket noi:

```text
ws://<ESP_IP>/ws
```

### Lay tat ca don

```json
{"cmd":"get_all"}
```

Response:

```json
{
  "don_hang": [
    {"table_id":3,"trang_thai":"dang_xu_ly","tong_gia":155,"so_mon":2,"timestamp":123456}
  ]
}
```

### Doi trang thai don

```json
{"cmd":"set_status","table_id":3,"status":"da_giao"}
```

`status` hop le: `dang_xu_ly`, `da_giao`, `thanh_toan`, `chua_thanh_toan`.

### Doi trang thai tung mon

```json
{"cmd":"set_mon_status","table_id":3,"mon_index":0,"mon_status":2}
```

`mon_status`: `0 chua_nau`, `1 dang_nau`, `2 xong`, `3 da_giao`.

### Xoa don

```json
{"cmd":"clear_table","table_id":3}
```

ESP goi `clearDon(table_id)` va broadcast update.

### Gui lenh AGV tu bep

```json
{
  "cmd": "agv_goto",
  "pos": "P2",
  "table": 3,
  "s1": "Mon slot 1",
  "s2": "Mon slot 2"
}
```

ESP hien tai relay nguyen JSON nay sang tat ca client o `/agv`.

### Lenh khac tu kitchen hien co trong HTML

HTML co gui:

```json
{"cmd":"agv_goto","pos":"Home","table":0}
```

```json
{"cmd":"agv_estop"}
```

Luu y: `main.cpp` hien moi xu ly `agv_goto` tren `/ws`; `agv_estop` chua co case xu ly, nen can bo sung neu dung that.

## AGV / Remote ESP -> ESP

Kenh rieng:

```text
ws://<ESP_IP>/agv
```

Khi ket noi thanh cong, ESP gui:

```json
{"event":"hello","role":"agv"}
```

Moi JSON AGV gui len `/agv` se duoc ESP relay sang kitchen `/ws` va them:

```json
{"_relay_from":"agv"}
```

De remote ESP dieu khien AGV ma khong can thiep order, nen dung `/agv` va format lenh rieng, vi du:

```json
{"cmd":"manual","move":"forward","speed":120}
```

```json
{"cmd":"manual","move":"backward","speed":120}
```

```json
{"cmd":"manual","move":"stop"}
```

```json
{"cmd":"tray","slot":1,"action":"open"}
```

```json
{"cmd":"tray","slot":1,"action":"close"}
```

```json
{"event":"agv_fb","state":"moving","pos":"P2","encoder_l":1234,"encoder_r":1230,"s1":"...","s2":"..."}
```

Phan parser cho cac lenh remote nay chua duoc implement trong `main.cpp`; hien tai `/agv` moi nhan feedback va relay ve kitchen.

## Controller ESP -> AGV Client

AGV client hien tai se nhan message qua `/agv` khi bep bam gui AGV:

```json
{
  "cmd": "agv_goto",
  "pos": "P2",
  "table": 3,
  "s1": "Mon slot 1",
  "s2": "Mon slot 2",
  "trip": 1
}
```

Trong do:

- `cmd`: loai lenh, hien co `agv_goto`.
- `pos`: diem den logic, vi du `Home`, `P1`, `P2`, ...
- `table`: ban can giao, `0` thuong la quay ve Home.
- `s1`, `s2`: ten mon/khay tren slot 1 va slot 2.
- `trip`: luot giao neu mot don can tach thanh nhieu chuyen.

Kitchen HTML co gui them `{"cmd":"agv_estop"}`, nhung `main.cpp` chua relay/xu ly lenh nay; nen can bo sung truoc khi AGV client phu thuoc vao no.

## JSON don hang day du

`buildDonJson()` tao dang:

```json
{
  "table_id": 3,
  "trang_thai": "dang_xu_ly",
  "tong_gia": 155,
  "timestamp": 123456,
  "so_mon": 2,
  "mon_an": [
    {
      "type": 1,
      "soluong": 2,
      "gia_don": 45,
      "tong_mon": 90,
      "status": "chua_nau",
      "img": "img1.jpg",
      "ten": "Xui cao hap",
      "loai": "Mon_chinh"
    }
  ]
}
```

## Diem can code tiep cho AGV

- Them struct rieng cho AGV: vi tri hien tai, target, encoder trai/phai, trang thai motor, trang thai nap khay.
- Them parser lenh `/agv`: `manual`, `goto`, `estop`, `tray`.
- Khi nhan `agv_goto` tu bep, chuyen thanh target AGV noi bo thay vi chi relay WebSocket.
- Gui feedback dinh ky qua `/ws` cho kitchen: `event=agv_fb`, `pos`, `state`, `encoder_l`, `encoder_r`, `s1`, `s2`.
- Tach logic dieu khien motor ra cac ham non-blocking de web server khong bi treo.
