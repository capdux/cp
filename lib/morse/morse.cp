// Source: Absorbed from /data/dev/bun/src/ (ITU-R M.1677-1 Zero-Alloc International Morse Code Codec & CW Pulse Timing Generator)

/// ITU-R M.1677-1 standard timing unit constants (in dit units).
pub const DIT_UNITS: u8 = 1;
pub const DAH_UNITS: u8 = 3;
pub const INTRA_CHAR_GAP: u8 = 1;
pub const INTER_CHAR_GAP: u8 = 3;
pub const INTER_WORD_GAP: u8 = 7;

/// Standard "PARIS " word length in dit units (43 active/gap units + 7 trailing word gap = 50).
pub const PARIS_WORD_UNITS: usize = 50;

/// Maximum symbol length supported by the International Morse lookup table (7 for '$').
pub const MAX_MORSE_CODE_LEN: usize = 7;

/// A single CW keying / buzzer / LED timing pulse in dit units.
/// `mark == true` represents key-down (signal on); `mark == false` represents key-up (silence).
pub struct MorsePulse {
    mark: bool,
    units: u8,
}

fn isAsciiWhitespace(ch: u8): bool {
    return ch == 0x20 || ch == 0x09 || ch == 0x0A || ch == 0x0D;
}

/// Converts speed in Words Per Minute (WPM, PARIS standard: 50 dit units per word)
/// to a single dit duration in milliseconds (`1200 / wpm`).
/// Returns `null` when `wpm == 0`.
pub fn wpmToDitMs(wpm: u32): u32? {
    if (wpm == 0) {
        return null;
    }
    return 1200 / wpm;
}

/// Looks up the ITU-R M.1677-1 / International Morse code string for an ASCII character.
/// Case-insensitive for `A..Z`; supports `0..9` and all 18 ITU/international punctuation symbols:
/// `. , ? ' ! / ( ) & : ; = + - _ " $ @`.
/// Returns `null` for unsupported characters (including whitespace).
pub fn charToMorse(ch: u8): ([]const u8)? {
    let up = ch;
    if (up >= 0x61 && up <= 0x7A) {
        up -= 32;
    }
    switch (up) {
        // Letters A..Z
        0x41 => return ".-",       // A
        0x42 => return "-...",     // B
        0x43 => return "-.-.",     // C
        0x44 => return "-..",      // D
        0x45 => return ".",        // E
        0x46 => return "..-.",     // F
        0x47 => return "--.",      // G
        0x48 => return "....",     // H
        0x49 => return "..",       // I
        0x4A => return ".---",     // J
        0x4B => return "-.-",      // K
        0x4C => return ".-..",     // L
        0x4D => return "--",       // M
        0x4E => return "-.",       // N
        0x4F => return "---",      // O
        0x50 => return ".--.",     // P
        0x51 => return "--.-",     // Q
        0x52 => return ".-.",      // R
        0x53 => return "...",      // S
        0x54 => return "-",        // T
        0x55 => return "..-",      // U
        0x56 => return "...-",     // V
        0x57 => return ".--",      // W
        0x58 => return "-..-",     // X
        0x59 => return "-.--",     // Y
        0x5A => return "--..",     // Z

        // Digits 0..9
        0x30 => return "-----",    // 0
        0x31 => return ".----",    // 1
        0x32 => return "..---",    // 2
        0x33 => return "...--",    // 3
        0x34 => return "....-",    // 4
        0x35 => return ".....",    // 5
        0x36 => return "-....",    // 6
        0x37 => return "--...",    // 7
        0x38 => return "---..",    // 8
        0x39 => return "----.",    // 9

        // 18 ITU / International Punctuation marks
        0x2E => return ".-.-.-",   // .
        0x2C => return "--..--",   // ,
        0x3F => return "..--..",   // ?
        0x27 => return ".----.",   // '
        0x21 => return "-.-.--",   // !
        0x2F => return "-..-.",    // /
        0x28 => return "-.--.",    // (
        0x29 => return "-.--.-",   // )
        0x26 => return ".-...",    // &
        0x3A => return "---...",   // :
        0x3B => return "-.-.-.",   // ;
        0x3D => return "-...-",    // =
        0x2B => return ".-.-.",    // +
        0x2D => return "-....-",   // -
        0x5F => return "..--.-",   // _
        0x22 => return ".-..-.",   // "
        0x24 => return "...-..-",  // $
        0x40 => return ".--.-.",   // @

        else => return null,
    }
}

