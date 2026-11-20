// Source: cp/cap/test/test_bencode_production_deep.cp
// Deep Production Test Suite for cap:bencode
// Covers:
//   1. Lossless encode/decode round-trip (ints, negative/0, binary with \x00, nested lists, unsorted input dict canonicalization)
//   2. Strict BEP-3 validation guards (i03e, i-0e, 02:ab, UnsortedDictKeys, DuplicateDictKey, TrailingBytes, MaxDepthExceeded)
//   3. TorrentParser.parseMetaInfo for single-file & multi-file .torrent (info_hash_hex, piece_length, pieces_count, total_size)
//   4. std.testing.allocator zero memory leak verification across success and error paths
//   5. Dynamic namespace mounting via `import cap from "cap"; cap.bencode`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { sha1Raw } from "cap:crypto";
import {
    BENCODE_MAX_DEPTH,
    BencodeError,
    BencodeKind,
    BencodeDictEntry,
    BencodeValue,
    BencodeDoc,
    BencodeCodec,
    TorrentMetaInfo,
    TorrentParser,
} from "cap:bencode";
import cap from "cap";

fn expectDecodeError(alloc: std.mem.Allocator, payload: []const u8, expected: BencodeError) {
    switch (BencodeCodec.decode(alloc, payload)) {
        doc => {
            let mut_doc = doc;
            mut_doc.deinit();
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == expected, true);
        },
    }
}

fn testRoundTripAndCanonicalEncoding()! {
    const alloc = std.testing.allocator;

    // 1. Integers: positive, zero, negative
    let d_pos = BencodeCodec.decode(alloc, "i42e")?;
    defer d_pos.deinit();
    expect_eq(d_pos.kind() == .Int, true);
    expect_eq(d_pos.asInt().?, 42 as i64);

    let d_zero = BencodeCodec.decode(alloc, "i0e")?;
    defer d_zero.deinit();
    expect_eq(d_zero.asInt().?, 0 as i64);

    let d_neg = BencodeCodec.decode(alloc, "i-9876543210e")?;
    defer d_neg.deinit();
    expect_eq(d_neg.asInt().?, -9876543210 as i64);

    // 2. Binary byte strings containing \x00
    const raw_bin = "5:a\x00b\xFFc";
    let d_bin = BencodeCodec.decode(alloc, raw_bin)?;
    defer d_bin.deinit();
    expect_eq(d_bin.kind() == .Bytes, true);
    expect_eq(std.mem.eql(u8, d_bin.asBytes().?, "a\x00b\xFFc"), true);

    // 3. Unsorted input dictionary -> canonical sorted encode -> decode round-trip
    let inner_list: [2]BencodeValue = [
        BencodeValue.makeInt(-7),
        BencodeValue.makeBytes("x\x00y"),
    ];
    let entries: [3]BencodeDictEntry = [
        BencodeDictEntry.init("zeta", BencodeValue.makeInt(100)),
        BencodeDictEntry.init("alpha", BencodeValue.makeList(&inner_list)),
        BencodeDictEntry.init("mid", BencodeValue.makeBytes("cp-bencode")),
    ];
    const root_val = BencodeValue.makeDict(&entries);
    const encoded = BencodeCodec.encode(alloc, root_val)?;
    defer alloc.free(encoded);

    const expected_canonical = "d5:alphali-7e3:x\x00ye3:mid10:cp-bencode4:zetai100ee";
    expect_eq(std.mem.eql(u8, encoded, expected_canonical), true);

    let decoded = BencodeCodec.decode(alloc, encoded)?;
    defer decoded.deinit();
    expect_eq(decoded.kind() == .Dict, true);
    expect_eq(decoded.get("zeta").?.asInt().?, 100 as i64);
    expect_eq(std.mem.eql(u8, decoded.get("mid").?.asBytes().?, "cp-bencode"), true);
    const alpha_list = decoded.get("alpha").?.asList().?;
    expect_eq(alpha_list.len, 2 as usize);
    expect_eq(alpha_list[0].asInt().?, -7 as i64);
    expect_eq(std.mem.eql(u8, alpha_list[1].asBytes().?, "x\x00y"), true);
}

fn testStrictBep3SecurityGuards()! {
    const alloc = std.testing.allocator;

    // Leading zeros in integer & string length
    expectDecodeError(alloc, "i03e", BencodeError.LeadingZeroNotAllowed);
    expectDecodeError(alloc, "i-03e", BencodeError.LeadingZeroNotAllowed);
    expectDecodeError(alloc, "02:ab", BencodeError.LeadingZeroNotAllowed);

    // Negative zero
    expectDecodeError(alloc, "i-0e", BencodeError.NegativeZeroNotAllowed);

    // Unsorted dictionary keys ("b" before "a")
    expectDecodeError(alloc, "d1:bi1e1:ai2ee", BencodeError.UnsortedDictKeys);

    // Duplicate dictionary key ("a" and "a")
    expectDecodeError(alloc, "d1:ai1e1:ai2ee", BencodeError.DuplicateDictKey);

    // Trailing bytes after valid top-level value
    expectDecodeError(alloc, "i42egarb", BencodeError.TrailingBytes);

    // Max nesting depth (64 allowed, 65th rejected)
    let ok_buf: [130]u8 = undefined;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        ok_buf[i] = 'l';
        ok_buf[64 + i] = 'e';
    }
    let ok_doc = BencodeCodec.decode(alloc, ok_buf[0..128])?;
    ok_doc.deinit();

    let deep_buf: [132]u8 = undefined;
    i = 0;
    while (i < 65) : (i += 1) {
        deep_buf[i] = 'l';
        deep_buf[65 + i] = 'e';
    }
    expectDecodeError(alloc, deep_buf[0..130], BencodeError.MaxDepthExceeded);
}

