// Type definitions for CP

export fn checkCastLayout(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): u8;
export fn writeU16Le(v: u16, out: []u8): bool;
export fn readU16Le(buf: []u8): u16?;
export fn writeU32Le(v: u32, out: []u8): bool;
export fn readU32Le(buf: []u8): u32?;
export fn writeU32Be(v: u32, out: []u8): bool;
export fn readU32Be(buf: []u8): u32?;
export fn writeU64Le(v: u64, out: []u8): bool;
export fn readU64Le(buf: []u8): u64?;
export fn castU32SliceToBytesLe(src: []u32, dst: []u8): usize?;
export fn castBytesToU32SliceLe(src: []u8, dst: []u32): usize?;
export fn swapBytesU32(v: u32): u32;
