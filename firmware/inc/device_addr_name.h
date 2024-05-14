/**
 * lookup specifying device address and name
 */

#pragma once

#include "ble_gap.h"

typedef struct {
    ble_gap_addr_t addr;
    char name[16];
} device_addr_t;

const device_addr_t device_name_lookup[2] = {   // addresses are little endian
    {.addr = {.addr = {0x37, 0x76, 0xBC, 0x83, 0x7C, 0xEF}},
     .name = "ALICE"},
    {.addr = {.addr = {0x4D, 0x47, 0xBC, 0x98, 0xA5, 0xEA}},
     .name = "BOB"}
};
