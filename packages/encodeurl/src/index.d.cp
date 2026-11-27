// Type definitions for CP

export fn needsUrlEncode(b: u8): bool;
export fn encodedUrlLen(url: string): usize;
export fn encodeUrl(url: string, out: []u8): usize;
export fn encodeUrlChecked(url: string, out: []u8): usize?;
export fn encodeUrlNormalized(url: string, out: []u8): usize;
export fn decodeUrl(url: string, out: []u8): usize;
export fn isEncodedUrl(url: string): bool;
