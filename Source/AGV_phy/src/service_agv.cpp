/*
  AGV WebSocket client firmware skeleton.

  Copy this file into the AGV ESP32 project as src/main.cpp.
  Required libraries in AGV project:
    - bblanchon/ArduinoJson
    - links2004/WebSockets
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// -------------------- CONFIG --------------------
// #define WIFI_SSID       "Hao_Lang_Khong_Pass"
// #define WIFI_PASSWORD   ""

/*

=== Routes đã kích hoạt ===
  Bàn 1-10 : http://192.168.1.102/1  ..  http://192.168.1.102/10
  Bếp      : http://192.168.1.102/100
  Kitchen WS: ws://192.168.1.102/ws
  AGV WS    : ws://192.168.1.102/agv
===========================
*/

#define WIFI_SSID       "nha_hang_1977"
#define WIFI_PASSWORD   "12345678"


#define CONTROLLER_HOST "192.168.1.102"
#define CONTROLLER_PORT 80
#define CONTROLLER_PATH "/agv"

#define AGV_FEEDBACK_INTERVAL_MS 1000

#define AGV_EVENT_FB        1
#define AGV_STS_IDLE        0
#define AGV_STS_INPROGESS   1
#define AGV_STS_DONE_TASK   2
#define AGV_POS_HOME        0

// -------------------- DATA CONTRACT --------------------
typedef struct {
    uint8_t Khay_nao;   // 1 or 2
    uint8_t ban_nao;    // 1..10, 0 = empty
} mess_to_AGV_t;

typedef struct {
    mess_to_AGV_t mes[2];  // mes[0] = khay 1, mes[1] = khay 2
    uint8_t status_home;   // 1 when kitchen/controller requests Home
    uint8_t new_req;       // 1 when a fresh request was received
} AGV_podcard_t;

typedef struct {
    uint8_t event;
    uint8_t Task_sts;
    uint8_t tposs;
} AGV_Sts_t;

WebSocketsClient wsClient;
AGV_podcard_t g_agv_podcard = {};
AGV_Sts_t g_agv_sts = {AGV_EVENT_FB, AGV_STS_IDLE, AGV_POS_HOME};

static portMUX_TYPE g_agvMux = portMUX_INITIALIZER_UNLOCKED;
static bool g_wsConnected = false;
static uint32_t g_lastFeedbackMs = 0;
static TaskHandle_t g_serviceTaskHandle = nullptr;

// -------------------- USER HOOKS --------------------
// Implement these in your AGV motor/control code if needed.
void onAGVNewRequest(const AGV_podcard_t& req) __attribute__((weak));
void onAGVNewRequest(const AGV_podcard_t& req) {
    (void)req;
}

void onAGVConnected() __attribute__((weak));
void onAGVConnected() {}

void onAGVDisconnected() __attribute__((weak));
void onAGVDisconnected() {}

// Optional helper for your robot logic: copy and clear new_req atomically.
bool agvTakeLatestRequest(AGV_podcard_t* out) {
    if (!out) return false;

    bool hasNew = false;
    portENTER_CRITICAL(&g_agvMux);
    if (g_agv_podcard.new_req) {
        *out = g_agv_podcard;
        g_agv_podcard.new_req = 0;
        hasNew = true;
    }
    portEXIT_CRITICAL(&g_agvMux);

    return hasNew;
}

void agvSetRequest(const AGV_podcard_t& req) {
    portENTER_CRITICAL(&g_agvMux);
    g_agv_podcard = req;
    portEXIT_CRITICAL(&g_agvMux);

    onAGVNewRequest(req);
}

static uint8_t parseTableFromSlot(const char* slotText, uint8_t fallbackTable)
{
    if (!slotText) {
        return fallbackTable;
    }

    const char* b = strchr(slotText, 'B');
    if (!b || !isdigit((unsigned char)b[1])) {
        return fallbackTable;
    }

    uint8_t bIndex = (uint8_t)atoi(b + 1);
    if (bIndex == 0) {
        return fallbackTable;
    }

    return ((bIndex + 1) / 2) + 1;
}

