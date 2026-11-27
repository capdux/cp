// Type definitions for CP

import {  } from "std";
export fn utf8proc_version(): [*:0]&u8!;
export fn utf8proc_unicode_version(): [*:0]&u8!;
export fn utf8proc_charwidth(codepoint: i32): i32;
export fn utf8proc_tolower(c: i32): i32;
export fn utf8proc_toupper(c: i32): i32;
export fn utf8proc_NFC(str: [*:0]&u8!): [*:0]u8!?;
export fn utf8proc_NFD(str: [*:0]&u8!): [*:0]u8!?;
export fn utf8proc_NFKC(str: [*:0]&u8!): [*:0]u8!?;
export fn utf8proc_NFKD(str: [*:0]&u8!): [*:0]u8!?;
export fn utf8proc_NFKC_Casefold(str: [*:0]&u8!): [*:0]u8!?;
export fn free(ptr: *auto!?);
export error Utf8ProcError { NormalizeFailed, OutOfMemory }

export fn versionString(): string;
export fn unicodeVersion(): string;
export fn charWidth(cp: i32): i32;
export fn toLower(cp: i32): i32;
export fn toUpper(cp: i32): i32;
export class NormString {
    raw: [*:0]u8!?,
    len: usize,
    fn nfc(input: string): !NormString;
    fn nfd(input: string): !NormString;
    fn nfkc(input: string): !NormString;
    fn nfkcCasefold(input: string): !NormString;
    fn runOp(input: string, mode: u8): !NormString;
    fn slice(): string;
    fn __cp_deinit(_: &mut NormString);
}

