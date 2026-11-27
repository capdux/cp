// Type definitions for CP

export fn nvecMaxNibbles(): usize;
export fn nvecGetRaw(data: [64]u8, idx: usize): u8;
export class NibbleVec {
    data: [64]u8,
    nibble_len: usize,
    fn new(): NibbleVec;
    fn fromBytes(bytes: &[]u8): NibbleVec;
    fn fromByteSlice(bytes: &[]u8): NibbleVec;
    fn len(): usize;
    fn isEmpty(): bool;
    fn byteLen(): usize;
    fn asBytes(_: auto): &[]u8;
    fn copyBytes(dst: []u8): usize;
    fn push(_: auto, nib: u8): bool;
    fn pop(_: auto): u8?;
    fn get(idx: usize): u8?;
    fn set(_: auto, idx: usize, nib: u8): bool;
    fn clear(_: auto);
    fn truncate(_: auto, new_len: usize);
    fn split(_: auto, idx: usize): NibbleVec;
    fn join(other: NibbleVec): NibbleVec;
    fn extend(_: auto, other: NibbleVec): bool;
    fn slice(start: usize, end: usize): NibbleVec;
    fn commonPrefixLen(other: NibbleVec): usize;
    fn hasPrefix(prefix: NibbleVec): bool;
    fn equals(other: NibbleVec): bool;
}

