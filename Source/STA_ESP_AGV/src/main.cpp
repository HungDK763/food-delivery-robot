/*
 ╔══════════════════════════════════════════════════════════════════╗
 ║  ESP32 Restaurant Order System  —  firmware v2.0                 ║
 ║  File: main.ino                                                  ║
 ║                                                                  ║
 ║  Thay đổi so với v1:                                             ║
 ║  • Struct MonAn_t / MonAn_Status_t / DonHang_Status_t theo spec  ║
 ║  • 16 món ăn (MAX_MON_AN 16), enum MonAn_t đầy đủ                ║
 ║  • Route http://<IP>/1 .. /10  (khách), /100 (bếp)               ║
 ║  • AGV WebSocket port 82 riêng biệt                              ║
 ║  • Trạng thái thanh toán (PAID / UNPAID) theo yêu cầu            ║
 ║  • Ảnh map: imgN.jpg  (N = MonAn_t value)                        ║
 ║                                                                  ║
 ║  Thư viện cần cài (Library Manager):                             ║
 ║  • ESPAsyncWebServer  (lacamera/me-no-dev)                       ║
 ║  • AsyncTCP           (dvarrel/me-no-dev)                        ║
 ║  • ArduinoJson        v7.x  (bblanchon)                          ║
 ╚══════════════════════════════════════════════════════════════════╝
*/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>
#include "HMI.h"

extern const uint8_t data_index_html_start[] asm("_binary_data_index_html_start");
extern const uint8_t data_index_html_end[] asm("_binary_data_index_html_end");
extern const uint8_t data_kitchen_html_start[] asm("_binary_data_kitchen_html_start");
extern const uint8_t data_kitchen_html_end[] asm("_binary_data_kitchen_html_end");

// ──────────────────────────────────────────────────────────────────
//  CẤU HÌNH  — chỉnh tại đây
// ──────────────────────────────────────────────────────────────────
#define WIFI_SSID       "nha_hang_1977"
#define WIFI_PASSWORD   "12345678"

// #define WIFI_SSID       "Hao_Lang_Khong_Pass"
// #define WIFI_PASSWORD   ""


// Static IP -> config with IP.
IPAddress local_IP(192,168,1,102);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(1,1,1,1);


// // ──────────────────────────────────────────────────────────────────
// #define WIFI_SSID       "AiPhoneCuaHung"
// #define WIFI_PASSWORD   "88888888"

#define PORT_WEB        80    // khách + bếp
#define PORT_AGV        82    // AGV WebSocket riêng

#define NUM_TABLES      10    // bàn 1 → 10
#define ID_KITCHEN      100   // bếp truy cập /100
#define FW_VERSION      "2.1-html-embed"

// ──────────────────────────────────────────────────────────────────
//  ENUM MÓN ĂN  (MonAn_t) — 16 món theo spec người dùng
//  value = ID dùng để map ảnh: imgN.jpg
// ──────────────────────────────────────────────────────────────────
#define MAX_MON_AN  16

typedef enum : uint8_t {
    MON_NONE            = 0x00,   // không chọn
    xui_cao_hap         = 0x01,   // Xủi cảo hấp          — img1.jpg
    mi_van_than         = 0x02,   // Mì vằn thắn           — img2.jpg
    vit_quay_bac_kinh   = 0x03,   // Vịt quay Bắc Kinh     — img3.jpg
    ha_cao_tom          = 0x04,   // Há cảo tôm            — img4.jpg
    com_chien_duong_chau= 0x05,   // Cơm chiên Dương Châu  — img5.jpg
    dau_phu_tu_xuyen    = 0x06,   // Đậu phụ Tứ Xuyên      — img6.jpg
    mi_xao_hai_san      = 0x07,   // Mì xào hải sản        — img7.jpg
    canh_chua_ca        = 0x08,   // Canh chua cá          — img8.jpg
    // ─── Tráng miệng ───────────────────────────────────────────
    banh_bao_nhan_dau   = 0x09,   // Bánh bao nhân đậu     — img9.jpg
    che_thai            = 0x0A,   // Chè Thái              — img10.jpg
    // ─── Nước uống ─────────────────────────────────────────────
    tra_dao             = 0x0B,   // Trà đào               — img11.jpg
    nuoc_cam_ep         = 0x0C,   // Nước cam ép           — img12.jpg
    tra_sua_tran_chau   = 0x0D,   // Trà sữa trân châu     — img13.jpg
    bia_333             = 0x0E,   // Bia 333               — img14.jpg
    nuoc_suoi           = 0x0F,   // Nước suối             — img15.jpg
    ca_phe_sua_da       = 0x10,   // Cà phê sữa đá         — img16.jpg
} MonAn_t;

