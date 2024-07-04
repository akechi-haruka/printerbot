#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <util/loadlibrary.h>
#include "chcusb/shim.h"
#include "printerbot/config.h"
#define SUPER_VERBOSE 1
#include "util/dprintf.h"

#define MAX_SHIM_COUNT 255

static FARPROC shim[MAX_SHIM_COUNT];
static struct printerbot_config config;

typedef int (*ogchcusb_imageformat_310)(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width,
                                        uint16_t height, uint8_t *inputImage, uint16_t *rResult);

typedef int (*ogchcusb_imageformat_330)(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width, uint16_t height, uint16_t *rResult);

int chcusb_imageformat_330(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width, uint16_t height, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d, %d, %d, %d)\n", __func__, format, ncomp, depth, width, height);

    width = config.to_width;
    height = config.to_height;

    // use correct declaration based on model
    int ret;
    if (config.to == 310) {
        // TODO: where do we get this parameter from, is this just image pixels??
        ret = ((ogchcusb_imageformat_310) shim[0])(format, ncomp, depth, width, height, NULL, rResult);
    } else if (config.to == 330){
        ret = ((ogchcusb_imageformat_330) shim[0])(format, ncomp, depth, width, height, rResult);
    } else {
        dprintf(NAME ": Unknown target printer: %d\n", config.to);
        *rResult = 2405;
        ret = 1;
    }
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


void chcusb330_shim_install(struct printerbot_config *cfg) {
    assert(cfg != NULL);

    memcpy(&config, cfg, sizeof(*cfg));

    char path[MAX_PATH];
    if (cfg->from == cfg->to){
        sprintf(path, ".\\C%dAusb_orig.dll", cfg->to);
    } else {
        sprintf(path, ".\\C%dAusb.dll", cfg->to);
    }
    HINSTANCE ptr = LoadLibraryA(path);
    if (ptr == NULL) {
        dprintf("LoadLibrary(%s) FAILED: %ld\n", path, GetLastError());
        exit(1);
        return;
    }
    int count = 0;
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_imageformat");
    dprintf(NAME ": Shimmed %d functions\n", count);

    dprintf(NAME ": CHC330 shim installed\n");
}

#pragma clang diagnostic pop