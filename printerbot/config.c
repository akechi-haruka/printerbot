#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <util/dprintf.h>

#include "printerbot/config.h"

void printerbot_config_load(
        struct printerbot_config *cfg,
        const char *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntA("printerbot", "enable", 1, filename);
    GetPrivateProfileStringA(
            "printerbot",
            "main_fw_path",
            ".\\DEVICE\\printer_main_fw.bin",
            cfg->main_fw_path,
            _countof(cfg->main_fw_path),
            filename);
    GetPrivateProfileStringA(
            "printerbot",
            "param_fw_path",
            ".\\DEVICE\\printer_param_fw.bin",
            cfg->param_fw_path,
            _countof(cfg->param_fw_path),
            filename);
    cfg->from = GetPrivateProfileIntA("printerbot", "from", -1, filename);
    cfg->to = GetPrivateProfileIntA("printerbot", "to", -1, filename);
    cfg->imagemode = GetPrivateProfileIntA("printerbot", "imagemode", 0, filename);
    cfg->allow_firmware_write = GetPrivateProfileIntA("printerbot", "i_want_to_possibly_brick_my_printer", 0, filename);
    cfg->from_width = GetPrivateProfileIntA("printerbot", "from_width", 0, filename);
    cfg->from_height = GetPrivateProfileIntA("printerbot", "from_height", 0, filename);
    cfg->to_width = GetPrivateProfileIntA("printerbot", "to_width", 0, filename);
    cfg->to_height = GetPrivateProfileIntA("printerbot", "to_height", 0, filename);
    cfg->rfid_port = GetPrivateProfileIntA("printerbot", "rfid_board", 4, filename);
    cfg->rfid_baud = GetPrivateProfileIntA("printerbot", "rfid_baud", 115200, filename);
    cfg->data_manipulation = GetPrivateProfileIntA("printerbot", "data_manipulation", 1, filename);

    if (cfg->from < 0 || cfg->to < 0){
        dprintf("Printerbot: invalid printer identifiers (or config not found)\n");
        exit(1);
    }

    if (cfg->from_width <= 0 || cfg->from_height <= 0 || cfg->to_width <= 0 || cfg->to_height <= 0){
        dprintf("Printerbot: invalid image size (or config not found)\n");
        exit(1);
    }

}
