/*
========================================================
FILE : AGV_TEST.ino
BOARD: ESP32 DEV MODULE
CORE : ESP32 ARDUINO CORE 3.X
========================================================
*/

#include <Arduino.h>

/* --------------------------------------------------
 * Define PWM
*/

#define PWM_FREQ           5000
#define PWM_RESOLUTION     8
#define PWM_MAX_DUTY       255

#define M11_PWM_CH         0
#define M12_PWM_CH         1
#define M21_PWM_CH         2
#define M22_PWM_CH         3

/* --------------------------------------------------- 
* define : 
*/
// =======================
// MOTOR 1
// =======================
#define M11_PIN   25
#define M12_PIN   26
// =======================
// MOTOR 2
// =======================
#define M21_PIN   27
#define M22_PIN   14
// =======================
// MOTOR 3
// =======================
#define M31_PIN   18
#define M32_PIN   19
// =======================
// MOTOR 4
// =======================
#define M41_PIN   21
#define M42_PIN   22
// =======================
// LIMIT SWITCH
// =======================
#define LIMIT_1T_PIN   34
#define LIMIT_1D_PIN   35
#define LIMIT_2T_PIN   32
#define LIMIT_2D_PIN   33
// =======================
// SPEAKER
// =======================
#define SPEAKER_PIN     23
/*
========================================================
GPIO MAP - INPUT / ENCODER
========================================================
*/

// Công tắc khay

#define KHAY1_PIN          4
#define KHAY2_PIN          13
// Limit quay

#define LIMIT_THUAN_PIN    36
#define LIMIT_NGUOC_PIN    39

// Encoder

#define ENC_A_PIN          16
#define ENC_B_PIN          17

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

static uint8_t PWM_ChannelFromPin(uint8_t pin)
{
    switch(pin)
    {
        case M11_PIN: return M11_PWM_CH;
        case M12_PIN: return M12_PWM_CH;
        case M21_PIN: return M21_PWM_CH;
        case M22_PIN: return M22_PWM_CH;
        default: return 0;
    }
}

static void PWM_AttachPin(uint8_t pin, uint8_t channel)
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(pin, PWM_FREQ, PWM_RESOLUTION);
#else
    ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(pin, channel);
#endif
}

static void PWM_WritePin(uint8_t pin, uint32_t duty)
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(pin, duty);
#else
    ledcWrite(PWM_ChannelFromPin(pin), duty);
#endif
}

uint8_t QUAY_TRAI();
uint8_t QUAY_PHAI();
uint8_t open_1();
uint8_t close_1();
void start_service_task();
void sen_rep(uint8_t event, uint8_t Task_sts, uint8_t tposs);

#define AGV_EVENT_FB        1
#define AGV_STS_IDLE        0
#define AGV_STS_INPROGESS   1
#define AGV_STS_DONE_TASK   2
#define AGV_POS_HOME        0

typedef struct {
    uint8_t Khay_nao;
    uint8_t ban_nao;
} mess_to_AGV_t;

typedef struct {
    mess_to_AGV_t mes[2];
    uint8_t status_home;
    uint8_t new_req;
} AGV_podcard_t;

bool agvTakeLatestRequest(AGV_podcard_t* out);



/*
 * debug val.
 */

uint8_t M1_PWM_Value = 0;
uint8_t M2_PWM_Value = 0;

/*
 * convert: 
 */

uint8_t SpeedToDuty(uint8_t speed)
{
    if(speed > 100)
        speed = 100;

    return map(speed, 0, 100, 0, PWM_MAX_DUTY);
}



/*
 * Motor 1
 */

void M1_Forward(uint8_t speed)
{
    if(speed > 100)
        return;

    uint8_t duty = SpeedToDuty(speed);

    M1_PWM_Value = speed;

    PWM_WritePin(M11_PIN, duty);
    PWM_WritePin(M12_PIN, 0);
}

void M1_Backward(uint8_t speed)
{
    if(speed > 100)
        return;

    uint8_t duty = SpeedToDuty(speed);

    M1_PWM_Value = speed;

    PWM_WritePin(M11_PIN, 0);
    PWM_WritePin(M12_PIN, duty);
}

