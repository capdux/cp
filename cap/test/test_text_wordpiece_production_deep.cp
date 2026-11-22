// Origin: Pure-CP Stdlib Test Suite (lib/text/wordpiece.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation WordPiece subword tokenizer
// and BERT/Embedding encoder input packer: greedy longest-match `##` subword splitting
// ("unaffable" -> ["un", "##aff", "##able"]), ASCII case-folding & punctuation splitting,
// OOV & overlong `[UNK]` fallback, single/pair `[CLS] A [SEP] B [SEP] [PAD]...` packing
// with `attention_mask` & `token_type_ids`, `max_seq_len` truncation preserving `[SEP]`,
// and defensive guards against empty vocab, empty text, invalid limits, and small buffers.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    WordPieceConfig,
    WordPieceVocab,
    EncodedSequenceStats,
    wordPieceEncodeWord,
    wordPieceEncodeText,
    wordPieceEncodePair,
} from "cap:text";

fn testGreedyLongestMatchUnaffable()! {
    // Classic BERT WordPiece example: "unaffable" -> ["un", "##aff", "##able"]
    // Include shorter distractors ("u", "##a", "##af", "##ab") to prove greedy longest match.
    const vocab_tokens: [14]string = [
        "[PAD]", "[UNK]", "[CLS]", "[SEP]",
        "u", "un", "##a", "##af", "##aff", "##ab", "##able", "##f", "##l", "##e",
    ];
    const vocab = WordPieceVocab.init(vocab_tokens[0..]);
    const cfg = WordPieceConfig{
        unk_id: 1,
        cls_id: 2,
        sep_id: 3,
        pad_id: 0,
        lowercase: true,
        max_word_chars: 100,
    };

    // Verify WordPieceVocab.lookup
    if (id := vocab.lookup("##aff")) {
        expect_eq(id, 8);
    } else {
        expect_eq(true, false);
    }
    expect_eq(vocab.lookup("##missing") == null, true);

    let out_arr: [16]u32 = undefined;
    let work_arr: [64]u8 = undefined;
    let out: []u32 = out_arr[0..];
    let work: []u8 = work_arr[0..];

    if (n := wordPieceEncodeWord(vocab, "unaffable", cfg, out, work)) {
        expect_eq(n, 3);
        expect_eq(out[0], 5);  // "un"
        expect_eq(out[1], 8);  // "##aff"
        expect_eq(out[2], 10); // "##able"
    } else {
        expect_eq(true, false);
    }
}

fn testCaseFoldingAndPunctuationSplitting()! {
    const vocab_tokens: [12]string = [
        "[PAD]", "[UNK]", "[CLS]", "[SEP]",
        "hello", "world", "cp", "!", ",", ".", "-", "World",
    ];
    const vocab = WordPieceVocab.init(vocab_tokens[0..]);

    let out_arr: [16]u32 = undefined;
    let work_arr: [64]u8 = undefined;
    let out: []u32 = out_arr[0..];
    let work: []u8 = work_arr[0..];

    // 1. lowercase = true: "Hello, World-CP!" -> ["hello", ",", "world", "-", "cp", "!"]
    const cfg_lower = WordPieceConfig{
        unk_id: 1,
        cls_id: 2,
        sep_id: 3,
        pad_id: 0,
        lowercase: true,
        max_word_chars: 100,
    };
    if (n := wordPieceEncodeText(vocab, "  Hello,   World-CP!\n", cfg_lower, out, work)) {
        expect_eq(n, 6);
        expect_eq(out[0], 4);  // "hello"
        expect_eq(out[1], 8);  // ","
        expect_eq(out[2], 5);  // "world"
        expect_eq(out[3], 10); // "-"
        expect_eq(out[4], 6);  // "cp"
        expect_eq(out[5], 7);  // "!"
    } else {
        expect_eq(true, false);
    }

    // 2. lowercase = false: "World." -> ["World" (11), "." (9)], while "Hello" -> [UNK] (1)
    const cfg_cased = WordPieceConfig{
        unk_id: 1,
        cls_id: 2,
        sep_id: 3,
        pad_id: 0,
        lowercase: false,
        max_word_chars: 100,
    };
    if (n := wordPieceEncodeText(vocab, "Hello World.", cfg_cased, out, work)) {
        expect_eq(n, 3);
        expect_eq(out[0], 1);  // "Hello" -> [UNK]
        expect_eq(out[1], 11); // "World" exact cased match
        expect_eq(out[2], 9);  // "."
    } else {
        expect_eq(true, false);
    }
}

