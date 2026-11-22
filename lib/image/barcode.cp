// Source: cp/lib/barcode/barcode.cp (Code 128, EAN-13, UPC-A Barcode Encoder & SVG/ASCII Renderer)
// Industrial-grade, pure CP, zero-C-dependency 1D barcode engine:
// - Code 128 with optimal A/B/C subset switching, Mod 103 checksum & 13-module Stop
// - EAN-13 with 12/13-digit check digit computation, L/G/R parity encoding & 95 modules
// - UPC-A with 11/12-digit check digit computation & EAN-13 95-module compatibility
// - BarcodeRenderer for run-length-merged SVG and UTF-8 block ASCII bar output

const std = @import("std");

pub err BarcodeError {
    EmptyInput,
    InvalidCharacter,
    InvalidLength,
    InvalidChecksum,
    OutOfMemory,
}

pub enum BarcodeFormat {
    Code128,
    Ean13,
    UpcA,
}

pub struct BarcodeBitPattern {
    format: BarcodeFormat = .Code128,
    text: []const u8 = "",
    modules: []bool = [],
    alloc: std.mem.Allocator = undefined,

    /// Releases heap memory owned by `modules` and `text`.
    pub fn deinit(*_) {
        if (_.modules.len > 0) {
            _.alloc.free(_.modules);
            _.modules = [];
        }
        if (_.text.len > 0) {
            _.alloc.free(_.text);
            _.text = "";
        }
    }
}

// ============================================================================
// EAN-13 & UPC-A Lookup Tables
// ============================================================================

// 7-bit L-code patterns (odd parity) for digits 0..9
const EAN_L_CODES: [10]u8 = [
    0b0001101, // 0
    0b0011001, // 1
    0b0010011, // 2
    0b0111101, // 3
    0b0100011, // 4
    0b0110001, // 5
    0b0101111, // 6
    0b0111011, // 7
    0b0110111, // 8
    0b0001011, // 9
];

// 7-bit G-code patterns (even parity) for digits 0..9
const EAN_G_CODES: [10]u8 = [
    0b0100111, // 0
    0b0110011, // 1
    0b0011011, // 2
    0b0100001, // 3
    0b0011101, // 4
    0b0111001, // 5
    0b0000101, // 6
    0b0010001, // 7
    0b0001001, // 8
    0b0010111, // 9
];

// 7-bit R-code patterns (even parity, bitwise complement of L) for digits 0..9
const EAN_R_CODES: [10]u8 = [
    0b1110010, // 0
    0b1100110, // 1
    0b1101100, // 2
    0b1000010, // 3
    0b1011100, // 4
    0b1001110, // 5
    0b1010000, // 6
    0b1000100, // 7
    0b1001000, // 8
    0b1110100, // 9
];

// First-digit parity mask for the left 6 digits (bit 5 = digit 1 ... bit 0 = digit 6; 0 = L, 1 = G)
const EAN13_PARITY: [10]u8 = [
    0b000000, // 0: LLLLLL
    0b001011, // 1: LLGLGG
    0b001101, // 2: LLGGLG
    0b001110, // 3: LLGGGL
    0b010011, // 4: LGLLGG
    0b011001, // 5: LGGLLG
    0b011100, // 6: LGGGLL
    0b010101, // 7: LGLGLG
    0b010110, // 8: LGLGGL
    0b011010, // 9: LGGLGL
];

// ============================================================================
// Code 128 Lookup Table (ISO/IEC 15417, symbols 0..105 as packed 6-nibble widths)
// Each nibble represents bar/space width (1..4) in order: B1 S1 B2 S2 B3 S3.
// Symbol 106 (Stop) is 13 modules: 1100011101011 (widths 2,3,3,1,1,1,2).
// ============================================================================

const CODE128_START_A: u8 = 103;
const CODE128_START_B: u8 = 104;
const CODE128_START_C: u8 = 105;
const CODE128_STOP: u8 = 106;

const CODE128_CODE_C: u8 = 99;
const CODE128_CODE_B: u8 = 100;
const CODE128_CODE_A: u8 = 101;

