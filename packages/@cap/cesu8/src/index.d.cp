// Type definitions for CP

export fn toCesu8(utf8: &[]u8, modified_nul: bool, out: []u8): i64;
export fn fromCesu8(cesu: &[]u8, modified_nul: bool, out: []u8): i64;
export fn isValidCesu8AsciiOrBmp(utf8: &[]u8, modified_nul: bool): bool;
