// Source: cp/cap/test/test_bpe_production_deep.cp
const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import { BpeError, BpeVocab, BpeTokenizer } from "cap:bpe";

test("BPE manual merge priority and lossless roundtrip") {
    const alloc = std.testing.allocator;
    let tok = BpeTokenizer.init(alloc);
    defer tok.deinit();

    try tok.vocab.addByteTokens();
    // rank 0: "a" + "b" -> "ab" (id 256)
    const ab_id = try tok.vocab.addMerge("a", "b");
    // rank 1: "ab" + "c" -> "abc" (id 257)
    const abc_id = try tok.vocab.addMerge("ab", "c");
    // rank 2: "b" + "c" -> "bc" (id 258)
    * = try tok.vocab.addMerge("b", "c");

    // Because "a"+"b" has rank 0 (< rank 2 of "b"+"c"), "abc" must merge as ("a"+"b")+"c" -> [abc_id]
    const ids = try tok.encode(alloc, "abcab");
    defer alloc.free(ids);

    expect_eq(ids.len, @as(usize, 2));
    expect_eq(ids[0], abc_id);
    expect_eq(ids[1], ab_id);

    const decoded = try tok.decode(alloc, ids);
    defer alloc.free(decoded);
    try std.testing.expectEqualStrings("abcab", decoded);
}

test("BPE special_tokens splitting vs encodeOrdinary injection guard") {
    const alloc = std.testing.allocator;
    let tok = BpeTokenizer.init(alloc);
    defer tok.deinit();

    try tok.vocab.addByteTokens();
    try tok.vocab.addSpecialToken("<|endoftext|>", 50256);
    try tok.vocab.addSpecialToken("<|im_start|>", 50257);

    const prompt = "<|im_start|>hi<|endoftext|>";
    const ids_special = try tok.encode(alloc, prompt);
    defer alloc.free(ids_special);

    // <|im_start|> (1) + 'h' (1) + 'i' (1) + <|endoftext|> (1) = 4 tokens
    expect_eq(ids_special.len, @as(usize, 4));
    expect_eq(ids_special[0], @as(u32, 50257));
    expect_eq(ids_special[3], @as(u32, 50256));

    const decoded_special = try tok.decode(alloc, ids_special);
    defer alloc.free(decoded_special);
    try std.testing.expectEqualStrings(prompt, decoded_special);

    // encodeOrdinary must NOT treat <|im_start|> or <|endoftext|> as special tokens
    const ids_ordinary = try tok.encodeOrdinary(alloc, prompt);
    defer alloc.free(ids_ordinary);
    expect_eq(ids_ordinary.len, prompt.len);
}

test("BPE loadTiktokenText base64 parsing and error guards") {
    const alloc = std.testing.allocator;

    // Base64: "YQ==" -> "a", "Yg==" -> "b", "YWI=" -> "ab"
    const tiktoken_data = "YQ== 0\nYg== 1\nYWI= 2\n";
    let tok = try BpeTokenizer.loadTiktoken(alloc, tiktoken_data);
    defer tok.deinit();

    const ids = try tok.encode(alloc, "aba");
    defer alloc.free(ids);
    expect_eq(ids.len, @as(usize, 2));
    expect_eq(ids[0], @as(u32, 2));
    expect_eq(ids[1], @as(u32, 0));

    // Unknown token id check
    let bad_ids = try alloc.alloc(u32, 2);
    defer alloc.free(bad_ids);
    bad_ids[0] = 0;
    bad_ids[1] = 99999;
    try std.testing.expectError(BpeError.UnknownTokenId, tok.decode(alloc, bad_ids));

    // Invalid format & invalid base64 checks
    try std.testing.expectError(BpeError.InvalidFormat, BpeTokenizer.loadTiktoken(alloc, "bad_line_without_rank\n"));
    try std.testing.expectError(BpeError.InvalidBase64, BpeTokenizer.loadTiktoken(alloc, "!!!not_base64!!! 0\n"));
}

test("BPE trainFromCorpus compression and UTF-8 safe truncateToMaxTokens") {
    const alloc = std.testing.allocator;
    const corpus = "ababab cdcdcd ababab cdcdcd ababab";
    let tok = try BpeTokenizer.trainFromCorpus(alloc, corpus, 260);
    defer tok.deinit();

    const full_count = try tok.countTokens(alloc, corpus);
    try std.testing.expect(full_count < corpus.len);

    // Multi-byte UTF-8 test: "a中b" -> 'a' (1 byte), '中' (3 bytes), 'b' (1 byte) = 5 byte tokens
    // Truncating to 2 tokens cuts inside '中' (first byte of 3-byte UTF-8), must roll back to "a"
    const utf8_text = "a中b";
    const truncated = try tok.truncateToMaxTokens(alloc, utf8_text, 2);
    defer alloc.free(truncated);
    try std.testing.expectEqualStrings("a", truncated);
    try std.testing.expect(std.unicode.utf8ValidateSlice(truncated));

    // Truncating to 4 tokens keeps complete "a中"
    const truncated4 = try tok.truncateToMaxTokens(alloc, utf8_text, 4);
    defer alloc.free(truncated4);
    try std.testing.expectEqualStrings("a中", truncated4);
    try std.testing.expect(std.unicode.utf8ValidateSlice(truncated4));
}

test("BPE cap.bpe dynamic mount and zero memory leak") {
    const alloc = std.testing.allocator;
    let tok = cap.bpe.BpeTokenizer.init(alloc);
    defer tok.deinit();

    try tok.vocab.addByteTokens();
    * = try tok.vocab.addMerge("c", "p");

    const ids = try tok.encode(alloc, "cp!");
    defer alloc.free(ids);
    expect_eq(ids.len, @as(usize, 2));

    const out = try tok.decode(alloc, ids);
    defer alloc.free(out);
    try std.testing.expectEqualStrings("cp!", out);
}
