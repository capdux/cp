// Type definitions for CP

export class BufferList {
    chunks: [64]&[]u8,
    chunk_count: usize,
    total_len: usize,
    fn len(): usize;
    fn isEmpty(): bool;
    fn append(_: auto, chunk: &[]u8): bool;
    fn get(index: usize): u8?;
    fn copyRange(start: usize, end: usize, dst: []u8): usize;
    fn consume(_: auto, n: usize);
    fn readU16BE(offset: usize): u16?;
    fn readU16LE(offset: usize): u16?;
    fn readU32BE(offset: usize): u32?;
    fn readU32LE(offset: usize): u32?;
    fn indexOf(needle: &[]u8, from_offset: usize): usize?;
}

export fn newBufferList(): BufferList;
