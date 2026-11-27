// Type definitions for CP

export fn bcBuildDecode32Lut(): [256]u8;
export fn bcBuildDecode37Lut(): [256]u8;
export fn bcEncodeDigit32(v: u8): u8;
export fn bcEncodeCheck37(v: u8): u8;
export fn bcDecodeDigit32(c: u8): u8;
export fn bcDecodeCheck37(c: u8): u8;
export fn encodeU64(val: u64, with_check: bool, out: []u8): usize;
export fn decodeU64(input: string, with_check: bool, out_val: &mut u64): bool;
export fn encodeBytes(data: &[]u8, out: []u8): usize;
export fn decodeBytes(input: string, out: []u8): i64;
export fn formatGrouped(raw: string, group_size: usize, out: []u8): usize;
