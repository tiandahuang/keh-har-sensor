/**
 * common includes and configurations
*/

#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "nordic_common.h"
#include "app_util.h"
#include "app_timer.h"


// config
#define V_STORE_DIV_INV         3           // v_store_div = v_store / V_STORE_DIV_INV
#define V_STORE_SAMP_PERIOD_MS  100

// voltage thresholds. resolution is ~21mV
#define V_STORE_LVL_BLE_INIT    2200        // run BLE init once storage cap is at 2.2V = 242uJ
#define V_STORE_LVL_SAMPLE      1800        // sample + send once storage cap is at 1.8V = 162uJ

// GPIO config
#define GPIO_V_STORE_DIV_IN     NRF_SAADC_INPUT_AIN2    // capacitor voltage -- this is on P.04/AIN2 (which expands to 3...)
#define GPIO_DIV_EN             5           // enable capacitor voltage divider

#define SPI_SCK                 9
#define SPI_MISO                10
#define SPI_MOSI                12

#define IMU_CS                  6
#define IMU_INT1                14
#define IMU_INT2                15

// test config
#define POWER_PROFILING_ENABLED 0           // enable power profiling -- runs IMU sample and BLE send on loop
#define POWER_PROFILING_PERIOD_MS 500       // period of power profiling loop

// BLE config
#define DEVICE_NAME_DEFAULT     "test"      // device name in BLE advertising
#define ENABLE_DEVICE_NAME      1           // enable device name in advertising

#define APP_ADV_INTERVAL        MSEC_TO_UNITS(20, UNIT_0_625_MS)    // advertising interval (in units of 0.625 ms)
#define APP_ADV_DURATION        MSEC_TO_UNITS(200, UNIT_10_MS)      // advertising duration (in units of 10 milliseconds)
#define MIN_CONN_INTERVAL       MSEC_TO_UNITS(8,  UNIT_1_25_MS)     /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL       MSEC_TO_UNITS(12, UNIT_1_25_MS)     /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY           150                                 /**< Slave latency. */
#define CONN_SUP_TIMEOUT        MSEC_TO_UNITS(30000, UNIT_10_MS)    /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(50) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(30000) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT 1                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define QWR_BUFFER_SIZE                 512

#define SEC_PARAM_BOND                  1                                       //!< Perform bonding.
#define SEC_PARAM_MITM                  0                                       //!< Man In The Middle protection not required.
#define SEC_PARAM_LESC                  0                                       //!< LE Secure Connections not enabled.
#define SEC_PARAM_KEYPRESS              0                                       //!< Keypress notifications not enabled.
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                    //!< No I/O capabilities.
#define SEC_PARAM_OOB                   0                                       //!< Out Of Band data not available.
#define SEC_PARAM_MIN_KEY_SIZE          7                                       //!< Minimum encryption key size.
#define SEC_PARAM_MAX_KEY_SIZE          16                                      //!< Maximum encryption key size.
