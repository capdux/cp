// Type definitions for CP

export class RdnAttr {
    attr: string,
    raw_value: string,
}

export fn isWs(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn fromHex(c: u8): i32;
export fn isValidAttrType(attr: string): bool;
export fn isEscapedAt(s: string, start: usize, idx: usize): bool;
export fn hasValidEscapes(raw: string): bool;
export fn parseDn(dn: string, out: []RdnAttr): usize;
export fn escapeValue(value: string, out: []u8): usize;
export fn unescapeValue(raw: string, out: []u8): usize;
export fn findAttr(dn: string, target_attr: string, out: []u8, out_len: &mut usize): bool;
export fn isValidDn(dn: string): bool;
