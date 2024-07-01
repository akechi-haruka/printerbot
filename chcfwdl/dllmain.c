#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <winuser.h>

#include "printerbot/config.h"
#include "util/dprintf.h"

struct printerbot_config cfg;

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    dprintf(NAME ": Initializing\n");

    printerbot_config_load(&cfg, ".\\printerbot.ini");

    chcfwdl_shim_install(&cfg);

    dprintf(NAME ": roll out!\n");

    return SUCCEEDED(hr);
}
