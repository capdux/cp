// Type definitions for CP

export fn isSpace(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn trimSlice(s: &[]u8): &[]u8;
export fn eqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn isStar(s: &[]u8): bool;
export fn parseQValue(s_in: &[]u8): i32;
export class ParsedItem {
    token: &[]u8,
    q: i32,
    valid: bool,
}

export fn parseHeaderItem(raw: &[]u8): ParsedItem;
export fn matchEncodingSpec(headerTok: &[]u8, cand: &[]u8): u8;
export fn isLangPrefix(prefix: &[]u8, full: &[]u8): bool;
export fn matchLanguageSpec(headerTok: &[]u8, cand: &[]u8): u8;
export fn findSlash(s: &[]u8): usize;
export fn matchMediaSpec(headerTok: &[]u8, cand: &[]u8): u8;
export fn matchByMode(mode: u8, headerTok: &[]u8, cand: &[]u8): u8;
export fn negotiateCore(accept_header: &[]u8, available: &[]&[]u8, mode: u8): i64;
export fn negotiateEncoding(accept_header: &[]u8, available: &[]&[]u8): i64;
export fn negotiateLanguage(accept_header: &[]u8, available: &[]&[]u8): i64;
export fn negotiateMediaType(accept_header: &[]u8, available: &[]&[]u8): i64;
