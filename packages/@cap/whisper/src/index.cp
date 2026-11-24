// @cap/whisper — whisper.cpp (v1.9.6) Local Speech-to-Text (ASR) Engine for CP
// License: MIT (Upstream: ggml-org/whisper.cpp)

import std from "std";

pub @c {
    fn cp_whisper_version(): [*:0]const u8;
    fn cp_whisper_print_system_info(): [*:0]const u8;
    fn cp_whisper_lang_max_id(): i32;
    fn cp_whisper_lang_id(lang: [*:0]const u8!): i32;
    fn cp_whisper_lang_str(id: i32): [*:0]const u8;
    fn cp_whisper_init_from_file(path: [*:0]const u8!): *!?;
    fn cp_whisper_free(ctx: *!?);
    fn cp_whisper_n_vocab(ctx: *!?): i32;
}

pub err WhisperError {
    InitFailed,
    OutOfMemory,
}

/// Returns the whisper.cpp library version string (e.g. "1.9.6").
pub fn versionString(): string {
    unsafe {
        return std.mem.span(cp_whisper_version());
    }
}

/// Returns whisper.cpp / ggml hardware and SIMD feature summary string.
pub fn systemInfo(): string {
    unsafe {
        return std.mem.span(cp_whisper_print_system_info());
    }
}

/// Returns the maximum valid language ID supported by Whisper.
pub fn langMaxId(): i32 {
    unsafe {
        return cp_whisper_lang_max_id();
    }
}

/// Returns the numeric language ID for a language code or name (e.g. "en", "zh"), or -1 if unknown.
pub fn langId(code: string): i32 {
    if (code.len >= 64) {
        return -1;
    }
    let buf: [64]u8 = undefined;
    @memcpy(buf[0..code.len], code);
    buf[code.len] = 0;
    unsafe {
        const ptr: [*:0]const u8 = @ptrCast(&buf[0]);
        return cp_whisper_lang_id(ptr);
    }
}

/// Returns the short language code string for a language ID (e.g. 0 -> "en"), or "" if out of range.
pub fn langStr(id: i32): string {
    unsafe {
        return std.mem.span(cp_whisper_lang_str(id));
    }
}

/// Owned RAII wrapper around `struct whisper_context *`.
/// Automatically releases the loaded Whisper model context on scope exit via `~init()`.
pub struct WhisperContext {
    ctx_ptr: *!? = null,

    /// Loads a GGML Whisper model from `path` using default context parameters.
    pub static fn initFromFile(path: string): WhisperContext! {
        const alloc = std.heap.page_allocator;
        const path_z = alloc.allocSentinel(u8, path.len, 0) catch {
            return WhisperError.OutOfMemory;
        };
        defer alloc.free(path_z);
        @memcpy(path_z[0..path.len], path);

        let ptr: *!? = null;
        unsafe {
            ptr = cp_whisper_init_from_file(path_z.ptr);
        }
        if (ptr == null) {
            return WhisperError.InitFailed;
        }
        return WhisperContext{ .ctx_ptr = ptr };
    }

    /// Returns true if the underlying `whisper_context*` handle is valid.
    pub fn isValid(): bool {
        return _.ctx_ptr != null;
    }

    /// Returns the vocabulary size of the loaded Whisper model, or 0 if invalid.
    pub fn nVocab(): i32 {
        if (p := _.ctx_ptr) {
            unsafe {
                return cp_whisper_n_vocab(p);
            }
        }
        return 0;
    }

    ~init() {
        if (p := _.ctx_ptr) {
            _.ctx_ptr = null;
            unsafe {
                cp_whisper_free(p);
            }
        }
    }
}
