// Type definitions for CP

export fn toUpperAscii(c: u8): u8;
export fn isUpperAlpha(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn countryLength(c0: u8, c1: u8): usize;
export fn electronicFormat(input: string, out: []u8): usize;
export fn friendlyFormat(input: string, out: []u8): usize;
export fn mod97(electronic: string): u32;
export fn isValid(input: string): bool;
export fn computeCheckDigits(country0: u8, country1: u8, bban: string, out_d0: &mut u8, out_d1: &mut u8): bool;
