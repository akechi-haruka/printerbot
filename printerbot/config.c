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

    if (cfg->from < 0 || cfg->to < 0){
        dprintf("Printerbot: invalid printer identifiers (or config not found)\n");
        exit(1);
    }
}