const CODE128_PATTERNS: [106]u32 = [
    0x212222, // 0
    0x222122, // 1
    0x222221, // 2
    0x121223, // 3
    0x121322, // 4
    0x131222, // 5
    0x122213, // 6
    0x122312, // 7
    0x132212, // 8
    0x221213, // 9
    0x221312, // 10
    0x231212, // 11
    0x112232, // 12
    0x122132, // 13
    0x122231, // 14
    0x113222, // 15
    0x123122, // 16
    0x123221, // 17
    0x223211, // 18
    0x221132, // 19
    0x221231, // 20
    0x213212, // 21
    0x223112, // 22
    0x312131, // 23
    0x311222, // 24
    0x321122, // 25
    0x321221, // 26
    0x312212, // 27
    0x322112, // 28
    0x322211, // 29
    0x212123, // 30
    0x212321, // 31
    0x232121, // 32
    0x111323, // 33
    0x131123, // 34
    0x131321, // 35
    0x112313, // 36
    0x132113, // 37
    0x132311, // 38
    0x211313, // 39
    0x231113, // 40
    0x231311, // 41
    0x112133, // 42
    0x112331, // 43
    0x132131, // 44
    0x113123, // 45
    0x113321, // 46
    0x133121, // 47
    0x313121, // 48
    0x211331, // 49
    0x231131, // 50
    0x213113, // 51
    0x213311, // 52
    0x213131, // 53
    0x311123, // 54
    0x311321, // 55
    0x331121, // 56
    0x312113, // 57
    0x312311, // 58
    0x332111, // 59
    0x314111, // 60
    0x221411, // 61
    0x431111, // 62
    0x111224, // 63
    0x111422, // 64
    0x121124, // 65
    0x121421, // 66
    0x141122, // 67
    0x141221, // 68
    0x112214, // 69
    0x112412, // 70
    0x122114, // 71
    0x122411, // 72
    0x142112, // 73
    0x142211, // 74
    0x241211, // 75
    0x221114, // 76
    0x413111, // 77
    0x241112, // 78
    0x134111, // 79
    0x111242, // 80
    0x121142, // 81
    0x121241, // 82
    0x114212, // 83
    0x124112, // 84
    0x124211, // 85
    0x411212, // 86
    0x421112, // 87
    0x421211, // 88
    0x212141, // 89
    0x214121, // 90
    0x412121, // 91
    0x111143, // 92
    0x111341, // 93
    0x131141, // 94
    0x114113, // 95
    0x114311, // 96
    0x411113, // 97
    0x411311, // 98
    0x113141, // 99
    0x114131, // 100
    0x311141, // 101
    0x411131, // 102
    0x211412, // 103: Start A
    0x211214, // 104: Start B
    0x211232, // 105: Start C
];

const CODE128_STOP_BITS: [13]bool = [
    true, true, false, false, false, true, true, true, false, true, false, true, true,
];

// ============================================================================
// Internal Helpers
// ============================================================================

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn allDigits(s: []const u8): bool {
    for (c of s) {
        if (!isDigit(c)) return false;
    }
    return true;
}

fn writeEan7(modules: []bool, offset: *usize, pattern: u8) {
    let p = pattern;
    let bit: usize = 0;
    while (bit < 7) : (bit += 1) {
        modules[offset.*] = (p & 0x40) != 0;
        p <<= 1;
        offset.* += 1;
    }
}

fn countConsecutiveDigits(text: []const u8, start: usize): usize {
    let i = start;
    while (i < text.len && isDigit(text[i])) : (i += 1) {}
    return i - start;
}

enum Code128Set {
    A,
    B,
    C,
}

fn preferCodeA(text: []const u8, start: usize): bool {
    let i = start;
    while (i < text.len) : (i += 1) {
        const c = text[i];
        if (c < 32) return true;
        if (c >= 96) return false;
    }
    return false;
}

fn appendUsizeDecimal(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: usize): ! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        out.append(alloc, '0') catch return BarcodeError.OutOfMemory;
        return;
    }
    while (v > 0) {
        tmp[n] = ('0' as u8) + u8(v % 10);
        n += 1;
        v /= 10;
    }
    while (n > 0) {
        n -= 1;
        out.append(alloc, tmp[n]) catch return BarcodeError.OutOfMemory;
    }
}

fn appendXmlEscaped(out: *std.ArrayList(u8), alloc: std.mem.Allocator, text: []const u8): ! {
    for (c of text) {
        if (c == '&') {
            out.appendSlice(alloc, "&amp;") catch return BarcodeError.OutOfMemory;
        } else if (c == '<') {
            out.appendSlice(alloc, "&lt;") catch return BarcodeError.OutOfMemory;
        } else if (c == '>') {
            out.appendSlice(alloc, "&gt;") catch return BarcodeError.OutOfMemory;
        } else if (c == '"') {
            out.appendSlice(alloc, "&quot;") catch return BarcodeError.OutOfMemory;
        } else if (c == '\'') {
            out.appendSlice(alloc, "&apos;") catch return BarcodeError.OutOfMemory;
        } else {
            out.append(alloc, c) catch return BarcodeError.OutOfMemory;
        }
    }
}

