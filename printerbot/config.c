#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "printerbot/config.h"

void printerbot_config_load(
        struct printerbot_config *cfg,
        const char *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->from = GetPrivateProfileIntA("printerbot", "from", -1, filename);
    cfg->to = GetPrivateProfileIntA("printerbot", "to", -1, filename);
    cfg->imagemode = GetPrivateProfileIntA("printerbot", "imagemode", 0, filename);

    if (from < 0 || to < 0){
        dprintf("Printerbot: invalid printer identifiers (or config not found)\n");
        exit(1);
    }
}
