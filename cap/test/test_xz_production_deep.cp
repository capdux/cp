// Source: cp/cap/test/test_xz_production_deep.cp (Production deep tests for cap:xz)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    XZ_HEADER_MAGIC,
    XZ_FOOTER_MAGIC,
    XZ_STREAM_HEADER_SIZE,
    XZ_STREAM_FOOTER_SIZE,
    CHECK_NONE,
    CHECK_CRC32,
    CHECK_CRC64,
    CHECK_SHA256,
    FILTER_DELTA,
    FILTER_X86,
    FILTER_POWERPC,
    FILTER_IA64,
    FILTER_ARM,
    FILTER_ARMTHUMB,
    FILTER_SPARC,
    FILTER_ARM64,
    FILTER_RISCV,
    FILTER_LZMA2,
    xzCrc32,
    XzVliResult,
    decodeXzVli,
    encodeXzVli,
    XzStreamHeader,
    XzStreamFooter,
    XzFilter,
    XzBlockHeader,
    XzIndexRecord,
    XzIndexRecordIter,
    XzIndex,
    isXz,
    parseXzHeader,
    parseXzFooter,
    parseXzBlockHeader,
    parseXzIndex,
} from "cap:pack";

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off + 0] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn slicesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. XZ 7-bit VLI 1B..9B encode/decode round-trip and >9B / 64-bit overflow rejection", () => {
    // IEEE 802.3 CRC32("123456789") == 0xCBF43926
    expect_eq(xzCrc32("123456789"), 0xCBF43926 as u32);

    let buf: [16]u8 = std.mem.zeroes([16]u8);
    let out: []u8 = buf[0..];

    // 1 byte: 0 and 127
    if (n := encodeXzVli(0, *out)) {
        expect_eq(n, 1 as usize);
        if (r := decodeXzVli(buf[0..n])) {
            expect_eq(r.value, 0 as u64);
            expect_eq(r.consumed, 1 as usize);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    if (n := encodeXzVli(127, *out)) {
        expect_eq(n, 1 as usize);
        if (r := decodeXzVli(buf[0..n])) {
            expect_eq(r.value, 127 as u64);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 2 bytes: 128 and 16383
    if (n := encodeXzVli(16383, *out)) {
        expect_eq(n, 2 as usize);
        if (r := decodeXzVli(buf[0..n])) {
            expect_eq(r.value, 16383 as u64);
            expect_eq(r.consumed, 2 as usize);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 9 bytes: max 63-bit integer 0x7FFFFFFFFFFFFFFF
    const max63: u64 = 0x7FFFFFFFFFFFFFFF;
    if (n := encodeXzVli(max63, *out)) {
        expect_eq(n, 9 as usize);
        if (r := decodeXzVli(buf[0..n])) {
            expect_eq(r.value, max63);
            expect_eq(r.consumed, 9 as usize);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Value > 63-bit must fail encode
    if (_ := encodeXzVli(0x8000000000000000, *out)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 10-byte VLI (9 continuation bytes with 0x80 set) must fail decode
    let over9: [10]u8 = [0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x01];
    if (_ := decodeXzVli(over9[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});

test("2. XZ 12-byte Stream Header and Stream Footer dual CRC32 verification", () => {
    expect_eq(XZ_STREAM_HEADER_SIZE, 12 as usize);
    expect_eq(XZ_STREAM_FOOTER_SIZE, 12 as usize);

    // Stream Header with CHECK_CRC64
    let hdr: [12]u8 = std.mem.zeroes([12]u8);
    let i: usize = 0;
    while (i < 6) {
        hdr[i] = XZ_HEADER_MAGIC[i];
        i += 1;
    }
    hdr[6] = 0x00;
    hdr[7] = CHECK_CRC64;
    putU32Le(hdr[0..], 8, xzCrc32(hdr[6..8]));

    expect_eq(isXz(hdr[0..]), true);
    if (h := parseXzHeader(hdr[0..])) {
        expect_eq(h.check_type, CHECK_CRC64);
        expect_eq(h.checkSize(), 8 as usize);
        expect_eq(slicesEq(h.checkName(), "crc64"), true);
    } else {
        expect_eq(true, false);
    }

    // Stream Header with CHECK_SHA256
    hdr[7] = CHECK_SHA256;
    putU32Le(hdr[0..], 8, xzCrc32(hdr[6..8]));
    if (h := parseXzHeader(hdr[0..])) {
        expect_eq(h.checkSize(), 32 as usize);
        expect_eq(slicesEq(h.checkName(), "sha256"), true);
    } else {
        expect_eq(true, false);
    }

    // Stream Footer with backward_size_raw = 3 -> (3 + 1) * 4 = 16 bytes
    let ftr: [12]u8 = std.mem.zeroes([12]u8);
    putU32Le(ftr[0..], 4, 3);
    ftr[8] = 0x00;
    ftr[9] = CHECK_CRC32;
    ftr[10] = XZ_FOOTER_MAGIC[0];
    ftr[11] = XZ_FOOTER_MAGIC[1];
    putU32Le(ftr[0..], 0, xzCrc32(ftr[4..10]));

    if (f := parseXzFooter(ftr[0..])) {
        expect_eq(f.backward_size, 16 as u64);
        expect_eq(f.check_type, CHECK_CRC32);
    } else {
        expect_eq(true, false);
    }

    expect_eq(CHECK_NONE, 0x00 as u8);
});

test("3. XZ Block Header with BCJ x86 + LZMA2 dual filter chain and lzma2DictSize() decoding", () => {
    // Build a 16-byte Block Header (encoded_size = 3 -> (3 + 1) * 4 = 16)
    // Layout:
    // [0] = 3 (16B)
    // [1] = 0xC1 (has_uncompressed=1, has_compressed=1, filter_count=2)
    // [2] = 0x64 (compressed_size = 100)
    // [3..5] = 0x80, 0x08 (uncompressed_size = 1024)
    // Filter 0: BCJ x86 (id=0x04, props_len=0) -> [5]=0x04, [6]=0x00
    // Filter 1: LZMA2 (id=0x21, props_len=1, prop=22 -> 8 MiB dict) -> [7]=0x21, [8]=0x01, [9]=22
    // Padding: [10]=0, [11]=0
    // CRC32: [12..16]
    let blk: [16]u8 = std.mem.zeroes([16]u8);
    blk[0] = 3;
    blk[1] = 0xC1;
    blk[2] = 100;
    blk[3] = 0x80;
    blk[4] = 0x08;
    blk[5] = FILTER_X86 as u8;
    blk[6] = 0;
    blk[7] = FILTER_LZMA2 as u8;
    blk[8] = 1;
    blk[9] = 22; // (2 | 0) << (11 + 11) = 2 << 22 = 8 MiB
    blk[10] = 0;
    blk[11] = 0;
    putU32Le(blk[0..], 12, xzCrc32(blk[0..12]));

    if (bh := parseXzBlockHeader(blk[0..])) {
        expect_eq(bh.header_size, 16 as usize);
        expect_eq(bh.filter_count, 2 as u8);
        expect_eq(bh.has_compressed_size, true);
        expect_eq(bh.has_uncompressed_size, true);
        expect_eq(bh.compressed_size, 100 as u64);
        expect_eq(bh.uncompressed_size, 1024 as u64);

        expect_eq(bh.filters[0].filter_id, FILTER_X86);
        expect_eq(bh.filters[0].props.len, 0 as usize);
        if (_ := bh.filters[0].lzma2DictSize()) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }

        expect_eq(bh.filters[1].filter_id, FILTER_LZMA2);
        expect_eq(bh.filters[1].props.len, 1 as usize);
        if (dict := bh.filters[1].lzma2DictSize()) {
            expect_eq(dict, (8 * 1024 * 1024) as u64);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Also test LZMA2 max dict prop == 40 -> 0xFFFFFFFF (4 GiB - 1)
    let max_prop: [1]u8 = [40];
    const f_max = XzFilter {
        filter_id: FILTER_LZMA2,
        props: max_prop[0..],
    };
    if (d := f_max.lzma2DictSize()) {
        expect_eq(d, 0xFFFFFFFF as u64);
    } else {
        expect_eq(true, false);
    }

    // Sanity check remaining filter constants
    expect_eq(FILTER_DELTA, 0x03 as u64);
    expect_eq(FILTER_POWERPC, 0x05 as u64);
    expect_eq(FILTER_IA64, 0x06 as u64);
    expect_eq(FILTER_ARM, 0x07 as u64);
    expect_eq(FILTER_ARMTHUMB, 0x08 as u64);
    expect_eq(FILTER_SPARC, 0x09 as u64);
    expect_eq(FILTER_ARM64, 0x0A as u64);
    expect_eq(FILTER_RISCV, 0x0B as u64);
});

test("4. XZ Index multi-record zero-copy iteration and CRC32 verification", () => {
    // Layout:
    // [0] = 0x00 (index indicator)
    // [1] = 2 (record_count)
    // Rec 0: unpadded=60, uncompressed=512 (0x80, 0x04)
    // Rec 1: unpadded=120, uncompressed=1024 (0x80, 0x08)
    // Total before padding = 1 + 1 + 3 + 3 = 8 bytes -> pad = 0
    // [8..12] = CRC32
    let idx: [12]u8 = std.mem.zeroes([12]u8);
    idx[0] = 0x00;
    idx[1] = 2;
    idx[2] = 60;
    idx[3] = 0x80;
    idx[4] = 0x04;
    idx[5] = 120;
    idx[6] = 0x80;
    idx[7] = 0x08;
    putU32Le(idx[0..], 8, xzCrc32(idx[0..8]));

    if (xi := parseXzIndex(idx[0..])) {
        expect_eq(xi.record_count, 2 as u64);
        expect_eq(xi.total_size, 12 as usize);

        let it = xi.records();
        if (r0 := it.next()) {
            expect_eq(r0.unpadded_size, 60 as u64);
            expect_eq(r0.uncompressed_size, 512 as u64);
        } else {
            expect_eq(true, false);
        }
        if (r1 := it.next()) {
            expect_eq(r1.unpadded_size, 120 as u64);
            expect_eq(r1.uncompressed_size, 1024 as u64);
        } else {
            expect_eq(true, false);
        }
        if (_ := it.next()) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed XZ rejection: bad magic, bad CRC32, non-zero padding, and truncation", () => {
    // 5a. Bad Stream Header magic & bad CRC32
    let hdr: [12]u8 = [0xFD, '7', 'z', 'X', 'Z', 0x00, 0x00, CHECK_CRC32, 0, 0, 0, 0];
    putU32Le(hdr[0..], 8, xzCrc32(hdr[6..8]));
    let bad_magic = hdr;
    bad_magic[0] = 0x00;
    expect_eq(isXz(bad_magic[0..]), false);
    if (_ := parseXzHeader(bad_magic[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let bad_crc = hdr;
    bad_crc[8] ^= 0xFF;
    if (_ := parseXzHeader(bad_crc[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Bad Footer magic & bad CRC32
    let ftr: [12]u8 = std.mem.zeroes([12]u8);
    ftr[10] = 'Y';
    ftr[11] = 'X'; // wrong
    putU32Le(ftr[0..], 0, xzCrc32(ftr[4..10]));
    if (_ := parseXzFooter(ftr[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Non-zero Block Header padding (even with recomputed CRC32!) must be rejected
    let bad_pad: [12]u8 = std.mem.zeroes([12]u8);
    bad_pad[0] = 2; // (2+1)*4 = 12 bytes
    bad_pad[1] = 0x00; // 1 filter, no sizes
    bad_pad[2] = FILTER_LZMA2 as u8;
    bad_pad[3] = 1;
    bad_pad[4] = 22;
    bad_pad[5] = 0x00;
    bad_pad[6] = 0x00;
    bad_pad[7] = 0x01; // non-zero padding!
    putU32Le(bad_pad[0..], 8, xzCrc32(bad_pad[0..8]));
    if (_ := parseXzBlockHeader(bad_pad[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. Truncated buffers
    if (_ := parseXzHeader(hdr[0..11])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
    if (_ := parseXzIndex(hdr[0..4])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