/// Decodes a single Morse codeword (`.` and `-` symbols) into its uppercase ASCII character.
/// Strictly rejects empty codewords, symbols other than `.`/`-`, codewords longer than 7 symbols,
/// and any unassigned Morse bit patterns.
pub fn morseToChar(code: []const u8): u8? {
    if (code.len == 0 || code.len > MAX_MORSE_CODE_LEN) {
        return null;
    }
    // Binary heap index: leading 1 bit + 0 for '.' (dit) + 1 for '-' (dah).
    let idx: u16 = 1;
    let i: usize = 0;
    while (i < code.len) {
        const sym = code[i];
        if (sym == 0x2E) {
            idx = idx << 1;
        } else if (sym == 0x2D) {
            idx = (idx << 1) | 1;
        } else {
            return null;
        }
        i += 1;
    }

    switch (idx) {
        // 1-symbol letters
        0b10 => return 0x45,       // E (.)
        0b11 => return 0x54,       // T (-)

        // 2-symbol letters
        0b100 => return 0x49,      // I (..)
        0b101 => return 0x41,      // A (.-)
        0b110 => return 0x4E,      // N (-.)
        0b111 => return 0x4D,      // M (--)

        // 3-symbol letters
        0b1000 => return 0x53,     // S (...)
        0b1001 => return 0x55,     // U (..-)
        0b1010 => return 0x52,     // R (.-.)
        0b1011 => return 0x57,     // W (.--)
        0b1100 => return 0x44,     // D (-..)
        0b1101 => return 0x4B,     // K (-.-)
        0b1110 => return 0x47,     // G (--.)
        0b1111 => return 0x4F,     // O (---)

        // 4-symbol letters
        0b10000 => return 0x48,    // H (....)
        0b10001 => return 0x56,    // V (...-)
        0b10010 => return 0x46,    // F (..-.)
        0b10100 => return 0x4C,    // L (.-..)
        0b10110 => return 0x50,    // P (.--.)
        0b10111 => return 0x4A,    // J (.---)
        0b11000 => return 0x42,    // B (-...)
        0b11001 => return 0x58,    // X (-..-)
        0b11010 => return 0x43,    // C (-.-.)
        0b11011 => return 0x59,    // Y (-.-.)
        0b11100 => return 0x5A,    // Z (--..)
        0b11101 => return 0x51,    // Q (--.-)

        // 5-symbol digits & punctuation
        0b100000 => return 0x35,   // 5 (.....)
        0b100001 => return 0x34,   // 4 (....-)
        0b100011 => return 0x33,   // 3 (...--)
        0b100111 => return 0x32,   // 2 (..---)
        0b101111 => return 0x31,   // 1 (.----)
        0b110000 => return 0x36,   // 6 (-....)
        0b111000 => return 0x37,   // 7 (--...)
        0b111100 => return 0x38,   // 8 (---..)
        0b111110 => return 0x39,   // 9 (----.)
        0b111111 => return 0x30,   // 0 (-----)
        0b101000 => return 0x26,   // & (.-...)
        0b101010 => return 0x2B,   // + (.-.-.)
        0b110001 => return 0x3D,   // = (-...-)
        0b110010 => return 0x2F,   // / (-..-.)
        0b110110 => return 0x28,   // ( (-.--.)

        // 6-symbol punctuation
        0b1010101 => return 0x2E,  // . (.-.-.-)
        0b1110011 => return 0x2C,  // , (--..--)
        0b1001100 => return 0x3F,  // ? (..--..)
        0b1011110 => return 0x27,  // ' (.----.)
        0b1101011 => return 0x21,  // ! (-.-.--)
        0b1101101 => return 0x29,  // ) (-.--.-)
        0b1111000 => return 0x3A,  // : (---...)
        0b1101010 => return 0x3B,  // ; (-.-.-.)
        0b1100001 => return 0x2D,  // - (-....-)
        0b1001101 => return 0x5F,  // _ (..--.-)
        0b1010010 => return 0x22,  // " (.-..-.)
        0b1011010 => return 0x40,  // @ (.--.-.)

        // 7-symbol punctuation
        0b10001001 => return 0x24, // $ (...-..-)

        else => return null,
    }
}

/// Encodes ASCII `text` into International Morse code inside caller-provided `dst`.
/// - Characters within a word are separated by a single space (`' '`).
/// - Words are separated by `" / "`.
/// - Leading, trailing, and repeated whitespace are collapsed/ignored.
/// - Returns `null` if any unsupported character is encountered or if `dst` is too small.
pub fn morseEncode(text: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;
    let has_emitted = false;
    let pending_word_gap = false;

    while (i < text.len) {
        const ch = text[i];
        i += 1;

        if (isAsciiWhitespace(ch)) {
            if (has_emitted) {
                pending_word_gap = true;
            }
            continue;
        }

        const code = charToMorse(ch) ?? return null;

        if (has_emitted) {
            if (pending_word_gap) {
                if (dst.len - out < 3) {
                    return null;
                }
                dst[out] = 0x20;
                dst[out + 1] = 0x2F;
                dst[out + 2] = 0x20;
                out += 3;
                pending_word_gap = false;
            } else {
                if (dst.len - out < 1) {
                    return null;
                }
                dst[out] = 0x20;
                out += 1;
            }
        }

        if (dst.len - out < code.len) {
            return null;
        }
        let k: usize = 0;
        while (k < code.len) {
            dst[out] = code[k];
            out += 1;
            k += 1;
        }
        has_emitted = true;
    }

    return out;
}