// ──────────────────────────────────────────────────────────────────
//  THÔNG TIN TĨNH TỪNG MÓN
// ──────────────────────────────────────────────────────────────────
struct MonAnInfo {
    MonAn_t  id;
    char     ten[32];
    uint32_t gia;       // nghìn đồng (VD: 45 = 45.000 VNĐ)
    char     loai[12];  // "Mon_chinh" | "Trang_miem" | "Nuoc"
};

// Đây là bảng tra cứu read-only, lưu trong flash (PROGMEM tuỳ chọn)
static const MonAnInfo MENU_TABLE[MAX_MON_AN] = {
    {xui_cao_hap,           "Xủi cảo hấp",          45, "Mon_chinh"},
    {mi_van_than,           "Mì vằn thắn",           42, "Mon_chinh"},
    {vit_quay_bac_kinh,     "Vịt quay Bắc Kinh",     85, "Mon_chinh"},
    {ha_cao_tom,            "Há cảo tôm",            48, "Mon_chinh"},
    {com_chien_duong_chau,  "Cơm chiên Dương Châu",  55, "Mon_chinh"},
    {dau_phu_tu_xuyen,      "Đậu phụ Tứ Xuyên",      38, "Mon_chinh"},
    {mi_xao_hai_san,        "Mì xào hải sản",        65, "Mon_chinh"},
    {canh_chua_ca,          "Canh chua cá",          40, "Mon_chinh"},
    {banh_bao_nhan_dau,     "Bánh bao nhân đậu",     25, "Trang_miem"},
    {che_thai,              "Chè Thái",              30, "Trang_miem"},
    {tra_dao,               "Trà đào",               28, "Nuoc"},
    {nuoc_cam_ep,           "Nước cam ép",           25, "Nuoc"},
    {tra_sua_tran_chau,     "Trà sữa trân châu",     32, "Nuoc"},
    {bia_333,               "Bia 333",               18, "Nuoc"},
    {nuoc_suoi,             "Nước suối",             10, "Nuoc"},
    {ca_phe_sua_da,         "Cà phê sữa đá",         22, "Nuoc"},
};

// Helper: tra cứu thông tin món theo ID
const MonAnInfo* getMonInfo(MonAn_t id) {
    if (id == MON_NONE) return nullptr;
    for (int i = 0; i < MAX_MON_AN; i++) {
        if (MENU_TABLE[i].id == id) return &MENU_TABLE[i];
    }
    return nullptr;
}

// ──────────────────────────────────────────────────────────────────
//  STRUCT TRẠNG THÁI MÓN ĂN — đã sửa: bỏ const Gia
// ──────────────────────────────────────────────────────────────────

// Trạng thái nấu từng món
typedef enum : uint8_t {
    MON_CHUA_NAU    = 0,
    MON_DANG_NAU    = 1,
    MON_XONG        = 2,
    MON_DA_GIAO     = 3,
} MonStatus_t;

typedef struct {
    MonAn_t  type;      // món gì
    uint8_t  Status;    // MonStatus_t — trạng thái nấu
    uint32_t Gia;       // giá (nghìn đồng) — copy từ MENU_TABLE
    uint8_t  Soluong;   // số lượng trong đơn
} MonAn_Status_t;

// ──────────────────────────────────────────────────────────────────
//  ENUM TRẠNG THÁI ĐƠN HÀNG
// ──────────────────────────────────────────────────────────────────
typedef enum : uint8_t {
    DON_TRONG       = 0x00,   // bàn chưa đặt
    DON_DANG_XU_LY  = 0x01,   // đang nấu / đang giao
    DON_DA_GIAO     = 0x02,   // tất cả món đã giao đến bàn
    DON_THANH_TOAN  = 0x03,   // khách đã bấm "Thanh toán"
    DON_CHUA_TT     = 0x04,   // đã giao nhưng chưa trả tiền (fallback)
} TrangThaiDon_t;

// ──────────────────────────────────────────────────────────────────
//  STRUCT ĐƠN HÀNG
// ──────────────────────────────────────────────────────────────────
typedef struct {
    uint8_t  Trang_thai;              // TrangThaiDon_t
    uint16_t ID;                      // ID bàn (1 → 10)
    MonAn_Status_t mon_an[MAX_MON_AN];// từng món trong đơn
    uint8_t  so_mon;                  // số món thực sự có trong đơn
    uint32_t tong_gia_tri;            // tổng giá trị (nghìn đồng)
    uint32_t timestamp;               // millis() lúc tạo đơn
} DonHang_Status_t;

// Khai báo trước các hàm
DonHang_Status_t* getDon(uint16_t table_id);
void clearDon(uint16_t table_id);
void tinhTong(DonHang_Status_t* d);
bool tatCaMonDaGiao(DonHang_Status_t* d);
String buildDonJson(DonHang_Status_t* d);
String buildMenuJson();
String buildAllDonJson();
void broadcastDonUpdate(uint16_t table_id);
void broadcastDonMoi(DonHang_Status_t* d);
void broadcastAGV(const char* event, const char* payload = nullptr);
void onKitchenWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len);
void onAGVWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                  AwsEventType type, void* arg, uint8_t* data, size_t len);
void handlePostOrder(AsyncWebServerRequest* req,
                     uint8_t* body, size_t len,
                     size_t idx, size_t total);
