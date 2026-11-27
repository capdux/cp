// Type definitions for CP

export fn nvecMaxNibbles(): usize;
export fn nvecGetRaw(data: [64]u8, idx: usize): u8;
export class NibbleVec {
    data: [64]u8,
    nibble_len: usize,
    fn new(): NibbleVec;
    fn fromBytes(bytes: &[]u8): NibbleVec;
    fn len(): usize;
    fn isEmpty(): bool;
    fn push(_: auto, nib: u8): bool;
    fn pop(_: auto): u8?;
    fn get(idx: usize): u8?;
    fn set(_: auto, idx: usize, nib: u8): bool;
    fn commonPrefixLen(other: NibbleVec): usize;
}

