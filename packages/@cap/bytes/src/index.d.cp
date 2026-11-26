// Type definitions for CP

export fn toLower(c: u8): u8;
export fn isSpace(c: u8): bool;
export fn unitMultiplier(unit: string, binary_default: bool): u64;
export fn parseBytes(input: string, binary_default: bool): i64;
export fn parse(input: string): i64;
export fn writeU64(val: u64, out: []u8): usize;
export fn writeStr(s: string, out: []u8): usize;
export fn formatBytes(bytes: u64, binary: bool, decimals: u32, out: []u8): usize;
export fn formatIec(bytes: u64, out: []u8): usize;
export fn formatSi(bytes: u64, out: []u8): usize;