void handlePayment(AsyncWebServerRequest* req,
                   uint8_t* body, size_t len,
                   size_t idx, size_t total);
void handleGetOrder(AsyncWebServerRequest* req);
void handleGetMenu(AsyncWebServerRequest* req);
void handleStatus(AsyncWebServerRequest* req);
void handleDebugFiles(AsyncWebServerRequest* req);
void handleTableRoute(AsyncWebServerRequest* req, uint16_t table_id);
void handleKitchenRoute(AsyncWebServerRequest* req);
void logSpiffsState();
void sendEmbeddedHtml(AsyncWebServerRequest* req, const uint8_t* start, const uint8_t* end);

// Global: 10 đơn hàng (1 bàn 1 đơn)
static DonHang_Status_t g_don_hang[NUM_TABLES];

// ──────────────────────────────────────────────────────────────────
//  HMI UART CALLBACKS
// ──────────────────────────────────────────────────────────────────
// Callback to send string to HMI (UART2)
static void hmi_uart_send_string(const char* str) {
    Serial2.write((const uint8_t*)str, strlen(str));
}

// Callback to get available bytes in UART2 buffer
static int hmi_uart_available(void) {
    return Serial2.available();
}

// Callback to read one byte from UART2
static int hmi_uart_read_byte(void) {
    if (Serial2.available()) {
        return Serial2.read();
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────
//  MAP AGV POSITION TO HMI VALUE
// ──────────────────────────────────────────────────────────────────
static uint8_t get_hmi_position_value(AGV_Position_t pos) {
    switch (pos) {
        case AGV_POS_HOME:    return 5;   // Home
        case AGV_POS_P1:      return 25;  // B1/B2
        case AGV_POS_P2:      return 40;  // B3/B4
        case AGV_POS_P3:      return 55;  // B5/B6
        case AGV_POS_P4:      return 70;  // B7/B8
        case AGV_POS_P5:      return 85;  // B9/B10
        case AGV_POS_MOVING:  return 50;  // Moving (middle position)
        default:              return 5;
    }
}

// ──────────────────────────────────────────────────────────────────
//  SERVER OBJECTS
// ──────────────────────────────────────────────────────────────────
AsyncWebServer webServer(PORT_WEB);
AsyncWebSocket wsKitchen("/ws");          // bếp + khách realtime
AsyncWebSocket wsAGV("/agv");             // AGV controller riêng

// ──────────────────────────────────────────────────────────────────
//  UTILITY FUNCTIONS
// ──────────────────────────────────────────────────────────────────
DonHang_Status_t* getDon(uint16_t table_id) {
    if (table_id < 1 || table_id > NUM_TABLES) return nullptr;
    return &g_don_hang[table_id - 1];
}

void clearDon(uint16_t table_id) {
    DonHang_Status_t* d = getDon(table_id);
    if (!d) return;
    memset(d, 0, sizeof(DonHang_Status_t));
    d->ID         = table_id;
    d->Trang_thai = DON_TRONG;
}

void tinhTong(DonHang_Status_t* d) {
    d->tong_gia_tri = 0;
    for (int i = 0; i < d->so_mon; i++) {
        d->tong_gia_tri += (uint32_t)d->mon_an[i].Soluong * d->mon_an[i].Gia;
    }
}

const char* tenTrangThaiDon(uint8_t s) {
    switch((TrangThaiDon_t)s) {
        case DON_TRONG:      return "trong";
        case DON_DANG_XU_LY: return "dang_xu_ly";
        case DON_DA_GIAO:    return "da_giao";
        case DON_THANH_TOAN: return "thanh_toan";
        case DON_CHUA_TT:    return "chua_thanh_toan";
        default:             return "unknown";
    }
}

const char* tenTrangThaiMon(uint8_t s) {
    switch((MonStatus_t)s) {
        case MON_CHUA_NAU: return "chua_nau";
        case MON_DANG_NAU: return "dang_nau";
        case MON_XONG:     return "xong";
        case MON_DA_GIAO:  return "da_giao";
        default:           return "unknown";
    }
}

// Kiểm tra toàn bộ món đã giao chưa
bool tatCaMonDaGiao(DonHang_Status_t* d) {
    for (int i = 0; i < d->so_mon; i++) {
        if (d->mon_an[i].Status != MON_DA_GIAO) return false;
    }
    return (d->so_mon > 0);
}

// ──────────────────────────────────────────────────────────────────
//  JSON BUILDERS
// ──────────────────────────────────────────────────────────────────
String buildDonJson(DonHang_Status_t* d) {
    JsonDocument doc;
    doc["table_id"]     = d->ID;
    doc["trang_thai"]   = tenTrangThaiDon(d->Trang_thai);
    doc["tong_gia"]     = d->tong_gia_tri;
    doc["timestamp"]    = d->timestamp;
    doc["so_mon"]       = d->so_mon;

    JsonArray items = doc["mon_an"].to<JsonArray>();
    for (int i = 0; i < d->so_mon; i++) {
        MonAn_Status_t* m = &d->mon_an[i];
        JsonObject obj = items.add<JsonObject>();
        obj["type"]       = (uint8_t)m->type;
        obj["soluong"]    = m->Soluong;
        obj["gia_don"]    = m->Gia;
        obj["tong_mon"]   = (uint32_t)m->Soluong * m->Gia;
        obj["status"]     = tenTrangThaiMon(m->Status);
        obj["img"]        = String("img") + String((uint8_t)m->type) + ".jpg";

        const MonAnInfo* info = getMonInfo(m->type);
        if (info) {
            obj["ten"]  = info->ten;
            obj["loai"] = info->loai;
        }
    }
    String out; serializeJson(doc, out); return out;
}

String buildMenuJson() {
    JsonDocument doc;
    JsonArray arr = doc["menu"].to<JsonArray>();
    for (int i = 0; i < MAX_MON_AN; i++) {
        JsonObject o   = arr.add<JsonObject>();
        o["id"]    = (uint8_t)MENU_TABLE[i].id;
        o["ten"]   = MENU_TABLE[i].ten;
        o["gia"]   = MENU_TABLE[i].gia;
        o["loai"]  = MENU_TABLE[i].loai;
        o["img"]   = String("img") + String((uint8_t)MENU_TABLE[i].id) + ".jpg";
    }
    String out; serializeJson(doc, out); return out;
}

String buildAllDonJson() {
    JsonDocument doc;
    JsonArray arr = doc["don_hang"].to<JsonArray>();
    for (int t = 0; t < NUM_TABLES; t++) {
        if (g_don_hang[t].Trang_thai != DON_TRONG) {
            JsonObject o     = arr.add<JsonObject>();
            o["table_id"]    = g_don_hang[t].ID;
            o["trang_thai"]  = tenTrangThaiDon(g_don_hang[t].Trang_thai);
            o["tong_gia"]    = g_don_hang[t].tong_gia_tri;
            o["so_mon"]      = g_don_hang[t].so_mon;
            o["timestamp"]   = g_don_hang[t].timestamp;
        }
    }
    String out; serializeJson(doc, out); return out;
}

// ──────────────────────────────────────────────────────────────────
//  WEBSOCKET BROADCAST
// ──────────────────────────────────────────────────────────────────
void broadcastDonUpdate(uint16_t table_id) {
    DonHang_Status_t* d = getDon(table_id);
    if (!d) return;
    JsonDocument doc;
    doc["event"]      = "don_update";
    doc["table_id"]   = table_id;
    doc["trang_thai"] = tenTrangThaiDon(d->Trang_thai);
    doc["tong_gia"]   = d->tong_gia_tri;
    String msg; serializeJson(doc, msg);
    wsKitchen.textAll(msg);
    Serial.printf("[WS] broadcast table=%d status=%s\n",
                  table_id, tenTrangThaiDon(d->Trang_thai));
}

void broadcastDonMoi(DonHang_Status_t* d) {
    JsonDocument doc;
    doc["event"]    = "don_moi";
    doc["table_id"] = d->ID;
    doc["tong_gia"] = d->tong_gia_tri;
    doc["so_mon"]   = d->so_mon;
    doc["data"]     = buildDonJson(d);
    String msg; serializeJson(doc, msg);
    wsKitchen.textAll(msg);
    Serial.printf("[WS] don_moi table=%d (%d mon)\n", d->ID, d->so_mon);
}

// AGV broadcast
void broadcastAGV(const char* event, const char* payload) {
    JsonDocument doc;
    doc["event"] = event;
    if (payload) doc["data"] = payload;
    doc["t"] = millis();
    String msg; serializeJson(doc, msg);
    wsAGV.textAll(msg);
    Serial.printf("[AGV] broadcast: %s\n", event);
}

// ──────────────────────────────────────────────────────────────────
//  WEBSOCKET KITCHEN EVENT HANDLER
// ──────────────────────────────────────────────────────────────────
void onKitchenWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Kitchen client #%u from %s\n",
                      (unsigned int)client->id(), client->remoteIP().toString().c_str());
        client->text(buildAllDonJson());

    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Kitchen client #%u disconnected\n", (unsigned int)client->id());

    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (!info->final || info->index != 0 || info->len != len
            || info->opcode != WS_TEXT) return;
        data[len] = 0;
        String raw((char*)data);

        JsonDocument doc;
        if (deserializeJson(doc, raw)) {
            client->text("{\"error\":\"bad_json\"}"); return;
        }
        const char* cmd = doc["cmd"] | "";

        // ── Đổi trạng thái cả đơn ────────────────────────────────
        if (strcmp(cmd, "set_status") == 0) {
            uint16_t tid = doc["table_id"] | 0;
            const char* st = doc["status"] | "";
            DonHang_Status_t* d = getDon(tid);
            if (!d) { client->text("{\"error\":\"invalid_table\"}"); return; }

            if      (strcmp(st,"dang_xu_ly")  == 0) d->Trang_thai = DON_DANG_XU_LY;
            else if (strcmp(st,"da_giao")     == 0) d->Trang_thai = DON_DA_GIAO;
            else if (strcmp(st,"thanh_toan")  == 0) d->Trang_thai = DON_THANH_TOAN;
            else if (strcmp(st,"chua_thanh_toan")==0) d->Trang_thai = DON_CHUA_TT;
            else { client->text("{\"error\":\"invalid_status\"}"); return; }

            broadcastDonUpdate(tid);
            client->text("{\"ok\":true}");
        }

        // ── Đổi trạng thái từng món ───────────────────────────────
        else if (strcmp(cmd, "set_mon_status") == 0) {
            uint16_t tid  = doc["table_id"]   | 0;
            uint8_t  midx = doc["mon_index"]  | 255;
            uint8_t  mst  = doc["mon_status"] | 0;
            DonHang_Status_t* d = getDon(tid);
            if (!d || midx >= d->so_mon) {
                client->text("{\"error\":\"invalid\"}"); return;
            }
            d->mon_an[midx].Status = (MonStatus_t)mst;
            if (tatCaMonDaGiao(d)) d->Trang_thai = DON_DA_GIAO;
            broadcastDonUpdate(tid);
            client->text("{\"ok\":true}");
        }

        // ── Xoá đơn (sau khi thanh toán xong) ────────────────────
        else if (strcmp(cmd, "clear_table") == 0) {
            uint16_t tid = doc["table_id"] | 0;
            clearDon(tid);
            broadcastDonUpdate(tid);
            client->text("{\"ok\":true}");
        }

        // ── Gửi lệnh AGV từ bếp ──────────────────────────────────
        else if (strcmp(cmd, "agv_goto") == 0) {
            // Bếp gửi: {"cmd":"agv_goto","pos":"P2","table":3,"s1":"Phở","s2":""}
            String relay;
            serializeJson(doc, relay);
            wsAGV.textAll(relay);     // relay thẳng sang AGV WS
            broadcastDonUpdate(doc["table"] | 0);
            client->text("{\"ok\":tru}");
            Serial.printf("[AGV] relay goto pos=%s table=%d\n",
                          doc["pos"].as<const char*>(), doc["table"].as<int>());
        }

        else if (strcmp(cmd, "agv_estop") == 0) {
            String relay;
            serializeJson(doc, relay);
            wsAGV.textAll(relay);
            client->text("{\"ok\":true}");
            Serial.println("[AGV] relay emergency stop");
        }

        // ── Lấy tất cả đơn ───────────────────────────────────────
        else if (strcmp(cmd, "get_all") == 0) {
            client->text(buildAllDonJson());
        }

        // ── Ping ─────────────────────────────────────────────────
        else if (strcmp(cmd, "ping") == 0) {
            client->text("{\"event\":\"pong\",\"t\":" + String(millis()) + "}");
        }

        else { client->text("{\"error\":\"unknown_cmd\"}"); }
    }
}

