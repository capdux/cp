// @cap/pcre2 — PCRE2 10.44 Perl-compatible regex engine with SLJIT JIT for CP
// License: BSD-3-Clause (Upstream: PCRE2Project/pcre2)

import std from "std";

pub @c {
    fn cp_pcre2_compile(pattern: [*]const u8!, pat_len: usize, opts: u32, jit: i32): *!?;
    fn cp_pcre2_match(
        re: *!?,
        subj: [*]const u8!,
        subj_len: usize,
        start_off: usize,
        out_start: *usize?!,
        out_end: *usize?!
    ): i32;
    fn cp_pcre2_free(re: *!?);
    fn cp_pcre2_is_jit(re: *!?): i32;
    fn cp_pcre2_version(): [*:0]const u8?;
}

pub const OPT_NONE: u32 = 0;
pub const OPT_CASELESS: u32 = 0x00000008;
pub const OPT_MULTILINE: u32 = 0x00000400;
pub const OPT_DOTALL: u32 = 0x00000020;
pub const OPT_UTF: u32 = 0x00080000;
pub const OPT_UCP: u32 = 0x00020000;

pub err Pcre2Error {
    CompileFailed,
    MatchFailed,
}

/// Zero-copy match span within the subject byte slice.
pub struct Pcre2Match {
    pub start: usize,
    pub end: usize,

    pub fn slice(subject: []const u8): []const u8 {
        return subject[_.start.._.end];
    }
}

/// Owned RAII compiled PCRE2 10.44 regular expression with optional SLJIT native JIT.
/// Automatically releases native PCRE2 code and match_data on scope exit.
pub struct Pcre2Regex {
    raw: *!? = null,

    pub static fn compile(pattern: []const u8): Pcre2Regex! {
        let re: *!? = null;
        unsafe {
            re = cp_pcre2_compile(pattern.ptr, pattern.len, OPT_UTF | OPT_UCP, 0);
        }
        if (re == null) return Pcre2Error.CompileFailed;
        return Pcre2Regex{ .raw = re };
    }

    pub static fn compileJit(pattern: []const u8): Pcre2Regex! {
        let re: *!? = null;
        unsafe {
            re = cp_pcre2_compile(pattern.ptr, pattern.len, OPT_UTF | OPT_UCP, 1);
        }
        if (re == null) return Pcre2Error.CompileFailed;
        return Pcre2Regex{ .raw = re };
    }

    pub static fn compileWithOptions(pattern: []const u8, opts: u32, jit: bool): Pcre2Regex! {
        let re: *!? = null;
        const jit_flag: i32 = if (jit) 1 else 0;
        unsafe {
            re = cp_pcre2_compile(pattern.ptr, pattern.len, opts, jit_flag);
        }
        if (re == null) return Pcre2Error.CompileFailed;
        return Pcre2Regex{ .raw = re };
    }

    pub static fn versionString(): string {
        unsafe {
            if (ver := cp_pcre2_version()) {
                return std.mem.span(ver);
            }
        }
        return "10.44";
    }

    pub fn isJitEnabled(): bool {
        if (_.raw == null) return false;
        let flag: i32 = 0;
        unsafe {
            flag = cp_pcre2_is_jit(_.raw);
        }
        return flag != 0;
    }

    pub fn find(subject: []const u8): Pcre2Match?! {
        return _.findFrom(subject, 0);
    }

    pub fn findFrom(subject: []const u8, start_off: usize): Pcre2Match?! {
        if (_.raw == null) return Pcre2Error.MatchFailed;
        if (start_off > subject.len) return null;
        let s: usize = 0;
        let e: usize = 0;
        let rc: i32 = 0;
        unsafe {
            rc = cp_pcre2_match(_.raw, subject.ptr, subject.len, start_off, &s, &e);
        }
        if (rc < 0) return Pcre2Error.MatchFailed;
        if (rc == 0) return null;
        return Pcre2Match{ .start = s, .end = e };
    }

    pub fn isMatch(subject: []const u8): bool! {
        const m = try _.find(subject);
        return m != null;
    }

    ~init() {
        if (p := _.raw) {
            unsafe { cp_pcre2_free(p); }
            _.raw = null;
        }
    }
}
