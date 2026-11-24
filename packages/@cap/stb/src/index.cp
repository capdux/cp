// @cap/stb — Image decoding, PNG encoding & high-quality resizing (nothings/stb)
// License: MIT / Public Domain (Upstream: Sean Barrett / nothings/stb)

pub @c {
    fn cp_stbi_load_from_memory(
        buffer: [*]const u8!,
        len: i32,
        x: *i32?!,
        y: *i32?!,
        channels_in_file: *i32?!,
        desired_channels: i32
    ): [*]u8?;
    fn cp_stbi_image_free(ptr: *!?);
    fn cp_stbi_write_png_to_mem(
        pixels: [*]const u8!,
        w: i32,
        h: i32,
        comp: i32,
        stride_bytes: i32,
        out_len: *i32?!
    ): [*]u8?;
    fn cp_stbiw_free(ptr: *!?);
    fn cp_stbir_resize_rgba8(
        in_pixels: [*]const u8!,
        in_w: i32,
        in_h: i32,
        out_pixels: [*]u8!,
        out_w: i32,
        out_h: i32
    ): i32;
}

pub err StbError {
    DecodeFailed,
    EncodeFailed,
    ResizeFailed,
    InvalidArg,
    BufferTooSmall,
}

/// Owned RAII decoded image buffer. Automatically freed via stbi_image_free on scope exit.
pub struct StbImage {
    raw: [*]u8? = null,
    pub width: i32 = 0,
    pub height: i32 = 0,
    pub channels: i32 = 0,

    pub static fn loadFromMemory(data: []const u8, desired_channels: i32): StbImage! {
        if (data.len == 0 or data.len > 0x7FFFFFFF) return StbError.InvalidArg;
        if (desired_channels < 0 or desired_channels > 4) return StbError.InvalidArg;
        let w: i32 = 0;
        let h: i32 = 0;
        let comp: i32 = 0;
        let ptr: [*]u8? = null;
        unsafe {
            ptr = cp_stbi_load_from_memory(
                data.ptr,
                @intCast(data.len),
                &w,
                &h,
                &comp,
                desired_channels
            );
        }
        if (ptr == null or w <= 0 or h <= 0) return StbError.DecodeFailed;
        const actual_ch: i32 = if (desired_channels > 0) desired_channels else comp;
        return StbImage{
            .raw = ptr,
            .width = w,
            .height = h,
            .channels = actual_ch,
        };
    }

    pub fn pixels(): []const u8 {
        if (p := _.raw) {
            const total: usize = @intCast(_.width * _.height * _.channels);
            return p[0..total];
        }
        return &.{};
    }

    ~init() {
        if (p := _.raw) {
            unsafe { cp_stbi_image_free(@ptrCast(p)); }
            _.raw = null;
            _.width = 0;
            _.height = 0;
            _.channels = 0;
        }
    }
}

/// Owned RAII PNG byte buffer allocated by stb_image_write. Automatically freed on scope exit.
pub struct PngBuffer {
    raw: [*]u8? = null,
    pub len: usize = 0,

    pub static fn encode(pixels: []const u8, width: i32, height: i32, channels: i32): PngBuffer! {
        if (width <= 0 or height <= 0 or channels < 1 or channels > 4) {
            return StbError.InvalidArg;
        }
        const needed: usize = @intCast(width * height * channels);
        if (pixels.len < needed) return StbError.BufferTooSmall;
        let out_len: i32 = 0;
        let ptr: [*]u8? = null;
        unsafe {
            ptr = cp_stbi_write_png_to_mem(
                pixels.ptr,
                width,
                height,
                channels,
                width * channels,
                &out_len
            );
        }
        if (ptr == null or out_len <= 0) return StbError.EncodeFailed;
        return PngBuffer{
            .raw = ptr,
            .len = @intCast(out_len),
        };
    }

    pub fn bytes(): []const u8 {
        if (p := _.raw) {
            return p[0.._.len];
        }
        return &.{};
    }

    ~init() {
        if (p := _.raw) {
            unsafe { cp_stbiw_free(@ptrCast(p)); }
            _.raw = null;
            _.len = 0;
        }
    }
}

/// Resizes an RGBA8 (4 bytes/pixel) image from `in_w x in_h` to `out_w x out_h` into `output`.
pub fn resizeRgba8(
    input: []const u8,
    in_w: i32,
    in_h: i32,
    output: []u8,
    out_w: i32,
    out_h: i32
): void! {
    if (in_w <= 0 or in_h <= 0 or out_w <= 0 or out_h <= 0) {
        return StbError.InvalidArg;
    }
    const in_need: usize = @intCast(in_w * in_h * 4);
    const out_need: usize = @intCast(out_w * out_h * 4);
    if (input.len < in_need or output.len < out_need) {
        return StbError.BufferTooSmall;
    }
    let ok: i32 = 0;
    unsafe {
        ok = cp_stbir_resize_rgba8(input.ptr, in_w, in_h, output.ptr, out_w, out_h);
    }
    if (ok == 0) return StbError.ResizeFailed;
}
