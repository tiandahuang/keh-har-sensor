/**
 * measure capacitor storage voltage
 */

#include "app_voltage.h"
#include "app_callbacks.h"
#include "app_debug.h"

#include "nrfx_saadc.h"
#include "nrf_gpio.h"
#include "nrf_pwr_mgmt.h"

#include "app_timer.h"

static nrf_saadc_value_t adc_ping_pong_buffer[2];
static volatile nrf_saadc_value_t * volatile read_pt = &adc_ping_pong_buffer[0];
static volatile nrf_saadc_value_t * volatile write_pt = &adc_ping_pong_buffer[1];
static volatile bool adc_sample_pend = false;
static volatile uint32_t adc_sample_timestamp_ticks = 0;
APP_TIMER_DEF(v_samp_timer_id);

// unit conversions -----------------------------------------------------------
static inline int32_t convert_adc_to_mv(int32_t adc, int32_t v_scale);
static inline int32_t convert_mv_to_adc(int32_t mv, int32_t v_scale_inv);
#define SWAP(x, y) do { typeof(x) _s = x; x = y; y = _s; } while (0)

// handlers -------------------------------------------------------------------
static void saadc_handler(nrfx_saadc_evt_t const *p_event);
static void v_samp_timer_handler(void *p_context);

static void voltage_saadc_init(void) {
    const nrf_saadc_channel_config_t channel_config = {
        // Vcap max is 5.25V, so we can use gain 1/3 (with internal reference 0.6V):
        // 5.25V / 3 = 1.75V < 0.6V * 3 = 1.8V
        .gain = VOLTAGE_SAADC_GAIN,
        // Vcap is run through a voltage divider so RC is very large
        // This value may need to be increased
        .acq_time = VOLTAGE_SAADC_ACQ_TIME,
        .pin_p = GPIO_V_STORE_DIV_IN,
        .resistor_p = NRF_SAADC_RESISTOR_DISABLED,  .resistor_n = NRF_SAADC_RESISTOR_DISABLED,
        .reference = NRF_SAADC_REFERENCE_INTERNAL,  .mode = NRF_SAADC_MODE_SINGLE_ENDED,
        .burst = NRF_SAADC_BURST_DISABLED,          .pin_n = NRF_SAADC_INPUT_DISABLED
    };
    const nrfx_saadc_config_t saadc_config = NRFX_SAADC_DEFAULT_CONFIG;

    nrfx_saadc_init(&saadc_config, saadc_handler);
    nrfx_saadc_channel_init(0, &channel_config);
}

static bool voltage_trig_sample(void) {
    nrfx_err_t err = NRFX_SUCCESS;
    adc_sample_pend = true;
    
    voltage_saadc_init();

    err |= nrfx_saadc_buffer_convert((nrf_saadc_value_t *)write_pt, 1);
    err |= nrfx_saadc_sample();

    return (err == NRFX_SUCCESS);
}

void voltage_init(void) {

    static bool initial_init = true;
    ret_code_t err_code = 0;

    if (initial_init) {
        // enable PMOS from capacitors to voltage divider
        nrf_gpio_cfg_output(GPIO_DIV_EN);
        nrf_gpio_pin_clear(GPIO_DIV_EN);

        // timer config
        err_code |= app_timer_create(&v_samp_timer_id,
                                    APP_TIMER_MODE_REPEATED,
                                    v_samp_timer_handler);
        initial_init = false;
    }

    err_code |= app_timer_start(v_samp_timer_id,
                                APP_TIMER_TICKS(V_STORE_SAMP_PERIOD_MS), 
                                NULL);
}

voltage_ret_t voltage_force_sample(uint32_t staleness_ticks, uint32_t wait_ticks, uint32_t *age) {

    voltage_ret_t ret;
    static const uint32_t pd_ticks = APP_TIMER_TICKS(V_STORE_SAMP_PERIOD_MS);
    uint32_t sample_age = voltage_get_measurement_age_ticks();

    if (sample_age < staleness_ticks) {
        // sample is still relatively fresh
        *age = sample_age;
        return VOLTAGE_RET_PREV_SAMPLE;
    } else if ((sample_age + wait_ticks) >= pd_ticks) {
        // wait for new sample to be triggered via app timer
        ret = VOLTAGE_RET_WAITED_FOR_SAMPLE;
    } else {
        // manually trigger sample
        voltage_trig_sample();
        ret = VOLTAGE_RET_FRESH_SAMPLE;
    }

    // wait for sample to be valid
    while (adc_sample_pend) nrf_pwr_mgmt_run();

    *age = voltage_get_measurement_age_ticks();
    return ret;
}

int32_t voltage_read_v_store(void) {
    return convert_adc_to_mv(*read_pt, V_STORE_DIV_INV);
}

void voltage_wait_for_v_store_thresh(int32_t thresh_mv) {
    int32_t thresh_adc = convert_mv_to_adc(thresh_mv, V_STORE_DIV_INV);
    while (adc_sample_pend) nrf_pwr_mgmt_run(); // wait for any valid sample
    while (*read_pt < thresh_adc) {
        // if (!debug_process())
        nrf_pwr_mgmt_run();
    }
}

uint32_t voltage_get_measurement_age_ticks() {
    return app_timer_cnt_diff_compute(
            app_timer_cnt_get(),
            adc_sample_timestamp_ticks);
}

// unit conversions -----------------------------------------------------------

static inline int32_t convert_adc_to_mv(int32_t adc, int32_t v_scale) {
    const int32_t adc_full_range = (1 << VOLTAGE_SAADC_RESOLUTION) - 1;
    const int32_t adc_range_scaler = VOLTAGE_SAADC_REF_MV * VOLTAGE_SAADC_GAIN_INVERSE;
    return ROUNDED_DIV(adc * adc_range_scaler * v_scale, adc_full_range);
}

static inline int32_t convert_mv_to_adc(int32_t mv, int32_t v_scale_inv) {
    const int32_t adc_full_range = (1 << VOLTAGE_SAADC_RESOLUTION) - 1;
    const int32_t adc_range_scaler = VOLTAGE_SAADC_REF_MV * VOLTAGE_SAADC_GAIN_INVERSE;
    return ROUNDED_DIV(mv * adc_full_range, adc_range_scaler * v_scale_inv);
}

// handlers -------------------------------------------------------------------

WEAK_CALLBACK_DEF(NRFX_SAADC_EVT_DONE)

static void saadc_handler(nrfx_saadc_evt_t const *p_event) {
    if (p_event->type != NRFX_SAADC_EVT_DONE) return;
    nrfx_saadc_uninit();

    adc_sample_timestamp_ticks = app_timer_cnt_get();
    adc_sample_pend = false;    // address any waits on a sample

    SWAP(read_pt, write_pt);
    CALLBACK_FUNC(NRFX_SAADC_EVT_DONE)();

    // debug_log("v store: %d", convert_adc_to_mv(*read_pt, V_STORE_DIV_INV));
}

static void v_samp_timer_handler(void *p_context) {
    voltage_trig_sample();
}