void M1_Stop()
{
    M1_PWM_Value = 0;

    PWM_WritePin(M11_PIN, 0);
    PWM_WritePin(M12_PIN, 0);
}

/*
 * Motor 2
 */

void M2_Forward(uint8_t speed)
{
    if(speed > 100)
        return;

    uint8_t duty = SpeedToDuty(speed);

    M2_PWM_Value = speed;

    PWM_WritePin(M21_PIN, duty);
    PWM_WritePin(M22_PIN, 0);
}

void M2_Backward(uint8_t speed)
{
    if(speed > 100)
        return;

    uint8_t duty = SpeedToDuty(speed);

    M2_PWM_Value = speed;

    PWM_WritePin(M21_PIN, 0);
    PWM_WritePin(M22_PIN, duty);
}

void M2_Stop()
{
    M2_PWM_Value = 0;

    PWM_WritePin(M21_PIN, 0);
    PWM_WritePin(M22_PIN, 0);
}

/*
 * Motor 3
 */

void M3_Open()
{
    digitalWrite(M32_PIN, HIGH);
    digitalWrite(M31_PIN, LOW);
}

void M3_Close()
{
    digitalWrite(M32_PIN, LOW);
    digitalWrite(M31_PIN, HIGH);
}

void M3_Stop()
{
    digitalWrite(M31_PIN, LOW);
    digitalWrite(M32_PIN, LOW);
}

/*
 * motor 4
*/

void M4_Open()
{
    digitalWrite(M42_PIN, HIGH);
    digitalWrite(M41_PIN, LOW);
}

void M4_Close()
{
    digitalWrite(M42_PIN, LOW);
    digitalWrite(M41_PIN, HIGH);
}

void M4_Stop()
{
    digitalWrite(M41_PIN, LOW);
    digitalWrite(M42_PIN, LOW);
}

/*
 * limit. 
*/

bool LIMIT_1T_Read(){
    return digitalRead(LIMIT_1T_PIN);
}

bool LIMIT_1D_Read(){
    return digitalRead(LIMIT_1D_PIN);
}

bool LIMIT_2T_Read(){
    return digitalRead(LIMIT_2T_PIN);
}

bool LIMIT_2D_Read(){
    return digitalRead(LIMIT_2D_PIN);
}

/*
 * testing
 */
void MotorManualTest()
{
    if(Serial.available() <= 0)
        return;

    int cmd = Serial.parseInt();

    Serial.print("CMD = ");
    Serial.println(cmd);

    switch(cmd)
    {
        /*
        ========================================
        MOTOR 3
        ========================================
        */

        case 31:

            Serial.println("MOTOR 3 -> M31");

            M3_Open();

            delay(1000);

            M3_Stop();

            break;

        case 32:

            Serial.println("MOTOR 3 -> M32");

            M3_Close();

            delay(1000);

            M3_Stop();

            break;

        /*
        ========================================
        MOTOR 4
        ========================================
        */

        case 41:

            Serial.println("MOTOR 4 -> M41");

            M4_Open();

            delay(1000);

            M4_Stop();

            break;

        case 42:

            Serial.println("MOTOR 4 -> M42");

            M4_Close();

            delay(1000);

            M4_Stop();

            break;
            
        
        case 11: 
            Serial.println("MOTOR 1 -> M11");
            M1_Forward(80);
            delay(3000);
            M1_Stop();

            break; 

        case 12: 
            Serial.println("MOTOR 1 -> M12");
            M1_Backward(80);
            delay(3000);
            M1_Stop();

            break; 

        case 21: 
            Serial.println("MOTOR 2 -> M21");
            M2_Forward(80);
            delay(3000);
            M2_Stop();

            break; 

        case 22: 
            Serial.println("MOTOR 3 -> M22");
            M2_Backward(80);
            delay(3000);
            M2_Stop();

            break; 
        
        case 51:
            while(QUAY_PHAI() == 0){

            }
            break; 
        
        case 52: 
            while(QUAY_TRAI() == 0){
                
            }
            break; 

        case 61: 
            while(open_1() == 0){
                
            }
            break; 

        case 62: 
            while(close_1() == 0){

            }
            break; 

        default:
            Serial.println("UNKNOWN CMD");

            break;
    }
}



