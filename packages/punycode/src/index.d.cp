// Type definitions for CP

export fn pcToLower(c: u8): u8;
export fn pcIsXnPrefix(s: &[]u8): bool;
export fn isAsciiOnly(s: string): bool;
export fn pcAdapt(delta_in: u32, numpoints: u32, firsttime: bool): u32;
export fn pcEncodeDigit(d: u32): u8;
export fn pcDecodeDigit(c: u8): u32;
export fn pcThreshold(k: u32, bias: u32): u32;
export fn pcDecodeUtf8(s: &[]u8, cps: []u32): i64;
export fn pcEncodeUtf8(cps: &[]u32, out: []u8): i64;
export fn encodeLabel(utf8_label: string, out: []u8): i64;
export fn decodeLabel(ascii_label: string, out_utf8: []u8): i64;
export fn pcWriteSlice(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn toAsciiDomain(utf8_domain: string, out: []u8): i64;
export fn toUnicodeDomain(ascii_domain: string, out: []u8): i64;
