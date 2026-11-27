// Type definitions for CP

import { fnv1a64, fnv1a64Update } from "../../fnv-hash/src/index.cp";
export fn c2Mix(x: u64): u64;
export fn c2Base36Char(v: u64): u8;
export fn c2LetterChar(v: u64): u8;
export fn isCuid2(id: string): bool;
export fn isCuid2WithBounds(id: string, min_len: usize, max_len: usize): bool;
export class Cuid2Generator {
    state: u64,
    counter: u64,
    fingerprint_hash: u64,
    id_len: usize,
    fn new(seed: u64, fingerprint: string, id_len: usize): Cuid2Generator;
    fn length(): usize;
    fn next(_: auto, timestamp_ms: u64, out: []u8): usize;
}

