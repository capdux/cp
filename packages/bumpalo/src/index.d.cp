// Type definitions for CP

export fn bmpIsValidAlign(alignment: usize): bool;
export fn bmpAlignUp(offset: usize, alignment: usize): usize?;
export fn bmpHandleValid(handle: BumpSlice, cursor: usize): bool;
export class BumpSlice {
    offset: usize,
    len: usize,
    fn empty(): BumpSlice;
    fn isEmpty(): bool;
    fn end(): usize;
}

export class BumpArena {
    buf: [4096]u8,
    cursor: usize,
    alloc_count: usize,
    fn new(): BumpArena;
    fn capacity(): usize;
    fn allocatedBytes(): usize;
    fn remaining(): usize;
    fn allocationCount(): usize;
    fn reset(_: auto);
    fn allocAligned(_: auto, size: usize, alignment: usize): BumpSlice?;
    fn allocBytes(_: auto, size: usize): BumpSlice?;
    fn allocSliceCopy(_: auto, src: &[]u8): BumpSlice?;
    fn allocStr(_: auto, s: string): BumpSlice?;
    fn readByte(handle: BumpSlice, idx: usize): u8?;
    fn writeByte(_: auto, handle: BumpSlice, idx: usize, val: u8): bool;
    fn matchesStr(handle: BumpSlice, expected: string): bool;
    fn checkpoint(): usize;
    fn rewind(_: auto, mark: usize): bool;
}

