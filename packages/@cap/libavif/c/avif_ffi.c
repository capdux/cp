#include "avif/avif.h"
#include <stddef.h>
#include <stdint.h>

const char* cp_avif_version(void) {
    return avifVersion();
}

const char* cp_avif_pixel_format_to_string(int32_t fmt) {
    if (fmt < 0 || fmt >= AVIF_PIXEL_FORMAT_COUNT) {
        return avifPixelFormatToString(AVIF_PIXEL_FORMAT_NONE);
    }
    return avifPixelFormatToString((avifPixelFormat)fmt);
}

void* cp_avif_image_create(uint32_t width, uint32_t height, uint32_t depth, int32_t yuv_format) {
    avifPixelFormat fmt = AVIF_PIXEL_FORMAT_YUV420;
    if (yuv_format >= 0 && yuv_format < AVIF_PIXEL_FORMAT_COUNT) {
        fmt = (avifPixelFormat)yuv_format;
    }
    return (void*)avifImageCreate(width, height, depth, fmt);
}

void cp_avif_image_destroy(void* img) {
    if (img != NULL) {
        avifImageDestroy((avifImage*)img);
    }
}

uint32_t cp_avif_image_width(void* img) {
    return img ? ((avifImage*)img)->width : 0;
}

uint32_t cp_avif_image_height(void* img) {
    return img ? ((avifImage*)img)->height : 0;
}

uint32_t cp_avif_image_depth(void* img) {
    return img ? ((avifImage*)img)->depth : 0;
}

int32_t cp_avif_image_create_and_destroy(uint32_t width, uint32_t height, uint32_t depth) {
    avifImage* img = avifImageCreate(width, height, depth, AVIF_PIXEL_FORMAT_YUV420);
    if (img == NULL) {
        return -1;
    }
    int32_t ok = (img->width == width && img->height == height && img->depth == depth) ? 0 : -2;
    avifImageDestroy(img);
    return ok;
}
