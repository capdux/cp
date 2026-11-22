// Origin: Pure-CP Stdlib (Zero-Alloc ISO/IEC 15417 Code 128 Barcode Codec) | Created: 2026-11-22

pub const CODE128_START_A: u8 = 103;
pub const CODE128_START_B: u8 = 104;
pub const CODE128_START_C: u8 = 105;
pub const CODE128_STOP: u8 = 106;
pub const CODE128_FNC1: u8 = 102;
pub const CODE128_CODE_C: u8 = 99;
pub const CODE128_CODE_B: u8 = 100;
pub const CODE128_CODE_A: u8 = 101;
pub const CODE128_SHIFT: u8 = 98;
pub const CODE128_MOD_PER_SYMBOL: usize = 11;
pub const CODE128_STOP_MODULES: usize = 13;

// Standard ISO/IEC 15417 bar/space patterns (MSB-first, 1 = bar, 0 = space).
// Symbols 0..105 occupy bits 0..10 (11 modules); Stop (106) occupies bits 0..12 (13 modules, 0b1100011101011).
const CODE128_PATTERNS: [107]u16 = [
    0x06CC, 0x066C, 0x0666, 0x0498, 0x048C, 0x044C, 0x04C8, 0x04C4,
    0x0464, 0x0648, 0x0644, 0x0624, 0x059C, 0x04DC, 0x04CE, 0x05CC,
    0x04EC, 0x04E6, 0x0672, 0x065C, 0x064E, 0x06E4, 0x0674, 0x076E,
    0x074C, 0x072C, 0x0726, 0x0764, 0x0734, 0x0732, 0x06D8, 0x06C6,
    0x0636, 0x0518, 0x0458, 0x0446, 0x0588, 0x0468, 0x0462, 0x0688,
    0x0628, 0x0622, 0x05B8, 0x058E, 0x046E, 0x05D8, 0x05C6, 0x0476,
    0x0776, 0x068E, 0x062E, 0x06E8, 0x06E2, 0x06EE, 0x0758, 0x0746,
    0x0716, 0x0768, 0x0762, 0x071A, 0x077A, 0x0642, 0x078A, 0x0530,
    0x050C, 0x04B0, 0x0486, 0x042C, 0x0426, 0x0590, 0x0584, 0x04D0,
    0x04C2, 0x0434, 0x0432, 0x0612, 0x0650, 0x07BA, 0x0614, 0x047A,
    0x053C, 0x04BC, 0x049E, 0x05E4, 0x04F4, 0x04F2, 0x07A4, 0x0794,
    0x0792, 0x06DE, 0x06F6, 0x07B6, 0x0578, 0x051E, 0x045E, 0x05E8,
    0x05E2, 0x07A8, 0x07A2, 0x05DE, 0x05EE, 0x075E, 0x07AE, 0x0684,
    0x0690, 0x069C, 0x18EB,
];

const CHARSET_A: u8 = 0;
const CHARSET_B: u8 = 1;
const CHARSET_C: u8 = 2;

fn isStartSymbol(sym: u8): bool {
    return sym == CODE128_START_A || sym == CODE128_START_B || sym == CODE128_START_C;
}

fn isDigit(ch: u8): bool {
    return ch >= '0' && ch <= '9';
}

fn countDigits(text: []const u8, start: usize): usize {
    let i: usize = start;
    while (i < text.len && isDigit(text[i])) {
        i += 1;
    }
    return i - start;
}

/// Returns the MSB-first bar/space bitmask for `symbol` (0..105 -> 11-bit, 106 Stop -> 13-bit).
/// Returns `null` if `symbol > 106`.
pub fn code128Pattern(symbol: u8): u16? {
    if (symbol > CODE128_STOP) {
        return null;
    }
    return CODE128_PATTERNS[symbol as usize];
}

/// Reverse-looks up an 11-bit bar/space pattern and returns the matching symbol value `0..105`, or `null`.
pub fn code128SymbolFromPattern(pat11: u16): u8? {
    if ((pat11 & 0xF800) != 0) {
        return null;
    }
    let i: usize = 0;
    while (i < 106) {
        if (CODE128_PATTERNS[i] == pat11) {
            return i as u8;
        }
        i += 1;
    }
    return null;
}

