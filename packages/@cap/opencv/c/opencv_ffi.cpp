// Thin C-ABI shim for @cap/opencv (core + imgproc, OpenCV 4.10.0)
// License: Apache-2.0

#include <cstdint>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

extern "C" {

const char* cp_cv_version(void) {
    return CV_VERSION;
}

int32_t cp_cv_gaussian_blur_u8(const uint8_t* src, uint8_t* dst, int32_t w, int32_t h, int32_t ksize) {
    if (!src || !dst || w <= 0 || h <= 0 || ksize <= 0 || (ksize & 1) == 0) {
        return -1;
    }
    try {
        cv::Mat in(h, w, CV_8UC1, const_cast<uint8_t*>(src));
        cv::Mat out(h, w, CV_8UC1, dst);
        cv::GaussianBlur(in, out, cv::Size(ksize, ksize), 0.0);
        return 0;
    } catch (...) {
        return -2;
    }
}

int32_t cp_cv_canny_u8(const uint8_t* src, uint8_t* dst, int32_t w, int32_t h, double t1, double t2) {
    if (!src || !dst || w <= 0 || h <= 0 || t1 < 0.0 || t2 < 0.0) {
        return -1;
    }
    try {
        cv::Mat in(h, w, CV_8UC1, const_cast<uint8_t*>(src));
        cv::Mat out(h, w, CV_8UC1, dst);
        cv::Canny(in, out, t1, t2);
        return 0;
    } catch (...) {
        return -2;
    }
}

int32_t cp_cv_resize_u8(const uint8_t* src, int32_t sw, int32_t sh, uint8_t* dst, int32_t dw, int32_t dh) {
    if (!src || !dst || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) {
        return -1;
    }
    try {
        cv::Mat in(sh, sw, CV_8UC1, const_cast<uint8_t*>(src));
        cv::Mat out(dh, dw, CV_8UC1, dst);
        cv::resize(in, out, cv::Size(dw, dh), 0.0, 0.0, cv::INTER_LINEAR);
        return 0;
    } catch (...) {
        return -2;
    }
}

} // extern "C"
