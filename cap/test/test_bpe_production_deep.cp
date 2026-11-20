// Source: cp/cap/test/test_bpe_production_deep.cp
// Deep Production Verification Suite for cap:bpe & cap.bpe
// Covers:
//   1. 256-byte base vocabulary 100% lossless round-trip for UTF-8 (CN/EN/Emoji/Control/High bytes)
//   2. train(corpus, target_vocab_size) frequency learning, token compression & lossless decode
//   3. addSpecialToken protection against internal BPE merges & longest-match splitting
//   4. Manual addMerge, error interception (InvalidVocabSize/DuplicateSpecialToken/UnknownTokenId/InvalidMergeRule) & zero-leak
//   5. `import cap from "cap"; cap.bpe` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BpeError,
    MergePair,
    BpeTokenizer,
} from "cap:bpe";

test("cap:bpe - 256 base byte vocabulary lossless UTF-8 round-trip") {
    const alloc = std.testing.allocator;
    let tok = BpeTokenizer.init(alloc) catch unreachable;
    defer tok.deinit();

    expect_eq(tok.vocabSize(), 256);
    expect_eq(tok.tokenBytes(0).?.len, 1);
    expect_eq(tok.tokenBytes(0).?[0], 0);
    expect_eq(tok.tokenBytes(255).?[0], 255);
    expect_eq(tok.tokenBytes(256) == null, true);

    const samples = [
        "",
        "Hello, CP World!",
        "中文分词无损测试：人工智能与编译器架构",
        "Emoji 🚀🔥🧠✨ & CJK 日本語 한국어",
        "\x00\x01\t\n\r\x7f\x80\xffbinary-safe",
    ];

    for (s of samples) {
        const ids = tok.encode(alloc, s) catch unreachable;
        defer alloc.free(ids);
        expect_eq(ids.len, s.len);

        const decoded = tok.decode(alloc, ids) catch unreachable;
        defer alloc.free(decoded);
        expect_eq(std.mem.eql(u8, decoded, s), true);
        expect_eq(tok.countTokens(alloc, s) catch unreachable, s.len);
    }
}

test("cap:bpe - train learns frequent merges, reduces token count & decodes losslessly") {
    const alloc = std.testing.allocator;
    let tok = BpeTokenizer.init(alloc) catch unreachable;
    defer tok.deinit();

    const corpus = "abababab cdcdcdcd abababab efefef abababab cdcdcdcd 机器学习 机器学习 机器学习!";
    tok.train(corpus, 276) catch unreachable;

    expect_eq(tok.vocabSize() > 256, true);
    expect_eq(tok.vocabSize() <= 276, true);

    const encoded = tok.encode(alloc, corpus) catch unreachable;
    defer alloc.free(encoded);

    // Trained BPE must compress repeated patterns significantly below raw byte length.
    expect_eq(encoded.len < corpus.len / 2, true);
    expect_eq(tok.countTokens(alloc, corpus) catch unreachable, encoded.len);

    const decoded = tok.decode(alloc, encoded) catch unreachable;
    defer alloc.free(decoded);
    expect_eq(std.mem.eql(u8, decoded, corpus), true);

    // Unseen mixed text must still round-trip 100% losslessly.
    const unseen = "ab cd ef 机器学习 🚀 unknown_xyz_999";
    const unseen_ids = tok.encode(alloc, unseen) catch unreachable;
    defer alloc.free(unseen_ids);
    const unseen_dec = tok.decode(alloc, unseen_ids) catch unreachable;
    defer alloc.free(unseen_dec);
    expect_eq(std.mem.eql(u8, unseen_dec, unseen), true);
}

