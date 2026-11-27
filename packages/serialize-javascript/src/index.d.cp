// Type definitions for CP

export fn sjsWriteStr(s: string, out: []u8, pos: usize): usize;
export fn sjsHex(n: u8): u8;
export fn escapeUnsafeJson(json_text: string, out: []u8): usize;
export fn serializeString(s: string, out: []u8): usize;
export fn hasUnsafeScriptChars(s: string): bool;
