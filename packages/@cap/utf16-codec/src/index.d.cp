// Type definitions for CP

export fn detectBom(data: &[]u8): u8;
export fn decodeUtf8Next(s: string, pos: usize, out_cp: &mut u32): i64;
export fn encodeUtf8Scalar(cp: u32, out: []u8, pos: usize): i64;
export fn countUtf16Units(utf8: string): i64;
export fn utf8ToUtf16(utf8: string, out: []u16): i64;
export fn utf16ToUtf8(utf16: &[]u16, out: []u8): i64;
export fn utf8ToUtf16LeBytes(utf8: string, out: []u8): i64;
export fn utf16LeBytesToUtf8(bytes: &[]u8, out: []u8): i64;
