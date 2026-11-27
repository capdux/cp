import { test, expect_eq } from "cap:test";
import {
    Utf8DecodeStatus,
    Utf8ChunkResult,
    decodeUtf8Chunk,
    isValidUtf8,
    LossyUtf8Decoder,
} from "../../packages/utf-8/src/index.cp";

fn main() {
    test("1. @cap/utf-8 — valid ASCII & multi-byte UTF-8 (2/3/4-byte)", () => {
        const s = "Hello, 世界! 🚀 café";
        const res = decodeUtf8Chunk(s);
        expect_eq(res.status == Utf8DecodeStatus.Ok, true);
        expect_eq(res.valid_prefix.len, s.len);
        expect_eq(res.consumed, s.len);
        expect_eq(res.error_len, 0 as usize);
        expect_eq(isValidUtf8(s), true);
        expect_eq(isValidUtf8(""), true);
    });

    test("2. @cap/utf-8 — rejects overlong encodings (C0/C1, E0 80, F0 80)", () => {
        // Overlong 2-byte NUL: C0 80
        const over2: [2]u8 = [0xC0 as u8, 0x80 as u8];
        const r2 = decodeUtf8Chunk(over2[0..2]);
        expect_eq(r2.status == Utf8DecodeStatus.InvalidSequence, true);
        expect_eq(r2.valid_prefix.len, 0 as usize);
        expect_eq(r2.consumed, 1 as usize);
        expect_eq(r2.error_len, 1 as usize);

        // Overlong 3-byte: E0 80 AF
        const over3: [3]u8 = [0xE0 as u8, 0x80 as u8, 0xAF as u8];
        const r3 = decodeUtf8Chunk(over3[0..3]);
        expect_eq(r3.status == Utf8DecodeStatus.InvalidSequence, true);
        expect_eq(r3.error_len, 1 as usize);
        expect_eq(isValidUtf8(over3[0..3]), false);

        // Overlong 4-byte: F0 82 82 AC
        const over4: [4]u8 = [0xF0 as u8, 0x82 as u8, 0x82 as u8, 0xAC as u8];
        const r4 = decodeUtf8Chunk(over4[0..4]);
        expect_eq(r4.status == Utf8DecodeStatus.InvalidSequence, true);
        expect_eq(r4.error_len, 1 as usize);
    });

    test("3. @cap/utf-8 — rejects UTF-16 surrogates (U+D800..U+DFFF) & > U+10FFFF", () => {
        // U+D800: ED A0 80
        const surr: [5]u8 = [0x61 as u8, 0x62 as u8, 0xED as u8, 0xA0 as u8, 0x80 as u8];
        const rs = decodeUtf8Chunk(surr[0..5]);
        expect_eq(rs.status == Utf8DecodeStatus.InvalidSequence, true);
        expect_eq(rs.valid_prefix.len, 2 as usize);
        expect_eq(rs.consumed, 3 as usize);
        expect_eq(rs.error_len, 1 as usize);

        // U+110000: F4 90 80 80
        const oob: [4]u8 = [0xF4 as u8, 0x90 as u8, 0x80 as u8, 0x80 as u8];
        const ro = decodeUtf8Chunk(oob[0..4]);
        expect_eq(ro.status == Utf8DecodeStatus.InvalidSequence, true);
        expect_eq(ro.error_len, 1 as usize);

        // Invalid lead byte F5
        const bad_lead: [1]u8 = [0xF5 as u8];
        expect_eq(isValidUtf8(bad_lead[0..1]), false);
    });

    test("4. @cap/utf-8 — distinguishes Incomplete trailing prefix vs InvalidSequence", () => {
        // "hi" + first 2 bytes of 4-byte emoji 🚀 (F0 9F 9A 80)
        const trunc: [4]u8 = [0x68 as u8, 0x69 as u8, 0xF0 as u8, 0x9F as u8];
        const rt = decodeUtf8Chunk(trunc[0..4]);
        expect_eq(rt.status == Utf8DecodeStatus.Incomplete, true);
        expect_eq(rt.valid_prefix.len, 2 as usize);
        expect_eq(rt.consumed, 2 as usize);
        expect_eq(rt.error_len, 2 as usize);

        // Valid 2-byte prefix of 3-byte char followed by ASCII 'A' -> InvalidSequence with error_len=2
        const bad_cont: [3]u8 = [0xE2 as u8, 0x82 as u8, 0x41 as u8];
        const rb = decodeUtf8Chunk(bad_cont[0..3]);
        expect_eq(rb.status == Utf8DecodeStatus.InvalidSequence, true);
        expect_eq(rb.valid_prefix.len, 0 as usize);
        expect_eq(rb.consumed, 2 as usize);
        expect_eq(rb.error_len, 2 as usize);
    });

    test("5. @cap/utf-8 — LossyUtf8Decoder reassembles split multi-byte chars across chunks", () => {
        let dec_u8: LossyUtf8Decoder = LossyUtf8Decoder.new();
        let out: [32]u8 = [0 as u8; 32];

        // Split "A世🚀Z" across 3 arbitrary chunk boundaries:
        // 'A' (41), '世' (E4 B8 96), '🚀' (F0 9F 9A 80), 'Z' (5A)
        const c1: [3]u8 = [0x41 as u8, 0xE4 as u8, 0xB8 as u8];
        const c2: [3]u8 = [0x96 as u8, 0xF0 as u8, 0x9F as u8];
        const c3: [3]u8 = [0x9A as u8, 0x80 as u8, 0x5A as u8];

        const w1 = dec_u8.push(c1[0..3], out[0..32]);
        expect_eq(w1, 1 as usize); // only 'A' emitted so far
        expect_eq(dec_u8.hasPending(), true);
        expect_eq(dec_u8.pendingLen(), 2 as usize);

        const w2 = dec_u8.push(c2[0..3], out[w1..32]);
        expect_eq(w2, 3 as usize); // '世' emitted
        expect_eq(dec_u8.pendingLen(), 2 as usize);

        const w3 = dec_u8.push(c3[0..3], out[w1 + w2 .. 32]);
        expect_eq(w3, 5 as usize); // '🚀' (4) + 'Z' (1)
        expect_eq(dec_u8.hasPending(), false);

        const wf = dec_u8.finish(out[w1 + w2 + w3 .. 32]);
        expect_eq(wf, 0 as usize);

        const total = w1 + w2 + w3;
        expect_eq(total, 9 as usize);
        expect_eq(out[0], 0x41 as u8);
        expect_eq(out[1], 0xE4 as u8);
        expect_eq(out[4], 0xF0 as u8);
        expect_eq(out[8], 0x5A as u8);
    });

    test("6. @cap/utf-8 — LossyUtf8Decoder Unicode Maximal Subpart U+FFFD & finish()", () => {
        let dec_u8: LossyUtf8Decoder = LossyUtf8Decoder.new();
        let out: [32]u8 = [0 as u8; 32];

        // Chunk 1 ends with valid 2-byte prefix of 3-byte sequence (E2 82)
        const c1: [2]u8 = [0xE2 as u8, 0x82 as u8];
        const w1 = dec_u8.push(c1[0..2], out[0..32]);
        expect_eq(w1, 0 as usize);
        expect_eq(dec_u8.pendingLen(), 2 as usize);

        // Chunk 2 starts with 'X' (invalid cont) then trailing incomplete lead F0
        const c2: [2]u8 = [0x58 as u8, 0xF0 as u8];
        const w2 = dec_u8.push(c2[0..2], out[0..32]);
        // Expected: U+FFFD (3 bytes) + 'X' (1 byte) = 4 bytes; F0 stays pending
        expect_eq(w2, 4 as usize);
        expect_eq(out[0], 0xEF as u8);
        expect_eq(out[1], 0xBF as u8);
        expect_eq(out[2], 0xBD as u8);
        expect_eq(out[3], 0x58 as u8);
        expect_eq(dec_u8.pendingLen(), 1 as usize);

        // finish() flushes pending F0 as a single U+FFFD
        const wf = dec_u8.finish(out[w2..32]);
        expect_eq(wf, 3 as usize);
        expect_eq(out[4], 0xEF as u8);
        expect_eq(out[5], 0xBF as u8);
        expect_eq(out[6], 0xBD as u8);
        expect_eq(dec_u8.hasPending(), false);
    });
}