// ============================================================================
// BarcodeEncoder
// ============================================================================

pub static struct BarcodeEncoder {
    /// Computes the GS1 EAN-13 check digit (0..9) for a 12-digit numeric string.
    static fn ean13CheckDigit(digits12: []const u8): u8 ! BarcodeError {
        if (digits12.len == 0) return BarcodeError.EmptyInput;
        if (digits12.len != 12) return BarcodeError.InvalidLength;
        if (!allDigits(digits12)) return BarcodeError.InvalidCharacter;

        let sum: u32 = 0;
        let i: usize = 0;
        while (i < 12) : (i += 1) {
            const d: u32 = digits12[i] - '0';
            if ((i % 2) == 0) {
                sum += d;
            } else {
                sum += d * 3;
            }
        }
        return u8((10 - (sum % 10)) % 10);
    }

    /// Verifies a 13-digit EAN-13 string's check digit.
    static fn verifyEan13(digits13: []const u8): bool {
        if (digits13.len != 13) return false;
        const expected = BarcodeEncoder.ean13CheckDigit(digits13[0..12]) catch return false;
        if (!isDigit(digits13[12])) return false;
        return (digits13[12] - '0') == expected;
    }

    /// Computes the UPC-A check digit (0..9) for an 11-digit numeric string.
    static fn upcaCheckDigit(digits11: []const u8): u8 ! BarcodeError {
        if (digits11.len == 0) return BarcodeError.EmptyInput;
        if (digits11.len != 11) return BarcodeError.InvalidLength;
        if (!allDigits(digits11)) return BarcodeError.InvalidCharacter;

        let sum: u32 = 0;
        let i: usize = 0;
        while (i < 11) : (i += 1) {
            const d: u32 = digits11[i] - '0';
            if ((i % 2) == 0) {
                sum += d * 3;
            } else {
                sum += d;
            }
        }
        return u8((10 - (sum % 10)) % 10);
    }

    /// Verifies a 12-digit UPC-A string's check digit.
    static fn verifyUpca(digits12: []const u8): bool {
        if (digits12.len != 12) return false;
        const expected = BarcodeEncoder.upcaCheckDigit(digits12[0..11]) catch return false;
        if (!isDigit(digits12[11])) return false;
        return (digits12[11] - '0') == expected;
    }

    /// Encodes a 12- or 13-digit EAN-13 barcode into a 95-module `BarcodeBitPattern`.
    /// A 12-digit input automatically receives its check digit; a 13-digit input is strictly verified.
    static fn encodeEan13(alloc: std.mem.Allocator, digits: []const u8): BarcodeBitPattern ! BarcodeError {
        if (digits.len == 0) return BarcodeError.EmptyInput;
        if (digits.len != 12 && digits.len != 13) return BarcodeError.InvalidLength;
        if (!allDigits(digits)) return BarcodeError.InvalidCharacter;

        const check = BarcodeEncoder.ean13CheckDigit(digits[0..12])?;
        if (digits.len == 13 && (digits[12] - '0') != check) {
            return BarcodeError.InvalidChecksum;
        }

        let text_buf = alloc.alloc(u8, 13) catch return BarcodeError.OutOfMemory;
        errdefer alloc.free(text_buf);
        for (i of 0..12) {
            text_buf[i] = digits[i];
        }
        text_buf[12] = '0' + check;

        let modules = alloc.alloc(bool, 95) catch return BarcodeError.OutOfMemory;
        errdefer alloc.free(modules);

        let pos: usize = 0;
        // Left guard: 101
        modules[pos] = true; pos += 1;
        modules[pos] = false; pos += 1;
        modules[pos] = true; pos += 1;

        const first_digit = text_buf[0] - '0';
        const parity_mask = EAN13_PARITY[first_digit];

        // Left 6 digits (indices 1..6)
        let p_mask: u8 = parity_mask << 2;
        let i_left: usize = 0;
        while (i_left < 6) : (i_left += 1) {
            const d = text_buf[1 + i_left] - '0';
            const use_g = (p_mask & 0x80) != 0;
            p_mask <<= 1;
            const pat = if (use_g) EAN_G_CODES[d] else EAN_L_CODES[d];
            writeEan7(modules, &pos, pat);
        }

        // Center guard: 01010
        modules[pos] = false; pos += 1;
        modules[pos] = true; pos += 1;
        modules[pos] = false; pos += 1;
        modules[pos] = true; pos += 1;
        modules[pos] = false; pos += 1;

        // Right 6 digits (indices 7..12)
        for (i of 0..6) {
            const d = text_buf[7 + i] - '0';
            writeEan7(modules, &pos, EAN_R_CODES[d]);
        }

        // Right guard: 101
        modules[pos] = true; pos += 1;
        modules[pos] = false; pos += 1;
        modules[pos] = true; pos += 1;

        return BarcodeBitPattern{
            format: .Ean13,
            text: text_buf,
            modules: modules,
            alloc: alloc,
        };
    }

