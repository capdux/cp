// Type definitions for CP

export fn isSpace(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn trimSlice(s: &[]u8): &[]u8;
export fn eqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn endsWithIgnoreCase(s: &[]u8, suffix: &[]u8): bool;
export fn isStar(s: &[]u8): bool;
export fn findSingleSlash(s: &[]u8): i64;
export fn stripParams(content_type: &[]u8): &[]u8;
export fn normalizeType(short_or_mime: &[]u8): &[]u8;
export fn matchStructuredSuffix(ctSub: &[]u8, plusSuffix: &[]u8): bool;
export fn mimeMatch(content_type: &[]u8, pattern: &[]u8): bool;
export fn matchAny(content_type: &[]u8, patterns: &[]&[]u8): i64;
