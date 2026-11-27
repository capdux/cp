// Type definitions for CP

export fn euIsHex(b: u8): bool;
export fn euNeedsEncode(b: u8): bool;
export fn encodeUrl(url: string, out: []u8): usize;
export fn isEncodedUrl(url: string): bool;