test("cap:bpe - addSpecialToken protects special markers from internal BPE splits") {
    const alloc = std.testing.allocator;
    let tok = BpeTokenizer.init(alloc) catch unreachable;
    defer tok.deinit();

    const id_im_start = tok.addSpecialToken("<|im_start|>") catch unreachable;
    const id_im_end = tok.addSpecialToken("<|im_end|>") catch unreachable;
    const id_eot = tok.addSpecialToken("<|endoftext|>") catch unreachable;
    const id_eot_long = tok.addSpecialToken("<|endoftext|><|pad|>") catch unreachable;

    expect_eq(id_im_start, 256);
    expect_eq(id_im_end, 257);
    expect_eq(id_eot, 258);
    expect_eq(id_eot_long, 259);

    // Even if we train heavily on substrings of the special tokens, special tokens must stay atomic.
    const corpus = "im_start im_start im_start im_end im_end endoftext endoftext <|im_start|>hi<|im_end|>";
    tok.train(corpus, 280) catch unreachable;

    const prompt = "<|im_start|>im_start<|im_end|><|endoftext|><|endoftext|><|pad|>";
    const ids = tok.encode(alloc, prompt) catch unreachable;
    defer alloc.free(ids);

    // First token must be id_im_start, last two must be id_eot and id_eot_long (longest match).
    expect_eq(ids[0], id_im_start);
    expect_eq(ids[ids.len - 2], id_eot);
    expect_eq(ids[ids.len - 1], id_eot_long);

    // Verify id_im_end appears intact in the middle.
    let found_im_end = false;
    for (tid of ids) {
        if (tid == id_im_end) {
            found_im_end = true;
        }
    }
    expect_eq(found_im_end, true);

    const decoded = tok.decode(alloc, ids) catch unreachable;
    defer alloc.free(decoded);
    expect_eq(std.mem.eql(u8, decoded, prompt), true);
}

test("cap:bpe - manual addMerge, rank priority, error interception & zero-leak") {
    const alloc = std.testing.allocator;
    let tok = BpeTokenizer.init(alloc) catch unreachable;
    defer tok.deinit();

    // Error: InvalidVocabSize (< 256)
    let got_invalid_vocab = false;
    tok.train("aaaa", 255) catch e => {
        if (e == BpeError.InvalidVocabSize) got_invalid_vocab = true;
    };
    expect_eq(got_invalid_vocab, true);

    // Error: DuplicateSpecialToken
    const sp_id = tok.addSpecialToken("<|bos|>") catch unreachable;
    let got_dup_sp = false;
    * = tok.addSpecialToken("<|bos|>") catch e => {
        if (e == BpeError.DuplicateSpecialToken) got_dup_sp = true;
    };
    expect_eq(got_dup_sp, true);

    // Error: InvalidMergeRule (out-of-bounds or special token)
    let got_invalid_merge_oob = false;
    * = tok.addMerge(999, 1) catch e => {
        if (e == BpeError.InvalidMergeRule) got_invalid_merge_oob = true;
    };
    expect_eq(got_invalid_merge_oob, true);

    let got_invalid_merge_sp = false;
    * = tok.addMerge(sp_id, u32('a')) catch e => {
        if (e == BpeError.InvalidMergeRule) got_invalid_merge_sp = true;
    };
    expect_eq(got_invalid_merge_sp, true);

    // Manual merges: rank 0 = ('a', 'b'), rank 1 = ('b', 'c')
    const id_ab = tok.addMerge(u32('a'), u32('b')) catch unreachable;
    const id_bc = tok.addMerge(u32('b'), u32('c')) catch unreachable;
    const id_abc = tok.addMerge(id_ab, u32('c')) catch unreachable;
    * = id_bc;

    // On "abc": ('a','b') has rank 0 < ('b','c') rank 1, so 'ab' merges first, then ('ab','c') -> id_abc!
    const ids = tok.encode(alloc, "abc") catch unreachable;
    defer alloc.free(ids);
    expect_eq(ids.len, 1);
    expect_eq(ids[0], id_abc);

    const decoded = tok.decode(alloc, ids) catch unreachable;
    defer alloc.free(decoded);
    expect_eq(std.mem.eql(u8, decoded, "abc"), true);

    // Error: UnknownTokenId in decode
    const bad_ids = [u32(0), u32(9999)];
    let got_unknown_id = false;
    switch (tok.decode(alloc, bad_ids)) {
        ok_buf => alloc.free(ok_buf),
        err(e) => {
            if (e == BpeError.UnknownTokenId) got_unknown_id = true;
        },
    }
    expect_eq(got_unknown_id, true);
}

test("cap:bpe - dynamic mounting via cap.bpe") {
    const alloc = std.testing.allocator;
    let tok = cap.bpe.BpeTokenizer.init(alloc) catch unreachable;
    defer tok.deinit();

    const pair = MergePair{
        left: u32('x'),
        right: u32('y'),
        new_id: 256,
        rank: 0,
    };
    expect_eq(pair.new_id, 256);

    tok.train("xyxyxyxy", 258) catch unreachable;
    const ids = tok.encode(alloc, "xyxy") catch unreachable;
    defer alloc.free(ids);
    expect_eq(ids.len < 4, true);

    const decoded_xy = tok.decode(alloc, ids) catch unreachable;
    defer alloc.free(decoded_xy);
    expect_eq(std.mem.eql(u8, decoded_xy, "xyxy"), true);
}