// ──────────────────────────────────────────────────────────────────
//  WEBSOCKET AGV EVENT HANDLER
// ──────────────────────────────────────────────────────────────────
void onAGVWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                  AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[AGV-WS] Client #%u connected\n", (unsigned int)client->id());
        client->text("{\"event\":\"hello\",\"role\":\"agv\"}");

    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[AGV-WS] Client #%u disconnected\n", (unsigned int)client->id());

    } else if (type == WS_EVT_DATA) {
        // Feedback từ AGV (vị trí thực, trạng thái motor...)
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (!info->final || info->opcode != WS_TEXT) return;
        data[len] = 0;
        String raw((char*)data);
        Serial.printf("[AGV-RX] %s\n", raw.c_str());

        // Relay feedback AGV → tất cả client bếp
        JsonDocument doc;
        if (!deserializeJson(doc, raw)) {
            // Update HMI position if AGV sends "pos"
            if (doc.containsKey("pos")) {
                const char* pos_str = doc["pos"];
                uint8_t hmi_val = 0;
                
                if (strcmp(pos_str, "Home") == 0 || strcmp(pos_str, "home") == 0) {
                    g_agv_position = AGV_POS_HOME;
                    hmi_val = 5;
                    Serial.println("[HMI] AGV Position: Home");
                } else if (strcmp(pos_str, "P1") == 0) {
                    g_agv_position = AGV_POS_P1;
                    hmi_val = 25;
                    Serial.println("[HMI] AGV Position: P1");
                } else if (strcmp(pos_str, "P2") == 0) {
                    g_agv_position = AGV_POS_P2;
                    hmi_val = 40;
                    Serial.println("[HMI] AGV Position: P2");
                } else if (strcmp(pos_str, "P3") == 0) {
                    g_agv_position = AGV_POS_P3;
                    hmi_val = 55;
                    Serial.println("[HMI] AGV Position: P3");
                } else if (strcmp(pos_str, "P4") == 0) {
                    g_agv_position = AGV_POS_P4;
                    hmi_val = 70;
                    Serial.println("[HMI] AGV Position: P4");
                } else if (strcmp(pos_str, "P5") == 0) {
                    g_agv_position = AGV_POS_P5;
                    hmi_val = 85;
                    Serial.println("[HMI] AGV Position: P5");
                } else if (strcmp(pos_str, "moving") == 0) {
                    g_agv_position = AGV_POS_MOVING;
                    hmi_val = 50;
                    Serial.println("[HMI] AGV Position: moving");
                }
                
                // Send updated position to HMI
                if (hmi_val > 0) {
                    HMI_send_position(hmi_val);
                }
            }
            
            doc["_relay_from"] = "agv";
            String relay; serializeJson(doc, relay);
            wsKitchen.textAll(relay);
        }
    }
}

