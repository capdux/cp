import { test, expect_eq } from "cap:test";
import {
    PemError,
    PemBlock,
    PemBundleIter,
    findPemDashScalar,
    findPemDashSimd,
    copyPemBytesScalar,
    copyPemBytesSimd,
    isValidBase64ChunkScalar,
    isValidBase64ChunkSimd,
    parse,
    parseFirst,
    encode,
    encodePemStrict,
    encodeChain,
    decodePemInto,
    freeBlocks,
} from "../../lib/auth/pem.cp";

const std = @import("std");

fn tryParsePem(alloc: std.mem.Allocator, slice: []const u8): ([]PemBlock)? {
    return parse(alloc, slice) catch return null;
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

fn runTestRfc7468BoundariesAndBase64(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Valid CRLF PEM with surrounding comments
    const valid_crlf =
        "# Leading comment\r\n" ++
        "-----BEGIN TEST DATA-----\r\n" ++
        "SGVsbG8gUkZDIDc0Njgh\r\n" ++
        "-----END TEST DATA-----\r\n" ++
        "# Trailing comment\r\n";

    let out_buf: [128]u8 = [0; 128];
    const dec_out = decodePemInto(valid_crlf, out_buf[0..]) catch unreachable;
    expect_true(bytesEqual(dec_out.label, "TEST DATA"))?;
    expect_true(bytesEqual(dec_out.bytes, "Hello RFC 7468!"))?;

    const blk = (parseFirst(alloc, valid_crlf) catch unreachable).?;
    let blk_mut = blk;
    defer blk_mut.deinit(alloc);
    expect_true(bytesEqual(blk_mut.label, "TEST DATA"))?;
    expect_true(bytesEqual(blk_mut.bytes, "Hello RFC 7468!"))?;

    // 2. encodePemStrict 64-char wrapping roundtrip (100 bytes -> 136 Base64 chars -> 3 lines)
    let raw_100: [100]u8 = [0; 100];
    let k: usize = 0;
    while (k < 100) : (k += 1) {
        raw_100[k] = ((k * 13 + 7) & 0xFF) as u8;
    }
    const pem_100 = encodePemStrict(alloc, "BINARY BLOB", raw_100[0..]) catch unreachable;
    defer alloc.free(pem_100);
    let out_100: [128]u8 = [0; 128];
    const dec_100 = decodePemInto(pem_100, out_100[0..]) catch unreachable;
    expect_true(bytesEqual(dec_100.bytes, raw_100[0..]))?;

    // 3. Reject invalid labels: empty, leading/trailing space, consecutive spaces
    const bad_labels: [4][]const u8 = [
        "-----BEGIN -----\nAQ==\n-----END -----\n",
        "-----BEGIN  CERT-----\nAQ==\n-----END  CERT-----\n",
        "-----BEGIN MY  KEY-----\nAQ==\n-----END MY  KEY-----\n",
        "-----BEGIN CERT -----\nAQ==\n-----END CERT -----\n",
    ];
    let li: usize = 0;
    while (li < bad_labels.len) : (li += 1) {
        let got_lbl = false;
        * = decodePemInto(bad_labels[li], out_buf[0..]) catch |e| {
            if (e == PemError.InvalidLabel) got_lbl = true;
        };
        expect_true(got_lbl)?;
    }

    // 4. Reject mismatched END label & nested BEGIN
    const mismatch_pem = "-----BEGIN CERT-----\nAQ==\n-----END KEY-----\n";
    let got_mm = false;
    * = decodePemInto(mismatch_pem, out_buf[0..]) catch |e| {
        if (e == PemError.MismatchedEndLabel) got_mm = true;
    };
    expect_true(got_mm)?;

    const nested_pem = "-----BEGIN CERT-----\n-----BEGIN INNER-----\nAQ==\n-----END CERT-----\n";
    let got_nest = false;
    * = decodePemInto(nested_pem, out_buf[0..]) catch |e| {
        if (e == PemError.MissingEndMarker) got_nest = true;
    };
    expect_true(got_nest)?;

    // 5. Reject non-canonical Base64 trailing non-zero bits ('AR==' has low 4 bits = 0001 != 0; 'QUJ=' has low 2 bits = 01 != 0)
    const non_canon_b64_2: []const u8 = "-----BEGIN T-----\nAR==\n-----END T-----\n";
    let got_nc2 = false;
    * = decodePemInto(non_canon_b64_2, out_buf[0..]) catch |e| {
        if (e == PemError.InvalidBase64) got_nc2 = true;
    };
    expect_true(got_nc2)?;

    const non_canon_b64_1: []const u8 = "-----BEGIN T-----\nQUJ=\n-----END T-----\n";
    let got_nc1 = false;
    * = decodePemInto(non_canon_b64_1, out_buf[0..]) catch |e| {
        if (e == PemError.InvalidBase64) got_nc1 = true;
    };
    expect_true(got_nc1)?;

    // Misplaced '=' & invalid char
    const misplaced_eq: []const u8 = "-----BEGIN T-----\nAQ==AQ==\n-----END T-----\n";
    let got_meq = false;
    * = decodePemInto(misplaced_eq, out_buf[0..]) catch |e| {
        if (e == PemError.InvalidBase64) got_meq = true;
    };
    expect_true(got_meq)?;
}

fn runTestPemBundleIterAndChain(): void! {
    const alloc = std.heap.page_allocator;
    const p1 = encode(alloc, "CERTIFICATE", "LeafCertBytes") catch unreachable;
    defer alloc.free(p1);
    const p2 = encode(alloc, "CERTIFICATE", "RootCACertBytes") catch unreachable;
    defer alloc.free(p2);

    const blocks = parse(alloc, "comment\n" ++
        "-----BEGIN CERTIFICATE-----\n" ++
        "TGVhZkNlcnRCeXRlcw==\n" ++
        "-----END CERTIFICATE-----\n" ++
        "between\n" ++
        "-----BEGIN CERTIFICATE-----\n" ++
        "Um9vdENBQ2VydEJ5dGVz\n" ++
        "-----END CERTIFICATE-----\n") catch unreachable;
    defer freeBlocks(alloc, blocks);
    expect_eq(blocks.len, 2);

    const chain_pem = encodeChain(alloc, blocks) catch unreachable;
    defer alloc.free(chain_pem);

    let it = PemBundleIter.init(chain_pem);
    let buf: [128]u8 = [0; 128];
    expect_true(it.hasNext())?;
    const d1 = it.nextDecoded(buf[0..]) catch unreachable;
    expect_true(bytesEqual(d1.label, "CERTIFICATE"))?;
    expect_true(bytesEqual(d1.bytes, "LeafCertBytes"))?;

    expect_true(it.hasNext())?;
    const d2 = it.nextDecoded(buf[0..]) catch unreachable;
    expect_true(bytesEqual(d2.bytes, "RootCACertBytes"))?;
    expect_eq(it.hasNext(), false);
}

fn runTestSimdParity(): void! {
    let text: [129]u8 = [' '; 129];
    expect_eq(findPemDashSimd(text[0..]) == null, findPemDashScalar(text[0..]) == null);
    const dash_pos: [4]usize = [0, 15, 16, 128];
    let di: usize = 0;
    while (di < dash_pos.len) : (di += 1) {
        const p = dash_pos[di];
        text[p] = '-';
        expect_eq(findPemDashSimd(text[0..]).?, findPemDashScalar(text[0..]).?);
        text[p] = ' ';
    }

    let src: [129]u8 = [0; 129];
    let dst_sc: [129]u8 = [0; 129];
    let dst_sm: [129]u8 = [0; 129];
    let i: usize = 0;
    while (i < 129) : (i += 1) {
        src[i] = ((i * 23 + 9) & 0xFF) as u8;
    }
    const lens: [7]usize = [0, 1, 15, 16, 17, 64, 129];
    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        @memset(dst_sc[0..n], 0);
        @memset(dst_sm[0..n], 0);
        copyPemBytesScalar(dst_sc[0..n], src[0..n]);
        copyPemBytesSimd(dst_sm[0..n], src[0..n]);
        expect_true(bytesEqual(dst_sc[0..n], dst_sm[0..n]))?;
    }

    const b64_ok = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const b64_bad = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef!hijklmnopqrstuvwxyz0123456789+/";
    expect_eq(isValidBase64ChunkSimd(b64_ok), isValidBase64ChunkScalar(b64_ok));
    expect_eq(isValidBase64ChunkSimd(b64_ok), true);
    expect_eq(isValidBase64ChunkSimd(b64_bad), isValidBase64ChunkScalar(b64_bad));
    expect_eq(isValidBase64ChunkSimd(b64_bad), false);
}

