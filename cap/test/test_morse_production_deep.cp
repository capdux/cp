// Source: cp/cap/test/test_morse_production_deep.cp (Production deep tests for cap:morse)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DIT_UNITS,
    DAH_UNITS,
    INTRA_CHAR_GAP,
    INTER_CHAR_GAP,
    INTER_WORD_GAP,
    PARIS_WORD_UNITS,
    MAX_MORSE_CODE_LEN,
    MorsePulse,
    wpmToDitMs,
    charToMorse,
    morseToChar,
    morseEncode,
    morseDecode,
    morseToPulses,
    morseDurationUnits,
} from "cap:morse";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. Complete 54-character ITU table (26 letters case-insensitive + 10 digits + 18 punctuation) roundtrip", () => {
    expect_eq(DIT_UNITS, 1 as u8);
    expect_eq(DAH_UNITS, 3 as u8);
    expect_eq(INTRA_CHAR_GAP, 1 as u8);
    expect_eq(INTER_CHAR_GAP, 3 as u8);
    expect_eq(INTER_WORD_GAP, 7 as u8);
    expect_eq(PARIS_WORD_UNITS, 50 as usize);
    expect_eq(MAX_MORSE_CODE_LEN, 7 as usize);

    // All 26 letters: uppercase & lowercase must map to identical Morse and reverse to uppercase
    const letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const lower = "abcdefghijklmnopqrstuvwxyz";
    let i: usize = 0;
    while (i < letters.len) {
        const up_code = charToMorse(letters[i]) ?? unreachable;
        const lo_code = charToMorse(lower[i]) ?? unreachable;
        expect_eq(bytesEq(up_code, lo_code), true);
        const rev = morseToChar(up_code) ?? unreachable;
        expect_eq(rev, letters[i]);
        i += 1;
    }

    // All 10 digits
    const digits = "0123456789";
    i = 0;
    while (i < digits.len) {
        const code = charToMorse(digits[i]) ?? unreachable;
        expect_eq(code.len, 5 as usize);
        const rev = morseToChar(code) ?? unreachable;
        expect_eq(rev, digits[i]);
        i += 1;
    }

    // All 18 ITU / International punctuation symbols: . , ? ' ! / ( ) & : ; = + - _ " $ @
    const punct = ".,?'!/()&:;=+-_\"$@";
    expect_eq(punct.len, 18 as usize);
    i = 0;
    while (i < punct.len) {
        const code = charToMorse(punct[i]) ?? unreachable;
        const rev = morseToChar(code) ?? unreachable;
        expect_eq(rev, punct[i]);
        i += 1;
    }

    // Spot-check critical known ITU codewords
    expect_eq(bytesEq(charToMorse(0x41) ?? unreachable, ".-"), true);       // A
    expect_eq(bytesEq(charToMorse(0x5A) ?? unreachable, "--.."), true);     // Z
    expect_eq(bytesEq(charToMorse(0x30) ?? unreachable, "-----"), true);    // 0
    expect_eq(bytesEq(charToMorse(0x39) ?? unreachable, "----."), true);    // 9
    expect_eq(bytesEq(charToMorse(0x40) ?? unreachable, ".--.-."), true);   // @
    expect_eq(bytesEq(charToMorse(0x24) ?? unreachable, "...-..-"), true);  // $
});

