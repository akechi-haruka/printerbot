#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define NAME "Printerbot[chcfwdl]"


struct printerbot_config {
    int from;
    int to;
    int imagemode;
};

void printerbot_config_load(
        struct io42io3_config *cfg,
        const char *filename);
