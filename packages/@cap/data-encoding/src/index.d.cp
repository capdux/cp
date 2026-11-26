// Type definitions for CP

export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn encodeBase32Core(input: string, alpha: string, pad: bool, out: []u8): string;
export fn base32Val(c: u8): u8?;
export fn base32HexVal(c: u8): u8?;
export fn crockfordVal(c: u8): u8?;
export fn decodeBase32Standard(encoded: string, is_hex: bool, out: []u8): string?;
export fn encodeBase32(input: string, pad: bool, out: []u8): string;
export fn decodeBase32(encoded: string, out: []u8): string?;
export fn encodeBase32Hex(input: string, pad: bool, out: []u8): string;
export fn decodeBase32Hex(encoded: string, out: []u8): string?;
export fn encodeCrockford(input: string, out: []u8): string;
export fn decodeCrockford(encoded: string, out: []u8): string?;
export fn b64UrlVal(c: u8): u8?;
export fn encodeBase64UrlNoPad(input: string, out: []u8): string;
export fn decodeBase64UrlNoPad(encoded: string, out: []u8): string?;