/*
 * monitor. 
 */
void MonitorAllInputs()
{
    // Lưu trữ trạng thái trước đó để so sánh (static giúp giữ giá trị sau mỗi lần thoát hàm)
    static int last_l1t = -1, last_l1d = -1;
    static int last_l2t = -1, last_l2d = -1;
    static int last_khay1 = -1, last_khay2 = -1;
    static int last_thuan = -1, last_nguoc = -1;
    static int last_enca = -1, last_encb = -1;

    // Đọc trạng thái hiện tại từ các chân GPIO
    int c_l1t   = digitalRead(LIMIT_1T_PIN);
    int c_l1d   = digitalRead(LIMIT_1D_PIN);
    int c_l2t   = digitalRead(LIMIT_2T_PIN);
    int c_l2d   = digitalRead(LIMIT_2D_PIN);
    int c_khay1 = digitalRead(KHAY1_PIN);
    int c_khay2 = digitalRead(KHAY2_PIN);
    int c_thuan = digitalRead(LIMIT_THUAN_PIN);
    int c_nguoc = digitalRead(LIMIT_NGUOC_PIN);
    int c_enca  = digitalRead(ENC_A_PIN);
    int c_encb  = digitalRead(ENC_B_PIN);

    // Kiểm tra xem có bất kỳ chân nào thay đổi trạng thái không
    if (c_l1t   != last_l1t   || c_l1d   != last_l1d   ||
        c_l2t   != last_l2t   || c_l2d   != last_l2d   ||
        c_khay1 != last_khay1 || c_khay2 != last_khay2 ||
        c_thuan != last_thuan || c_nguoc != last_nguoc ||
        c_enca  != last_enca ) // || c_encb  != last_encb) 
    {
        // Cập nhật lại trạng thái cũ
        last_l1t   = c_l1t;   last_l1d   = c_l1d;
        last_l2t   = c_l2t;   last_l2d   = c_l2d;
        last_khay1 = c_khay1; last_khay2 = c_khay2;
        last_thuan = c_thuan; last_nguoc = c_nguoc;
        last_enca  = c_enca;  last_encb  = c_encb;

        // Tiến hành in ra màn hình trên một dòng rành mạch
        Serial.println("\n--- [INPUT CHANGE DETECTED] ---");
        Serial.printf("L1T: %d | L1D: %d | L2T: %d | L2D: %d | ", c_l1t, c_l1d, c_l2t, c_l2d);
        Serial.printf("KHAY1: %d | KHAY2: %d | ", c_khay1, c_khay2);
        Serial.printf("LIMIT_THUAN: %d | LIMIT_NGUOC: %d | ", c_thuan, c_nguoc);
        Serial.printf("ENC_A: %d | ENC_B: %d\n", c_enca, c_encb);
        Serial.println("-------------------------------");
    }
}

/*
 * Macro: 
 */
// Công tắc hành trình M1 và M2
#define LIM_1T      (!digitalRead(LIMIT_1T_PIN))
#define LIM_1D      (!digitalRead(LIMIT_1D_PIN))
#define LIM_2T      (!digitalRead(LIMIT_2T_PIN))
#define LIM_2D      (!digitalRead(LIMIT_2D_PIN))

// Công tắc khay
#define KHAY_1      (!digitalRead(KHAY1_PIN))
#define KHAY_2      (!digitalRead(KHAY2_PIN))

// Giới hạn quay
#define LIM_THUAN   (!digitalRead(LIMIT_THUAN_PIN))
#define LIM_NGUOC   (!digitalRead(LIMIT_NGUOC_PIN))