fn testOovAndOverlongWordFallbackToUnk()! {
    const vocab_tokens: [9]string = [
        "[PAD]", "[UNK]", "[CLS]", "[SEP]",
        "un", "##aff", "##able", "ok", "a",
    ];
    const vocab = WordPieceVocab.init(vocab_tokens[0..]);
    const cfg = WordPieceConfig{
        unk_id: 1,
        cls_id: 2,
        sep_id: 3,
        pad_id: 0,
        lowercase: true,
        max_word_chars: 6,
    };

    let out_arr: [16]u32 = undefined;
    let work_arr: [64]u8 = undefined;
    let out: []u32 = out_arr[0..];
    let work: []u8 = work_arr[0..];

    // Partial prefix matches ("un"), but suffix ("xyz") is OOV -> entire word collapses to single [UNK]
    if (n := wordPieceEncodeWord(vocab, "unxyz", cfg, out, work)) {
        expect_eq(n, 1);
        expect_eq(out[0], 1);
    } else {
        expect_eq(true, false);
    }

    // Overlong word (> max_word_chars=6): "unaffable" (9 chars) -> single [UNK]
    if (n := wordPieceEncodeWord(vocab, "unaffable", cfg, out, work)) {
        expect_eq(n, 1);
        expect_eq(out[0], 1);
    } else {
        expect_eq(true, false);
    }

    // Text mixing valid word, OOV word, and overlong word
    if (n := wordPieceEncodeText(vocab, "ok unxyz unaffable a", cfg, out, work)) {
        expect_eq(n, 4);
        expect_eq(out[0], 7); // "ok"
        expect_eq(out[1], 1); // [UNK]
        expect_eq(out[2], 1); // [UNK]
        expect_eq(out[3], 8); // "a"
    } else {
        expect_eq(true, false);
    }
}

fn testEncodePairPackingMasksTypesAndPadding()! {
    const vocab_tokens: [11]string = [
        "[PAD]", "[UNK]", "[CLS]", "[SEP]",
        "un", "##aff", "##able", "hello", "world", "!", "query",
    ];
    const vocab = WordPieceVocab.init(vocab_tokens[0..]);
    const cfg = WordPieceConfig{
        unk_id: 1,
        cls_id: 2,
        sep_id: 3,
        pad_id: 0,
        lowercase: true,
        max_word_chars: 100,
    };

    let ids_arr: [12]u32 = undefined;
    let mask_arr: [12]u8 = undefined;
    let types_arr: [12]u8 = undefined;
    let work_arr: [64]u8 = undefined;
    let ids: []u32 = ids_arr[0..];
    let mask: []u8 = mask_arr[0..];
    let types: []u8 = types_arr[0..];
    let work: []u8 = work_arr[0..];

    // Pair: A="query!" -> [10, 9], B="Unaffable" -> [4, 5, 6]
    // Sequence: [CLS](2) query(10) !(9) [SEP](3) un(4) ##aff(5) ##able(6) [SEP](3) [PAD](0) [PAD](0)
    if (st := wordPieceEncodePair(vocab, "query!", "Unaffable", cfg, 10, true, ids, mask, types, work)) {
        expect_eq(st.total_len, 10);
        expect_eq(st.real_tokens, 8);
        expect_eq(st.truncated, false);

        // input_ids
        expect_eq(ids[0], 2);  // [CLS]
        expect_eq(ids[1], 10); // query
        expect_eq(ids[2], 9);  // !
        expect_eq(ids[3], 3);  // [SEP]
        expect_eq(ids[4], 4);  // un
        expect_eq(ids[5], 5);  // ##aff
        expect_eq(ids[6], 6);  // ##able
        expect_eq(ids[7], 3);  // [SEP]
        expect_eq(ids[8], 0);  // [PAD]
        expect_eq(ids[9], 0);  // [PAD]

        // attention_mask: 1 for real tokens (0..8), 0 for PAD (8..10)
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            expect_eq(mask[i], 1);
        }
        expect_eq(mask[8], 0);
        expect_eq(mask[9], 0);

        // token_type_ids: 0 for [CLS]+A+[SEP] (0..4), 1 for B+[SEP] (4..8), 0 for PAD (8..10)
        expect_eq(types[0], 0);
        expect_eq(types[1], 0);
        expect_eq(types[2], 0);
        expect_eq(types[3], 0);
        expect_eq(types[4], 1);
        expect_eq(types[5], 1);
        expect_eq(types[6], 1);
        expect_eq(types[7], 1);
        expect_eq(types[8], 0);
        expect_eq(types[9], 0);
    } else {
        expect_eq(true, false);
    }

    // Single sentence mode (text_b == ""), pad_to_max = false
    if (st := wordPieceEncodePair(vocab, "Hello world", "", cfg, 10, false, ids, mask, types, work)) {
        expect_eq(st.total_len, 4);
        expect_eq(st.real_tokens, 4);
        expect_eq(st.truncated, false);
        expect_eq(ids[0], 2); // [CLS]
        expect_eq(ids[1], 7); // hello
        expect_eq(ids[2], 8); // world
        expect_eq(ids[3], 3); // [SEP]
        expect_eq(mask[3], 1);
        expect_eq(types[3], 0);
    } else {
        expect_eq(true, false);
    }
}

