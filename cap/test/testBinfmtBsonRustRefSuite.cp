import { test, expect_eq } from "cap:test";
import {
    BSON_MAX_DEPTH,
    BSON_TYPE_DOUBLE,
    BSON_TYPE_UTF8,
    BSON_TYPE_BINARY,
    BSON_TYPE_BOOLEAN,
    BSON_TYPE_REGEX,
    BSON_TYPE_INT32,
    BSON_TYPE_DECIMAL128,
    BsonError,
    ObjectId,
    BsonBinary,
    BsonRegex,
    BsonTimestamp,
    BsonField,
    BsonValue,
    findCStringNulScalar,
    findCStringNulSimd,
    isBsonAsciiScalar,
    isBsonAsciiSimd,
    copyBsonBytesScalar,
    copyBsonBytesSimd,
    deinitDocument,
    getField,
    eqlFields,
    encodeDocument,
    decodeDocument,
    decodeDocumentWithMaxDepth,
    validateBsonDocumentWithMaxDepth,
    validateBsonDocument,
    findBsonField,
} from "../../lib/binfmt/bson.cp";

const std = @import("std");

fn tryDecodeDoc(alloc: std.mem.Allocator, slice: []const u8): ([]BsonField)? {
    return decodeDocument(alloc, slice) catch return null;
}

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn runTestBsonAllTypesAndLookup(): void! {
    const alloc = std.heap.page_allocator;
    const oid = ObjectId.fromHex("507f1f77bcf86cd799439011") catch unreachable;
    const oid_hex = oid.toHex(alloc) catch unreachable;
    defer alloc.free(oid_hex);
    expect_true(bytesEqual(oid_hex, "507f1f77bcf86cd799439011"))?;

    let sub_fields: [1]BsonField = [
        BsonField.init("nested", BsonValue{ int32: 42 }),
    ];
    let arr_items: [2]BsonValue = [
        BsonValue{ utf8: "item0" },
        BsonValue{ boolean: true },
    ];
    // Valid Binary Subtype 0x02: 4-byte LE inner_len (= 3) + 3 bytes payload
    const bin02_payload: [7]u8 = [0x03, 0x00, 0x00, 0x00, 0xAA, 0xBB, 0xCC];
    const dec128_bytes: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];

    let fields: [15]BsonField = [
        BsonField.init("_id", BsonValue{ object_id: oid }),
        BsonField.init("dbl", BsonValue{ double: 3.14159 }),
        BsonField.init("str", BsonValue{ utf8: "Hello BSON 1.1 \xe4\xb8\xad\xe6\x96\x87!" }),
        BsonField.init("doc", BsonValue{ document: sub_fields[0..] }),
        BsonField.init("arr", BsonValue{ array: arr_items[0..] }),
        BsonField.init("bin", BsonValue{ binary: BsonBinary{ subtype: 0x02, data: bin02_payload[0..] } }),
        BsonField.init("flag", BsonValue{ boolean: false }),
        BsonField.init("dt", BsonValue{ utc_datetime: 1700000000000 }),
        BsonField.init("nul", BsonValue.null_val),
        BsonField.init("rx", BsonValue{ regex: BsonRegex{ pattern: "^abc.*$", options: "im" } }),
        BsonField.init("i32", BsonValue{ int32: -123456 }),
        BsonField.init("ts", BsonValue{ timestamp: BsonTimestamp{ increment: 7, timestamp: 1700000000 } }),
        BsonField.init("i64", BsonValue{ int64: 9000000000000 }),
        BsonField.init("d128", BsonValue{ decimal128: dec128_bytes }),
        BsonField.init("mink", BsonValue.min_key),
    ];

    const encoded = encodeDocument(alloc, fields[0..]) catch unreachable;
    defer alloc.free(encoded);

    validateBsonDocument(encoded) catch unreachable;

    // Zero-heap field lookup
    const f_i32 = findBsonField(encoded, "i32").?;
    expect_eq(f_i32.elem_type, BSON_TYPE_INT32);
    expect_eq(f_i32.value_bytes.len, 4);

    const f_rx = findBsonField(encoded, "rx").?;
    expect_eq(f_rx.elem_type, BSON_TYPE_REGEX);

    const f_dec = findBsonField(encoded, "d128").?;
    expect_eq(f_dec.elem_type, BSON_TYPE_DECIMAL128);
    expect_eq(f_dec.value_bytes.len, 16);
    expect_true(findBsonField(encoded, "missing") == null)?;

    // Full roundtrip decode
    const decoded = decodeDocument(alloc, encoded) catch unreachable;
    defer deinitDocument(alloc, decoded);
    expect_true(eqlFields(fields[0..], decoded))?;
}