/// Computes the ISO/IEC 15417 Mod-103 checksum over `symbols` (Start + data symbols, excluding checksum & Stop).
/// Formula: `(symbols[0] + sum_{i>=1} (i * symbols[i])) % 103`.
/// Returns `null` if `symbols` is empty, does not begin with Start A/B/C, or contains any symbol > 105.
pub fn code128Checksum(symbols: []const u8): u8? {
    if (symbols.len == 0 || !isStartSymbol(symbols[0])) {
        return null;
    }
    let sum: u64 = symbols[0] as u64;
    let i: usize = 1;
    while (i < symbols.len) {
        const s: u8 = symbols[i];
        if (s > 105) {
            return null;
        }
        sum += (i as u64) * (s as u64);
        i += 1;
    }
    return (sum % 103) as u8;
}

/// Encodes ASCII text (`0x20..0x7F`) into Code 128B symbols: `[Start B, data..., checksum, Stop]`.
/// Returns number of symbols written, or `null` on unsupported byte or insufficient `out_symbols`.
pub fn encodeCode128B(text: []const u8, out_symbols: *[]u8): usize? {
    const needed: usize = text.len + 3;
    if (out_symbols.*.len < needed) {
        return null;
    }

    out_symbols.*[0] = CODE128_START_B;
    let sum: u64 = CODE128_START_B as u64;
    let i: usize = 0;
    while (i < text.len) {
        const ch: u8 = text[i];
        if (ch < 0x20 || ch > 0x7F) {
            return null;
        }
        const sym: u8 = ch - 32;
        out_symbols.*[i + 1] = sym;
        sum += ((i + 1) as u64) * (sym as u64);
        i += 1;
    }

    out_symbols.*[text.len + 1] = (sum % 103) as u8;
    out_symbols.*[text.len + 2] = CODE128_STOP;
    return needed;
}

/// Encodes an even-length ASCII digit string into Code 128C symbols: `[Start C, pairs..., checksum, Stop]`.
/// Returns number of symbols written, or `null` on odd length, non-digit byte, or insufficient `out_symbols`.
pub fn encodeCode128C(digits: []const u8, out_symbols: *[]u8): usize? {
    if ((digits.len & 1) != 0) {
        return null;
    }
    const pairs: usize = digits.len / 2;
    const needed: usize = pairs + 3;
    if (out_symbols.*.len < needed) {
        return null;
    }

    out_symbols.*[0] = CODE128_START_C;
    let sum: u64 = CODE128_START_C as u64;
    let i: usize = 0;
    while (i < pairs) {
        const d0: u8 = digits[i * 2];
        const d1: u8 = digits[i * 2 + 1];
        if (!isDigit(d0) || !isDigit(d1)) {
            return null;
        }
        const sym: u8 = (d0 - '0') * 10 + (d1 - '0');
        out_symbols.*[i + 1] = sym;
        sum += ((i + 1) as u64) * (sym as u64);
        i += 1;
    }

    out_symbols.*[pairs + 1] = (sum % 103) as u8;
    out_symbols.*[pairs + 2] = CODE128_STOP;
    return needed;
}

