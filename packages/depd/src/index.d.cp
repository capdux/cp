// Type definitions for CP

import { eqIgnoreCase, trim } from "cap:ascii";
import { fnv64OffsetBasis, mulFnv64Prime } from "../../@sindresorhus/fnv1a/src/index.cp";

export const MAX_SEEN_HASHES: usize;

export struct DepdOptions {
    no_deprecation: bool,
    trace_deprecation: bool,
    fn defaults(): DepdOptions;
}

export fn depdHashKey(ns: string, msg: string): u64;
export fn copySlice(dst: []u8, pos: usize, src: string): usize;
export fn isNamespaceSilenced(namespace: string, no_deprecation_env: string): bool;
export fn isNamespaceTraced(namespace: string, trace_deprecation_env: string): bool;

export struct Deprecator {
    namespace: string,
    silenced: bool,
    no_deprecation: bool,
    trace_deprecation: bool,
    seen_hashes: [64]u64,
    seen_count: usize,
    emitted_count: u64,
    fn new(namespace: string): Deprecator;
    fn withOptions(namespace: string, opts: DepdOptions): Deprecator;
    fn setSilenced(_: auto, flag: bool);
    fn setNoDeprecation(_: auto, flag: bool);
    fn setTraceDeprecation(_: auto, flag: bool);
    fn isSilenced(): bool;
    fn isTraceEnabled(): bool;
    fn seenCount(): usize;
    fn hasWarned(msg: string): bool;
    fn warnOnce(_: auto, msg: string, site: string, out: []u8): usize;
    fn warnWithTrace(_: auto, msg: string, site: string, trace: string, out: []u8): usize;
    fn deprecate(_: auto, msg: string, site: string, out: []u8): usize;
    fn deprecateFn(_: auto, fn_name: string, msg: string, site: string, out: []u8): usize;
    fn reset(_: auto);
}

export fn formatDeprecationLen(namespace: string, msg: string, site: string): usize;
export fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize;
export fn formatDeprecationWithTrace(namespace: string, msg: string, site: string, trace: string, include_trace: bool, out: []u8): usize;
