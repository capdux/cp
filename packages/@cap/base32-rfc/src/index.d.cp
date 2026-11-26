// Type definitions for CP

export fn toUpperAscii(b: u8): u8;
export fn decodeStdChar(b: u8): i32;
export fn decodeHexChar(b: u8): i32;
export fn encodedLen(byte_len: usize, pad: bool): usize;
export fn encodeWithAlphabet(data: &[]u8, pad: bool, alphabet: string, out: []u8): usize;
export fn decodeWithMode(input: string, is_hex: bool, out: []u8): i64;
export fn encode(data: &[]u8, pad: bool, out: []u8): usize;
export fn decode(input: string, out: []u8): i64;
export fn encodeHex(data: &[]u8, pad: bool, out: []u8): usize;
export fn decodeHex(input: string, out: []u8): i64;
