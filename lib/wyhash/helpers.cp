// Source: cp/lib/wyhash/helpers.cp (Practical hashing helpers for strings, integers, and hash combining)

import { wyhash, wyhash64 } from "./core";

const GOLDEN_RATIO_64: u64 = 0x9e3779b97f4a7c15;

pub fn hashString(s: string): u64 {
    return wyhash(0, s);
}

pub fn hashInt(val: u64): u64 {
    return wyhash64(val, GOLDEN_RATIO_64);
}

pub fn combineHashes(h1: u64, h2: u64): u64 {
    return wyhash64(h1, h2);
}
