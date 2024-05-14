/**
 * wrapper for nrfx spim (easyDMA)
 */

#pragma once

#include "app_common.h"
#include "app_callbacks.h"

#define APP_SPI_MAX_TRANSFER_LEN        256

void app_spi_init(void);
void app_spi_deinit(void);
int app_spi_readwrite(uint8_t *tx, uint8_t *rx, uint8_t len, callback_t xfer_done);
int app_spi_readwrite_reg(uint8_t reg, uint8_t *tx, uint8_t *rx, uint8_t len, callback_t xfer_done);
