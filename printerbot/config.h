#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// seriously, don't
#define BRICK_PRINTER_MAGIC_ID 0x9111849

struct printerbot_config {
    bool enable;
    int from;
    int to;

    char main_fw_path[MAX_PATH];
    char param_fw_path[MAX_PATH];

    int rfid_port;
    int rfid_baud;

    int imagemode;
    int from_width;
    int from_height;
    int to_width;
    int to_height;

    int allow_firmware_write;
    int data_manipulation;
};

void printerbot_config_load(
        struct printerbot_config *cfg,
        const char *filename);
