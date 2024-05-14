/**
 * function calls for event triggers
 */

#include "app_callbacks.h"
#include "app_debug.h"
#include "app_ble_nus.h"
#include "app_accelerometer.h"
#include "app_voltage.h"
#include "nrf_pwr_mgmt.h"

// global buffers for sharing data

uint8_t accelerometer_data_buf[ACCELEROMETER_N_SAMPLES * 6] = { 0 };
uint16_t accelerometer_num_data = 0;

// BLE events

static bool send_pend = false;
static volatile bool accel_pend = false;
static volatile bool connected = false;

#if POWER_PROFILING_ENABLED == 0
#pragma message "power profiling disabled -- running in one shot sampling mode"


void app_sched_accelerometer_wake(void *p_event_data, uint16_t event_size) {
    debug_log("waking accelerometer");
    accel_pend = true;
    accelerometer_wake(true, true);
}

// Fresh ADC sample -- check schedule condition to wake accelerometer
CALLBACK_DEF(NRFX_SAADC_EVT_DONE) {
    if (!connected) return;
    int32_t v_store = voltage_read_v_store();
    
    // debug_log("v store: %d mv", v_store);
    if (!accel_pend && v_store > V_STORE_LVL_SAMPLE) {
        app_sched_event_put(NULL, 0, app_sched_accelerometer_wake);
    }
}

// NUS connected
CALLBACK_DEF_APP_SCHED(BLE_GAP_EVT_CONNECTED) {
    debug_log("NUS connected");
    connected = true;
}

// NUS notifications enabled -- send data
CALLBACK_DEF_APP_SCHED(BLE_NUS_EVT_COMM_STARTED) {
    debug_log("NUS notifications enabled");
    if (send_pend) {    // notifications enabled after watermark interrupt
        debug_log("notifications enabled, sending pending data");
        send_pend = false;
        ble_send(accelerometer_data_buf, accelerometer_num_data);
    }
}

// Accelerometer watermark interrupt raised
CALLBACK_DEF_APP_SCHED(ACCELEROMETER_DATA_READY) {
    // fetch accelerometer data -- 1. init spi, 2. fetch data, 3. sleep accel, 4. deinit spi
    accelerometer_num_data = accelerometer_fetch_data(true, true, true);
    accelerometer_copy_data(accelerometer_data_buf, accelerometer_num_data);
    debug_log("ACCELEROMETER_DATA_READY: %d", accelerometer_num_data);

    accel_pend = false;
    send_pend = ble_send(accelerometer_data_buf, accelerometer_num_data) == NRF_SUCCESS;
}

// NUS disconnected -- reset
CALLBACK_DEF_APP_SCHED(BLE_GAP_EVT_DISCONNECTED) {
    debug_log("NUS disconnected");
    accelerometer_sleep(true, true);
    send_pend = connected = false;

    // kill everything
    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
}

#else   // POWER_PROFILING_ENABLED == 1
#pragma message "power profiling enabled -- running in looped sampling mode"

// NUS connected -- do nothing here
CALLBACK_DEF_APP_SCHED(BLE_NUS_EVT_CONNECTED)       { debug_log("NUS connected"); }
// NUS notifications enabled
CALLBACK_DEF_APP_SCHED(BLE_NUS_EVT_COMM_STARTED)    { debug_log("NUS notifications enabled"); }
// NUS disconnected -- reset
CALLBACK_DEF_APP_SCHED(BLE_GAP_EVT_DISCONNECTED)    { debug_log("NUS disconnected. Resetting."); }

// Accelerometer watermark interrupt raised
CALLBACK_DEF_APP_SCHED(ACCELEROMETER_DATA_READY) {
    // fetch accelerometer data -- 1. init spi, 2. fetch data, 3. sleep accel, 4. deinit spi
    accelerometer_num_data = accelerometer_fetch_data(true, true, true);
    accelerometer_copy_data(accelerometer_data_buf, accelerometer_num_data);
    debug_log("ACCELEROMETER_DATA_READY: %d", accelerometer_num_data);

    ble_send(accelerometer_data_buf, accelerometer_num_data);
}

#endif