fn runTestMalformedFuzz(): void! {
    const alloc = std.heap.page_allocator;
    let seed: u64 = 0x3C6EF372FE94F82B;
    let buf: [256]u8 = [0; 256];
    let out_buf: [256]u8 = [0; 256];
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
        if ((case_idx % 4) == 0 && len >= 20) {
            copyPemBytesSimd(buf[0..16], "-----BEGIN A----");
        }

        const slice = buf[0..len];
        * = decodePemInto(slice, out_buf[0..]) catch {};
        if (blocks := tryParsePem(alloc, slice)) {
            freeBlocks(alloc, blocks);
        }
    }
}

fn main() {
    test("1. RFC 7468 boundaries, canonical Base64 trailing bits & encodePemStrict/decodePemInto", () => {
        runTestRfc7468BoundariesAndBase64()?;
    });
    test("2. Multi-block PemBundleIter & encodeChain roundtrip", () => {
        runTestPemBundleIterAndChain()?;
    });
    test("3. SIMD vs Scalar 0-diff parity (findPemDash, copyPemBytes, isValidBase64Chunk)", () => {
        runTestSimdParity()?;
    });
    test("4. 1,300 deterministic PRNG malformed PEM fuzz cases (0 panic / 0 OOB)", () => {
        runTestMalformedFuzz()?;
    });
}
