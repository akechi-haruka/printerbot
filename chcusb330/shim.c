#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "chcusb/shim.h"

#include "printerbot/config.h"

#define SUPER_VERBOSE 1

#include "util/dprintf.h"
#include "util/brickprotect.h"

#define MAX_SHIM_COUNT 255

static FARPROC shim[MAX_SHIM_COUNT];
static struct printerbot_config config;

typedef int (*ogchcusb_imageformat_330)(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width, uint16_t height, uint16_t *rResult);

int chcusb_imageformat_330(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width, uint16_t height, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d, %d, %d, %d)", __func__, format, ncomp, depth, width, height);
    int ret = ((ogchcusb_imageformat_330) shim[0])(format, ncomp, depth, width, height, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


void chcusb330_shim_install(struct printerbot_config *cfg) {
    assert(cfg != NULL);

    memcpy(&config, cfg, sizeof(*cfg));

    char path[MAX_PATH];
    sprintf(path, ".\\C%dAusb.dll", cfg->to);
    HINSTANCE ptr = LoadLibraryA(path);
    if (ptr == NULL) {
        dprintf("LoadLibrary(%s) FAILED: %ld\n", path, GetLastError());
        exit(1);
        return;
    }
    int count = 0;
    shim[count++] = GetProcAddress(ptr, "chcusb_imageformat");
    for (int i = 0; i < count; i++) {
        if (shim[i] == NULL) {
            dprintf(NAME "NON-IMPORTED FUNCTION AT INDEX %d!!\n", i);
        }
    }
    dprintf(NAME ": Shimmed %d functions\n", count);

    dprintf(NAME ": CHC330 shim installed\n");
}

#pragma clang diagnostic pop