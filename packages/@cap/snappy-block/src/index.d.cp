// Type definitions for CP

export fn maxCompressedLen(input_len: usize): usize;
export class VarintRes {
    ok: bool,
    value: u32,
    bytes_read: usize,
}

export fn decodeVarint32(src: &[]u8): VarintRes;
export fn encodeVarint32(val: u32, dst: []u8, start_pos: usize): i64;
export fn uncompressedLen(src: &[]u8): i64;
export fn readU32LE(buf: &[]u8, pos: usize): u32;
export fn hash4(buf: &[]u8, pos: usize): usize;
export fn match4(buf: &[]u8, a: usize, b: usize): bool;
export fn emitLiteral(src: &[]u8, start: usize, len: usize, dst: []u8, start_pos: usize): i64;
export fn emitCopyChunk(offset: usize, chunk_len: usize, dst: []u8, start_pos: usize): i64;
export fn emitCopy(offset: usize, total_len: usize, dst: []u8, start_pos: usize): i64;
export fn compress(src: &[]u8, dst: []u8): i64;
export fn decompress(src: &[]u8, dst: []u8): i64;
