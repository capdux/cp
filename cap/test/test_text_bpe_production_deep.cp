// Origin: Pure-CP Stdlib Test Suite (lib/text/bpe.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation byte-level BPE tokenizer:
// merge-rank priority ordering, multi-level merge trees, lossless UTF-8 & raw binary
// round-trip, atomic special control token protection & longest-match splitting,
// exact token counting parity, and defensive guards against unknown IDs, cyclic merges,
// and buffer/stack exhaustion.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BpeMerge,
    SpecialToken,
    BpeVocab,
    bpeEncodeSegment,
    bpeEncodeWithSpecials,
    bpeDecode,
    bpeCountTokens,
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

fn testRankPriorityOrderChangesTokenization()! {
    const empty_specials: []const SpecialToken = [];

    // Vocab A: (a,b) has rank 0 (higher priority), (b,c) has rank 1.
    // On "abc" (97, 98, 99), (a,b) merges first -> [256, 99].
    const merges_a: [2]BpeMerge = [
        BpeMerge{ left: 98, right: 99, merged_id: 257, rank: 1 },
        BpeMerge{ left: 97, right: 98, merged_id: 256, rank: 0 },
    ];
    const vocab_a = BpeVocab.init(merges_a[0..], empty_specials);

    // Vocab B: (b,c) has rank 0 (higher priority), (a,b) has rank 1.
    // On "abc", (b,c) merges first -> [97, 257].
    const merges_b: [2]BpeMerge = [
        BpeMerge{ left: 97, right: 98, merged_id: 256, rank: 1 },
        BpeMerge{ left: 98, right: 99, merged_id: 257, rank: 0 },
    ];
    const vocab_b = BpeVocab.init(merges_b[0..], empty_specials);

    let work_arr: [32]u32 = undefined;
    let work: []u32 = work_arr[0..];

    if (na := bpeEncodeSegment(vocab_a, "abc", work)) {
        expect_eq(na, 2);
        expect_eq(work[0], 256);
        expect_eq(work[1], 99);
    } else {
        expect_eq(true, false);
    }

    if (nb := bpeEncodeSegment(vocab_b, "abc", work)) {
        expect_eq(nb, 2);
        expect_eq(work[0], 97);
        expect_eq(work[1], 257);
    } else {
        expect_eq(true, false);
    }

    // Multi-level hierarchical merges + non-overlapping left-to-right folding:
    // "aaaa" -> [256, 256] -> [257]; "aaaaa" -> [256, 256, 97] -> [257, 97]
    const merges_multi: [3]BpeMerge = [
        BpeMerge{ left: 97, right: 97, merged_id: 256, rank: 0 },
        BpeMerge{ left: 256, right: 256, merged_id: 257, rank: 1 },
        BpeMerge{ left: 257, right: 98, merged_id: 258, rank: 2 },
    ];
    const vocab_multi = BpeVocab.init(merges_multi[0..], empty_specials);

    if (n4 := bpeEncodeSegment(vocab_multi, "aaaa", work)) {
        expect_eq(n4, 1);
        expect_eq(work[0], 257);
    } else {
        expect_eq(true, false);
    }

    if (n5 := bpeEncodeSegment(vocab_multi, "aaaab", work)) {
        expect_eq(n5, 1);
        expect_eq(work[0], 258);
    } else {
        expect_eq(true, false);
    }

    let decoded_arr: [32]u8 = undefined;
    let stack_arr: [16]u32 = undefined;
    let decoded: []u8 = decoded_arr[0..];
    let stack: []u32 = stack_arr[0..];
    const toks_258: [1]u32 = [258];
    if (m := bpeDecode(vocab_multi, toks_258[0..], decoded, stack)) {
        expect_eq(m, 5);
        expect_eq(bytesEqual(decoded[0..m], "aaaab"), true);
    } else {
        expect_eq(true, false);
    }
}

