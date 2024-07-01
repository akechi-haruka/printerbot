#pragma once

#include <stdint.h>

uint8_t* bicubicresize(const uint8_t* in, uint32_t src_width, uint32_t src_height, uint32_t dest_width, uint32_t dest_height, uint32_t *out_len, int channel_count);