    /// Encodes an 11- or 12-digit UPC-A barcode into a 95-module `BarcodeBitPattern`
    /// (compatible with EAN-13 via leading `'0'` prefix).
    static fn encodeUpcA(alloc: std.mem.Allocator, digits: []const u8): BarcodeBitPattern ! BarcodeError {
        if (digits.len == 0) return BarcodeError.EmptyInput;
        if (digits.len != 11 && digits.len != 12) return BarcodeError.InvalidLength;
        if (!allDigits(digits)) return BarcodeError.InvalidCharacter;

        const check = BarcodeEncoder.upcaCheckDigit(digits[0..11])?;
        if (digits.len == 12 && (digits[11] - '0') != check) {
            return BarcodeError.InvalidChecksum;
        }

        let ean13_buf: [13]u8 = ['0'; 13];
        for (i of 0..11) {
            ean13_buf[1 + i] = digits[i];
        }
        ean13_buf[12] = ('0' as u8) + check;

        let pat = BarcodeEncoder.encodeEan13(alloc, &ean13_buf)?;
        errdefer pat.deinit();

        let upca_text = alloc.alloc(u8, 12) catch return BarcodeError.OutOfMemory;
        for (i of 0..12) {
            upca_text[i] = ean13_buf[1 + i];
        }
        alloc.free(pat.text);
        pat.text = upca_text;
        pat.format = .UpcA;
        return pat;
    }

