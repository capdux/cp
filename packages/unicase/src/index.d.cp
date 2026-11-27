// Type definitions for CP

import { fnv64OffsetBasis, fnv1a64Update } from "../../fnv-hash/src/index.cp";
export fn ucDecodeUtf8(s: string, pos: usize, step_out: []usize): u32;
export fn ucEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;
export fn foldAsciiByte(b: u8): u8;
export fn foldCodepoint(cp: u32, folded_out: []u32): usize;
export fn caseFold(src: string, out: []u8): usize;
export fn eqAscii(a: string, b: string): bool;
export fn cmpAscii(a: string, b: string): i32;
export fn cmpUnicode(a: string, b: string): i32;
export fn eqUnicode(a: string, b: string): bool;
export fn hashAscii(s: string): u64;
export fn hashUnicode(s: string): u64;
export fn startsWithIgnoreCase(haystack: string, prefix: string): bool;
export fn endsWithIgnoreCase(haystack: string, suffix: string): bool;
export fn containsIgnoreCase(haystack: string, needle: string): bool;
export class Ascii {
    raw: string,
    fn new(s: string): Ascii;
    fn intoInner(): string;
    fn eq(other: Ascii): bool;
    fn cmp(other: Ascii): i32;
    fn hash(): u64;
}

export class UniCase {
    raw: string,
    fn new(s: string): UniCase;
    fn ascii(s: string): UniCase;
    fn intoInner(): string;
    fn eq(other: UniCase): bool;
    fn cmp(other: UniCase): i32;
    fn hash(): u64;
}

