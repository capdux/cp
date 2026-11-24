#include <stddef.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

unsigned char* cp_stbi_load_from_memory(
    const unsigned char* buffer,
    int len,
    int* x,
    int* y,
    int* channels_in_file,
    int desired_channels
) {
    return stbi_load_from_memory(buffer, len, x, y, channels_in_file, desired_channels);
}

void cp_stbi_image_free(void* retval_from_stbi_load) {
    if (retval_from_stbi_load != NULL) {
        stbi_image_free(retval_from_stbi_load);
    }
}

unsigned char* cp_stbi_write_png_to_mem(
    const unsigned char* pixels,
    int w,
    int h,
    int comp,
    int stride_bytes,
    int* out_len
) {
    if (pixels == NULL || w <= 0 || h <= 0 || comp <= 0 || out_len == NULL) {
        return NULL;
    }
    return stbi_write_png_to_mem(pixels, (ptrdiff_t)stride_bytes, w, h, comp, out_len);
}

void cp_stbiw_free(void* ptr) {
    if (ptr != NULL) {
        STBIW_FREE(ptr);
    }
}

int cp_stbir_resize_rgba8(
    const unsigned char* in_pixels,
    int in_w,
    int in_h,
    unsigned char* out_pixels,
    int out_w,
    int out_h
) {
    if (in_pixels == NULL || out_pixels == NULL || in_w <= 0 || in_h <= 0 || out_w <= 0 || out_h <= 0) {
        return 0;
    }
    unsigned char* res = stbir_resize_uint8_srgb(
        in_pixels,
        in_w,
        in_h,
        0,
        out_pixels,
        out_w,
        out_h,
        0,
        STBIR_RGBA
    );
    return res != NULL ? 1 : 0;
}