fn testTruncationPreservesClsAndSep()! {
    const vocab_tokens: [10]string = [
        "[PAD]", "[UNK]", "[CLS]", "[SEP]",
        "one", "two", "three", "four", "five", "six",
    ];
    const vocab = WordPieceVocab.init(vocab_tokens[0..]);
    const cfg = WordPieceConfig{
        unk_id: 1,
        cls_id: 2,
        sep_id: 3,
        pad_id: 0,
        lowercase: true,
        max_word_chars: 100,
    };

    let ids_arr: [8]u32 = undefined;
    let mask_arr: [8]u8 = undefined;
    let types_arr: [8]u8 = undefined;
    let work_arr: [64]u8 = undefined;
    let ids: []u32 = ids_arr[0..];
    let mask: []u8 = mask_arr[0..];
    let types: []u8 = types_arr[0..];
    let work: []u8 = work_arr[0..];

    // Pair with A=4 tokens, B=2 tokens, max_seq_len=7 -> content_budget=4
    // longest_first trims A from 4 -> 2 while keeping B=2:
    // [CLS] one two [SEP] five six [SEP]
    if (st := wordPieceEncodePair(vocab, "one two three four", "five six", cfg, 7, true, ids, mask, types, work)) {
        expect_eq(st.total_len, 7);
        expect_eq(st.real_tokens, 7);
        expect_eq(st.truncated, true);
        expect_eq(ids[0], 2); // [CLS]
        expect_eq(ids[1], 4); // one
        expect_eq(ids[2], 5); // two
        expect_eq(ids[3], 3); // [SEP]
        expect_eq(ids[4], 8); // five
        expect_eq(ids[5], 9); // six
        expect_eq(ids[6], 3); // [SEP]
        expect_eq(types[3], 0);
        expect_eq(types[4], 1);
        expect_eq(types[6], 1);
    } else {
        expect_eq(true, false);
    }

    // Minimal pair max_seq_len=3 -> [CLS] [SEP] [SEP], truncated=true
    if (st := wordPieceEncodePair(vocab, "one two", "three", cfg, 3, false, ids, mask, types, work)) {
        expect_eq(st.total_len, 3);
        expect_eq(st.real_tokens, 3);
        expect_eq(st.truncated, true);
        expect_eq(ids[0], 2); // [CLS]
        expect_eq(ids[1], 3); // [SEP]
        expect_eq(ids[2], 3); // [SEP]
    } else {
        expect_eq(true, false);
    }
}