// ──────────────────────────────────────────────────────────────────
//  REST API  —  POST /api/order
//  Body: {"table_id":3,"mon_an":[{"type":1,"soluong":2},{"type":7,"soluong":1}]}
// ──────────────────────────────────────────────────────────────────
void handlePostOrder(AsyncWebServerRequest* req,
                     uint8_t* body, size_t len,
                     size_t /*idx*/, size_t /*total*/)
{
    String raw((char*)body, len);
    JsonDocument doc;
    if (deserializeJson(doc, raw)) {
        req->send(400, "application/json", "{\"error\":\"bad_json\"}"); return;
    }

    uint16_t tid = doc["table_id"] | 0;
    DonHang_Status_t* d = getDon(tid);
    if (!d) { req->send(400, "application/json", "{\"error\":\"invalid_table\"}"); return; }

    clearDon(tid);
    d->Trang_thai = DON_DANG_XU_LY;
    d->timestamp  = millis();

    JsonArray arr = doc["mon_an"].as<JsonArray>();
    for (JsonObject item : arr) {
        if (d->so_mon >= MAX_MON_AN) break;
        MonAn_t  t  = (MonAn_t)(item["type"].as<uint8_t>());
        uint8_t  sl = item["soluong"] | 1;
        const MonAnInfo* info = getMonInfo(t);
        if (!info) continue;

        MonAn_Status_t& slot = d->mon_an[d->so_mon];
        slot.type    = t;
        slot.Soluong = sl;
        slot.Gia     = info->gia;  // copy giá
        slot.Status  = MON_CHUA_NAU;
        d->so_mon++;
    }

    tinhTong(d);
    broadcastDonMoi(d);

    Serial.printf("[ORDER] table=%d so_mon=%d tong=%lu\n",
                  tid, d->so_mon, d->tong_gia_tri);

    req->send(200, "application/json",
              "{\"ok\":true,\"table_id\":" + String(tid) +
              ",\"tong_gia\":" + String(d->tong_gia_tri) + "}");
}

