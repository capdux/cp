// Type definitions for CP

import { fnv64OffsetBasis, mulFnv64Prime } from "../../fnv-hash/src/index.cp";
export fn depdHashKey(ns: string, msg: string): u64;
export fn copySlice(dst: []u8, pos: usize, src: string): usize;
export class Deprecator {
    namespace: string,
    silenced: bool,
    seen_hashes: [64]u64,
    seen_count: usize,
    emitted_count: u64,
    fn new(namespace: string): Deprecator;
    fn setSilenced(_: auto, flag: bool);
    fn hasWarned(msg: string): bool;
    fn warnOnce(_: auto, msg: string, site: string, out: []u8): usize;
    fn reset(_: auto);
}

export fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize;
