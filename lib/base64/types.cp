// Source: bun/src/base64/base64.cp -> cp/lib/base64/types.cp (Base64 variants, errors & lookup tables)

/// Supported RFC 4648 Base64 encoding variants.
pub enum Base64Variant {
    /// Standard Base64 (`+` and `/`) with `=` padding.
    Standard,
    /// Standard Base64 (`+` and `/`) without padding.
    StandardNoPad,
    /// URL-safe Base64 (`-` and `_`) with `=` padding.
    UrlSafe,
    /// URL-safe Base64 (`-` and `_`) without padding.
    UrlSafeNoPad,
}

/// Unified error set for `cap:base64` encoding and decoding operations.
pub err Base64Error {
    /// Encountered a byte outside the selected Base64 alphabet.
    InvalidCharacter,
    /// Input length is invalid (e.g., `len % 4 == 1`, or unpadded length in padded mode).
    InvalidLength,
    /// Padding character `=` is misplaced, missing, unexpected, or excessive.
    InvalidPadding,
    /// RFC 4648 §3.5: unused trailing padding bits in the final quantum are non-zero.
    NonCanonicalPaddingBits,
    /// Provided destination buffer is too small for the encoded/decoded output.
    DestinationTooSmall,
    /// Memory allocation failed.
    OutOfMemory,
}

pub const STANDARD_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
pub const URL_SAFE_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
pub const PAD_CHAR: u8 = '=';
pub const INVALID_INDEX: u8 = 255;

fn buildDecodeTable(alphabet: []const u8): [256]u8 {
    let table: [256]u8 = [INVALID_INDEX; 256];
    let i: usize = 0;
    while (i < 64) {
        table[alphabet[i] as usize] = i as u8;
        i += 1;
    }
    return table;
}

pub const STANDARD_DECODE_TABLE: [256]u8 = buildDecodeTable(STANDARD_ALPHABET);
pub const URL_SAFE_DECODE_TABLE: [256]u8 = buildDecodeTable(URL_SAFE_ALPHABET);

/// Returns `true` if the variant requires `=` padding.
pub fn hasPadding(variant: Base64Variant): bool {
    switch (variant) {
        .Standard, .UrlSafe => return true,
        .StandardNoPad, .UrlSafeNoPad => return false,
    }
}

/// Returns the 64-byte encoding alphabet for `variant`.
pub fn alphabetFor(variant: Base64Variant): []const u8 {
    switch (variant) {
        .Standard, .StandardNoPad => return STANDARD_ALPHABET,
        .UrlSafe, .UrlSafeNoPad => return URL_SAFE_ALPHABET,
    }
}

/// Returns the 256-byte O(1) decoding lookup table for `variant`.
pub fn decodeTableFor(variant: Base64Variant): [256]u8 {
    switch (variant) {
        .Standard, .StandardNoPad => return STANDARD_DECODE_TABLE,
        .UrlSafe, .UrlSafeNoPad => return URL_SAFE_DECODE_TABLE,
    }
}

/// Returns `true` if `c` is ASCII whitespace ignored by MIME Base64 decoding (` `, `\r`, `\n`, `\t`).
pub fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}
