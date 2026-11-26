// Type definitions for CP

export fn isHexDigit(b: u8): bool;
export fn hexUpper(nibble: u8): u8;
export fn isSafeUriByte(b: u8): bool;
export fn encodeUrl(url: &[]u8, dst: []u8): i64;
export fn isEncodedUrl(url: &[]u8): bool;
export fn isRegExpMeta(b: u8): bool;
export fn escapeRegExp(src: &[]u8, dst: []u8): i64;