/// Encodes ASCII text (`0x20..0x7F`) with automatic Code B / Code C switching.
/// Runs of >= 4 digits (or an all-digit 2-char string at start) are compressed using Code C pairs.
/// Returns number of symbols written, or `null` on unsupported byte or insufficient `out_symbols`.
pub fn encodeCode128Auto(text: []const u8, out_symbols: *[]u8): usize? {
    // Validate all characters upfront.
    let v: usize = 0;
    while (v < text.len) {
        if (text[v] < 0x20 || text[v] > 0x7F) {
            return null;
        }
        v += 1;
    }

    if (out_symbols.*.len < 3) {
        return null;
    }

    const lead_digits: usize = countDigits(text, 0);
    let charset: u8 = CHARSET_B;
    let out_pos: usize = 0;
    let pos: usize = 0;

    if ((lead_digits >= 4 && (lead_digits & 1) == 0) || (text.len == 2 && lead_digits == 2)) {
        charset = CHARSET_C;
        out_symbols.*[0] = CODE128_START_C;
        out_pos = 1;
    } else if (lead_digits >= 4) {
        // Odd run >= 5 at start: emit Start C for even prefix, or Start B for 1 digit then Code C.
        // Using Start C directly for floor(lead_digits / 2) pairs is always <= Start B + 1 + Code C.
        charset = CHARSET_C;
        out_symbols.*[0] = CODE128_START_C;
        out_pos = 1;
    } else {
        charset = CHARSET_B;
        out_symbols.*[0] = CODE128_START_B;
        out_pos = 1;
    }

    while (pos < text.len) {
        if (charset == CHARSET_C) {
            const dlen: usize = countDigits(text, pos);
            if (dlen >= 2) {
                if (out_pos + 3 > out_symbols.*.len) {
                    return null;
                }
                const sym: u8 = (text[pos] - '0') * 10 + (text[pos + 1] - '0');
                out_symbols.*[out_pos] = sym;
                out_pos += 1;
                pos += 2;
            } else {
                if (out_pos + 3 > out_symbols.*.len) {
                    return null;
                }
                out_symbols.*[out_pos] = CODE128_CODE_B;
                out_pos += 1;
                charset = CHARSET_B;
            }
        } else {
            const dlen: usize = countDigits(text, pos);
            if (dlen >= 4) {
                if ((dlen & 1) == 1) {
                    // Emit 1 digit in Code B so the remaining even digit run compresses cleanly in Code C.
                    if (out_pos + 3 > out_symbols.*.len) {
                        return null;
                    }
                    out_symbols.*[out_pos] = text[pos] - 32;
                    out_pos += 1;
                    pos += 1;
                }
                if (out_pos + 3 > out_symbols.*.len) {
                    return null;
                }
                out_symbols.*[out_pos] = CODE128_CODE_C;
                out_pos += 1;
                charset = CHARSET_C;
            } else {
                if (out_pos + 3 > out_symbols.*.len) {
                    return null;
                }
                out_symbols.*[out_pos] = text[pos] - 32;
                out_pos += 1;
                pos += 1;
            }
        }
    }

    if (out_pos + 2 > out_symbols.*.len) {
        return null;
    }
    if (chk := code128Checksum(out_symbols.*[0..out_pos])) {
        out_symbols.*[out_pos] = chk;
        out_symbols.*[out_pos + 1] = CODE128_STOP;
        return out_pos + 2;
    }
    return null;
}

/// Renders a validated Code 128 symbol sequence `[Start, ..., Checksum, Stop]` into a `0`/`1` module byte stream.
/// Total output length is `(symbols.len - 1) * 11 + 13`.
pub fn renderCode128Modules(symbols: []const u8, out_modules: *[]u8): usize? {
    if (symbols.len < 3) {
        return null;
    }
    if (!isStartSymbol(symbols[0]) || symbols[symbols.len - 1] != CODE128_STOP) {
        return null;
    }

    const total: usize = (symbols.len - 1) * CODE128_MOD_PER_SYMBOL + CODE128_STOP_MODULES;
    if (out_modules.*.len < total) {
        return null;
    }

    let out_idx: usize = 0;
    let i: usize = 0;
    while (i + 1 < symbols.len) {
        const sym: u8 = symbols[i];
        if (sym > 105) {
            return null;
        }
        const pat: u16 = CODE128_PATTERNS[sym as usize];
        let b: usize = 0;
        while (b < CODE128_MOD_PER_SYMBOL) {
            const shift: u4 = (10 - b) as u4;
            out_modules.*[out_idx] = ((pat >> shift) & 1) as u8;
            out_idx += 1;
            b += 1;
        }
        i += 1;
    }

    const stop_pat: u16 = CODE128_PATTERNS[CODE128_STOP as usize];
    let sb: usize = 0;
    while (sb < CODE128_STOP_MODULES) {
        const shift: u4 = (12 - sb) as u4;
        out_modules.*[out_idx] = ((stop_pat >> shift) & 1) as u8;
        out_idx += 1;
        sb += 1;
    }

    return total;
}