    /// Encodes an ASCII string (0..127) using Code 128 with automatic optimal A/B/C subset
    /// switching, Mod 103 check symbol, and 13-module Stop pattern (`11 * symbols + 2` modules).
    static fn encodeCode128(alloc: std.mem.Allocator, text: []const u8): BarcodeBitPattern ! BarcodeError {
        if (text.len == 0) return BarcodeError.EmptyInput;
        for (c of text) {
            if (c >= 128) return BarcodeError.InvalidCharacter;
        }

        let syms: std.ArrayList(u8) = .empty;
        defer syms.deinit(alloc);

        // Choose optimal initial code set
        const init_digits = countConsecutiveDigits(text, 0);
        let current: Code128Set = .B;
        if (init_digits >= 4 || (init_digits == 2 && text.len == 2)) {
            current = .C;
            syms.append(alloc, CODE128_START_C) catch return BarcodeError.OutOfMemory;
        } else if (preferCodeA(text, 0)) {
            current = .A;
            syms.append(alloc, CODE128_START_A) catch return BarcodeError.OutOfMemory;
        } else {
            current = .B;
            syms.append(alloc, CODE128_START_B) catch return BarcodeError.OutOfMemory;
        }

        let i: usize = 0;
        while (i < text.len) {
            if (current == .C) {
                if (i + 1 < text.len && isDigit(text[i]) && isDigit(text[i + 1])) {
                    const pair: u8 = u8(text[i] - '0') * u8(10) + u8(text[i + 1] - '0');
                    syms.append(alloc, pair) catch return BarcodeError.OutOfMemory;
                    i += 2;
                } else if (preferCodeA(text, i)) {
                    syms.append(alloc, CODE128_CODE_A) catch return BarcodeError.OutOfMemory;
                    current = .A;
                } else {
                    syms.append(alloc, CODE128_CODE_B) catch return BarcodeError.OutOfMemory;
                    current = .B;
                }
            } else {
                const digit_run = countConsecutiveDigits(text, i);
                if (digit_run >= 4) {
                    if ((digit_run % 2) == 1) {
                        // Emit one digit in A/B so remaining even digit run (>= 4) pairs in Code C
                        syms.append(alloc, text[i] - 32) catch return BarcodeError.OutOfMemory;
                        i += 1;
                    }
                    syms.append(alloc, CODE128_CODE_C) catch return BarcodeError.OutOfMemory;
                    current = .C;
                } else {
                    const c = text[i];
                    if (current == .A) {
                        if (c < 32) {
                            syms.append(alloc, c + 64) catch return BarcodeError.OutOfMemory;
                            i += 1;
                        } else if (c < 96) {
                            syms.append(alloc, c - 32) catch return BarcodeError.OutOfMemory;
                            i += 1;
                        } else {
                            syms.append(alloc, CODE128_CODE_B) catch return BarcodeError.OutOfMemory;
                            current = .B;
                            syms.append(alloc, c - 32) catch return BarcodeError.OutOfMemory;
                            i += 1;
                        }
                    } else {
                        if (c >= 32) {
                            syms.append(alloc, c - 32) catch return BarcodeError.OutOfMemory;
                            i += 1;
                        } else {
                            syms.append(alloc, CODE128_CODE_A) catch return BarcodeError.OutOfMemory;
                            current = .A;
                            syms.append(alloc, c + 64) catch return BarcodeError.OutOfMemory;
                            i += 1;
                        }
                    }
                }
            }
        }

        // Compute Mod 103 checksum
        let sum: u32 = syms.items[0];
        let k: usize = 1;
        while (k < syms.items.len) : (k += 1) {
            sum += u32(k) * u32(syms.items[k]);
        }
        const check_sym: u8 = u8(sum % 103);
        syms.append(alloc, check_sym) catch return BarcodeError.OutOfMemory;
        syms.append(alloc, CODE128_STOP) catch return BarcodeError.OutOfMemory;

        const total_modules = 11 * syms.items.len + 2;
        let modules = alloc.alloc(bool, total_modules) catch return BarcodeError.OutOfMemory;
        errdefer alloc.free(modules);

        let text_copy = alloc.alloc(u8, text.len) catch return BarcodeError.OutOfMemory;
        errdefer alloc.free(text_copy);
        for (idx of 0..text.len) {
            text_copy[idx] = text[idx];
        }

        let pos: usize = 0;
        for (s_idx of 0..syms.items.len - 1) {
            const sym = syms.items[s_idx];
            const packed_w = CODE128_PATTERNS[sym];
            const widths: [6]usize = [
                usize((packed_w >> 20) & 0xF),
                usize((packed_w >> 16) & 0xF),
                usize((packed_w >> 12) & 0xF),
                usize((packed_w >> 8) & 0xF),
                usize((packed_w >> 4) & 0xF),
                usize(packed_w & 0xF),
            ];
            let elem: usize = 0;
            while (elem < 6) : (elem += 1) {
                const w = widths[elem];
                const is_bar = (elem % 2) == 0;
                let step: usize = 0;
                while (step < w) : (step += 1) {
                    modules[pos] = is_bar;
                    pos += 1;
                }
            }
        }

        // Stop symbol (13 modules)
        let bi: usize = 0;
        while (bi < 13) : (bi += 1) {
            modules[pos] = CODE128_STOP_BITS[bi];
            pos += 1;
        }

        return BarcodeBitPattern{
            format: .Code128,
            text: text_copy,
            modules: modules,
            alloc: alloc,
        };
    }
}

// ============================================================================
// BarcodeRenderer
// ============================================================================