// Encoder (Thường encoder để nguyên hoặc đảo tùy thuộc vào cách bạn bắt cạnh)
#define ENC_A       (!digitalRead(ENC_A_PIN))
#define ENC_B       (!digitalRead(ENC_B_PIN))

// giá trị tốc độ. 
uint8_t speed_1 = 80; 
uint8_t speed_2 = 80;
/*
 * control motor 2. 
 */
uint8_t QUAY_PHAI(){
    uint8_t status = 0; 
    if(!LIM_THUAN){
        M2_Forward(speed_2); 
        status = 0; 
    }else{
        M2_Stop(); 
        status = 1;  
    }
    return status; 

}

uint8_t QUAY_TRAI(){
    uint8_t status = 0; 
    if(!LIM_NGUOC){
        M2_Backward(speed_2); 
        status = 0;
    }else{
        M2_Stop(); 
        status = 1;
    }
    return status; 

}

/*
 * control motor 3.
 */
uint8_t open_1(){
    uint8_t status = 0; 
    if(!LIM_1T){
        M3_Open(); 
        status = 0;
    }else{
        M3_Stop(); 
        status = 1; 
    }
    return status; 

}

uint8_t close_1(){
    uint8_t status = 0; 
    if(!LIM_1D){
        M3_Close(); 
        status = 0;
    }else{
        M3_Stop(); 
        status = 1; 
    }        
    return status; 

}

/*
 * control motor 4.
 */
uint8_t open_2(){
    uint8_t status = 0; 
    if(!LIM_2T){
        M4_Open(); 
        status = 0;
    }else{
        M4_Stop(); 
        status = 1; 
    }
    return status; 
}

uint8_t close_2(){
    uint8_t status = 0; 
    if(!LIM_2D){
        M4_Close(); 
        status = 0;
    }else{
        M4_Stop(); 
        status = 1; 
    }
    return status; 
}

/*
 * Encoder read. 
 */
// volatile long encoderCount = 0; // Lưu vị trí hiện tại (số xung)
// volatile long distance  = 0; 
// void IRAM_ATTR doEncoderA() 
// {
//     if (digitalRead(ENC_B_PIN) == HIGH) {
//         encoderCount++; // Quay thuận thì tăng vị trí
//     } else {
//         encoderCount--; // Quay ngược thì giảm vị trí
//     }
//     distance = (long)((float)(encoderCount/360) * 210 ); // mm
// }

volatile long encoderCount = 0; // Lưu vị trí hiện tại (số xung)
#define distance ((long)((encoderCount * 210) / 360))

void IRAM_ATTR doEncoderA() 
{
    if (digitalRead(ENC_B_PIN) == HIGH) {
        encoderCount++; // Quay thuận thì tăng vị trí
    } else {
        encoderCount--; // Quay ngược thì giảm vị trí
    }
}


/*
 Speeaker.
*/
void Speaker_On(){
    digitalWrite(SPEAKER_PIN, HIGH);
}

void Speaker_Off(){
    digitalWrite(SPEAKER_PIN, LOW);
}

/*
 * void go home: 
 */



/*
 * PID CONFIGURATION FOR AGV:
 * Các hệ số điều khiển và biến trạng thái tính toán real-time
 * Cần tinh chỉnh Kp, Ki, Kd tùy thuộc vào tải trọng thực tế của xe
 */
// Hệ số PID (Bạn sẽ cần tinh chỉnh các số này trên thực tế)
float Kp = 1.2;   // Tỉ lệ: Càng xa đích càng chạy nhanh
float Ki = 0.000;  // Tích phân: Triệt tiêu sai số bù (tự tăng ga nếu bị kẹt gần đích)
float Kd = 0.000;   // Vi phân: Hãm phanh chống lao quá đà (giúp xe dừng mượt)

// Biến lưu trạng thái PID
long last_error = 0;
float integral = 0;
unsigned long last_pid_time = 0;