// POST /api/payment  — khách bấm thanh toán
void handlePayment(AsyncWebServerRequest* req,
                   uint8_t* body, size_t len,
                   size_t, size_t)
{
    JsonDocument doc;
    deserializeJson(doc, (char*)body, len);
    uint16_t tid = doc["table_id"] | 0;
    DonHang_Status_t* d = getDon(tid);
    if (!d) { req->send(400, "application/json", "{\"error\":\"invalid_table\"}"); return; }

    d->Trang_thai = DON_THANH_TOAN;
    broadcastDonUpdate(tid);
    req->send(200, "application/json", "{\"ok\":true}");
    Serial.printf("[PAY] table=%d thanh toán\n", tid);
}

// GET /api/order?table=N
void handleGetOrder(AsyncWebServerRequest* req) {
    if (req->hasParam("table")) {
        uint16_t tid = req->getParam("table")->value().toInt();
        DonHang_Status_t* d = getDon(tid);
        if (!d) { req->send(400, "application/json", "{\"error\":\"invalid\"}"); return; }
        req->send(200, "application/json", buildDonJson(d));
    } else {
        req->send(200, "application/json", buildAllDonJson());
    }
}

// GET /api/menu
void handleGetMenu(AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildMenuJson());
}

// GET /api/status
void handleStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["fw"]          = FW_VERSION;
    doc["ip"]          = WiFi.localIP().toString();
    doc["uptime_ms"]   = millis();
    doc["ws_clients"]  = wsKitchen.count();
    doc["agv_clients"] = wsAGV.count();
    doc["free_heap"]   = ESP.getFreeHeap();
    int active = 0;
    for (int i = 0; i < NUM_TABLES; i++)
        if (g_don_hang[i].Trang_thai != DON_TRONG) active++;
    doc["active_tables"] = active;
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
}

