// Type definitions for CP

export fn isDigit(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn compareImpl(a: string, b: string, ignore_case: bool): i32;
export fn compareNatural(a: string, b: string): i32;
export fn compareNaturalIgnoreCase(a: string, b: string): i32;
export fn isSortedNatural(items: &[]string): bool;
export fn sortNatural(items: []string);
