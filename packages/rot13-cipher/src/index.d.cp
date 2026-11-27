// Type definitions for CP

export fn shiftLetter(c: u8, norm_shift: u8): u8;
export fn rot13(input: string, out: []u8): usize;
export fn rot13InPlace(buf: []u8);
export fn rot47(input: string, out: []u8): usize;
export fn caesar(input: string, shift: i32, out: []u8): usize;
export fn atbash(input: string, out: []u8): usize;
