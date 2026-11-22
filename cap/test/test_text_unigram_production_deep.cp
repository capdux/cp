// Origin: Pure-CP Stdlib Test Suite (lib/text/unigram.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation SentencePiece Unigram tokenizer:
// Viterbi global maximum log-likelihood segmentation beating greedy longest-match,
// normalizeSentencePiece + unigramEncodeViterbi + unigramDecode lossless round-trip,
// UTF-8 multibyte Chinese & Emoji unknown codepoint fallback without splitting codepoints,
// empty text/vocab edge cases, NaN/Inf log-probability guards, and buffer exhaustion safety.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    UnigramPiece,
    UnigramVocab,
    UnigramResult,
    unigramEncodeViterbi,
    unigramDecode,
    normalizeSentencePiece,
} from "cap:text";

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn approxEqual(a: f64, b: f64, eps: f64): bool {
    const diff = a - b;
    return diff >= -eps && diff <= eps;
}

fn testViterbiBeatsGreedyLongestMatch()! {
    // Classic Viterbi vs Greedy case on "abcd":
    // Greedy longest-match would pick "abc" (-5.0) + "d" (-5.0) = -10.0.
    // Global maximum likelihood Viterbi MUST pick "ab" (-1.2) + "cd" (-1.3) = -2.5.
    const pieces: [6]UnigramPiece = [
        UnigramPiece{ piece: "abc", log_prob: -5.0, id: 1 },
        UnigramPiece{ piece: "d", log_prob: -5.0, id: 2 },
        UnigramPiece{ piece: "ab", log_prob: -1.2, id: 3 },
        UnigramPiece{ piece: "cd", log_prob: -1.3, id: 4 },
        UnigramPiece{ piece: "a", log_prob: -4.0, id: 5 },
        UnigramPiece{ piece: "bcd", log_prob: -4.0, id: 6 },
    ];
    const vocab = UnigramVocab.init(pieces[0..], 0, -15.0);

    let out_ids_arr: [16]u32 = undefined;
    let dp_s_arr: [16]f64 = undefined;
    let dp_p_arr: [16]usize = undefined;
    let dp_id_arr: [16]u32 = undefined;
    let out_ids: []u32 = out_ids_arr[0..];
    let dp_s: []f64 = dp_s_arr[0..];
    let dp_p: []usize = dp_p_arr[0..];
    let dp_id: []u32 = dp_id_arr[0..];

    if (res := unigramEncodeViterbi(vocab, "abcd", out_ids, dp_s, dp_p, dp_id)) {
        expect_eq(res.token_count, 2);
        expect_eq(res.unk_count, 0);
        expect_eq(approxEqual(res.total_log_prob, -2.5, 1.0e-9), true);
        expect_eq(out_ids[0], 3); // "ab"
        expect_eq(out_ids[1], 4); // "cd"
    } else {
        expect_eq(true, false);
    }
}