/// Decodes International Morse code from `morse` into uppercase ASCII inside `dst`.
/// - Characters are separated by 1..2 whitespace characters.
/// - Words are separated by `"/"` (e.g. `" / "`) or 3+ consecutive whitespace characters.
/// - Strictly validates every codeword and output buffer capacity; returns `null` on error.
pub fn morseDecode(morse: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;
    let pending_word_space = false;

    while (i < morse.len) {
        let ws_count: usize = 0;
        while (i < morse.len && isAsciiWhitespace(morse[i])) {
            ws_count += 1;
            i += 1;
        }
        if (i >= morse.len) {
            break;
        }

        if (out > 0 && ws_count >= 3) {
            pending_word_space = true;
        }

        const tok_start = i;
        while (i < morse.len && !isAsciiWhitespace(morse[i])) {
            i += 1;
        }
        const token = morse[tok_start..i];

        if (token.len == 1 && token[0] == 0x2F) {
            if (out > 0) {
                pending_word_space = true;
            }
            continue;
        }

        const ch = morseToChar(token) ?? return null;

        if (pending_word_space) {
            if (dst.len - out < 1) {
                return null;
            }
            dst[out] = 0x20;
            out += 1;
            pending_word_space = false;
        }

        if (dst.len - out < 1) {
            return null;
        }
        dst[out] = ch;
        out += 1;
    }

    return out;
}

/// Converts ASCII `text` directly into a zero-alloc sequence of ITU-R M.1677-1
/// `MorsePulse` mark/space intervals inside `out`.
/// Uses `DIT_UNITS (1)`, `DAH_UNITS (3)`, `INTRA_CHAR_GAP (1)`, `INTER_CHAR_GAP (3)`,
/// and `INTER_WORD_GAP (7)`.
/// Returns the number of pulses written, or `null` on unsupported character or buffer overflow.
pub fn morseToPulses(text: []const u8, out: []MorsePulse): usize? {
    let i: usize = 0;
    let count: usize = 0;
    let has_emitted = false;
    let pending_word_gap = false;

    while (i < text.len) {
        const ch = text[i];
        i += 1;

        if (isAsciiWhitespace(ch)) {
            if (has_emitted) {
                pending_word_gap = true;
            }
            continue;
        }

        const code = charToMorse(ch) ?? return null;

        if (has_emitted) {
            if (count >= out.len) {
                return null;
            }
            if (pending_word_gap) {
                out[count] = MorsePulse { mark: false, units: INTER_WORD_GAP };
                pending_word_gap = false;
            } else {
                out[count] = MorsePulse { mark: false, units: INTER_CHAR_GAP };
            }
            count += 1;
        }

        let k: usize = 0;
        while (k < code.len) {
            if (k > 0) {
                if (count >= out.len) {
                    return null;
                }
                out[count] = MorsePulse { mark: false, units: INTRA_CHAR_GAP };
                count += 1;
            }
            if (count >= out.len) {
                return null;
            }
            const u: u8 = if (code[k] == 0x2D) DAH_UNITS else DIT_UNITS;
            out[count] = MorsePulse { mark: true, units: u };
            count += 1;
            k += 1;
        }

        has_emitted = true;
    }

    return count;
}

/// Computes the exact ITU-R M.1677-1 total duration in dit units for `text`
/// (excluding any trailing inter-word silence; e.g. `"PARIS"` == 43, `"PARIS"` + 7 == 50).
/// Returns `null` if any unsupported character is encountered.
pub fn morseDurationUnits(text: []const u8): usize? {
    let i: usize = 0;
    let total: usize = 0;
    let has_emitted = false;
    let pending_word_gap = false;

    while (i < text.len) {
        const ch = text[i];
        i += 1;

        if (isAsciiWhitespace(ch)) {
            if (has_emitted) {
                pending_word_gap = true;
            }
            continue;
        }

        const code = charToMorse(ch) ?? return null;

        if (has_emitted) {
            if (pending_word_gap) {
                total += INTER_WORD_GAP as usize;
                pending_word_gap = false;
            } else {
                total += INTER_CHAR_GAP as usize;
            }
        }

        let k: usize = 0;
        while (k < code.len) {
            if (k > 0) {
                total += INTRA_CHAR_GAP as usize;
            }
            if (code[k] == 0x2D) {
                total += DAH_UNITS as usize;
            } else {
                total += DIT_UNITS as usize;
            }
            k += 1;
        }

        has_emitted = true;
    }

    return total;
}