// Cấu hình giới hạn tốc độ điều khiển
#define MIN_PID_SPEED  55  // Tốc độ tối thiểu để thắng ma sát (xe không bị đứng im rên)
#define MAX_PID_SPEED  85  // Tốc độ tối đa cho phép khi chạy PID
uint8_t Go_pos_PID(long target_pos)
{
    unsigned long current_time = millis();
    if (current_time - last_pid_time < 20)
    {
        return 0; 
    }
    long error = target_pos - distance; 
    if (abs(error) <= 5) 
    {
        M1_Stop();       
        integral = 0;    
        last_error = 0;  
        return 1;       
    }
    float P = Kp * (float)error;
    
    // integral += error * dt;
    // integral = constrain(integral, -1000, 1000); 
    // float I = Ki * integral;
    // float D = Kd * ((error - last_error) / dt);
    // last_error = error; 
    float output = P;
    float final_speed_1 = fabs(output);
    uint8_t final_speed = 0;
    if(final_speed_1 >= MAX_PID_SPEED){
        final_speed = MAX_PID_SPEED; 
    }   else if(final_speed_1 <= MIN_PID_SPEED){
        final_speed = MIN_PID_SPEED; 
    }else{
        final_speed = (uint8_t)final_speed_1; 
    }
    // Điều khiển hướng chạy
    if (error > 0) {
        M1_Forward(final_speed);
    } else {
        M1_Backward(final_speed);
    }

    return 0; // Vẫn đang trên đường chạy
}


long Target_Distance = 0;

/*
 * void AGV_Position_Control_Serial():
 * Hàm quét lệnh nhập từ Serial Monitor và cập nhật mục tiêu di chuyển
 * Tích hợp bộ lọc thời gian 200ms để in debug không gây lag chip
 */
void AGV_Position_Control_Serial()
{
    // ĐỌC DỮ LIỆU TỪ SERIAL MONITOR
    if (Serial.available() > 0) 
    {
        long input_val = Serial.parseInt();
        if (input_val != 0 || Serial.peek() == '0') 
        {
            Target_Distance = input_val;
            Serial.printf("\n======> ĐÍCH MỚI: %ld mm <======\n", Target_Distance);
        }
    }

    // GỌI PID ĐIỀU KHIỂN
    uint8_t status = Go_pos_PID(Target_Distance);

    // IN ĐỒNG BỘ DEBUG (Mỗi 200ms)
    static unsigned long last_print = 0;
    if (millis() - last_print >= 200) 
    {
        last_print = millis();
        long sai_so = Target_Distance - distance;

        Serial.printf("[PID RUN] Đích: %ld | Hiện tại: %ld | Sai lệch: %ld mm | ", Target_Distance, distance, sai_so);
        
        if (status == 1) {
            Serial.println("Trạng thái: ĐỒNG TÂM ĐÍCH (STOP)");
        } else {
            // Xem giá trị biến toàn cục M1_PWM_Value để biết mức PWM hiện tại
            Serial.printf("Trạng thái: ĐANG CHẠY | Tốc độ cấp: %d %%\n", M1_PWM_Value);
        }
    }
}

/* Init */
void IO_Init()
{
    /* Init PWM --------------------------- */
    PWM_AttachPin(M11_PIN, M11_PWM_CH);
    PWM_AttachPin(M12_PIN, M12_PWM_CH);

    PWM_AttachPin(M21_PIN, M21_PWM_CH);
    PWM_AttachPin(M22_PIN, M22_PWM_CH);

    /* Output control----------------------- */ 
    pinMode(M31_PIN, OUTPUT);
    pinMode(M32_PIN, OUTPUT);

    pinMode(M41_PIN, OUTPUT);
    pinMode(M42_PIN, OUTPUT);

    pinMode(SPEAKER_PIN, OUTPUT);
    /*INPUT  ------------------------------- */

    pinMode(LIMIT_1T_PIN, INPUT);
    pinMode(LIMIT_1D_PIN, INPUT);
    pinMode(LIMIT_2T_PIN, INPUT_PULLUP);
    pinMode(LIMIT_2D_PIN, INPUT_PULLUP);

    pinMode(KHAY1_PIN, INPUT_PULLUP);
    pinMode(KHAY2_PIN, INPUT_PULLUP);

    pinMode(LIMIT_THUAN_PIN, INPUT);
    pinMode(LIMIT_NGUOC_PIN, INPUT);

    pinMode(ENC_A_PIN, INPUT_PULLUP);
    pinMode(ENC_B_PIN, INPUT_PULLUP);

/* PWM setup ------------------------------ */
    PWM_WritePin(M11_PIN, 0);
    PWM_WritePin(M12_PIN, 0);

    PWM_WritePin(M21_PIN, 0);
    PWM_WritePin(M22_PIN, 0);

/* Pull down -------------------------------*/
    digitalWrite(M31_PIN, LOW);
    digitalWrite(M32_PIN, LOW);

    digitalWrite(M41_PIN, LOW);
    digitalWrite(M42_PIN, LOW);

    digitalWrite(SPEAKER_PIN, LOW);

/* encoder  Gắn ngắt vào chân A, kích hoạt khi chân A chuyển từ LOW lên HIGH (RISING) */
    attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), doEncoderA, RISING);
}