fn testUtf8AndRawBinaryLosslessRoundTrip()! {
    const empty_specials: []const SpecialToken = [];
    // Mix ASCII merges, UTF-8 multi-byte prefix merges (0xE4 0xBD = first 2 bytes of '你'),
    // and raw binary byte merges (0x00 0xFF -> 260, 0x80 0xFE -> 261).
    const merges: [6]BpeMerge = [
        BpeMerge{ left: 104, right: 101, merged_id: 256, rank: 0 },   // "he"
        BpeMerge{ left: 108, right: 108, merged_id: 257, rank: 1 },   // "ll"
        BpeMerge{ left: 256, right: 257, merged_id: 258, rank: 2 },   // "hell"
        BpeMerge{ left: 0xE4, right: 0xBD, merged_id: 259, rank: 3 }, // UTF-8 lead+cont
        BpeMerge{ left: 0x00, right: 0xFF, merged_id: 260, rank: 4 }, // raw binary pair
        BpeMerge{ left: 0x80, right: 0xFE, merged_id: 261, rank: 5 }, // raw binary pair
    ];
    const vocab = BpeVocab.init(merges[0..], empty_specials);

    let work_arr: [128]u32 = undefined;
    let decoded_arr: [128]u8 = undefined;
    let stack_arr: [64]u32 = undefined;
    let work: []u32 = work_arr[0..];
    let decoded: []u8 = decoded_arr[0..];
    let stack: []u32 = stack_arr[0..];

    // 1. Mixed UTF-8 (ASCII + Chinese + Emoji)
    const utf8_text = "hello 你好世界! CP标准库 🚀 hello";
    if (n := bpeEncodeSegment(vocab, utf8_text, work)) {
        // Must strictly compress compared to raw byte length
        expect_eq(n < utf8_text.len, true);
        if (m := bpeDecode(vocab, work[0..n], decoded, stack)) {
            expect_eq(m, utf8_text.len);
            expect_eq(bytesEqual(decoded[0..m], utf8_text), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 2. Arbitrary raw binary bytes including NUL (0x00), high bytes (0x80..0xFF)
    const raw_bin: [12]u8 = [0x00, 0xFF, 0x01, 0x7F, 0x80, 0xFE, 0x00, 0xFF, 0xC0, 0x80, 0xFE, 0x55];
    if (n := bpeEncodeSegment(vocab, raw_bin[0..], work)) {
        // Two (0x00, 0xFF) pairs and two (0x80, 0xFE) pairs merged -> 12 - 4 = 8 tokens
        expect_eq(n, 8);
        if (m := bpeDecode(vocab, work[0..n], decoded, stack)) {
            expect_eq(m, raw_bin.len);
            expect_eq(bytesEqual(decoded[0..m], raw_bin[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testSpecialTokensAtomicProtectionAndLongestMatch()! {
    // Merges that would eagerly split "<|im_start|>" if special tokens were not atomic
    const merges: [5]BpeMerge = [
        BpeMerge{ left: 60, right: 124, merged_id: 260, rank: 0 },  // "<|"
        BpeMerge{ left: 105, right: 109, merged_id: 261, rank: 1 }, // "im"
        BpeMerge{ left: 115, right: 116, merged_id: 262, rank: 2 }, // "st"
        BpeMerge{ left: 124, right: 62, merged_id: 263, rank: 3 },  // "|>"
        BpeMerge{ left: 104, right: 105, merged_id: 264, rank: 4 }, // "hi"
    ];
    // Include shorter overlapping prefix "<|im" before "<|im_start|>" to verify longest-match
    const specials: [4]SpecialToken = [
        SpecialToken{ text: "<|im", id: 1003 },
        SpecialToken{ text: "<|im_start|>", id: 1000 },
        SpecialToken{ text: "<|im_end|>", id: 1001 },
        SpecialToken{ text: "<|endoftext|>", id: 1002 },
    ];
    const vocab = BpeVocab.init(merges[0..], specials[0..]);

    const prompt = "<|im_start|>hi im st<|im_end|><|endoftext|>";
    let out_toks_arr: [64]u32 = undefined;
    let seg_work_arr: [64]u32 = undefined;
    let out_toks: []u32 = out_toks_arr[0..];
    let seg_work: []u32 = seg_work_arr[0..];

    if (n := bpeEncodeWithSpecials(vocab, prompt, out_toks, seg_work)) {
        // Expected token sequence:
        // [1000, 264("hi"), 32(' '), 261("im"), 32(' '), 262("st"), 1001, 1002]
        expect_eq(n, 8);
        expect_eq(out_toks[0], 1000);
        expect_eq(out_toks[1], 264);
        expect_eq(out_toks[2], 32);
        expect_eq(out_toks[3], 261);
        expect_eq(out_toks[4], 32);
        expect_eq(out_toks[5], 262);
        expect_eq(out_toks[6], 1001);
        expect_eq(out_toks[7], 1002);

        // Lossless decode back to exact prompt
        let decoded_arr: [128]u8 = undefined;
        let stack_arr: [32]u32 = undefined;
        let decoded: []u8 = decoded_arr[0..];
        let stack: []u32 = stack_arr[0..];
        if (m := bpeDecode(vocab, out_toks[0..n], decoded, stack)) {
            expect_eq(m, prompt.len);
            expect_eq(bytesEqual(decoded[0..m], prompt), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testCountTokensMatchesEncodeExactly()! {
    const merges: [3]BpeMerge = [
        BpeMerge{ left: 116, right: 104, merged_id: 256, rank: 0 }, // "th"
        BpeMerge{ left: 256, right: 101, merged_id: 257, rank: 1 }, // "the"
        BpeMerge{ left: 105, right: 110, merged_id: 258, rank: 2 }, // "in"
    ];
    const specials: [2]SpecialToken = [
        SpecialToken{ text: "<|im_start|>", id: 1000 },
        SpecialToken{ text: "<|im_end|>", id: 1001 },
    ];
    const vocab = BpeVocab.init(merges[0..], specials[0..]);

    let out_toks_arr: [128]u32 = undefined;
    let seg_work_arr: [128]u32 = undefined;
    let out_toks: []u32 = out_toks_arr[0..];
    let seg_work: []u32 = seg_work_arr[0..];

    const samples: [5]string = [
        "",
        "the thing in the bin",
        "<|im_start|><|im_end|>",
        "<|im_start|>the 中文 in<|im_end|>the",
        "no merges here 12345",
    ];

    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        const s = samples[i];
        const enc_opt = bpeEncodeWithSpecials(vocab, s, out_toks, seg_work);
        const cnt_opt = bpeCountTokens(vocab, s, seg_work);
        if (enc_n := enc_opt) {
            if (cnt_n := cnt_opt) {
                expect_eq(cnt_n, enc_n);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    }
}

fn testEmptySingleByteAndEmptySpecialSafety()! {
    const empty_merges: []const BpeMerge = [];
    // Include an empty-text special token to ensure it is safely ignored (no infinite loop)
    const specials: [2]SpecialToken = [
        SpecialToken{ text: "", id: 9999 },
        SpecialToken{ text: "<|eos|>", id: 500 },
    ];
    const vocab = BpeVocab.init(empty_merges, specials[0..]);

    let out_toks_arr: [16]u32 = undefined;
    let seg_work_arr: [16]u32 = undefined;
    let decoded_arr: [16]u8 = undefined;
    let stack_arr: [1]u32 = undefined;
    let out_toks: []u32 = out_toks_arr[0..];
    let seg_work: []u32 = seg_work_arr[0..];
    let decoded: []u8 = decoded_arr[0..];
    let empty_stack: []u32 = stack_arr[0..0];

    // Empty input
    if (n0 := bpeEncodeWithSpecials(vocab, "", out_toks, seg_work)) {
        expect_eq(n0, 0);
    } else {
        expect_eq(true, false);
    }
    if (c0 := bpeCountTokens(vocab, "", seg_work)) {
        expect_eq(c0, 0);
    } else {
        expect_eq(true, false);
    }
    if (d0 := bpeDecode(vocab, out_toks[0..0], decoded, empty_stack)) {
        expect_eq(d0, 0);
    } else {
        expect_eq(true, false);
    }

    // Single byte + special token works even with zero-length expand_stack (fast path)
    if (n := bpeEncodeWithSpecials(vocab, "Z<|eos|>", out_toks, seg_work)) {
        expect_eq(n, 2);
        expect_eq(out_toks[0], 90);
        expect_eq(out_toks[1], 500);
        if (m := bpeDecode(vocab, out_toks[0..n], decoded, empty_stack)) {
            expect_eq(m, 8);
            expect_eq(bytesEqual(decoded[0..m], "Z<|eos|>"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testDefensiveGuardsUnknownIdCyclesAndBuffers()! {
    const empty_specials: []const SpecialToken = [];
    // Malformed vocab with both direct self-cycle (300 -> 300, 65)
    // and indirect 2-cycle (301 -> 302, 66; 302 -> 301, 67).
    const bad_merges: [3]BpeMerge = [
        BpeMerge{ left: 300, right: 65, merged_id: 300, rank: 0 },
        BpeMerge{ left: 302, right: 66, merged_id: 301, rank: 1 },
        BpeMerge{ left: 301, right: 67, merged_id: 302, rank: 2 },
    ];
    const bad_vocab = BpeVocab.init(bad_merges[0..], empty_specials);

    let decoded_arr: [32]u8 = undefined;
    let stack_arr: [32]u32 = undefined;
    let decoded: []u8 = decoded_arr[0..];
    let stack: []u32 = stack_arr[0..];

    // 1. Unknown token ID must return null
    const unknown_toks: [1]u32 = [77777];
    expect_eq(bpeDecode(bad_vocab, unknown_toks[0..], decoded, stack) == null, true);

    // 2. Direct self-cycle must return null cleanly
    const self_cycle_toks: [1]u32 = [300];
    expect_eq(bpeDecode(bad_vocab, self_cycle_toks[0..], decoded, stack) == null, true);

    // 3. Indirect cycle must return null cleanly (stack or expansion guard)
    const indirect_cycle_toks: [1]u32 = [301];
    expect_eq(bpeDecode(bad_vocab, indirect_cycle_toks[0..], decoded, stack) == null, true);

    // 4. Buffer exhaustion guards (seg_work too small, out_tokens too small, out_bytes too small, expand_stack too small)
    const ok_merges: [1]BpeMerge = [
        BpeMerge{ left: 65, right: 66, merged_id: 256, rank: 0 },
    ];
    const ok_vocab = BpeVocab.init(ok_merges[0..], empty_specials);

    let tiny_work_arr: [2]u32 = undefined;
    let tiny_work: []u32 = tiny_work_arr[0..];
    let big_out_arr: [16]u32 = undefined;
    let big_out: []u32 = big_out_arr[0..];
    expect_eq(bpeEncodeSegment(ok_vocab, "ABCD", tiny_work) == null, true);
    expect_eq(bpeEncodeWithSpecials(ok_vocab, "ABCD", big_out, tiny_work) == null, true);
    expect_eq(bpeCountTokens(ok_vocab, "ABCD", tiny_work) == null, true);

    let tiny_out_arr: [1]u32 = undefined;
    let tiny_out: []u32 = tiny_out_arr[0..];
    let big_work_arr: [16]u32 = undefined;
    let big_work: []u32 = big_work_arr[0..];
    expect_eq(bpeEncodeWithSpecials(ok_vocab, "ABCD", tiny_out, big_work) == null, true);

    const toks_256: [1]u32 = [256];
    let tiny_bytes_arr: [1]u8 = undefined;
    let tiny_bytes: []u8 = tiny_bytes_arr[0..];
    expect_eq(bpeDecode(ok_vocab, toks_256[0..], tiny_bytes, stack) == null, true);

    let tiny_stack_arr: [1]u32 = undefined;
    let tiny_stack: []u32 = tiny_stack_arr[0..];
    expect_eq(bpeDecode(ok_vocab, toks_256[0..], decoded, tiny_stack) == null, true);

    // 5. Namespace access via `cap.text.*`
    if (n := cap.text.bpeEncodeWithSpecials(ok_vocab, "AB", big_out, big_work)) {
        expect_eq(n, 1);
        expect_eq(big_out[0], 256);
        if (m := cap.text.bpeDecode(ok_vocab, big_out[0..n], decoded, stack)) {
            expect_eq(m, 2);
            expect_eq(bytesEqual(decoded[0..m], "AB"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

pub fn main()! {
    test("1. text/bpe: rank priority ordering and multi-level merge trees", () => {
        testRankPriorityOrderChangesTokenization()?;
    });
    test("2. text/bpe: arbitrary UTF-8 and raw binary lossless encode-decode round-trip", () => {
        testUtf8AndRawBinaryLosslessRoundTrip()?;
    });
    test("3. text/bpe: atomic special control token protection and longest-match splitting", () => {
        testSpecialTokensAtomicProtectionAndLongestMatch()?;
    });
    test("4. text/bpe: bpeCountTokens exact parity with bpeEncodeWithSpecials", () => {
        testCountTokensMatchesEncodeExactly()?;
    });
    test("5. text/bpe: empty input, single byte, and empty special token safety", () => {
        testEmptySingleByteAndEmptySpecialSafety()?;
    });
    test("6. text/bpe: unknown ID, cyclic merges, buffer exhaustion, and cap.text namespace", () => {
        testDefensiveGuardsUnknownIdCyclesAndBuffers()?;
    });
}
