// Type definitions for CP

export fn parseHexNibble(c: u8): u8?;
export fn hexNibbleLower(v: u8): u8;
export fn uuidToBytes(uuid: string, out16: []u8): bool;
export fn bytesToUuid(bytes16: []u8, out36: []u8): string;
export fn validateAlphabet(alphabet: string): bool;
export fn findInAlphabet(alphabet: string, c: u8): u32?;
export fn maxEncodedLen(base: u32): usize;
export fn shortenUuidWith(uuid: string, alphabet: string, out: []u8): string?;
export fn expandUuidWith(short_id: string, alphabet: string, out36: []u8): string?;
export fn shortenUuid(uuid: string, out: []u8): string?;
export fn expandUuid(short_id: string, out36: []u8): string?;
