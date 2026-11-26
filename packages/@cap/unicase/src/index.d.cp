// Type definitions for CP

export fn foldAsciiLower(b: u8): u8;
export fn foldAsciiUpper(b: u8): u8;
export fn mulWrap64(a: u64, b: u64): u64;
export fn eqIgnoreAsciiCase(a: string, b: string): bool;
export fn cmpIgnoreAsciiCase(a: string, b: string): i32;
export fn startsWithIgnoreCase(s: string, prefix: string): bool;
export fn endsWithIgnoreCase(s: string, suffix: string): bool;
export fn indexOfIgnoreCase(haystack: string, needle: string): usize?;
export fn containsIgnoreCase(haystack: string, needle: string): bool;
export fn hashIgnoreCase64(s: string): u64;
export fn toLowerAscii(s: string, out: []u8): string;
export fn toUpperAscii(s: string, out: []u8): string;
export class UniCase {
    inner: string,
    fn new(s: string): UniCase;
    fn asStr(): string;
    fn equals(other: UniCase): bool;
    fn cmp(other: UniCase): i32;
    fn hash64(): u64;
}