fn runTestSecurityBoundaries(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Negative / <5 doc_size & missing 0x00 terminator
    const neg_doc: [5]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0x00];
    let got_neg = false;
    * = validateBsonDocument(neg_doc[0..]) catch |e| {
        if (e == BsonError.InvalidDocumentSize) got_neg = true;
    };
    expect_true(got_neg)?;

    const bad_term: [5]u8 = [0x05, 0x00, 0x00, 0x00, 0x01];
    let got_term = false;
    * = validateBsonDocument(bad_term[0..]) catch |e| {
        if (e == BsonError.MissingTerminator) got_term = true;
    };
    expect_true(got_term)?;

    // 2. Boolean != 0x00 / 0x01 rejected
    // doc_size=9: [9,0,0,0] + [0x08] + ['b',0] + [0x02] + [0x00]
    const bad_bool: [9]u8 = [0x09, 0x00, 0x00, 0x00, BSON_TYPE_BOOLEAN, 'b', 0x00, 0x02, 0x00];
    let got_bool = false;
    * = validateBsonDocument(bad_bool[0..]) catch |e| {
        if (e == BsonError.InvalidBoolean) got_bool = true;
    };
    expect_true(got_bool)?;

    // 3. String missing NUL & invalid UTF-8 rejected
    // doc_size=14: [14,0,0,0] + [0x02] + ['s',0] + [2,0,0,0] + ['a','b'] + [0]
    const bad_str_nul: [14]u8 = [0x0E, 0x00, 0x00, 0x00, BSON_TYPE_UTF8, 's', 0x00, 0x02, 0x00, 0x00, 0x00, 'a', 'b', 0x00];
    let got_str_nul = false;
    * = validateBsonDocument(bad_str_nul[0..]) catch |e| {
        if (e == BsonError.MissingTerminator) got_str_nul = true;
    };
    expect_true(got_str_nul)?;

    const bad_utf8: [14]u8 = [0x0E, 0x00, 0x00, 0x00, BSON_TYPE_UTF8, 's', 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00];
    let got_utf8 = false;
    * = validateBsonDocument(bad_utf8[0..]) catch |e| {
        if (e == BsonError.InvalidUtf8) got_utf8 = true;
    };
    expect_true(got_utf8)?;

    // 4. Binary Subtype 0x02 mismatched inner length rejected
    // doc_size=17: [17,0,0,0] + [0x05] + ['b',0] + [5,0,0,0] + [0x02] + [99,0,0,0, 0xAA] + [0]
    const bad_bin02: [18]u8 = [
        0x12, 0x00, 0x00, 0x00,
        BSON_TYPE_BINARY, 'b', 0x00,
        0x05, 0x00, 0x00, 0x00,
        0x02,
        0x63, 0x00, 0x00, 0x00, 0xAA,
        0x00,
    ];
    let got_bin02 = false;
    * = validateBsonDocument(bad_bin02[0..]) catch |e| {
        if (e == BsonError.InvalidDocumentSize) got_bin02 = true;
    };
    expect_true(got_bin02)?;

    // 5. MaxDepthExceeded (> BSON_MAX_DEPTH): build 66-level nested doc directly
    let cur: []u8 = alloc.dupe(u8, &[0x05, 0x00, 0x00, 0x00, 0x00]) catch unreachable;
    let d: usize = 0;
    while (d < 66) : (d += 1) {
        const next_len = 8 + cur.len;
        let next_buf = alloc.alloc(u8, next_len) catch unreachable;
        next_buf[0] = (next_len & 0xFF) as u8;
        next_buf[1] = ((next_len >> 8) & 0xFF) as u8;
        next_buf[2] = ((next_len >> 16) & 0xFF) as u8;
        next_buf[3] = ((next_len >> 24) & 0xFF) as u8;
        next_buf[4] = 0x03; // BSON_TYPE_DOCUMENT
        next_buf[5] = 'n';
        next_buf[6] = 0x00;
        copyBsonBytesSimd(next_buf[7 .. 7 + cur.len], cur);
        next_buf[next_len - 1] = 0x00;
        alloc.free(cur);
        cur = next_buf;
    }
    defer alloc.free(cur);

    let got_depth_val = false;
    * = validateBsonDocument(cur) catch |e| {
        if (e == BsonError.MaxDepthExceeded) got_depth_val = true;
    };
    expect_true(got_depth_val)?;

    let got_depth_dec = false;
    * = decodeDocumentWithMaxDepth(alloc, cur, BSON_MAX_DEPTH) catch |e| {
        if (e == BsonError.MaxDepthExceeded) got_depth_dec = true;
    };
    expect_true(got_depth_dec)?;
}

