# AGV API

Tai lieu tom tat cac API chinh trong `src/main.cpp`.

## Ham tien ich

| API | Chuc nang | Dau vao | Dau ra |
|---|---|---|---|
| `SpeedToDuty(uint8_t speed)` | Doi toc do `%` sang gia tri PWM duty. Gioi han toi da 100%. | `speed`: 0-100 (%) | `uint8_t`: duty 0-255 |

## Dieu khien Motor 1

| API | Chuc nang | Dau vao | Dau ra |
|---|---|---|---|
| `M1_Forward(uint8_t speed)` | Cho Motor 1 chay thuan. | `speed`: 0-100 (%) | Khong |
| `M1_Backward(uint8_t speed)` | Cho Motor 1 chay nguoc. | `speed`: 0-100 (%) | Khong |
| `M1_Stop()` | Dung Motor 1. | Khong | Khong |

## Dieu khien Motor 2

| API | Chuc nang | Dau vao | Dau ra |
|---|---|---|---|
| `M2_Forward(uint8_t speed)` | Cho Motor 2 chay thuan. | `speed`: 0-100 (%) | Khong |
| `M2_Backward(uint8_t speed)` | Cho Motor 2 chay nguoc. | `speed`: 0-100 (%) | Khong |
| `M2_Stop()` | Dung Motor 2. | Khong | Khong |
| `QUAY_TRAI()` | Quay trai den khi cham limit thuan. | Khong | `0`: dang chay, `1`: da dung |
| `QUAY_PHAI()` | Quay phai den khi cham limit nguoc. | Khong | `0`: dang chay, `1`: da dung |

## Dieu khien Motor 3 va Motor 4

| API | Chuc nang | Dau vao | Dau ra |
|---|---|---|---|
| `M3_Open()` | Mo Motor 3. | Khong | Khong |
| `M3_Close()` | Dong Motor 3. | Khong | Khong |
| `M3_Stop()` | Dung Motor 3. | Khong | Khong |
| `M4_Open()` | Mo Motor 4. | Khong | Khong |
| `M4_Close()` | Dong Motor 4. | Khong | Khong |
| `M4_Stop()` | Dung Motor 4. | Khong | Khong |
| `open_1()` | Mo co cau 1 den khi cham limit tren. | Khong | `0`: dang chay, `1`: da dung |
| `close_1()` | Dong co cau 1 den khi cham limit duoi. | Khong | `0`: dang chay, `1`: da dung |
| `open_2()` | Mo co cau 2 den khi cham limit. | Khong | `0`: dang chay, `1`: da dung |
| `close_2()` | Dong co cau 2 den khi cham limit. | Khong | `0`: dang chay, `1`: da dung |

## Doc tin hieu vao

| API | Chuc nang | Dau vao | Dau ra |
|---|---|---|---|
| `LIMIT_1T_Read()` | Doc limit 1 tren. | Khong | `bool`: muc logic GPIO |
| `LIMIT_1D_Read()` | Doc limit 1 duoi. | Khong | `bool`: muc logic GPIO |
| `LIMIT_2T_Read()` | Doc limit 2 tren. | Khong | `bool`: muc logic GPIO |
| `LIMIT_2D_Read()` | Doc limit 2 duoi. | Khong | `bool`: muc logic GPIO |
| `MonitorAllInputs()` | In Serial khi input thay doi. | Khong | Khong |

## Encoder, coi va dieu khien vi tri

| API | Chuc nang | Dau vao | Dau ra |
|---|---|---|---|
| `doEncoderA()` | Ham ngat encoder, cap nhat `encoderCount`. | Xung encoder A | Khong |
| `Speaker_On()` | Bat coi. | Khong | Khong |
| `Speaker_Off()` | Tat coi. | Khong | Khong |
| `Go_home()` | Dua AGV ve gan vi tri 0 mm. | Khong | `0`: dang chay, `1`: da ve home |
| `Go_pos_PID(long target_pos)` | Dieu khien Motor 1 den vi tri muc tieu bang PID P. | `target_pos`: vi tri dich, don vi mm | `0`: dang chay, `1`: da toi dich |
| `AGV_Position_Control_Serial()` | Doc vi tri dich tu Serial va goi PID dieu khien. | Gia tri nhap tu Serial Monitor | Khong |

## Kiem thu va khoi tao

| API | Chuc nang | Dau vao | Dau ra |
|---|---|---|---|
| `MotorManualTest()` | Test motor/lenh bang Serial. | Ma lenh Serial: `11`, `12`, `21`, `22`, `31`, `32`, `41`, `42`, `51`, `52`, `61`, `62` | Khong |
| `IO_Init()` | Cau hinh PWM, GPIO input/output, interrupt encoder. | Khong | Khong |
| `setup()` | Khoi tao Serial va IO. | Khong | Khong |
| `loop()` | Vong lap chinh, dang goi `AGV_Position_Control_Serial()`. | Khong | Khong |

## Bien va macro quan trong

| Ten | Y nghia |
|---|---|
| `distance` | Vi tri hien tai tinh tu encoder, don vi mm. |
| `encoderCount` | So xung encoder da dem. |
| `speed_1`, `speed_2` | Toc do mac dinh cho Motor 1 va Motor 2. |
| `Target_Distance` | Vi tri dich nhan tu Serial. |
| `LIM_1T`, `LIM_1D`, `LIM_2T`, `LIM_2D` | Macro doc limit, da dao logic bang `!digitalRead(...)`. |
| `LIM_THUAN`, `LIM_NGUOC` | Macro doc limit quay, da dao logic. |
| `KHAY_1`, `KHAY_2` | Macro doc cong tac khay, da dao logic. |
