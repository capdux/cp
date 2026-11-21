// Source: cp/cap/test/test_rlp_production_deep.cp (Production deep tests for cap:rlp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RlpKind,
    RlpItem,
    RlpListIter,
    RlpWriter,
    decodeRlpItem,
    decodeRlpU64,
    encodedBytesLen,
    encodedU64Len,
    encodedListHeaderLen,
} from "cap:rlp";

test("1. Ethereum official vectors: dog, [cat, dog], empty string, empty list, and canonical integers", () => {
    let buf: [64]u8 = [0; 64];

    // "dog" => [0x83, 'd', 'o', 'g']
    let w1 = RlpWriter.init(buf[0..64]);
    expect_eq(w1.writeBytes("dog"), true);
    const enc_dog = w1.written();
    const exp_dog: [4]u8 = [0x83, 'd', 'o', 'g'];
    expect_eq(std.mem.eql(u8, enc_dog, exp_dog[0..4]), true);
    const dec_dog = decodeRlpItem(enc_dog) ?? unreachable;
    expect_eq(dec_dog.kind == RlpKind.bytes, true);
    expect_eq(std.mem.eql(u8, dec_dog.payload, "dog"), true);
    expect_eq(dec_dog.total_bytes, 4);

    // ["cat", "dog"] => [0xC8, 0x83, 'c', 'a', 't', 0x83, 'd', 'o', 'g']
    let w2 = RlpWriter.init(buf[0..64]);
    const list_body = encodedBytesLen("cat") + encodedBytesLen("dog");
    expect_eq(list_body, 8);
    expect_eq(w2.writeListHeader(list_body), true);
    expect_eq(w2.writeBytes("cat"), true);
    expect_eq(w2.writeBytes("dog"), true);
    const enc_list = w2.written();
    const exp_list: [9]u8 = [0xC8, 0x83, 'c', 'a', 't', 0x83, 'd', 'o', 'g'];
    expect_eq(std.mem.eql(u8, enc_list, exp_list[0..9]), true);

    const dec_list = decodeRlpItem(enc_list) ?? unreachable;
    expect_eq(dec_list.kind == RlpKind.list, true);
    let it = RlpListIter.init(dec_list.payload);
    const e0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e0.payload, "cat"), true);
    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.payload, "dog"), true);
    expect_eq(it.next() == null, true);
    expect_eq(it.isDone(), true);

    // "" => [0x80], [] => [0xC0]
    let w3 = RlpWriter.init(buf[0..64]);
    expect_eq(w3.writeBytes(""), true);
    expect_eq(w3.written().len, 1);
    expect_eq(w3.written()[0], 0x80);

    let w4 = RlpWriter.init(buf[0..64]);
    expect_eq(w4.writeListHeader(0), true);
    expect_eq(w4.written().len, 1);
    expect_eq(w4.written()[0], 0xC0);

    // Integers: 0 => [0x80], 15 => [0x0F], 1024 => [0x82, 0x04, 0x00]
    let w5 = RlpWriter.init(buf[0..64]);
    expect_eq(w5.writeU64(0), true);
    expect_eq(w5.written()[0], 0x80);
    expect_eq(decodeRlpU64(w5.written()) ?? 999, 0);

    let w6 = RlpWriter.init(buf[0..64]);
    expect_eq(w6.writeU64(15), true);
    expect_eq(w6.written().len, 1);
    expect_eq(w6.written()[0], 0x0F);
    expect_eq(decodeRlpU64(w6.written()) ?? 0, 15);

    let w7 = RlpWriter.init(buf[0..64]);
    expect_eq(w7.writeU64(1024), true);
    const exp_1024: [3]u8 = [0x82, 0x04, 0x00];
    expect_eq(std.mem.eql(u8, w7.written(), exp_1024[0..3]), true);
    expect_eq(decodeRlpU64(w7.written()) ?? 0, 1024);
});

test("2. long byte strings (56+ bytes) and long lists round-trip accurately", () => {
    const long_str = "Lorem ipsum dolor sit amet, consectetur adipisicing elit"; // 56 bytes
    expect_eq(long_str.len, 56);
    expect_eq(encodedBytesLen(long_str), 58); // 0xB8, 0x38, <56 bytes>

    let buf: [128]u8 = [0; 128];
    let w = RlpWriter.init(buf[0..128]);
    expect_eq(w.writeListHeader(58), true);
    expect_eq(encodedListHeaderLen(58), 2); // 0xF8, 0x3A
    expect_eq(w.writeBytes(long_str), true);

    const encoded = w.written();
    expect_eq(encoded.len, 60);
    expect_eq(encoded[0], 0xF8);
    expect_eq(encoded[1], 58);
    expect_eq(encoded[2], 0xB8);
    expect_eq(encoded[3], 56);

    const outer = decodeRlpItem(encoded) ?? unreachable;
    expect_eq(outer.kind == RlpKind.list, true);
    expect_eq(outer.total_bytes, 60);

    let it = RlpListIter.init(outer.payload);
    const inner = it.next() ?? unreachable;
    expect_eq(inner.kind == RlpKind.bytes, true);
    expect_eq(std.mem.eql(u8, inner.payload, long_str), true);
    expect_eq(it.next() == null, true);
    expect_eq(it.isDone(), true);
});

