/**
 * wrapper for nrfx spim (easyDMA)
 */

#include "app_spi.h"
#include "nrfx_spim.h"
#include "nrf_gpio.h"
#include "nrf_pwr_mgmt.h"

#define SPI_INSTANCE 1
static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);

static uint8_t m_tx_buf[APP_SPI_MAX_TRANSFER_LEN] = {0};
static uint8_t m_rx_buf[APP_SPI_MAX_TRANSFER_LEN] = {0};

static uint8_t *rx_req = NULL;
static uint8_t rx_ofs = 0;
static uint8_t xfer_len = 0;

static bool initialized = false;

callback_t spi_xfer_callback = NULL;
static volatile bool spi_xfer_done = false;
void spim_event_handler(nrfx_spim_evt_t const * p_event, void *p_context) {
    spi_xfer_done = true;

    if (rx_req) {   // copy back received bytes
        memcpy(rx_req, &m_rx_buf[rx_ofs], xfer_len - rx_ofs);
    }

    if (spi_xfer_callback) spi_xfer_callback();
}

void app_spi_init(void) {
    if (initialized) return;
    static const nrfx_spim_config_t spi_config = {
        .frequency      = NRF_SPIM_FREQ_1M, .ss_active_high = false,
        .ss_pin         = IMU_CS,           .miso_pin       = SPI_MISO,
        .mosi_pin       = SPI_MOSI,         .sck_pin        = SPI_SCK,
        
        .irq_priority   = NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY,
        .orc            = 0xFF,
        .mode           = NRF_SPIM_MODE_0,
        .bit_order      = NRF_SPIM_BIT_ORDER_MSB_FIRST
    };

    initialized = true;
    nrfx_spim_init(&spi, &spi_config, spim_event_handler, NULL);
}

void app_spi_deinit(void) {
    if (!initialized) return;
    initialized = false;
    nrfx_spim_uninit(&spi);
}

// perform readwrite on spi. 
// xfer_done callback is called upon transfer completion.
// pass null to xfer_done to block.
// pass null to tx or rx to ignore.
int app_spi_readwrite(uint8_t *tx, uint8_t *rx, uint8_t len, callback_t xfer_done) {
    nrfx_err_t result;
    spi_xfer_done = false;
    spi_xfer_callback = xfer_done;

    xfer_len = len;
    rx_req = rx;
    rx_ofs = 0;

    if (tx) {
        memcpy(m_tx_buf, tx, len);
    }
    else {
        memset(m_tx_buf, 0, len);
    }

    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buf, len, m_rx_buf, len);
    result = nrfx_spim_xfer(&spi, &xfer_desc, 0);
    if (result != NRFX_SUCCESS) return (int)result;

    if (xfer_done) return 0;  // callback supplied; return to caller

    // no callback, block
    while (!spi_xfer_done) {
        nrf_pwr_mgmt_run();
    }

    return 0;
}

// perform readwrite with register byte on spi. 
// xfer_done callback is called upon transfer completion.
// pass null to xfer_done to block.
// pass null to tx or rx to ignore.
int app_spi_readwrite_reg(uint8_t reg, uint8_t *tx, uint8_t *rx, uint8_t len, callback_t xfer_done) {
    nrfx_err_t result;
    spi_xfer_done = false;
    spi_xfer_callback = xfer_done;

    xfer_len = len + 1;
    rx_req = rx;
    rx_ofs = 1;

    if (tx) {
        memcpy(&m_tx_buf[1], tx, len);
    }
    else {
        memset(m_tx_buf, 0, len + 1);
    }
    m_tx_buf[0] = reg;

    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buf, len + 1, m_rx_buf, len + 1);
    result = nrfx_spim_xfer(&spi, &xfer_desc, 0);
    if (result != NRFX_SUCCESS) return (int)result;

    if (xfer_done) return 0;  // callback supplied; return to caller

    // no callback, block
    while (!spi_xfer_done) {
        nrf_pwr_mgmt_run();
    }

    return 0;
}


