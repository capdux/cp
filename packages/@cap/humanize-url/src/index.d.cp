// Type definitions for CP

export fn hurlIsSpace(c: u8): bool;
export fn hurlTrim(s: string): string;
export fn hurlToLowerAscii(c: u8): u8;
export fn hurlStartsWithLower(s: string, lower_prefix: string): bool;
export fn hurlHasDot(s: string): bool;
export fn humanizeUrlSlice(url: string): string;
export fn humanizeUrl(url: string, out: []u8): usize;