fn runTestSimdParity(): void! {
    let buf: [129]u8 = [0; 129];
    let i: usize = 0;
    while (i < 129) : (i += 1) {
        buf[i] = ('a' + ((i % 26) as u8)) as u8;
    }

    // 3a. findCStringNulSimd vs Scalar
    expect_eq(findCStringNulSimd(buf[0..]) == null, findCStringNulScalar(buf[0..]) == null);
    const nul_positions: [5]usize = [0, 15, 16, 17, 128];
    let np: usize = 0;
    while (np < nul_positions.len) : (np += 1) {
        const p = nul_positions[np];
        const saved = buf[p];
        buf[p] = 0x00;
        expect_eq(findCStringNulSimd(buf[0..]).?, findCStringNulScalar(buf[0..]).?);
        buf[p] = saved;
    }

    // 3b. isBsonAsciiSimd vs Scalar
    expect_eq(isBsonAsciiSimd(buf[0..]), isBsonAsciiScalar(buf[0..]));
    buf[64] = 0xC3;
    expect_eq(isBsonAsciiSimd(buf[0..]), isBsonAsciiScalar(buf[0..]));
    buf[64] = 'a';

    // 3c. copyBsonBytesSimd vs Scalar
    let dst_sc: [129]u8 = [0; 129];
    let dst_sm: [129]u8 = [0; 129];
    const lens: [8]usize = [0, 1, 15, 16, 17, 31, 64, 129];
    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        @memset(dst_sc[0..n], 0);
        @memset(dst_sm[0..n], 0);
        copyBsonBytesScalar(dst_sc[0..n], buf[0..n]);
        copyBsonBytesSimd(dst_sm[0..n], buf[0..n]);
        expect_true(bytesEqual(dst_sc[0..n], dst_sm[0..n]))?;
    }
}

fn runTestMalformedFuzz(): void! {
    const alloc = std.heap.page_allocator;
    let seed: u64 = 0x6A09E667F3BCC908;
    let buf: [256]u8 = [0; 256];
    let case_idx: usize = 0;
    while (case_idx < 1300) : (case_idx += 1) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        const len: usize = ((seed % 255) as usize) + 1;

        let j: usize = 0;
        while (j < len) : (j += 1) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            buf[j] = (seed & 0xFF) as u8;
        }
        if ((case_idx % 4) == 0 && len >= 5) {
            buf[0] = (len & 0xFF) as u8;
            buf[1] = 0;
            buf[2] = 0;
            buf[3] = 0;
            buf[len - 1] = 0x00;
        }

        const slice = buf[0..len];
        * = validateBsonDocument(slice) catch {};
        * = findBsonField(slice, "k");
        if (fields := tryDecodeDoc(alloc, slice)) {
            deinitDocument(alloc, fields);
        }
    }
}

fn main() {
    test("1. MongoDB BSON 1.1 all types encode/decode, Binary-0x02, zero-heap validate & lookup", () => {
        runTestBsonAllTypesAndLookup()?;
    });
    test("2. Security boundaries: doc_size, terminator, Boolean, UTF-8, Binary-0x02 & depth=64", () => {
        runTestSecurityBoundaries()?;
    });
    test("3. SIMD vs Scalar 0-diff parity (findCStringNul, isBsonAscii, copyBsonBytes)", () => {
        runTestSimdParity()?;
    });
    test("4. 1,300 deterministic PRNG malformed BSON fuzz cases (0 panic / 0 OOB)", () => {
        runTestMalformedFuzz()?;
    });
}