pub static struct BarcodeRenderer {
    /// Renders `pattern` as a `<svg xmlns="http://www.w3.org/2000/svg" ...>` vector image,
    /// merging consecutive dark modules into single `<rect>` elements.
    static fn toSvg(
        alloc: std.mem.Allocator,
        pattern: BarcodeBitPattern,
        bar_width_px: usize,
        height_px: usize,
        show_text: bool,
    ): ([]u8) ! BarcodeError {
        const bw = if (bar_width_px == 0) 1 else bar_width_px;
        const bh = if (height_px == 0) 40 else height_px;
        const quiet_modules: usize = 10;
        const total_modules = pattern.modules.len + 2 * quiet_modules;
        const svg_w = total_modules * bw;
        const text_pad: usize = if (show_text) 20 else 0;
        const pad_y: usize = 4;
        const svg_h = bh + text_pad + pad_y * 2;

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 ") catch return BarcodeError.OutOfMemory;
        appendUsizeDecimal(&out, alloc, svg_w) catch return BarcodeError.OutOfMemory;
        out.append(alloc, ' ') catch return BarcodeError.OutOfMemory;
        appendUsizeDecimal(&out, alloc, svg_h) catch return BarcodeError.OutOfMemory;
        out.appendSlice(alloc, "\" width=\"") catch return BarcodeError.OutOfMemory;
        appendUsizeDecimal(&out, alloc, svg_w) catch return BarcodeError.OutOfMemory;
        out.appendSlice(alloc, "\" height=\"") catch return BarcodeError.OutOfMemory;
        appendUsizeDecimal(&out, alloc, svg_h) catch return BarcodeError.OutOfMemory;
        out.appendSlice(alloc, "\" shape-rendering=\"crispEdges\">\n<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n") catch return BarcodeError.OutOfMemory;

        let i: usize = 0;
        while (i < pattern.modules.len) {
            if (!pattern.modules[i]) {
                i += 1;
                continue;
            }
            const run_start = i;
            while (i < pattern.modules.len && pattern.modules[i]) : (i += 1) {}
            const x = (quiet_modules + run_start) * bw;
            const w = (i - run_start) * bw;

            out.appendSlice(alloc, "<rect x=\"") catch return BarcodeError.OutOfMemory;
            appendUsizeDecimal(&out, alloc, x) catch return BarcodeError.OutOfMemory;
            out.appendSlice(alloc, "\" y=\"") catch return BarcodeError.OutOfMemory;
            appendUsizeDecimal(&out, alloc, pad_y) catch return BarcodeError.OutOfMemory;
            out.appendSlice(alloc, "\" width=\"") catch return BarcodeError.OutOfMemory;
            appendUsizeDecimal(&out, alloc, w) catch return BarcodeError.OutOfMemory;
            out.appendSlice(alloc, "\" height=\"") catch return BarcodeError.OutOfMemory;
            appendUsizeDecimal(&out, alloc, bh) catch return BarcodeError.OutOfMemory;
            out.appendSlice(alloc, "\" fill=\"#000000\"/>\n") catch return BarcodeError.OutOfMemory;
        }

        if (show_text && pattern.text.len > 0) {
            const text_x = svg_w / 2;
            const text_y = pad_y + bh + 15;
            out.appendSlice(alloc, "<text x=\"") catch return BarcodeError.OutOfMemory;
            appendUsizeDecimal(&out, alloc, text_x) catch return BarcodeError.OutOfMemory;
            out.appendSlice(alloc, "\" y=\"") catch return BarcodeError.OutOfMemory;
            appendUsizeDecimal(&out, alloc, text_y) catch return BarcodeError.OutOfMemory;
            out.appendSlice(alloc, "\" text-anchor=\"middle\" font-family=\"monospace\" font-size=\"14\" fill=\"#000000\">") catch return BarcodeError.OutOfMemory;
            appendXmlEscaped(&out, alloc, pattern.text) catch return BarcodeError.OutOfMemory;
            out.appendSlice(alloc, "</text>\n") catch return BarcodeError.OutOfMemory;
        }

        out.appendSlice(alloc, "</svg>\n") catch return BarcodeError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return BarcodeError.OutOfMemory;
    }

    /// Renders `pattern` as a multi-row UTF-8 block bar string (`█` for bar, ` ` for space).
    static fn toAscii(alloc: std.mem.Allocator, pattern: BarcodeBitPattern): ([]u8) ! BarcodeError {
        const rows: usize = 4;
        const quiet: usize = 2;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let r: usize = 0;
        while (r < rows) : (r += 1) {
            let q1: usize = 0;
            while (q1 < quiet) : (q1 += 1) {
                out.append(alloc, ' ') catch return BarcodeError.OutOfMemory;
            }
            for (m of pattern.modules) {
                if (m) {
                    out.appendSlice(alloc, "\xe2\x96\x88") catch return BarcodeError.OutOfMemory;
                } else {
                    out.append(alloc, ' ') catch return BarcodeError.OutOfMemory;
                }
            }
            let q2: usize = 0;
            while (q2 < quiet) : (q2 += 1) {
                out.append(alloc, ' ') catch return BarcodeError.OutOfMemory;
            }
            out.append(alloc, '\n') catch return BarcodeError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return BarcodeError.OutOfMemory;
    }
}
