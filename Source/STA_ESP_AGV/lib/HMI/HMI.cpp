#include "HMI.h"

// Global AGV position state
AGV_Position_t g_agv_position = AGV_POS_HOME;

// HMI control stubs - implement UART handling in your HMI library
uint8_t HMI_is_EMG() {
    // placeholder: return 0 until UART handling implemented
    return 0;
}

uint8_t HMI_is_HOME() {
    // placeholder: return 0 until UART handling implemented
    return 0;
}