test("2. Text codec roundtrip for SOS, HELLO WORLD 2026!, whitespace normalization & word separators", () => {
    let enc_buf: [256]u8 = std.mem.zeroes([256]u8);
    let dec_buf: [128]u8 = std.mem.zeroes([128]u8);

    // Classic SOS
    const sos_len = morseEncode("SOS", enc_buf[0..]) ?? unreachable;
    expect_eq(bytesEq(enc_buf[0..sos_len], "... --- ..."), true);
    const sos_dec_len = morseDecode(enc_buf[0..sos_len], dec_buf[0..]) ?? unreachable;
    expect_eq(bytesEq(dec_buf[0..sos_dec_len], "SOS"), true);

    // Mixed case + digits + punctuation + repeated/leading/trailing whitespace
    const hw_len = morseEncode("  Hello   World\t2026!  ", enc_buf[0..]) ?? unreachable;
    const expected_hw = ".... . .-.. .-.. --- / .-- --- .-. .-.. -.. / ..--- ----- ..--- -.... -.-.--";
    expect_eq(bytesEq(enc_buf[0..hw_len], expected_hw), true);

    const hw_dec_len = morseDecode(enc_buf[0..hw_len], dec_buf[0..]) ?? unreachable;
    expect_eq(bytesEq(dec_buf[0..hw_dec_len], "HELLO WORLD 2026!"), true);

    // Decode with 3+ consecutive spaces as word separator (no '/')
    const spaced_morse = ".... . .-.. .-.. ---   .-- --- .-. .-.. -..";
    const sp_dec_len = morseDecode(spaced_morse, dec_buf[0..]) ?? unreachable;
    expect_eq(bytesEq(dec_buf[0..sp_dec_len], "HELLO WORLD"), true);

    // Decode with punctuation slash codeword (-..-.) vs word separator (/)
    const slash_enc_len = morseEncode("A/B C", enc_buf[0..]) ?? unreachable;
    expect_eq(bytesEq(enc_buf[0..slash_enc_len], ".- -..-. -... / -.-."), true);
    const slash_dec_len = morseDecode(enc_buf[0..slash_enc_len], dec_buf[0..]) ?? unreachable;
    expect_eq(bytesEq(dec_buf[0..slash_dec_len], "A/B C"), true);
});

test("3. ITU-R M.1677-1 PARIS standard duration units & WPM-to-dit-ms conversion", () => {
    // PARIS = 43 units; with trailing INTER_WORD_GAP (7) = 50 units
    const paris_units = morseDurationUnits("PARIS") ?? unreachable;
    expect_eq(paris_units, 43 as usize);
    expect_eq(paris_units + (INTER_WORD_GAP as usize), PARIS_WORD_UNITS);

    // Case-insensitive & whitespace-collapsed PARIS
    expect_eq(morseDurationUnits("  paris  ") ?? unreachable, 43 as usize);
    expect_eq(morseDurationUnits("PARIS PARIS") ?? unreachable, 93 as usize); // 43 + 7 + 43

    // SOS: S(5) + gap(3) + O(11) + gap(3) + S(5) = 27 units
    expect_eq(morseDurationUnits("SOS") ?? unreachable, 27 as usize);

    // WPM to dit ms (1200 / wpm)
    expect_eq(wpmToDitMs(0) == null, true);
    expect_eq(wpmToDitMs(5) ?? unreachable, 240 as u32);
    expect_eq(wpmToDitMs(12) ?? unreachable, 100 as u32);
    expect_eq(wpmToDitMs(20) ?? unreachable, 60 as u32);
    expect_eq(wpmToDitMs(60) ?? unreachable, 20 as u32);
    expect_eq(wpmToDitMs(1200) ?? unreachable, 1 as u32);
});

test("4. CW keying pulse sequence generation (morseToPulses) structure & exact unit sum", () => {
    let pulses: [64]MorsePulse = std.mem.zeroes([64]MorsePulse);

    // "E T" -> dit(1), word_gap(7), dah(3) => 3 pulses, total 11 units
    const et_cnt = morseToPulses("E T", pulses[0..]) ?? unreachable;
    expect_eq(et_cnt, 3 as usize);
    expect_eq(pulses[0].mark, true);
    expect_eq(pulses[0].units, DIT_UNITS);
    expect_eq(pulses[1].mark, false);
    expect_eq(pulses[1].units, INTER_WORD_GAP);
    expect_eq(pulses[2].mark, true);
    expect_eq(pulses[2].units, DAH_UNITS);

    // "PARIS" pulse sum must exactly equal morseDurationUnits("PARIS") == 43
    // and marks/spaces must strictly alternate starting and ending with mark == true
    const p_cnt = morseToPulses("PARIS", pulses[0..]) ?? unreachable;
    expect_eq(p_cnt > 0, true);
    expect_eq(pulses[0].mark, true);
    expect_eq(pulses[p_cnt - 1].mark, true);

    let sum_units: usize = 0;
    let i: usize = 0;
    while (i < p_cnt) {
        sum_units += pulses[i].units as usize;
        if (i > 0) {
            expect_eq(pulses[i].mark != pulses[i - 1].mark, true);
        }
        i += 1;
    }
    expect_eq(sum_units, 43 as usize);
    expect_eq(sum_units, morseDurationUnits("PARIS") ?? unreachable);
});

