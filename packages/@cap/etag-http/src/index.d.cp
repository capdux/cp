// Type definitions for CP

import { fnv1a64, formatHex64 } from "../../fnv-hash/src/index.cp";
export class ParsedEtag {
    valid: bool,
    weak: bool,
    opaque_tag: string,
}

export fn etgPush(out: []u8, pos: usize, s: string): usize;
export fn etgPushHexU64(out: []u8, pos: usize, v: u64): usize;
export fn etgStrEq(a: string, b: string): bool;
export fn generateEtag(entity: &[]u8, weak: bool, out: []u8): usize;
export fn generateStatEtag(size_bytes: u64, mtime_ms: u64, out: []u8): usize;
export fn parseEtag(raw: string): ParsedEtag;
export fn compareStrong(a: string, b: string): bool;
export fn compareWeak(a: string, b: string): bool;