void handleAgvGoto(JsonDocument& doc) {
    AGV_podcard_t req = {};

    const char* pos = doc["pos"] | "";
    uint8_t table = doc["table"] | 0;

    if (strcmp(pos, "Home") == 0 || table == 0) {
        req.status_home = 1;
        req.new_req = 1;
        agvSetRequest(req);
        Serial.println("[AGV] Request: HOME");
        return;
    }

    const char* s1 = doc["s1"] | "";
    const char* s2 = doc["s2"] | "";

    if (table >= 1 && table <= 10) {
        if (strlen(s1) > 0) {
            req.mes[0].Khay_nao = 1;
            req.mes[0].ban_nao = parseTableFromSlot(s1, table);
        }
        if (strlen(s2) > 0) {
            req.mes[1].Khay_nao = 2;
            req.mes[1].ban_nao = parseTableFromSlot(s2, table);
        }
    }

    req.status_home = 0;
    req.new_req = 1;
    agvSetRequest(req);

    Serial.printf("[AGV] Request: khay1->ban%d khay2->ban%d\n",
                  req.mes[0].ban_nao, req.mes[1].ban_nao);
}

void handleWsText(const char* payload) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[WS] Bad JSON: %s\n", err.c_str());
        return;
    }

    const char* cmd = doc["cmd"] | "";
    const char* event = doc["event"] | "";

    if (strcmp(event, "hello") == 0) {
        Serial.println("[WS] Server hello");
        return;
    }

    if (strcmp(cmd, "agv_goto") == 0) {
        handleAgvGoto(doc);
        return;
    }

    if (strcmp(cmd, "agv_estop") == 0) {
        AGV_podcard_t req = {};
        req.new_req = 1;
        agvSetRequest(req);
        Serial.println("[AGV] Request: ESTOP placeholder");
        return;
    }

    Serial.printf("[WS] Ignored packet: %s\n", payload);
}

void wsEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            g_wsConnected = true;
            Serial.println("[WS] Connected to controller");
            onAGVConnected();
            break;

        case WStype_DISCONNECTED:
            g_wsConnected = false;
            Serial.println("[WS] Disconnected");
            onAGVDisconnected();
            break;

        case WStype_TEXT:
            payload[length] = 0;
            Serial.printf("[WS] RX: %s\n", (char*)payload);
            handleWsText((char*)payload);
            break;

        default:
            break;
    }
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("[WiFi] Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
    }

    Serial.printf("\n[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
}

void setupWebSocket() {
    wsClient.begin(CONTROLLER_HOST, CONTROLLER_PORT, CONTROLLER_PATH);
    wsClient.onEvent(wsEvent);
    wsClient.setReconnectInterval(2000);
    wsClient.enableHeartbeat(15000, 3000, 2);
}

void sendAgvFeedback(const char* status, const char* pos) {
    if (!g_wsConnected) return;

    JsonDocument doc;
    doc["event"] = "agv_fb";
    doc["status"] = status;
    doc["pos"] = pos;

    String out;
    serializeJson(doc, out);
    wsClient.sendTXT(out);
}

static const char* agvStatusToText(uint8_t status)
{
    switch (status) {
        case AGV_STS_IDLE: return "idle";
        case AGV_STS_INPROGESS: return "inprogess";
        case AGV_STS_DONE_TASK: return "done_task";
        default: return "idle";
    }
}

static void agvPosToText(uint8_t pos, char* out, size_t outSize)
{
    if (pos == AGV_POS_HOME) {
        snprintf(out, outSize, "Home");
        return;
    }

    snprintf(out, outSize, "B%d", pos);
}

void sen_rep(uint8_t event, uint8_t Task_sts, uint8_t tposs)
{
    g_agv_sts.event = event;
    g_agv_sts.Task_sts = Task_sts;
    g_agv_sts.tposs = tposs;

    if (event != AGV_EVENT_FB) {
        return;
    }

    char posText[8];
    agvPosToText(tposs, posText, sizeof(posText));
    sendAgvFeedback(agvStatusToText(Task_sts), posText);
}

/* 
 * init/ 
 */

void init_service()
{
    connectWiFi();
    setupWebSocket(); 
}

void loop_service() {
    wsClient.loop();
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (millis() - g_lastFeedbackMs >= AGV_FEEDBACK_INTERVAL_MS) {
        g_lastFeedbackMs = millis();
        sen_rep(g_agv_sts.event, g_agv_sts.Task_sts, g_agv_sts.tposs);
    }

    // Your AGV logic can poll agvTakeLatestRequest(&req) here.
}

static void service_task(void* param)
{
    (void)param;

    init_service();

    for (;;) {
        loop_service();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void start_service_task()
{
    if (g_serviceTaskHandle != nullptr) {
        return;
    }

    xTaskCreatePinnedToCore(
        service_task,
        "agv_service",
        8192,
        nullptr,
        1,
        &g_serviceTaskHandle,
        0
    );
}