void handleDebugFiles(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["fw"] = FW_VERSION;
    doc["spiffs_index"] = SPIFFS.exists("/index.html");
    doc["spiffs_kitchen"] = SPIFFS.exists("/kitchen.html");
    doc["embedded_index_bytes"] = data_index_html_end - data_index_html_start;
    doc["embedded_kitchen_bytes"] = data_kitchen_html_end - data_kitchen_html_start;

    JsonArray files = doc["spiffs_files"].to<JsonArray>();
    File root = SPIFFS.open("/");
    if (root && root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            JsonObject item = files.add<JsonObject>();
            item["name"] = file.name();
            item["size"] = file.size();
            file = root.openNextFile();
        }
    }

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

// ──────────────────────────────────────────────────────────────────
//  ROUTE  /N  (N = 1..10 → khách, 100 → bếp)
//  Serve SPIFFS file tương ứng, inject table_id vào header
// ──────────────────────────────────────────────────────────────────
void handleTableRoute(AsyncWebServerRequest* req, uint16_t table_id) {
    // Serve index.html với cookie / meta chứa table_id
    // (JS phía client đọc từ URL path thay vì query string)
    if (SPIFFS.exists("/index.html")) {
        req->send(SPIFFS, "/index.html", "text/html");
        return;
    }
    sendEmbeddedHtml(req, data_index_html_start, data_index_html_end);
}

void handleKitchenRoute(AsyncWebServerRequest* req) {
    if (SPIFFS.exists("/kitchen.html")) {
        req->send(SPIFFS, "/kitchen.html", "text/html");
        return;
    }
    sendEmbeddedHtml(req, data_kitchen_html_start, data_kitchen_html_end);
}

void sendEmbeddedHtml(AsyncWebServerRequest* req, const uint8_t* start, const uint8_t* end) {
    size_t len = end - start;
    if (len == 0) {
        req->send(500, "text/plain", "embedded html missing");
        return;
    }

    AsyncWebServerResponse* response = req->beginResponse(
        "text/html",
        len,
        [start, len](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
            if (index >= len) {
                return 0;
            }
            size_t chunk = len - index;
            if (chunk > maxLen) {
                chunk = maxLen;
            }
            memcpy(buffer, start + index, chunk);
            return chunk;
        }
    );
    req->send(response);
}

void logSpiffsState() {
    Serial.printf("[FW] %s\n", FW_VERSION);
    Serial.printf("[EMBED] index.html: %u bytes\n", (unsigned int)(data_index_html_end - data_index_html_start));
    Serial.printf("[EMBED] kitchen.html: %u bytes\n", (unsigned int)(data_kitchen_html_end - data_kitchen_html_start));
    Serial.printf("[SPIFFS] /index.html: %s\n", SPIFFS.exists("/index.html") ? "FOUND" : "MISSING");
    Serial.printf("[SPIFFS] /kitchen.html: %s\n", SPIFFS.exists("/kitchen.html") ? "FOUND" : "MISSING");

    File root = SPIFFS.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("[SPIFFS] Cannot open root directory");
        return;
    }

    Serial.println("[SPIFFS] Files:");
    File file = root.openNextFile();
    while (file) {
        Serial.printf("  %s (%u bytes)\n", file.name(), (unsigned int)file.size());
        file = root.openNextFile();
    }
}

