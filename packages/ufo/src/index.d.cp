// Type definitions for CP

export fn ufoIsAlpha(c: u8): bool;
export fn ufoIsSchemeChar(c: u8): bool;
export fn ufoProtocolEnd(url: string): usize;
export fn ufoCopyBytes(src: string, out: []u8, offset: usize): usize;
export fn hasProtocol(url: string): bool;
export fn isRelative(url: string): bool;
export fn withoutLeadingSlash(s: string): string;
export fn withoutTrailingSlash(s: string): string;
export fn withLeadingSlash(s: string, out: []u8): usize;
export fn withTrailingSlash(s: string, out: []u8): usize;
export fn cleanDoubleSlashes(url: string, out: []u8): usize;
export fn joinUrl(base: string, segment: string, out: []u8): usize;
