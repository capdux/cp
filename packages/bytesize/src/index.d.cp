// Type definitions for CP

import { isSpace, isDigit, toLowerByte, trim } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export fn bszWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn bszUnitMultiplier(s: string): u64?;
export fn bszFormatScaled(bytes: u64, unit: u64, suffix: string, out: []u8): usize;
export fn formatSi(bytes: u64, out: []u8): usize;
export fn formatIec(bytes: u64, out: []u8): usize;
export fn parseByteSize(input: string): u64?;
export class ByteSize {
    val: u64,
    fn b(n: u64): ByteSize;
    fn kb(n: u64): ByteSize;
    fn mb(n: u64): ByteSize;
    fn gb(n: u64): ByteSize;
    fn tb(n: u64): ByteSize;
    fn pb(n: u64): ByteSize;
    fn kib(n: u64): ByteSize;
    fn mib(n: u64): ByteSize;
    fn gib(n: u64): ByteSize;
    fn tib(n: u64): ByteSize;
    fn pib(n: u64): ByteSize;
    fn asU64(): u64;
    fn add(other: ByteSize): ByteSize;
    fn subSaturating(other: ByteSize): ByteSize;
    fn displaySi(out: []u8): usize;
    fn displayIec(out: []u8): usize;
}