fn testSentencePieceNormalizeEncodeDecodeRoundTrip()! {
    const pieces: [7]UnigramPiece = [
        UnigramPiece{ piece: "▁hello", log_prob: -1.1, id: 10 },
        UnigramPiece{ piece: "▁world", log_prob: -1.2, id: 11 },
        UnigramPiece{ piece: "▁CP", log_prob: -1.3, id: 12 },
        UnigramPiece{ piece: "▁你好", log_prob: -1.4, id: 13 },
        UnigramPiece{ piece: "!", log_prob: -0.9, id: 14 },
        UnigramPiece{ piece: "▁", log_prob: -2.5, id: 15 },
        UnigramPiece{ piece: "hello", log_prob: -3.0, id: 16 },
    ];
    const vocab = UnigramVocab.init(pieces[0..], 0, -12.0);

    const original = "hello world CP 你好!";
    let norm_arr: [128]u8 = undefined;
    let norm: []u8 = norm_arr[0..];

    if (norm_len := normalizeSentencePiece(original, true, norm)) {
        // Leading ▁ (3 bytes) + 3 internal spaces expanded to ▁ (+2 each) => original.len + 9
        expect_eq(norm_len, original.len + 9);
        expect_eq(norm[0], 0xE2);
        expect_eq(norm[1], 0x96);
        expect_eq(norm[2], 0x81);

        let out_ids_arr: [32]u32 = undefined;
        let dp_s_arr: [129]f64 = undefined;
        let dp_p_arr: [129]usize = undefined;
        let dp_id_arr: [129]u32 = undefined;
        let out_ids: []u32 = out_ids_arr[0..];
        let dp_s: []f64 = dp_s_arr[0..];
        let dp_p: []usize = dp_p_arr[0..];
        let dp_id: []u32 = dp_id_arr[0..];

        if (res := unigramEncodeViterbi(vocab, norm[0..norm_len], out_ids, dp_s, dp_p, dp_id)) {
            expect_eq(res.token_count, 5);
            expect_eq(res.unk_count, 0);
            expect_eq(out_ids[0], 10); // "▁hello"
            expect_eq(out_ids[1], 11); // "▁world"
            expect_eq(out_ids[2], 12); // "▁CP"
            expect_eq(out_ids[3], 13); // "▁你好"
            expect_eq(out_ids[4], 14); // "!"

            // Decode with strip_leading_space = true -> exact original string
            let decoded_arr: [128]u8 = undefined;
            let decoded: []u8 = decoded_arr[0..];
            if (m := unigramDecode(vocab, out_ids[0..res.token_count], true, decoded)) {
                expect_eq(m, original.len);
                expect_eq(bytesEqual(decoded[0..m], original), true);
            } else {
                expect_eq(true, false);
            }

            // Decode with strip_leading_space = false -> preserves leading space
            if (m2 := unigramDecode(vocab, out_ids[0..res.token_count], false, decoded)) {
                expect_eq(m2, original.len + 1);
                expect_eq(decoded[0], ' ');
                expect_eq(bytesEqual(decoded[1..m2], original), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testUtf8MultibyteUnknownCodepointFallback()! {
    // Vocab knows "CP" and "!" only; Chinese '中' (3 bytes), '文' (3 bytes), and Emoji '🚀' (4 bytes)
    // must each fall back to exactly ONE `unk_id` (total 3 unks, NOT 10 byte-level unks!).
    const pieces: [2]UnigramPiece = [
        UnigramPiece{ piece: "CP", log_prob: -1.0, id: 1 },
        UnigramPiece{ piece: "!", log_prob: -1.0, id: 2 },
    ];
    const vocab = UnigramVocab.init(pieces[0..], 99, -8.5);

    const input = "CP中文🚀!";
    let out_ids_arr: [16]u32 = undefined;
    let dp_s_arr: [32]f64 = undefined;
    let dp_p_arr: [32]usize = undefined;
    let dp_id_arr: [32]u32 = undefined;
    let out_ids: []u32 = out_ids_arr[0..];
    let dp_s: []f64 = dp_s_arr[0..];
    let dp_p: []usize = dp_p_arr[0..];
    let dp_id: []u32 = dp_id_arr[0..];

    if (res := unigramEncodeViterbi(vocab, input, out_ids, dp_s, dp_p, dp_id)) {
        // Expected: ["CP"(1), UNK('中'=99), UNK('文'=99), UNK('🚀'=99), "!"(2)]
        expect_eq(res.token_count, 5);
        expect_eq(res.unk_count, 3);
        expect_eq(approxEqual(res.total_log_prob, -1.0 - 8.5 * 3.0 - 1.0, 1.0e-9), true);
        expect_eq(out_ids[0], 1);
        expect_eq(out_ids[1], 99);
        expect_eq(out_ids[2], 99);
        expect_eq(out_ids[3], 99);
        expect_eq(out_ids[4], 2);
    } else {
        expect_eq(true, false);
    }
}

fn testEmptyTextEmptyVocabAndLeadingSpaces()! {
    const empty_pieces: []const UnigramPiece = [];
    const empty_vocab = UnigramVocab.init(empty_pieces, 7, -5.0);

    let out_ids_arr: [16]u32 = undefined;
    let dp_s_arr: [16]f64 = undefined;
    let dp_p_arr: [16]usize = undefined;
    let dp_id_arr: [16]u32 = undefined;
    let norm_arr: [32]u8 = undefined;
    let decoded_arr: [32]u8 = undefined;
    let out_ids: []u32 = out_ids_arr[0..];
    let dp_s: []f64 = dp_s_arr[0..];
    let dp_p: []usize = dp_p_arr[0..];
    let dp_id: []u32 = dp_id_arr[0..];
    let norm: []u8 = norm_arr[0..];
    let decoded: []u8 = decoded_arr[0..];

    // 1. Empty string normalization returns 0 even when add_dummy_prefix = true
    if (n0 := normalizeSentencePiece("", true, norm)) {
        expect_eq(n0, 0);
    } else {
        expect_eq(true, false);
    }

    // 2. Empty text Viterbi encode returns 0 tokens, 0.0 log_prob, 0 unks
    if (r0 := unigramEncodeViterbi(empty_vocab, "", out_ids, dp_s, dp_p, dp_id)) {
        expect_eq(r0.token_count, 0);
        expect_eq(r0.unk_count, 0);
        expect_eq(r0.total_log_prob, 0.0);
    } else {
        expect_eq(true, false);
    }

    // 3. Empty vocab on "hi" falls back to 2 single-codepoint unks
    if (r_unk := unigramEncodeViterbi(empty_vocab, "hi", out_ids, dp_s, dp_p, dp_id)) {
        expect_eq(r_unk.token_count, 2);
        expect_eq(r_unk.unk_count, 2);
        expect_eq(out_ids[0], 7);
        expect_eq(out_ids[1], 7);
        expect_eq(approxEqual(r_unk.total_log_prob, -10.0, 1.0e-9), true);
    } else {
        expect_eq(true, false);
    }

    // 4. Empty token slice decode returns 0
    if (d0 := unigramDecode(empty_vocab, out_ids[0..0], true, decoded)) {
        expect_eq(d0, 0);
    } else {
        expect_eq(true, false);
    }
}

fn testDefensiveGuardsNanBuffersAndNamespace()! {
    let zero_val: f64 = 0.0;
    const nan_val: f64 = zero_val / zero_val;

    // 1. NaN in piece log_prob -> null
    const nan_pieces: [1]UnigramPiece = [
        UnigramPiece{ piece: "a", log_prob: nan_val, id: 1 },
    ];
    const nan_vocab_1 = UnigramVocab.init(nan_pieces[0..], 0, -10.0);

    // 2. NaN in unk_penalty -> null
    const ok_pieces: [2]UnigramPiece = [
        UnigramPiece{ piece: "▁cp", log_prob: -1.0, id: 1 },
        UnigramPiece{ piece: "a", log_prob: -2.0, id: 2 },
    ];
    const nan_vocab_2 = UnigramVocab.init(ok_pieces[0..], 0, nan_val);
    const ok_vocab = UnigramVocab.init(ok_pieces[0..], 0, -10.0);

    let out_ids_arr: [16]u32 = undefined;
    let dp_s_arr: [16]f64 = undefined;
    let dp_p_arr: [16]usize = undefined;
    let dp_id_arr: [16]u32 = undefined;
    let out_ids: []u32 = out_ids_arr[0..];
    let dp_s: []f64 = dp_s_arr[0..];
    let dp_p: []usize = dp_p_arr[0..];
    let dp_id: []u32 = dp_id_arr[0..];

    expect_eq(unigramEncodeViterbi(nan_vocab_1, "a", out_ids, dp_s, dp_p, dp_id) == null, true);
    expect_eq(unigramEncodeViterbi(nan_vocab_2, "a", out_ids, dp_s, dp_p, dp_id) == null, true);

    // 3. Insufficient DP buffers (len <= text.len) and output buffers -> null
    let tiny_s_arr: [2]f64 = undefined;
    let tiny_s: []f64 = tiny_s_arr[0..];
    expect_eq(unigramEncodeViterbi(ok_vocab, "aaa", out_ids, tiny_s, dp_p, dp_id) == null, true);

    let tiny_out_arr: [1]u32 = undefined;
    let tiny_out: []u32 = tiny_out_arr[0..];
    expect_eq(unigramEncodeViterbi(ok_vocab, "aaa", tiny_out, dp_s, dp_p, dp_id) == null, true);

    let tiny_bytes_arr: [2]u8 = undefined;
    let tiny_bytes: []u8 = tiny_bytes_arr[0..];
    expect_eq(normalizeSentencePiece("cp", true, tiny_bytes) == null, true);

    // 4. Unknown ID in unigramDecode -> null, and tiny decode buffer -> null
    const bad_ids: [1]u32 = [9999];
    let dec_buf_arr: [16]u8 = undefined;
    let dec_buf: []u8 = dec_buf_arr[0..];
    expect_eq(unigramDecode(ok_vocab, bad_ids[0..], true, dec_buf) == null, true);

    const id_cp: [1]u32 = [1];
    let tiny_dec_arr: [1]u8 = undefined;
    let tiny_dec: []u8 = tiny_dec_arr[0..];
    expect_eq(unigramDecode(ok_vocab, id_cp[0..], true, tiny_dec) == null, true);

    // 5. Namespace access via `cap.text.*`
    let norm_arr: [16]u8 = undefined;
    let norm: []u8 = norm_arr[0..];
    if (nl := cap.text.normalizeSentencePiece("cp", true, norm)) {
        if (res := cap.text.unigramEncodeViterbi(ok_vocab, norm[0..nl], out_ids, dp_s, dp_p, dp_id)) {
            expect_eq(res.token_count, 1);
            expect_eq(out_ids[0], 1);
            if (dl := cap.text.unigramDecode(ok_vocab, out_ids[0..1], true, dec_buf)) {
                expect_eq(dl, 2);
                expect_eq(bytesEqual(dec_buf[0..dl], "cp"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

pub fn main()! {
    test("1. text/unigram: Viterbi maximum log-likelihood beats greedy longest-match", () => {
        testViterbiBeatsGreedyLongestMatch()?;
    });
    test("2. text/unigram: normalizeSentencePiece + Viterbi + decode lossless round-trip", () => {
        testSentencePieceNormalizeEncodeDecodeRoundTrip()?;
    });
    test("3. text/unigram: UTF-8 multibyte Chinese & Emoji single-codepoint unk fallback", () => {
        testUtf8MultibyteUnknownCodepointFallback()?;
    });
    test("4. text/unigram: empty text, empty vocab, and zero-length decode edge cases", () => {
        testEmptyTextEmptyVocabAndLeadingSpaces()?;
    });
    test("5. text/unigram: NaN probability guards, buffer limits, unknown ID & cap.text", () => {
        testDefensiveGuardsNanBuffersAndNamespace()?;
    });
}