/*
 * hàm thực thi: 
 */

#define space_table_table 1500u
#define Home_possition 0u

#define AGV_WAIT_LOAD_MS       3000UL
#define AGV_TAKE_TIMEOUT_MS    30000UL
#define AGV_CLOSE_DELAY_MS     5000UL
#define AGV_ESTOP_HOLD_MS      30000UL

typedef enum {  
    AGV_JOB_IDLE = 0,
    AGV_JOB_PREP_OPEN,
    AGV_JOB_WAIT_LOAD,
    AGV_JOB_CLOSE_LOAD,
    AGV_JOB_MOVE_TABLE,
    AGV_JOB_TURN_TABLE,
    AGV_JOB_OPEN_DELIVER,
    AGV_JOB_WAIT_TAKE,
    AGV_JOB_CLOSE_DELIVER,
    AGV_JOB_DONE_REPORT,
    AGV_JOB_RETURN_HOME,
    AGV_JOB_ESTOP
} AGV_JobStep_t;

typedef struct {
    uint8_t table;
    uint8_t slotMask;
    uint8_t done;
} AGV_DeliveryPoint_t;

static AGV_JobStep_t g_jobStep = AGV_JOB_IDLE;
static AGV_JobStep_t g_stepBeforeEstop = AGV_JOB_IDLE;
static AGV_DeliveryPoint_t g_points[2] = {};
static uint8_t g_pointCount = 0;
static uint8_t g_pointIndex = 0;
static uint8_t g_activeSlotMask = 0;
static uint32_t g_stepMillis = 0;
static uint8_t g_doneSent = 0;
static uint8_t g_idleSent = 1;

static long AGV_TablePosition(uint8_t table)
{
    return (table == 0) ? Home_possition : (long)(((table + 1) / 2) * space_table_table);
}

static void AGV_StopAll()
{
    M1_Stop();
    M2_Stop();
    M3_Stop();
    M4_Stop();
    Speaker_Off();
}

static uint8_t AGV_HasLoad(uint8_t slotMask)
{
    if ((slotMask & 0x01) && !KHAY_1) return 0;
    if ((slotMask & 0x02) && !KHAY_2) return 0;
    return 1;
}

static uint8_t AGV_LoadTaken(uint8_t slotMask)
{
    if ((slotMask & 0x01) && KHAY_1) return 0;
    if ((slotMask & 0x02) && KHAY_2) return 0;
    return 1;
}

static uint8_t AGV_OpenSlots(uint8_t slotMask)
{
    uint8_t ok = 1;
    if (slotMask & 0x01) ok &= open_1();
    if (slotMask & 0x02) ok &= open_2();
    return ok;
}

static uint8_t AGV_CloseSlots(uint8_t slotMask)
{
    uint8_t ok = 1;
    if (slotMask & 0x01) ok &= close_1();
    if (slotMask & 0x02) ok &= close_2();
    return ok;
}

