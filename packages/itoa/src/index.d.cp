// Type definitions for CP

export fn formatU64(val: u64, out: []u8): usize;
export fn formatI64(val: i64, out: []u8): usize;
export fn formatU32(val: u32, out: []u8): usize;
export fn formatI32(val: i32, out: []u8): usize;
export fn formatUsize(val: usize, out: []u8): usize;
export fn formatHexU64(val: u64, uppercase: bool, out: []u8): usize;
export fn formatRadixU64(val: u64, radix: u8, out: []u8): usize;
