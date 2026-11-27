// cap/test/testCapBencode.cp
// Comprehensive unit tests for @cap/bencode (BitTorrent BEP-3 parity)

import { test, expect_eq } from "cap:test";
import {
    KIND_INT,
    KIND_BYTES,
    KIND_LIST,
    KIND_DICT,
    BencodeWriter,
    BencodeIntResult,
    BencodeBytesResult,
    peekKind,
    decodeInt,
    decodeBytes,
    validate,
    dictLookup,
} from "../../packages/bendy/src/index.cp";

fn main() {
    test("1. @cap/bencode — signed/zero/negative integers & strict rejection of i-0e/i01e/ie", () => {
        let buf: [64]u8 = undefined;
        let w: BencodeWriter = BencodeWriter.init(buf[0..64]);
        expect_eq(w.writeInt(0), true);
        expect_eq(w.writeInt(42), true);
        expect_eq(w.writeInt(-123456), true);

        const data: []const u8 = w.written();
        expect_eq(peekKind(data, 0), KIND_INT);

        const r0: BencodeIntResult = decodeInt(data, 0);
        expect_eq(r0.ok, true);
        expect_eq(r0.value, 0 as i64);

        const r1: BencodeIntResult = decodeInt(data, r0.next_pos);
        expect_eq(r1.ok, true);
        expect_eq(r1.value, 42 as i64);

        const r2: BencodeIntResult = decodeInt(data, r1.next_pos);
        expect_eq(r2.ok, true);
        expect_eq(r2.value, -123456 as i64);
        expect_eq(r2.next_pos, data.len);

        // Strict rejections
        expect_eq(decodeInt("i-0e", 0).ok, false);
        expect_eq(decodeInt("i01e", 0).ok, false);
        expect_eq(decodeInt("i00e", 0).ok, false);
        expect_eq(decodeInt("ie", 0).ok, false);
        expect_eq(decodeInt("i-e", 0).ok, false);
        expect_eq(decodeInt("i42", 0).ok, false);
    });

    test("2. @cap/bencode — empty & binary byte strings containing NUL", () => {
        let buf: [64]u8 = undefined;
        let w: BencodeWriter = BencodeWriter.init(buf[0..64]);
        const bin: [4]u8 = [0x41, 0x00, 0xFF, 0x42];

        expect_eq(w.writeBytes(""), true);
        expect_eq(w.writeBytes(bin[0..4]), true);
        const data: []const u8 = w.written();
        expect_eq(peekKind(data, 0), KIND_BYTES);

        const b0: BencodeBytesResult = decodeBytes(data, 0);
        expect_eq(b0.ok, true);
        expect_eq(b0.len, 0 as usize);

        const b1: BencodeBytesResult = decodeBytes(data, b0.next_pos);
        expect_eq(b1.ok, true);
        expect_eq(b1.len, 4 as usize);
        expect_eq(data[b1.start], 0x41 as u8);
        expect_eq(data[b1.start + 1], 0x00 as u8);
        expect_eq(data[b1.start + 2], 0xFF as u8);
        expect_eq(data[b1.start + 3], 0x42 as u8);

        // Reject leading zero length and truncation
        expect_eq(decodeBytes("01:a", 0).ok, false);
        expect_eq(decodeBytes("5:abc", 0).ok, false);
    });

    test("3. @cap/bencode — nested list & dict encoding and full validate", () => {
        let buf: [128]u8 = undefined;
        let w: BencodeWriter = BencodeWriter.init(buf[0..128]);
        expect_eq(w.beginDict(), true);
        expect_eq(w.writeBytes("items"), true);
        expect_eq(w.beginList(), true);
        expect_eq(w.writeInt(1), true);
        expect_eq(w.writeBytes("two"), true);
        expect_eq(w.end(), true);
        expect_eq(w.writeBytes("meta"), true);
        expect_eq(w.beginDict(), true);
        expect_eq(w.writeBytes("ok"), true);
        expect_eq(w.writeInt(1), true);
        expect_eq(w.end(), true);
        expect_eq(w.end(), true);

        const doc: []const u8 = w.written();
        expect_eq(peekKind(doc, 0), KIND_DICT);
        expect_eq(validate(doc), true);
        expect_eq(validate("le"), true);
        expect_eq(validate("de"), true);
    });

    test("4. @cap/bencode — dictLookup extracts nested dict & list values zero-copy", () => {
        const doc = "d4:infod4:name4:test4:sizei1024ee4:listli1ei2ee4:porti6881ee";
        expect_eq(validate(doc), true);

        const port_res: BencodeBytesResult = dictLookup(doc, "port");
        expect_eq(port_res.ok, true);
        const port_int: BencodeIntResult = decodeInt(doc, port_res.start);
        expect_eq(port_int.ok, true);
        expect_eq(port_int.value, 6881 as i64);

        const list_res: BencodeBytesResult = dictLookup(doc, "list");
        expect_eq(list_res.ok, true);
        expect_eq(peekKind(doc, list_res.start), KIND_LIST);
        expect_eq(validate(doc[list_res.start..(list_res.start + list_res.len)]), true);

        const info_res: BencodeBytesResult = dictLookup(doc, "info");
        expect_eq(info_res.ok, true);
        const info_slice: []const u8 = doc[info_res.start..(info_res.start + info_res.len)];
        const size_res: BencodeBytesResult = dictLookup(info_slice, "size");
        expect_eq(size_res.ok, true);
        expect_eq(decodeInt(info_slice, size_res.start).value, 1024 as i64);

        expect_eq(dictLookup(doc, "missing").ok, false);
    });

    test("5. @cap/bencode — rejects unsorted dict keys, duplicate keys & trailing garbage", () => {
        // Unsorted keys: "b" before "a"
        expect_eq(validate("d1:bi1e1:ai2ee"), false);
        // Duplicate keys: "a" twice
        expect_eq(validate("d1:ai1e1:ai2ee"), false);
        // Non-string key (integer key)
        expect_eq(validate("di1ei2ee"), false);
        // Trailing garbage after valid value
        expect_eq(validate("i42eEXTRA"), false);
        expect_eq(validate("d1:ai1ee0:"), false);
        // Unterminated containers
        expect_eq(validate("li1e"), false);
        expect_eq(validate("d1:ai1e"), false);
    });

    test("6. @cap/bencode — BencodeWriter buffer overflow protection", () => {
        let tiny: [5]u8 = undefined;
        let w: BencodeWriter = BencodeWriter.init(tiny[0..5]);
        expect_eq(w.writeInt(123), true); // "i123e" = 5 bytes, exact fit
        expect_eq(w.len(), 5 as usize);
        expect_eq(w.overflow, false);

        // Next write must fail cleanly and latch overflow
        expect_eq(w.writeInt(0), false);
        expect_eq(w.overflow, true);
        expect_eq(w.writeBytes("x"), false);
        expect_eq(w.len(), 5 as usize);
    });
}