static void AGV_AddPoint(uint8_t table, uint8_t slotMask)
{
    if (table == 0 || slotMask == 0) return;

    for (uint8_t i = 0; i < g_pointCount; i++) {
        if (g_points[i].table == table) {
            g_points[i].slotMask |= slotMask;
            return;
        }
    }

    if (g_pointCount < 2) {
        g_points[g_pointCount].table = table;
        g_points[g_pointCount].slotMask = slotMask;
        g_points[g_pointCount].done = 0;
        g_pointCount++;
    }
}

static void AGV_SortPointsByDistance()
{
    if (g_pointCount == 2 && AGV_TablePosition(g_points[1].table) < AGV_TablePosition(g_points[0].table)) {
        AGV_DeliveryPoint_t tmp = g_points[0];
        g_points[0] = g_points[1];
        g_points[1] = tmp;
    }
}

static uint8_t AGV_IsEstopReq(const AGV_podcard_t& req)
{
    return req.new_req && !req.status_home && req.mes[0].ban_nao == 0 && req.mes[1].ban_nao == 0;
}

static void AGV_StartHome()
{
    AGV_StopAll();
    g_jobStep = AGV_JOB_RETURN_HOME;
    g_doneSent = 1;
    g_idleSent = 0;
}

static void AGV_StartDelivery(const AGV_podcard_t& req)
{
    memset(g_points, 0, sizeof(g_points));
    g_pointCount = 0;
    g_pointIndex = 0;
    g_activeSlotMask = 0;
    g_doneSent = 0;
    g_idleSent = 0;

    AGV_AddPoint(req.mes[0].ban_nao, 0x01);
    AGV_AddPoint(req.mes[1].ban_nao, 0x02);
    if (req.mes[0].ban_nao) g_activeSlotMask |= 0x01;
    if (req.mes[1].ban_nao) g_activeSlotMask |= 0x02;
    AGV_SortPointsByDistance();

    if (g_pointCount == 0) {
        AGV_StartHome();
        return;
    }

    sen_rep(AGV_EVENT_FB, AGV_STS_INPROGESS, AGV_POS_HOME);
    g_stepMillis = millis();
    g_jobStep = AGV_JOB_PREP_OPEN;
}

static void AGV_CheckNewRequest()
{
    AGV_podcard_t req = {};
    if (!agvTakeLatestRequest(&req)) return;

    if (req.status_home) {
        AGV_StartHome();
        return;
    }

    if (AGV_IsEstopReq(req)) {
        AGV_StopAll();
        g_stepBeforeEstop = g_jobStep;
        g_stepMillis = millis();
        g_jobStep = AGV_JOB_ESTOP;
        return;
    }

    AGV_StartDelivery(req);
}

