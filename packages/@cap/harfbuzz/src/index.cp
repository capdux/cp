// @cap/harfbuzz — HarfBuzz (v15.2.0) OpenType Text Shaping Engine for CP
// License: MIT (Old MIT / HarfBuzz License, Upstream: harfbuzz/harfbuzz)

import std from "std";

pub @c {
    fn cp_hb_version_string(): [*:0]const u8;
    fn cp_hb_buffer_create(): *!?;
    fn cp_hb_buffer_destroy(buf: *!?);
    fn cp_hb_buffer_reset(buf: *!?);
    fn cp_hb_buffer_add_utf8(buf: *!?, text: [*]const u8!, len: i32);
    fn cp_hb_buffer_guess_segment_properties(buf: *!?);
    fn cp_hb_buffer_get_length(buf: *!?): u32;
    fn cp_hb_buffer_get_direction(buf: *!?): i32;
}

pub err HarfBuzzError {
    BufferCreateFailed,
}

/// Text direction constants matching `hb_direction_t`.
pub const HB_DIRECTION_INVALID: i32 = 0;
pub const HB_DIRECTION_LTR: i32 = 4;
pub const HB_DIRECTION_RTL: i32 = 5;
pub const HB_DIRECTION_TTB: i32 = 6;
pub const HB_DIRECTION_BTT: i32 = 7;

/// Returns the linked HarfBuzz library version string (e.g. `"15.2.0"`).
pub fn versionString(): string {
    unsafe {
        return std.mem.span(cp_hb_version_string());
    }
}

/// Owned RAII wrapper around a HarfBuzz shaping buffer (`hb_buffer_t`).
/// Automatically destroys the underlying buffer on scope exit via `~init()`.
pub struct HbBuffer {
    raw: *!? = null,

    /// Allocates and returns a new empty HarfBuzz buffer.
    pub static fn create(): HbBuffer! {
        let handle: *!? = null;
        unsafe {
            handle = cp_hb_buffer_create();
        }
        if (handle == null) {
            return HarfBuzzError.BufferCreateFailed;
        }
        return HbBuffer{ .raw = handle };
    }

    /// Appends UTF-8 encoded text into the shaping buffer.
    pub fn addUtf8(text: string) {
        if (p := _.raw) {
            const n: i32 = @intCast(text.len);
            unsafe {
                cp_hb_buffer_add_utf8(p, text.ptr, n);
            }
        }
    }

    /// Guesses unset segment properties (direction, script, language) from buffer contents.
    pub fn guessSegmentProperties() {
        if (p := _.raw) {
            unsafe {
                cp_hb_buffer_guess_segment_properties(p);
            }
        }
    }

    /// Returns the number of Unicode codepoints / glyphs currently in the buffer.
    pub fn length(): u32 {
        if (p := _.raw) {
            unsafe {
                return cp_hb_buffer_get_length(p);
            }
        }
        return 0;
    }

    /// Returns the resolved text direction (`HB_DIRECTION_*`).
    pub fn direction(): i32 {
        if (p := _.raw) {
            unsafe {
                return cp_hb_buffer_get_direction(p);
            }
        }
        return HB_DIRECTION_INVALID;
    }

    /// Clears all contents and resets buffer properties for reuse.
    pub fn reset() {
        if (p := _.raw) {
            unsafe {
                cp_hb_buffer_reset(p);
            }
        }
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            unsafe {
                cp_hb_buffer_destroy(p);
            }
        }
    }
}
