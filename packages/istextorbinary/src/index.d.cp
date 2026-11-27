// Type definitions for CP

export fn msToLower(c: u8): u8;
export fn msIsSpace(c: u8): bool;
export fn msIsTagTerminator(c: u8): bool;
export fn msStartsWithIgnoreCase(s: &[]u8, prefix: &[]u8): bool;
export fn msMatchTag(s: &[]u8, tagPrefix: &[]u8): bool;
export fn isBinary(data: &[]u8): bool;
export fn isText(data: &[]u8): bool;
export fn sniffContentType(data: &[]u8): u8;
export fn contentTypeToMime(ct: u8): string;
export fn sniffMime(data: &[]u8): string;