test("3. nested set-theoretic list [[], [[]], [[], [[]]]] decodes recursively via RlpListIter", () => {
    // Official Yellow Paper vector: [ 0xC7, 0xC0, 0xC1, 0xC0, 0xC3, 0xC0, 0xC1, 0xC0 ]
    const vec: [8]u8 = [0xC7, 0xC0, 0xC1, 0xC0, 0xC3, 0xC0, 0xC1, 0xC0];
    const root = decodeRlpItem(vec[0..8]) ?? unreachable;
    expect_eq(root.kind == RlpKind.list, true);
    expect_eq(root.total_bytes, 8);

    let it = RlpListIter.init(root.payload);

    // 0: []
    const i0 = it.next() ?? unreachable;
    expect_eq(i0.kind == RlpKind.list, true);
    expect_eq(i0.payload.len, 0);

    // 1: [[]]
    const i1 = it.next() ?? unreachable;
    expect_eq(i1.kind == RlpKind.list, true);
    let it1 = RlpListIter.init(i1.payload);
    const i1_0 = it1.next() ?? unreachable;
    expect_eq(i1_0.kind == RlpKind.list, true);
    expect_eq(i1_0.payload.len, 0);
    expect_eq(it1.isDone(), true);

    // 2: [[], [[]]]
    const i2 = it.next() ?? unreachable;
    expect_eq(i2.kind == RlpKind.list, true);
    let it2 = RlpListIter.init(i2.payload);
    const i2_0 = it2.next() ?? unreachable;
    expect_eq(i2_0.payload.len, 0);
    const i2_1 = it2.next() ?? unreachable;
    expect_eq(i2_1.payload.len, 1);
    expect_eq(it2.isDone(), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.isDone(), true);
});

test("4. strict canonical validation rejects all non-canonical encodings", () => {
    // Non-canonical single byte < 0x80 with 0x81 prefix: [0x81, 0x05] and [0x81, 0x7F]
    const nc_single1: [2]u8 = [0x81, 0x05];
    const nc_single2: [2]u8 = [0x81, 0x7F];
    expect_eq(decodeRlpItem(nc_single1[0..2]) == null, true);
    expect_eq(decodeRlpItem(nc_single2[0..2]) == null, true);

    // Valid single byte >= 0x80 with 0x81 prefix: [0x81, 0x80]
    const ok_single: [2]u8 = [0x81, 0x80];
    const ok_dec = decodeRlpItem(ok_single[0..2]) ?? unreachable;
    expect_eq(ok_dec.payload.len, 1);
    expect_eq(ok_dec.payload[0], 0x80);

    // Leading zero in long string length: [0xB8, 0x00] and [0xB9, 0x00, 0x38, ...]
    const nc_lz_str: [2]u8 = [0xB8, 0x00];
    expect_eq(decodeRlpItem(nc_lz_str[0..2]) == null, true);

    // Long string prefix used for len < 56: [0xB8, 0x37, <55 bytes>]
    let nc_short_long: [57]u8 = [0x41; 57];
    nc_short_long[0] = 0xB8;
    nc_short_long[1] = 55;
    expect_eq(decodeRlpItem(nc_short_long[0..57]) == null, true);

    // Long list prefix used for len < 56 and leading zero length
    let nc_list: [57]u8 = [0x01; 57];
    nc_list[0] = 0xF8;
    nc_list[1] = 55;
    expect_eq(decodeRlpItem(nc_list[0..57]) == null, true);
    nc_list[1] = 0x00;
    expect_eq(decodeRlpItem(nc_list[0..57]) == null, true);

    // Non-canonical integer encodings: 0x00 (leading zero for 0), [0x82, 0x00, 0x01], >8 bytes, list
    const int_zero_bad: [1]u8 = [0x00];
    const int_lz_bad: [3]u8 = [0x82, 0x00, 0x01];
    const int_9bytes: [10]u8 = [0x89, 1, 2, 3, 4, 5, 6, 7, 8, 9];
    const empty_list: [1]u8 = [0xC0];
    expect_eq(decodeRlpU64(int_zero_bad[0..1]) == null, true);
    expect_eq(decodeRlpU64(int_lz_bad[0..3]) == null, true);
    expect_eq(decodeRlpU64(int_9bytes[0..10]) == null, true);
    expect_eq(decodeRlpU64(empty_list[0..1]) == null, true);
});

test("5. truncated inputs, buffer exhaustion in RlpWriter, and max u64 boundary behave safely", () => {
    const empty: [0]u8 = [];
    expect_eq(decodeRlpItem(empty[0..0]) == null, true);
    expect_eq(decodeRlpU64(empty[0..0]) == null, true);

    // Truncated short string (claims 3 bytes, only 2 present)
    const trunc_str: [3]u8 = [0x83, 'a', 'b'];
    expect_eq(decodeRlpItem(trunc_str[0..3]) == null, true);

    // Truncated long prefix (claims 2-byte length, only 1 byte present)
    const trunc_hdr: [2]u8 = [0xB9, 0x01];
    expect_eq(decodeRlpItem(trunc_hdr[0..2]) == null, true);

    // RlpWriter atomic failure on tiny buffer
    let tiny: [2]u8 = [0; 2];
    let wt = RlpWriter.init(tiny[0..2]);
    expect_eq(wt.writeBytes("dog"), false); // needs 4 bytes
    expect_eq(wt.written().len, 0);

    // Max u64 round-trip (0xFFFFFFFFFFFFFFFF => 9 bytes: 0x88 + 8x 0xFF)
    let u64_buf: [16]u8 = [0; 16];
    let wu = RlpWriter.init(u64_buf[0..16]);
    const max_val: u64 = 0xFFFFFFFFFFFFFFFF;
    expect_eq(encodedU64Len(max_val), 9);
    expect_eq(wu.writeU64(max_val), true);
    expect_eq(wu.written().len, 9);
    expect_eq(wu.written()[0], 0x88);
    expect_eq(decodeRlpU64(wu.written()) ?? 0, max_val);
});
