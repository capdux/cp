// @cap/dav1d — VideoLAN dav1d (v1.5.4) Royalty-Free AV1 Video Decoder for CP
// License: BSD-2-Clause (Upstream: videolan/dav1d)

import std from "std";

pub @c {
    fn cp_dav1d_version(): [*:0]const u8;
    fn cp_dav1d_version_api(): u32;
    fn cp_dav1d_open(n_threads: i32, max_frame_delay: i32): *!?;
    fn cp_dav1d_flush(ctx: *!?);
    fn cp_dav1d_close(ctx: *!?);
}

pub err Dav1dError {
    OpenFailed,
}

/// Returns the linked dav1d library version string (e.g. `"1.5.4"`).
pub fn version(): string {
    unsafe {
        return std.mem.span(cp_dav1d_version());
    }
}

/// Returns the packed dav1d API version (`0x00XXYYZZ`).
pub fn versionApi(): u32 {
    unsafe {
        return cp_dav1d_version_api();
    }
}

/// Owned RAII wrapper around a `Dav1dContext` AV1 decoder instance.
/// Automatically closes the decoder context on scope exit via `~init()`.
pub struct Dav1dDecoder {
    raw: *!? = null,

    /// Opens a new AV1 decoder instance (`n_threads = 0` auto-detects cores, `max_frame_delay = 1` for low latency).
    pub static fn open(n_threads: i32, max_frame_delay: i32): Dav1dDecoder! {
        let handle: *!? = null;
        unsafe {
            handle = cp_dav1d_open(n_threads, max_frame_delay);
        }
        if (handle == null) {
            return Dav1dError.OpenFailed;
        }
        return Dav1dDecoder{ .raw = handle };
    }

    /// Returns true if the underlying decoder context is open.
    pub fn isOpen(): bool {
        return _.raw != null;
    }

    /// Flushes all delayed frames and resets internal decoder state (e.g. on seek).
    pub fn flush() {
        if (p := _.raw) {
            unsafe {
                cp_dav1d_flush(p);
            }
        }
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            unsafe {
                cp_dav1d_close(p);
            }
        }
    }
}
