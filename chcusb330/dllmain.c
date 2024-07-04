#include <windows.h>

#include <stdbool.h>

#include "chcusb/shim.h"
#include "chcusb330/shim.h"

#include "printerbot/config.h"

#include "util/dprintf.h"

struct printerbot_config cfg;

static bool initialized = false;

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx) {

    // main dll is implicitely initialized

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    if (initialized){
        dprintf(NAME ": Double-initialization\n");
        return TRUE;
    }

    dprintf(NAME ": Initializing\n");

    printerbot_config_load(&cfg, ".\\printerbot.ini");

    if (!cfg.enable) {
        dprintf(NAME ": Disabled\n");
        return TRUE;
    }

    chcusb330_shim_install(&cfg);

    dprintf(NAME ": roll out!\n");

    initialized = true;

    return TRUE;
}