fn decodeSingleSymbol(
    sym: u8,
    charset: *u8,
    shift_once: *bool,
    out_text: *[]u8,
    out_pos: *usize,
): bool {
    if (sym >= CODE128_START_A) {
        return false;
    }

    const active: u8 = charset.*;
    if (active == CHARSET_A) {
        if (sym == CODE128_CODE_C) {
            charset.* = CHARSET_C;
            shift_once.* = false;
            return true;
        }
        if (sym == CODE128_CODE_B) {
            charset.* = CHARSET_B;
            shift_once.* = false;
            return true;
        }
        if (sym == CODE128_SHIFT) {
            charset.* = CHARSET_B;
            shift_once.* = true;
            return true;
        }
        if (sym == CODE128_CODE_A || sym == CODE128_FNC1 || sym == 97 || sym == 96) {
            // FNC1..FNC4 control symbols produce no printable output.
            return true;
        }
        if (out_pos.* >= out_text.*.len) {
            return false;
        }
        if (sym <= 63) {
            out_text.*[out_pos.*] = sym + 32;
        } else {
            out_text.*[out_pos.*] = sym - 64;
        }
        out_pos.* += 1;
        if (shift_once.*) {
            charset.* = CHARSET_B;
            shift_once.* = false;
        }
        return true;
    }

    if (active == CHARSET_B) {
        if (sym == CODE128_CODE_C) {
            charset.* = CHARSET_C;
            shift_once.* = false;
            return true;
        }
        if (sym == CODE128_CODE_A) {
            charset.* = CHARSET_A;
            shift_once.* = false;
            return true;
        }
        if (sym == CODE128_SHIFT) {
            charset.* = CHARSET_A;
            shift_once.* = true;
            return true;
        }
        if (sym == CODE128_CODE_B || sym == CODE128_FNC1 || sym == 97 || sym == 96) {
            return true;
        }
        if (out_pos.* >= out_text.*.len) {
            return false;
        }
        out_text.*[out_pos.*] = sym + 32;
        out_pos.* += 1;
        if (shift_once.*) {
            charset.* = CHARSET_A;
            shift_once.* = false;
        }
        return true;
    }

    // CHARSET_C
    if (sym == CODE128_CODE_B) {
        charset.* = CHARSET_B;
        shift_once.* = false;
        return true;
    }
    if (sym == CODE128_CODE_A) {
        charset.* = CHARSET_A;
        shift_once.* = false;
        return true;
    }
    if (sym == CODE128_FNC1) {
        return true;
    }
    if (sym > 99) {
        return false;
    }
    if (out_pos.* + 2 > out_text.*.len) {
        return false;
    }
    out_text.*[out_pos.*] = ('0' as u8) + (sym / 10);
    out_text.*[out_pos.* + 1] = ('0' as u8) + (sym % 10);
    out_pos.* += 2;
    return true;
}

/// Decodes a Code 128 symbol sequence `[Start, data..., Checksum, Stop]` into `out_text`.
/// Validates Start A/B/C, Mod-103 checksum, and trailing Stop 106; supports A/B/C switching & Shift.
pub fn decodeCode128Symbols(symbols: []const u8, out_text: *[]u8): usize? {
    if (symbols.len < 3) {
        return null;
    }
    if (symbols[symbols.len - 1] != CODE128_STOP) {
        return null;
    }

    const payload_end: usize = symbols.len - 2;
    if (expected_chk := code128Checksum(symbols[0..payload_end])) {
        if (symbols[payload_end] != expected_chk) {
            return null;
        }
    } else {
        return null;
    }

    let charset: u8 = CHARSET_B;
    if (symbols[0] == CODE128_START_A) {
        charset = CHARSET_A;
    } else if (symbols[0] == CODE128_START_B) {
        charset = CHARSET_B;
    } else if (symbols[0] == CODE128_START_C) {
        charset = CHARSET_C;
    } else {
        return null;
    }

    let shift_once: bool = false;
    let out_pos: usize = 0;
    let i: usize = 1;
    while (i < payload_end) {
        if (!decodeSingleSymbol(symbols[i], *charset, *shift_once, out_text, *out_pos)) {
            return null;
        }
        i += 1;
    }
    return out_pos;
}

