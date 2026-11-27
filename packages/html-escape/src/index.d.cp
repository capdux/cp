// Type definitions for CP

export fn eqBytes(a: string, b: string): bool;
export fn appendStr(dst: []u8, pos: usize, src: string): usize;
export fn needsEscape(input: string): bool;
export fn escapeHtml(input: string, out: []u8): usize;
export fn encodeUtf8(cp: u32, out: []u8, pos: usize): usize;
export fn decodeNamedEntity(name: string): u32;
export fn decodeNumericEntity(body: string): u32;
export fn unescapeHtml(input: string, out: []u8): usize;