// ──────────────────────────────────────────────────────────────────
//  SETUP
// ──────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== ESP32 Restaurant v2.0 ===");

    // ── HMI Serial2 ──────────────────────────────────────────────
    // GPIO16 (RX2), GPIO17 (TX2), baud 9600
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    HMI_init(hmi_uart_send_string, hmi_uart_available, hmi_uart_read_byte);
    delay(100);
    // Send initial position (Home = 5)
    HMI_send_position(get_hmi_position_value(AGV_POS_HOME));
    Serial.println("[HMI] UART2 initialized (9600 baud)");

    // Khởi tạo tất cả đơn
    for (int i = 0; i < NUM_TABLES; i++) {
        memset(&g_don_hang[i], 0, sizeof(DonHang_Status_t));
        g_don_hang[i].ID         = (uint16_t)(i + 1);
        g_don_hang[i].Trang_thai = DON_TRONG;
    }

    // SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR] SPIFFS mount failed");
    } else {
        Serial.println("[OK] SPIFFS mounted");
        logSpiffsState();
    }

    // WiFi  //h123
    // WiFi.mode(WIFI_STA);
    // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // -> 

    WiFi.mode(WIFI_STA);

    // cấu hình IP trước khi connect
    if (!WiFi.config(
            local_IP,
            gateway,
            subnet,
            primaryDNS,
            secondaryDNS
        ))
    {
        Serial.println("[ERROR] Static IP config failed");
    }

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );


    // 
    Serial.print("[WiFi] Connecting");
    uint8_t tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) {
        delay(500); Serial.print("."); tries++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[ERROR] WiFi failed! Restarting...");
        ESP.restart();
    }
    Serial.printf("\n[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());

    // ── WebSocket handlers ────────────────────────────────────────
    wsKitchen.onEvent(onKitchenWsEvent);
    wsAGV.onEvent(onAGVWsEvent);
    webServer.addHandler(&wsKitchen);
    webServer.addHandler(&wsAGV);

    // ── Routes /1 .. /10  (khách từng bàn) ───────────────────────
    for (int t = 1; t <= NUM_TABLES; t++) {
        String path = "/" + String(t);
        uint16_t tid = (uint16_t)t;
        webServer.on(path.c_str(), HTTP_GET,
            [tid](AsyncWebServerRequest* req){ handleTableRoute(req, tid); });
    }

    // ── Route /100  (bếp) ─────────────────────────────────────────
    webServer.on("/100", HTTP_GET, [](AsyncWebServerRequest* req){
        handleKitchenRoute(req);
    });

    // ── REST API ──────────────────────────────────────────────────
    webServer.on("/api/menu",    HTTP_GET,  handleGetMenu);
    webServer.on("/api/order",   HTTP_GET,  handleGetOrder);
    webServer.on("/api/status",  HTTP_GET,  handleStatus);
    webServer.on("/debug/files", HTTP_GET,  handleDebugFiles);

    webServer.on("/api/order",   HTTP_POST,
        [](AsyncWebServerRequest* r){}, nullptr, handlePostOrder);

    webServer.on("/api/payment", HTTP_POST,
        [](AsyncWebServerRequest* r){}, nullptr, handlePayment);

    // ── Static files (ảnh, CSS, JS phụ) ──────────────────────────
    webServer.serveStatic("/img/", SPIFFS, "/img/");
    webServer.serveStatic("/",     SPIFFS, "/").setDefaultFile("index.html");

    webServer.onNotFound([](AsyncWebServerRequest* req){
        req->send(404, "application/json", "{\"error\":\"not_found\"}");
    });

    webServer.begin();



    Serial.println("\n=== Routes đã kích hoạt ===");
    Serial.printf("  Bàn 1-10 : http://%s/1  ..  http://%s/10\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.localIP().toString().c_str());
    Serial.printf("  Bếp      : http://%s/100\n",
                  WiFi.localIP().toString().c_str());
    Serial.printf("  Kitchen WS: ws://%s/ws\n",
                  WiFi.localIP().toString().c_str());
    Serial.printf("  AGV WS    : ws://%s/agv\n",
                  WiFi.localIP().toString().c_str());
    Serial.println("===========================\n");
}

// ──────────────────────────────────────────────────────────────────
//  LOOP
// ──────────────────────────────────────────────────────────────────
void loop() {
    // ── HMI Update (process incoming UART data) ──────────────────
    HMI_update();

    wsKitchen.cleanupClients();
    wsAGV.cleanupClients();

    // ── Handle HMI button commands ────────────────────────────────
    if (HMI_is_EMG()) {
        // Send emergency stop to AGV (same as kitchen_controller)
        JsonDocument doc;
        doc["cmd"] = "agv_estop";
        String msg; serializeJson(doc, msg);
        wsAGV.textAll(msg);
        Serial.println("[HMI] EMG command -> AGV");
        HMI_clear_EMG();
    }
    
    if (HMI_is_HOME()) {
        // Send return home to AGV (same as kitchen_controller)
        JsonDocument doc;
        doc["cmd"] = "agv_goto";
        doc["pos"] = "Home";
        doc["table"] = 0;
        String msg; serializeJson(doc, msg);
        wsAGV.textAll(msg);
        g_agv_position = AGV_POS_HOME;
        HMI_send_position(get_hmi_position_value(AGV_POS_HOME));
        Serial.println("[HMI] HOME command -> AGV");
        HMI_clear_HOME();
    }

    // ── Periodic HMI AGV position update at 1Hz ────────────────────
    static uint32_t lastHmiPositionUpdate = 0;
    if (millis() - lastHmiPositionUpdate >= 1000) {
        lastHmiPositionUpdate = millis();
        HMI_send_position(get_hmi_position_value(g_agv_position));
    }

    // ── WiFi watchdog ────────────────────────────────────────────
    static uint32_t lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 15000) {
        lastWifiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Reconnecting...");
            WiFi.reconnect();
        }
    }
}
