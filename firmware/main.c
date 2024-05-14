/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "app_common.h"
#include "app_debug.h"
#include "nrf.h"
#include "nrf_pwr_mgmt.h"
#include "nrfx_gpiote.h"
#include "nrfx_clock.h"
#include "nrf_sdh.h"

#include "app_timer.h"
#include "app_scheduler.h"

#include "app_ble_nus.h"
#include "app_accelerometer.h"
#include "app_voltage.h"

#define APP_SCHED_EVENT_SIZE    APP_TIMER_SCHED_EVENT_DATA_SIZE
#define APP_SCHED_QUEUE_SIZE    10

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
    app_error_handler(42, line_num, p_file_name);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void) {
    if (debug_process() == false) {
        nrf_pwr_mgmt_run();
    }
}

#if POWER_PROFILING_ENABLED
static void start_sampling_accel(void *p_event_data, uint16_t event_size) {
    debug_log("(debug timer) v store %d", voltage_read_v_store());
    debug_log("(debug timer) accelerometer set to wake");
    accelerometer_wake(true, true);
}

APP_TIMER_DEF(accel_sample_timer_id);
void accel_sample_handler(void *p_context) {
    app_sched_event_put(NULL, 0, start_sampling_accel);
}

void power_profiling_init(void) {
    debug_log("power profiling enabled. starting timer with period "STRINGIFY(POWER_PROFILING_PERIOD_MS)" ms");
    
    ret_code_t err_code;
    err_code = app_timer_create(&accel_sample_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                accel_sample_handler);
    err_code = app_timer_start(accel_sample_timer_id,
                               APP_TIMER_TICKS(POWER_PROFILING_PERIOD_MS),
                               NULL);
}
#endif

/**@brief Application main function.
 */
int main(void) {

    // Initialize.
    nrf_pwr_mgmt_init();
    debug_init();
    nrfx_gpiote_init();
    APP_SCHED_INIT(APP_SCHED_EVENT_SIZE, APP_SCHED_QUEUE_SIZE);
    nrf_sdh_enable_request();

    app_timer_init();
    accelerometer_init();
    voltage_init();

    #if POWER_PROFILING_ENABLED

    power_profiling_init();

    #else 

    debug_log("finished HW init. waiting for enough energy to init BLE."); debug_force_flush();
    voltage_wait_for_v_store_thresh(V_STORE_LVL_BLE_INIT);

    #endif
    
    debug_log("initializing BLE");

    // Start BLE
    ble_all_services_init();
    advertising_start(false);

    app_timer_init();
    voltage_init();

    // Start execution.
    debug_log("initialization finished");

    // Enter main loop.
    while (true) {
        app_sched_execute();
        idle_state_handle();
    }
}

