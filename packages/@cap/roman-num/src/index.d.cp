// Type definitions for CP

export fn toUpperAscii(b: u8): u8;
export fn romanCharVal(b: u8): i32;
export fn toRoman(num: u16, out: []u8): usize;
export fn parseRoman(input: string): i32;
export fn isValidRoman(input: string): bool;