void AGV_Task_Loop()
{
    AGV_CheckNewRequest();

    if (g_jobStep == AGV_JOB_IDLE) {
        if (!g_idleSent) {
            sen_rep(AGV_EVENT_FB, AGV_STS_IDLE, AGV_POS_HOME);
            g_idleSent = 1;
        }
        return;
    }

    if (g_jobStep == AGV_JOB_ESTOP) {
        if (millis() - g_stepMillis >= AGV_ESTOP_HOLD_MS) {
            g_jobStep = g_stepBeforeEstop;
        }
        return;
    }

    AGV_DeliveryPoint_t* p = (g_pointIndex < g_pointCount) ? &g_points[g_pointIndex] : nullptr;

    switch (g_jobStep) {
        case AGV_JOB_PREP_OPEN:
            if (AGV_OpenSlots(0x03) ) {
                g_stepMillis = 0;
                g_jobStep = AGV_JOB_WAIT_LOAD;
            }
            break;

        case AGV_JOB_WAIT_LOAD:
            if (!AGV_HasLoad(g_activeSlotMask)) {
                g_stepMillis = 0;
                break;
            }
            if (g_stepMillis == 0) {
                g_stepMillis = millis();
            }
            if (millis() - g_stepMillis >= AGV_WAIT_LOAD_MS) {
                g_jobStep = AGV_JOB_CLOSE_LOAD;
            }
            break;

        case AGV_JOB_CLOSE_LOAD:
            if (AGV_CloseSlots(0x03)) {
                g_jobStep = AGV_JOB_MOVE_TABLE;
            }
            break;

        case AGV_JOB_MOVE_TABLE:
            if (p && Go_pos_PID(AGV_TablePosition(p->table))) {
                g_jobStep = AGV_JOB_TURN_TABLE;
            }
            break;

        case AGV_JOB_TURN_TABLE:
            if (p && ((p->table % 2 == 0) ? QUAY_PHAI() : QUAY_TRAI())) {
                g_jobStep = AGV_JOB_OPEN_DELIVER;
            }
            break;

        case AGV_JOB_OPEN_DELIVER:
            if (p) {
                Speaker_On();
                if (AGV_OpenSlots(p->slotMask)) {
                    g_stepMillis = millis();
                    g_jobStep = AGV_JOB_WAIT_TAKE;
                }
            }
            break;

        case AGV_JOB_WAIT_TAKE:
            Speaker_Off();
            if (!p || AGV_LoadTaken(p->slotMask) || millis() - g_stepMillis >= AGV_TAKE_TIMEOUT_MS) {
                g_stepMillis = millis();
                g_jobStep = AGV_JOB_CLOSE_DELIVER;
            }
            break;

        case AGV_JOB_CLOSE_DELIVER:
            if (millis() - g_stepMillis >= AGV_CLOSE_DELAY_MS && p && AGV_CloseSlots(p->slotMask)) {
                p->done = 1;
                g_pointIndex++;
                g_jobStep = (g_pointIndex >= g_pointCount) ? AGV_JOB_DONE_REPORT : AGV_JOB_MOVE_TABLE;
            }
            break;

        case AGV_JOB_DONE_REPORT:
            if (!g_doneSent) {
                uint8_t lastTable = (g_pointCount > 0) ? g_points[g_pointCount - 1].table : AGV_POS_HOME;
                sen_rep(AGV_EVENT_FB, AGV_STS_DONE_TASK, lastTable);
                g_doneSent = 1;
            }
            g_jobStep = AGV_JOB_RETURN_HOME;
            break;

        case AGV_JOB_RETURN_HOME:
            if (Go_pos_PID(Home_possition) && QUAY_TRAI()) {
                AGV_StopAll();
                sen_rep(AGV_EVENT_FB, AGV_STS_IDLE, AGV_POS_HOME);
                g_idleSent = 1;
                g_jobStep = AGV_JOB_IDLE;
            }
            break;

        default:
            break;
    }
}

/*
 * Setup. 
 */

uint8_t flag_init = 1; 


void setup()
{
    Serial.begin(115200);

    IO_Init();
    start_service_task();
    Serial.println("ESP32 START OK");
}

/*
 * loop 
 */
// #define space_table_table 250u  // khoảng cách giữa 2 bàn. 



void loop()
{
  while(flag_init == 1){
    if(close_1() && close_2() && QUAY_TRAI() && Go_pos_PID(Home_possition)){
        flag_init = 0; 
    }
  }
  AGV_Task_Loop();
  // AGV_Position_Control_Serial();
  static uint32_t last_millis = 0;
  if(millis() - last_millis > 2000 ){
      last_millis = millis();
      Serial.printf("gia tri LIMIT_1TL %d, LIMIT_2TL %d, LIMIT_1DL %d, LIMIT_2DL %d, KHAY_1 %d, KHAY_2 %d, LIM_THUAN %d, LIM_NGUOC %d   \n ", LIM_1T, LIM_2T, LIM_1D, LIM_2D, KHAY_1, KHAY_2, LIM_THUAN, LIM_NGUOC);
      // sen_rep(AGV_EVENT_FB, AGV_STS_IDLE, 3);
      Serial.printf("Vị trí Encoder hiện tại: %ld\n", distance);

  }
  // MotorManualTest(); 
    // uint8_t stt = QUAY_PHAI(); 
    // Serial.printf("Vị trí hiệu tại: %ld\n", stt);
    // MonitorAllInputs(); 
}

