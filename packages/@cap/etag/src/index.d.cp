// Type definitions for CP

export class EntityTag {
    weak: bool,
    tag: string,
}

export fn etgIsSpace(b: u8): bool;
export fn etgIsEtagChar(b: u8): bool;
export fn etgStrEq(a: string, b: string): bool;
export fn parseEtag(input: string, out: &mut EntityTag): bool;
export fn formatEtag(weak: bool, tag: string, out: []u8): usize;
export fn strongEqual(a: EntityTag, b: EntityTag): bool;
export fn weakEqual(a: EntityTag, b: EntityTag): bool;
export fn etgHexLower(nibble: u8): u8;
export fn etgWriteHexU64(val: u64, out: []u8, start: usize): usize;
export fn etgHexLenU64(val: u64): usize;
export fn generateStatEtag(size: u64, mtime_ms: u64, out: []u8): usize;
export fn etgFnv1a64(data: &[]u8): u64;
export fn generateContentEtag(data: &[]u8, weak: bool, out: []u8): usize;
export fn etgMatchHeaderList(header: string, current: EntityTag, require_strong: bool): bool;
export fn matchIfNoneMatch(header: string, current: EntityTag): bool;
export fn matchIfMatch(header: string, current: EntityTag): bool;
export fn compareStrong(a: string, b: string): bool;
export fn compareWeak(a: string, b: string): bool;
