// Type definitions for CP

export error Base58Error { InvalidAlphabet, InvalidCharacter, BufferTooSmall, InputTooLarge, ChecksumMismatch, InvalidLength }

export fn findInAlphabet(alphabet: &[]u8, ch: u8): u8?;
export fn encodeWithAlphabet(input: &[]u8, alphabet: &[]u8, out: []u8): &[]u8 ! Base58Error;
export fn decodeWithAlphabet(input: &[]u8, alphabet: &[]u8, out: []u8): &[]u8 ! Base58Error;
export fn encode(input: &[]u8, out: []u8): &[]u8 ! Base58Error;
export fn decode(input: &[]u8, out: []u8): &[]u8 ! Base58Error;
export fn doubleSha256(data: &[]u8): [32]u8;
export fn encodeCheck(payload: &[]u8, out: []u8): &[]u8 ! Base58Error;
export fn decodeCheck(input: &[]u8, out: []u8): &[]u8 ! Base58Error;
