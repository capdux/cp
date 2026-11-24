// @cap/utf8proc — Unicode 16.0 normalization (NFC/NFD/NFKC/NFKD) & charwidth for CP
// License: MIT (Upstream: JuliaStrings / utf8proc v2.10.0)

import std from "std";

pub @c {
    fn utf8proc_version(): [*:0]const u8;
    fn utf8proc_unicode_version(): [*:0]const u8;
    fn utf8proc_charwidth(codepoint: i32): i32;
    fn utf8proc_tolower(c: i32): i32;
    fn utf8proc_toupper(c: i32): i32;
    fn utf8proc_NFC(str: [*:0]const u8!): [*:0]u8?;
    fn utf8proc_NFD(str: [*:0]const u8!): [*:0]u8?;
    fn utf8proc_NFKC(str: [*:0]const u8!): [*:0]u8?;
    fn utf8proc_NFKD(str: [*:0]const u8!): [*:0]u8?;
    fn utf8proc_NFKC_Casefold(str: [*:0]const u8!): [*:0]u8?;
    fn free(ptr: *!?);
}

pub err Utf8ProcError {
    NormalizeFailed,
    OutOfMemory,
}

pub fn versionString(): string {
    unsafe { return std.mem.span(utf8proc_version()); }
}

pub fn unicodeVersion(): string {
    unsafe { return std.mem.span(utf8proc_unicode_version()); }
}

pub fn charWidth(cp: i32): i32 {
    unsafe { return utf8proc_charwidth(cp); }
}

pub fn toLower(cp: i32): i32 {
    unsafe { return utf8proc_tolower(cp); }
}

pub fn toUpper(cp: i32): i32 {
    unsafe { return utf8proc_toupper(cp); }
}

/// Owned RAII normalized UTF-8 string allocated by utf8proc (freed automatically via libc free).
pub struct NormString {
    raw: [*:0]u8? = null,
    len: usize = 0,

    pub static fn nfc(input: string): NormString! {
        return NormString.runOp(input, 0);
    }

    pub static fn nfd(input: string): NormString! {
        return NormString.runOp(input, 1);
    }

    pub static fn nfkc(input: string): NormString! {
        return NormString.runOp(input, 2);
    }

    pub static fn nfkcCasefold(input: string): NormString! {
        return NormString.runOp(input, 3);
    }

    static fn runOp(input: string, mode: u8): NormString! {
        const alloc = std.heap.page_allocator;
        const z = alloc.allocSentinel(u8, input.len, 0) catch return Utf8ProcError.OutOfMemory;
        defer alloc.free(z);
        @memcpy(z[0..input.len], input);

        let res: [*:0]u8? = null;
        unsafe {
            if (mode == 0) {
                res = utf8proc_NFC(z.ptr);
            } else if (mode == 1) {
                res = utf8proc_NFD(z.ptr);
            } else if (mode == 2) {
                res = utf8proc_NFKC(z.ptr);
            } else {
                res = utf8proc_NFKC_Casefold(z.ptr);
            }
        }
        if (p := res) {
            const sp = std.mem.span(p);
            return NormString{ .raw = p, .len = sp.len };
        }
        return Utf8ProcError.NormalizeFailed;
    }

    pub fn slice(): string {
        if (p := _.raw) {
            return p[0.._.len];
        }
        return "";
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            _.len = 0;
            unsafe { free(@ptrCast(p)); }
        }
    }
}