fn testTorrentMetaInfoSingleAndMultiFile()! {
    const alloc = std.testing.allocator;

    // 1. Single-file torrent
    const single_info = "d6:lengthi1048576e4:name9:ubuntu.cp12:piece lengthi262144e6:pieces40:01234567890123456789abcdefghij0123456789e";
    const single_torrent = "d8:announce27:http://tracker.cp.org:6969/4:info" ++ single_info ++ "e";

    let expected_hash: [20]u8 = [0; 20];
    sha1Raw(&expected_hash, single_info);
    const expected_hex = std.fmt.bytesToHex(expected_hash, .lower);

    let meta_single = TorrentParser.parseMetaInfo(alloc, single_torrent)?;
    defer meta_single.deinit();

    expect_eq(std.mem.eql(u8, &meta_single.info_hash, &expected_hash), true);
    expect_eq(std.mem.eql(u8, meta_single.infoHashHex(), expected_hex[0..40]), true);
    expect_eq(meta_single.infoHashHex().len, 40 as usize);
    expect_eq(std.mem.eql(u8, meta_single.announce, "http://tracker.cp.org:6969/"), true);
    expect_eq(std.mem.eql(u8, meta_single.name, "ubuntu.cp"), true);
    expect_eq(meta_single.piece_length, 262144 as u64);
    expect_eq(meta_single.pieces_count, 2 as usize);
    expect_eq(meta_single.total_size, 1048576 as u64);

    // 2. Multi-file torrent
    const multi_info = "d5:filesld6:lengthi1500e4:pathl6:a.bin0eed6:lengthi2500e4:pathl3:dir5:b.bineee4:name8:pkg_root12:piece lengthi16384e6:pieces20:12345678901234567890e";
    const multi_torrent = "d4:info" ++ multi_info ++ "e";

    let expected_multi_hash: [20]u8 = [0; 20];
    sha1Raw(&expected_multi_hash, multi_info);
    const expected_multi_hex = std.fmt.bytesToHex(expected_multi_hash, .lower);

    let meta_multi = TorrentParser.parseMetaInfo(alloc, multi_torrent)?;
    defer meta_multi.deinit();

    expect_eq(std.mem.eql(u8, meta_multi.infoHashHex(), expected_multi_hex[0..40]), true);
    expect_eq(std.mem.eql(u8, meta_multi.name, "pkg_root"), true);
    expect_eq(meta_multi.piece_length, 16384 as u64);
    expect_eq(meta_multi.pieces_count, 1 as usize);
    expect_eq(meta_multi.total_size, 4000 as u64);
    expect_eq(meta_multi.announce.len, 0 as usize);
}

fn testZeroMemoryLeakOnErrorPaths()! {
    const alloc = std.testing.allocator;

    // Decode fails mid-way after allocating nested keys/lists/bytes -> must free all cleanly
    expectDecodeError(alloc, "d3:fool3:bar3:baze3:aaa1:xe", BencodeError.UnsortedDictKeys);
    expectDecodeError(alloc, "l3:abc4:defg", BencodeError.UnexpectedEof);
    expectDecodeError(alloc, "d3:key5:valueeEXTRA", BencodeError.TrailingBytes);

    // Invalid torrent missing info dict
    switch (TorrentParser.parseMetaInfo(alloc, "d8:announce3:abce")) {
        m => {
            let mm = m;
            mm.deinit();
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == BencodeError.InvalidTorrent, true);
        },
    }
}

fn testCapDynamicNamespaceMount()! {
    const alloc = std.testing.allocator;
    let doc = cap.bencode.BencodeCodec.decode(alloc, "d3:cap7:bencode3:veri1ee")?;
    defer doc.deinit();
    expect_eq(std.mem.eql(u8, doc.get("cap").?.asBytes().?, "bencode"), true);
    expect_eq(doc.get("ver").?.asInt().?, 1 as i64);
    expect_eq(cap.bencode.BENCODE_MAX_DEPTH, 64 as usize);
}

test("bencode: lossless round-trip and canonical dictionary encoding", () => {
    testRoundTripAndCanonicalEncoding() catch unreachable;
});

test("bencode: strict BEP-3 security guards (leading zero, neg zero, unsorted/dup keys, trailing, max depth)", () => {
    testStrictBep3SecurityGuards() catch unreachable;
});

test("bencode: TorrentParser single-file & multi-file MetaInfo and SHA-1 info_hash", () => {
    testTorrentMetaInfoSingleAndMultiFile() catch unreachable;
});

test("bencode: std.testing.allocator zero memory leak across error paths", () => {
    testZeroMemoryLeakOnErrorPaths() catch unreachable;
});

test("bencode: dynamic cap.bencode namespace mounting", () => {
    testCapDynamicNamespaceMount() catch unreachable;
});
