#pragma once

#define NAME "Printerbot[chcusb330]"

#include "printerbot/config.h"

void chcusb330_shim_install(struct printerbot_config* cfg);

int chcusb_imageformat_330(
        uint16_t format,
        uint16_t ncomp,
        uint16_t depth,
        uint16_t width,
        uint16_t height,
        uint16_t * rResult
);
