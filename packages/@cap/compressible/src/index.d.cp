// Type definitions for CP

export fn cmprIsSpace(b: u8): bool;
export fn cmprToLower(b: u8): u8;
export fn cmprEqlIgnoreCase(a: string, b: string): bool;
export fn cmprHasPrefixIgnoreCase(s: string, prefix: string): bool;
export fn cmprHasSuffixIgnoreCase(s: string, suffix: string): bool;
export fn cmprFindSlash(s: string): usize;
export fn stripContentTypeParams(ct: string): string;
export fn isCompressible(content_type: string): bool;
