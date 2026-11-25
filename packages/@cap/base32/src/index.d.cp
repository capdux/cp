// Type definitions for CP

export error Base32Error { InvalidCharacter, InvalidLength, InvalidPadding, BufferTooSmall }

export enum Base32Alphabet {
    Rfc4648,
    Rfc4648Hex,
    Crockford,
    ZBase32
}

export fn alphabetTable(alphabet: Base32Alphabet): &[]u8;
export fn decodeCharRfc4648(ch: u8): u8?;
export fn decodeCharRfc4648Hex(ch: u8): u8?;
export fn decodeCharCrockford(ch: u8): u8?;
export fn decodeCharZBase32(ch: u8): u8?;
export fn decodeChar(alphabet: Base32Alphabet, ch: u8): u8?;
export fn encodedLen(input_len: usize, padded: bool): usize;
export fn decodedMaxLen(input_len: usize): usize;
export fn encode(input: &[]u8, alphabet: Base32Alphabet, padded: bool, out: []u8): &[]u8 ! Base32Error;
export fn decode(input: &[]u8, alphabet: Base32Alphabet, out: []u8): &[]u8 ! Base32Error;
export fn encodeStd(input: &[]u8, out: []u8): &[]u8 ! Base32Error;
export fn decodeStd(input: &[]u8, out: []u8): &[]u8 ! Base32Error;
export fn encodeStdNoPad(input: &[]u8, out: []u8): &[]u8 ! Base32Error;
export fn encodeHex(input: &[]u8, out: []u8): &[]u8 ! Base32Error;
export fn decodeHex(input: &[]u8, out: []u8): &[]u8 ! Base32Error;
export fn encodeCrockford(input: &[]u8, out: []u8): &[]u8 ! Base32Error;
export fn decodeCrockford(input: &[]u8, out: []u8): &[]u8 ! Base32Error;
