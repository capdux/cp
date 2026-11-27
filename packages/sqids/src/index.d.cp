// Type definitions for CP

import { isDigit, toLower } from "cap:ascii";
import { bytesEq, startsWithBytes, endsWithBytes, findByteInBytes, indexOfBytes } from "cap:bin";
import { writeStr } from "cap:fmt";
export enum SqidsError {
    None = 0,
    AlphabetTooShort = 1,
    AlphabetTooLong = 2,
    AlphabetNonAscii = 3,
    AlphabetDuplicateChar = 4
}

export class SqidsOptions {
    alphabet: string,
    min_length: u8,
}

export fn hasDigit(s: string): bool;
export fn isBlockedId(id: &[]u8): bool;
export fn shuffleAlphabet(buf: []u8);
export fn reverseBytes(buf: []u8);
export fn appendToId(num: u64, alphabet: &[]u8, dst: []u8, pos: usize): usize?;
export fn chunkToNumber(chunk: &[]u8, alphabet: &[]u8): u64?;
export class Sqids {
    alphabet: [MAX_ALPHABET_LEN]u8,
    alphabet_len: usize,
    min_length: u8,
    valid: bool,
    err: SqidsError,
    fn initDefault(): Sqids;
    fn init(opts: SqidsOptions): Sqids;
    fn isValid(): bool;
    fn encode(numbers: &[]u64, out: []u8): string;
    fn encodeOne(n: u64, out: []u8): string;
    fn decode(id: string, out: []u64): usize;
    fn decodeOne(id: string): u64?;
}