fn readModuleBits(modules: []const u8, offset: usize, width: usize): u16? {
    let acc: u16 = 0;
    let i: usize = 0;
    while (i < width) {
        const m: u8 = modules[offset + i];
        if (m > 1) {
            return null;
        }
        acc = (acc << 1) | (m as u16);
        i += 1;
    }
    return acc;
}

/// Decodes a `0`/`1` Code 128 module stream directly into `out_text` with zero heap allocation.
/// Validates framing (Start A/B/C + trailing 13-module Stop), 11-module symbol patterns, and Mod-103 checksum.
pub fn decodeCode128Modules(modules: []const u8, out_text: *[]u8): usize? {
    // Minimum valid barcode: Start(11) + Checksum(11) + Stop(13) = 35 modules.
    if (modules.len < CODE128_MOD_PER_SYMBOL * 2 + CODE128_STOP_MODULES) {
        return null;
    }
    const body_mods: usize = modules.len - CODE128_STOP_MODULES;
    if ((body_mods % CODE128_MOD_PER_SYMBOL) != 0) {
        return null;
    }

    // Validate trailing 13-module Stop pattern.
    if (stop_bits := readModuleBits(modules, body_mods, CODE128_STOP_MODULES)) {
        if (stop_bits != CODE128_PATTERNS[CODE128_STOP as usize]) {
            return null;
        }
    } else {
        return null;
    }

    const sym_count: usize = body_mods / CODE128_MOD_PER_SYMBOL; // includes Start + data + Checksum
    const payload_end: usize = sym_count - 1; // index of Checksum symbol

    // Pass 1: validate Start, all 11-bit patterns, and Mod-103 checksum (O(1) stack).
    let start_sym: u8 = 0;
    if (p0 := readModuleBits(modules, 0, CODE128_MOD_PER_SYMBOL)) {
        if (s0 := code128SymbolFromPattern(p0)) {
            if (!isStartSymbol(s0)) {
                return null;
            }
            start_sym = s0;
        } else {
            return null;
        }
    } else {
        return null;
    }

    let sum: u64 = start_sym as u64;
    let idx: usize = 1;
    while (idx < payload_end) {
        if (pat := readModuleBits(modules, idx * CODE128_MOD_PER_SYMBOL, CODE128_MOD_PER_SYMBOL)) {
            if (sym := code128SymbolFromPattern(pat)) {
                sum += (idx as u64) * (sym as u64);
            } else {
                return null;
            }
        } else {
            return null;
        }
        idx += 1;
    }

    if (chk_pat := readModuleBits(modules, payload_end * CODE128_MOD_PER_SYMBOL, CODE128_MOD_PER_SYMBOL)) {
        if (chk_sym := code128SymbolFromPattern(chk_pat)) {
            if (chk_sym != ((sum % 103) as u8)) {
                return null;
            }
        } else {
            return null;
        }
    } else {
        return null;
    }

    // Pass 2: decode validated data symbols into out_text.
    let charset: u8 = CHARSET_B;
    if (start_sym == CODE128_START_A) {
        charset = CHARSET_A;
    } else if (start_sym == CODE128_START_B) {
        charset = CHARSET_B;
    } else {
        charset = CHARSET_C;
    }

    let shift_once: bool = false;
    let out_pos: usize = 0;
    let di: usize = 1;
    while (di < payload_end) {
        if (pat := readModuleBits(modules, di * CODE128_MOD_PER_SYMBOL, CODE128_MOD_PER_SYMBOL)) {
            if (sym := code128SymbolFromPattern(pat)) {
                if (!decodeSingleSymbol(sym, *charset, *shift_once, out_text, *out_pos)) {
                    return null;
                }
            } else {
                return null;
            }
        } else {
            return null;
        }
        di += 1;
    }
    return out_pos;
}
