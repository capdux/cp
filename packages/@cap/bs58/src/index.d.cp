// Type definitions for CP

export enum Base58Alphabet {
    Bitcoin,
    Ripple,
    Flickr
}

export fn alphabetChars(alpha: Base58Alphabet): string;
export fn charIndex(alphabet: string, c: u8): i8;
export fn isValidBase58(encoded: string, alpha: Base58Alphabet): bool;
export fn encodeBase58With(input: string, alpha: Base58Alphabet, out: []u8): string?;
export fn decodeBase58With(encoded: string, alpha: Base58Alphabet, out: []u8): string?;
export fn encodeBase58(input: string, out: []u8): string?;
export fn decodeBase58(encoded: string, out: []u8): string?;
