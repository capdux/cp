// Type definitions for CP

import {  } from "std";
export fn cp_pcre2_compile(pattern: [*]&u8!, pat_len: usize, opts: u32, jit: i32): *auto!?;
export fn cp_pcre2_match(re: *auto!?, subj: [*]&u8!, subj_len: usize, start_off: usize, out_start: *usize!?, out_end: *usize!?): i32;
export fn cp_pcre2_free(re: *auto!?);
export fn cp_pcre2_is_jit(re: *auto!?): i32;
export fn cp_pcre2_version(): [*:0]&u8!?;
export error Pcre2Error { CompileFailed, MatchFailed }

export class Pcre2Match {
    start: usize,
    end: usize,
    fn slice(subject: &[]u8): &[]u8;
}

export class Pcre2Regex {
    raw: *auto!?,
    fn compile(pattern: &[]u8): !Pcre2Regex;
    fn compileJit(pattern: &[]u8): !Pcre2Regex;
    fn compileWithOptions(pattern: &[]u8, opts: u32, jit: bool): !Pcre2Regex;
    fn versionString(): string;
    fn isJitEnabled(): bool;
    fn find(subject: &[]u8): !Pcre2Match?;
    fn findFrom(subject: &[]u8, start_off: usize): !Pcre2Match?;
    fn isMatch(subject: &[]u8): !bool;
    fn __cp_deinit(_: &mut Pcre2Regex);
}

