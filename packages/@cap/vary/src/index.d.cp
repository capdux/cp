// Type definitions for CP

export fn isWs(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn eqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn isTchar(c: u8): bool;
export fn isValidFieldName(field: &[]u8): bool;
export fn isWildcard(tok: &[]u8): bool;
export fn hasField(header: &[]u8, field: &[]u8): bool;
export fn headerHasWildcard(src: &[]u8): bool;
export fn validateFieldList(field_list: &[]u8): bool;
export fn dstContainsToken(dst: &[]u8, written: usize, tok: &[]u8): bool;
export fn appendTokens(src: &[]u8, dst: []u8, start_pos: usize): i64;
export fn append(header: &[]u8, field: &[]u8, dst: []u8): i64;
export fn appendMany(header: &[]u8, fields: &[]&[]u8, dst: []u8): i64;
