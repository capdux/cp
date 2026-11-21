// Source: Absorbed from /data/dev/bun/src/runtime/ffi/FFIObject.cp | Synced: 2026-11-20T11:20:58Z
// lib/ffi/cstring.cp
// Zero-Copy C NUL-Terminated String Wrapper (Bun CString Parity)

const std = @import("std");

pub struct CString {
    pub ptr: cstr? = null;
    pub byte_len: usize = 0;

    /// Constructs a zero-copy `CString` from a non-null C NUL-terminated pointer.
    pub static fn fromPtr(raw: cstr): CString {
        let n: usize = 0;
        unsafe {
            n = std.mem.len(raw);
        }
        return CString {
            ptr: raw,
            byte_len: n,
        };
    }

    /// Constructs a zero-copy `CString` from a nullable C NUL-terminated pointer.
    pub static fn fromOptionalPtr(raw: cstr?): CString? {
        if (p := raw) {
            return CString.fromPtr(p);
        }
        return null;
    }

    /// Constructs a zero-copy `CString` from a generic nullable `void*` pointer.
    pub static fn fromRawPtr(raw: *!?): CString? {
        if (p := raw) {
            unsafe {
                const c_ptr: cstr = @ptrCast(p);
                return CString.fromPtr(c_ptr);
            }
        }
        return null;
    }

    /// Constructs a zero-copy `CString` from a sentinel-terminated CP/Zig slice.
    pub static fn fromSlice(s: [:0]const u8): CString {
        return CString {
            ptr: s.ptr,
            byte_len: s.len,
        };
    }

    pub fn isNull(): bool {
        return _.ptr == null;
    }

    pub fn isEmpty(): bool {
        return _.byte_len == 0;
    }

    pub fn len(): usize {
        return _.byte_len;
    }

    /// Returns a zero-copy `[]const u8` slice view of the underlying C string bytes (excluding NUL).
    pub fn asSlice(): []const u8 {
        if (p := _.ptr) {
            if (_.byte_len == 0) {
                return "";
            }
            unsafe {
                return p[0.._.byte_len];
            }
        }
        return "";
    }

    pub fn byteAt(index: usize): u8? {
        if (index >= _.byte_len) {
            return null;
        }
        const s = _.asSlice();
        return s[index];
    }

    pub fn eql(other: []const u8): bool {
        return std.mem.eql(u8, _.asSlice(), other);
    }
}