test("5. Strict rejection of unsupported characters, malformed codewords & buffer overflows", () => {
    let enc_buf: [32]u8 = std.mem.zeroes([32]u8);
    let dec_buf: [32]u8 = std.mem.zeroes([32]u8);
    let pulses: [8]MorsePulse = std.mem.zeroes([8]MorsePulse);

    // Unsupported characters in charToMorse / morseEncode / morseToPulses / morseDurationUnits
    expect_eq(charToMorse(0x20) == null, true); // space
    expect_eq(charToMorse(0x23) == null, true); // '#'
    expect_eq(charToMorse(0x7E) == null, true); // '~'
    expect_eq(charToMorse(0x80) == null, true); // non-ASCII
    expect_eq(morseEncode("HELLO#WORLD", enc_buf[0..]) == null, true);
    expect_eq(morseToPulses("A~B", pulses[0..]) == null, true);
    expect_eq(morseDurationUnits("BAD\x00CHAR") == null, true);

    // Malformed codewords in morseToChar / morseDecode
    expect_eq(morseToChar("") == null, true);
    expect_eq(morseToChar("........") == null, true);  // 8 symbols > MAX_MORSE_CODE_LEN
    expect_eq(morseToChar("..--") == null, true);      // unassigned 4-symbol pattern
    expect_eq(morseToChar(".-a") == null, true);       // invalid symbol
    expect_eq(morseDecode("... ........ ---", dec_buf[0..]) == null, true);
    expect_eq(morseDecode(".- // -...", dec_buf[0..]) == null, true);
    expect_eq(morseDecode(".- ..-- -...", dec_buf[0..]) == null, true);

    // Output buffer overflow guards (exact-fit succeeds, 1-byte short returns null)
    // "SOS" encodes to 11 bytes ("... --- ...")
    expect_eq(morseEncode("SOS", enc_buf[0..10]) == null, true);
    expect_eq(morseEncode("SOS", enc_buf[0..11]) ?? unreachable, 11 as usize);

    // "SOS" decodes to 3 bytes
    expect_eq(morseDecode("... --- ...", dec_buf[0..2]) == null, true);
    expect_eq(morseDecode("... --- ...", dec_buf[0..3]) ?? unreachable, 3 as usize);

    // "A B" decodes to 3 bytes (tests word space boundary overflow)
    expect_eq(morseDecode(".- / -...", dec_buf[0..1]) == null, true);
    expect_eq(morseDecode(".- / -...", dec_buf[0..2]) == null, true);
    expect_eq(morseDecode(".- / -...", dec_buf[0..3]) ?? unreachable, 3 as usize);

    // "E T" requires 3 pulses
    expect_eq(morseToPulses("E T", pulses[0..2]) == null, true);
    expect_eq(morseToPulses("E T", pulses[0..3]) ?? unreachable, 3 as usize);
});

test("6. Empty & whitespace-only inputs zero-alloc edge behavior", () => {
    let enc_buf: [4]u8 = std.mem.zeroes([4]u8);
    let dec_buf: [4]u8 = std.mem.zeroes([4]u8);
    let pulses: [4]MorsePulse = std.mem.zeroes([4]MorsePulse);

    expect_eq(morseEncode("", enc_buf[0..0]) ?? unreachable, 0 as usize);
    expect_eq(morseEncode("   \t\r\n  ", enc_buf[0..0]) ?? unreachable, 0 as usize);
    expect_eq(morseDecode("", dec_buf[0..0]) ?? unreachable, 0 as usize);
    expect_eq(morseDecode("   /   ", dec_buf[0..0]) ?? unreachable, 0 as usize);
    expect_eq(morseToPulses("", pulses[0..0]) ?? unreachable, 0 as usize);
    expect_eq(morseToPulses("   ", pulses[0..0]) ?? unreachable, 0 as usize);
    expect_eq(morseDurationUnits("") ?? unreachable, 0 as usize);
    expect_eq(morseDurationUnits(" \t\n ") ?? unreachable, 0 as usize);
});