fn testDefensiveGuardsEmptyVocabEmptyTextAndBuffers()! {
    const empty_tokens: []const string = [];
    const empty_vocab = WordPieceVocab.init(empty_tokens);
    const default_cfg = WordPieceConfig.default();

    let ids_arr: [16]u32 = undefined;
    let mask_arr: [16]u8 = undefined;
    let types_arr: [16]u8 = undefined;
    let work_arr: [64]u8 = undefined;
    let ids: []u32 = ids_arr[0..];
    let mask: []u8 = mask_arr[0..];
    let types: []u8 = types_arr[0..];
    let work: []u8 = work_arr[0..];

    // 1. Empty vocab maps words to default unk_id (100)
    if (n := wordPieceEncodeText(empty_vocab, "alpha beta", default_cfg, ids, work)) {
        expect_eq(n, 2);
        expect_eq(ids[0], 100);
        expect_eq(ids[1], 100);
    } else {
        expect_eq(true, false);
    }

    // 2. Empty text produces 0 tokens in wordPieceEncodeText and [CLS] [SEP] in wordPieceEncodePair
    if (n0 := wordPieceEncodeText(empty_vocab, "", default_cfg, ids, work)) {
        expect_eq(n0, 0);
    } else {
        expect_eq(true, false);
    }
    if (st := wordPieceEncodePair(empty_vocab, "", "", default_cfg, 4, true, ids, mask, types, work)) {
        expect_eq(st.total_len, 4);
        expect_eq(st.real_tokens, 2);
        expect_eq(st.truncated, false);
        expect_eq(ids[0], 101); // default [CLS]
        expect_eq(ids[1], 102); // default [SEP]
        expect_eq(ids[2], 0);   // default [PAD]
        expect_eq(mask[1], 1);
        expect_eq(mask[2], 0);
    } else {
        expect_eq(true, false);
    }

    // 3. Invalid max_seq_len (<2 for single, <3 for pair) -> null
    expect_eq(wordPieceEncodePair(empty_vocab, "a", "", default_cfg, 1, false, ids, mask, types, work) == null, true);
    expect_eq(wordPieceEncodePair(empty_vocab, "a", "b", default_cfg, 2, false, ids, mask, types, work) == null, true);

    // 4. Insufficient output or work buffers -> null
    let tiny_ids_arr: [1]u32 = undefined;
    let tiny_ids: []u32 = tiny_ids_arr[0..];
    let tiny_work_arr: [2]u8 = undefined;
    let tiny_work: []u8 = tiny_work_arr[0..];
    expect_eq(wordPieceEncodeText(empty_vocab, "alpha beta", default_cfg, tiny_ids, work) == null, true);
    expect_eq(wordPieceEncodeWord(empty_vocab, "hello", default_cfg, ids, tiny_work) == null, true);
    expect_eq(wordPieceEncodePair(empty_vocab, "a", "b", default_cfg, 6, true, tiny_ids, mask, types, work) == null, true);

    // 5. Namespace access via `cap.text.*`
    const mini_tokens: [5]string = ["[PAD]", "[UNK]", "[CLS]", "[SEP]", "cp"];
    const mini_vocab = cap.text.WordPieceVocab.init(mini_tokens[0..]);
    const mini_cfg = cap.text.WordPieceConfig{
        unk_id: 1,
        cls_id: 2,
        sep_id: 3,
        pad_id: 0,
        lowercase: true,
        max_word_chars: 100,
    };
    if (st := cap.text.wordPieceEncodePair(mini_vocab, "CP", "", mini_cfg, 4, false, ids, mask, types, work)) {
        expect_eq(st.real_tokens, 3);
        expect_eq(ids[0], 2);
        expect_eq(ids[1], 4);
        expect_eq(ids[2], 3);
    } else {
        expect_eq(true, false);
    }
}

pub fn main()! {
    test("1. text/wordpiece: greedy longest-match ## subword splitting (unaffable)", () => {
        testGreedyLongestMatchUnaffable()?;
    });
    test("2. text/wordpiece: ASCII case-folding and independent punctuation splitting", () => {
        testCaseFoldingAndPunctuationSplitting()?;
    });
    test("3. text/wordpiece: OOV and overlong max_word_chars fallback to [UNK]", () => {
        testOovAndOverlongWordFallbackToUnk()?;
    });
    test("4. text/wordpiece: pair packing [CLS] A [SEP] B [SEP] with masks, types & padding", () => {
        testEncodePairPackingMasksTypesAndPadding()?;
    });
    test("5. text/wordpiece: max_seq_len longest_first truncation preserves [CLS] and [SEP]", () => {
        testTruncationPreservesClsAndSep()?;
    });
    test("6. text/wordpiece: defensive guards (empty vocab/text, small bufs) & cap.text", () => {
        testDefensiveGuardsEmptyVocabEmptyTextAndBuffers()?;
    });
}
