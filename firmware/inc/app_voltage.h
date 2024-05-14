/**
 * measure capacitor storage voltage
 */

#pragma once

#include "app_common.h"

#define VOLTAGE_SAADC_REF_MV        600
#define VOLTAGE_SAADC_GAIN_INVERSE  3
#define VOLTAGE_SAADC_GAIN          CONCAT_2(NRF_SAADC_GAIN1_, VOLTAGE_SAADC_GAIN_INVERSE)
#define VOLTAGE_SAADC_ACQ_TIME      NRF_SAADC_ACQTIME_3US
#define VOLTAGE_SAADC_RESOLUTION    8

typedef enum {
    VOLTAGE_RET_PREV_SAMPLE = 0,
    VOLTAGE_RET_WAITED_FOR_SAMPLE,
    VOLTAGE_RET_FRESH_SAMPLE
} voltage_ret_t;

void voltage_init(void);
voltage_ret_t voltage_force_sample(uint32_t staleness_ticks, uint32_t wait_ticks, uint32_t *age);
int32_t voltage_read_v_store(void);
void voltage_wait_for_v_store_thresh(int32_t thresh_mv);
uint32_t voltage_get_measurement_age_ticks();
