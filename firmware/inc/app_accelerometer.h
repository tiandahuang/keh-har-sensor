/**
 * application specific accelerometer driver for BMA400
 */

#pragma once

#include "app_common.h"

#define ACCELEROMETER_N_SAMPLES   18  // for some reason this results in 16 valid samples

int accelerometer_init(void);
void accelerometer_wake(bool init_spi, bool deinit_spi);
void accelerometer_sleep(bool init_spi, bool deinit_spi);
uint16_t accelerometer_fetch_data(bool init_spi, bool deinit_spi, bool sleep);
void accelerometer_copy_data(uint8_t *data_ptr, uint16_t data_len);
