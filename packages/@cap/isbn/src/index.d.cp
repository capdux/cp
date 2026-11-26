// Type definitions for CP

export fn isbnToUpper(c: u8): u8;
export fn isbnIsDigit(c: u8): bool;
export fn stripHyphens(input: string, out: []u8): usize;
export fn isbnNormalizeExact(input: string, expected_len: usize, out: []u8): bool;
export fn computeIsbn13CheckDigit(prefix12: string): u8;
export fn isValidIsbn10(input: string): bool;
export fn isValidEan13(input: string): bool;
export fn isValidIsbn13(input: string): bool;
export fn isValidIssn(input: string): bool;
export fn isValidIsbn(input: string): bool;
export fn isbn10To13(isbn10: string, out: []u8): usize;
export fn isbn13To10(isbn13: string, out: []u8): usize;
