// @cap/libavif — AOMediaCodec libavif (v1.2.1) AVIF/HEIF Image Container Library for CP
// License: BSD-2-Clause (Upstream: AOMediaCodec/libavif)

import std from "std";

pub @c {
    fn cp_avif_version(): [*:0]const u8?;
    fn cp_avif_pixel_format_to_string(fmt: i32): [*:0]const u8?;
    fn cp_avif_image_create(width: u32, height: u32, depth: u32, yuv_format: i32): *!?;
    fn cp_avif_image_destroy(img: *!?);
    fn cp_avif_image_width(img: *!?): u32;
    fn cp_avif_image_height(img: *!?): u32;
    fn cp_avif_image_depth(img: *!?): u32;
    fn cp_avif_image_create_and_destroy(width: u32, height: u32, depth: u32): i32;
}

pub const AVIF_PIXEL_FORMAT_NONE: i32 = 0;
pub const AVIF_PIXEL_FORMAT_YUV444: i32 = 1;
pub const AVIF_PIXEL_FORMAT_YUV422: i32 = 2;
pub const AVIF_PIXEL_FORMAT_YUV420: i32 = 3;
pub const AVIF_PIXEL_FORMAT_YUV400: i32 = 4;

pub err AvifError {
    InvalidArgument,
    AllocFailed,
}

/// Returns the linked libavif version string (e.g. `"1.2.1"`).
pub fn version(): string {
    unsafe {
        if (ver := cp_avif_version()) {
            return std.mem.span(ver);
        }
    }
    return "1.2.1";
}

/// Returns the human-readable name of an AVIF YUV pixel format (e.g. `"YUV420"`).
pub fn pixelFormatName(fmt: i32): string {
    unsafe {
        if (name := cp_avif_pixel_format_to_string(fmt)) {
            return std.mem.span(name);
        }
    }
    return "None";
}

/// Smoke-tests allocating and destroying an `avifImage` of the given dimensions and bit depth.
pub fn imageCreateAndDestroy(width: u32, height: u32, depth: u32): void! {
    if (width == 0 or height == 0) return AvifError.InvalidArgument;
    let rc: i32 = 0;
    unsafe {
        rc = cp_avif_image_create_and_destroy(width, height, depth);
    }
    if (rc != 0) return AvifError.AllocFailed;
}

/// Owned RAII wrapper around an `avifImage` instance.
/// Automatically frees the image on scope exit via `~init()`.
pub struct AvifImage {
    raw: *!? = null,

    /// Creates a new `AvifImage` with the given dimensions, bit depth (8, 10, or 12), and YUV format.
    pub static fn create(w: u32, h: u32, d: u32, yuv_format: i32): AvifImage! {
        if (w == 0 or h == 0) return AvifError.InvalidArgument;
        let handle: *!? = null;
        unsafe {
            handle = cp_avif_image_create(w, h, d, yuv_format);
        }
        if (handle == null) return AvifError.AllocFailed;
        return AvifImage{ .raw = handle };
    }

    /// Returns the image width in pixels.
    pub fn width(): u32 {
        if (p := _.raw) {
            unsafe {
                return cp_avif_image_width(p);
            }
        }
        return 0;
    }

    /// Returns the image height in pixels.
    pub fn height(): u32 {
        if (p := _.raw) {
            unsafe {
                return cp_avif_image_height(p);
            }
        }
        return 0;
    }

    /// Returns the image bit depth (e.g. 8, 10, 12).
    pub fn depth(): u32 {
        if (p := _.raw) {
            unsafe {
                return cp_avif_image_depth(p);
            }
        }
        return 0;
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            unsafe {
                cp_avif_image_destroy(p);
            }
        }
    }
}
