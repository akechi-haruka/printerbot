#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define NAME "Printerbot[chcfwdl]"

// seriously, don't
#define BRICK_PRINTER_MAGIC_ID 0x9111849

struct printerbot_config {
    int from;
    int to;
    int imagemode;
    bool enable;
    int allow_firmware_write;
    char main_fw_path[MAX_PATH];
    char param_fw_path[MAX_PATH];
};

void printerbot_config_load(
        struct printerbot_config *cfg,
        const char *filename);
