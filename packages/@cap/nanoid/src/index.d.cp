// Type definitions for CP

export error NanoidError { InvalidAlphabet, InvalidSize, BufferTooSmall, EntropyExhausted }

export fn fillCsprng(buf: []u8);
export fn calcBitmask(alphabet_len: usize): u8;
export fn entropyBits(alphabet_len: usize, size: usize): f64;
export fn generateFromBytes(alphabet: &[]u8, size: usize, random_bytes: &[]u8, out: []u8): &[]u8 ! NanoidError;
export fn nanoidCustom(alphabet: &[]u8, size: usize, out: []u8): &[]u8 ! NanoidError;
export fn nanoidLen(size: usize, out: []u8): &[]u8 ! NanoidError;
export fn nanoid(): [21]u8;
export fn isValidCustom(id: &[]u8, alphabet: &[]u8, expected_len: usize): bool;
export fn isValidNanoid(id: &[]u8): bool;
