#include <stdint.h>
#include <stdlib.h>

#include "imagemanipulation.h"

static inline unsigned char getpixel(const uint8_t *in, uint32_t src_width, uint32_t src_height, unsigned y, unsigned x, int channel, int channel_count) {
    if (x < src_width && y < src_height)
        return in[(y * channel_count * src_width) + (channel_count * x) + channel];

    return 0;
}

static inline unsigned char saturate(float x) {
    return x > 255.0f ? 255
                      : x < 0.0f ? 0
                                 : (uint8_t) x;
}

uint8_t* bicubicresize(const uint8_t *in, uint32_t src_width, uint32_t src_height, uint32_t dest_width, uint32_t dest_height, uint32_t *out_len, int channel_count) {

    if (src_height == dest_height && src_width == dest_width) {
        return in;
    }

    uint8_t *out = malloc(dest_width * dest_height * channel_count);
    *out_len = dest_width * dest_height * channel_count;

    const float tx = (float)(src_width) / dest_width;
    const float ty = (float)(src_height) / dest_height;
    const uint32_t row_stride = dest_width * channel_count;

    unsigned char C[5] = {0};

    for (unsigned i = 0; i < dest_height; ++i) {
        for (unsigned j = 0; j < dest_width; ++j) {
            const float x = (float)(tx * j);
            const float y = (float)(ty * i);
            const float dx = tx * j - x, dx2 = dx * dx, dx3 = dx2 * dx;
            const float dy = ty * i - y, dy2 = dy * dy, dy3 = dy2 * dy;

            for (int k = 0; k < channel_count; ++k) {
                for (int jj = 0; jj < 4; ++jj) {
                    const int idx = y - 1 + jj;
                    float a0 = getpixel(in, src_width, src_height, idx, x, k, channel_count);
                    float d0 = getpixel(in, src_width, src_height, idx, x - 1, k, channel_count) - a0;
                    float d2 = getpixel(in, src_width, src_height, idx, x + 1, k, channel_count) - a0;
                    float d3 = getpixel(in, src_width, src_height, idx, x + 2, k, channel_count) - a0;
                    float a1 = -(1.0f / 3.0f) * d0 + d2 - (1.0f / 6.0f) * d3;
                    float a2 = 0.5f * d0 + 0.5f * d2;
                    float a3 = -(1.0f / 6.0f) * d0 - 0.5f * d2 + (1.0f / 6.0f) * d3;
                    C[jj] = a0 + a1 * dx + a2 * dx2 + a3 * dx3;

                    d0 = C[0] - C[1];
                    d2 = C[2] - C[1];
                    d3 = C[3] - C[1];
                    a0 = C[1];
                    a1 = -(1.0f / 3.0f) * d0 + d2 - (1.0f / 6.0f) * d3;
                    a2 = 0.5f * d0 + 0.5f * d2;
                    a3 = -(1.0f / 6.0f) * d0 - 0.5f * d2 + (1.0f / 6.0f) * d3;
                    out[i * row_stride + j * channel_count + k] = saturate(a0 + a1 * dy + a2 * dy2 + a3 * dy3);
                }
            }
        }
    }

    return out;
}