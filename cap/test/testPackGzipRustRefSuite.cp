import { test, expect_eq } from "cap:test";
import {
    GZIP_MAGIC_ID1,
    GZIP_MAGIC_ID2,
    GZIP_FEXTRA,
    GZIP_FNAME,
    GZIP_FCOMMENT,
    GZIP_FHCRC,
    GzipError,
    GzipHeader,
    parseGzipHeader,
    parseZlibHeader,
    inflateCopyStoredScalar,
    inflateCopyStoredSimd,
    inflateCopyMatchScalar,
    inflateCopyMatchSimd,
    deflateMatchLenScalar,
    deflateMatchLenSimd,
    inflateRawWithLimit,
    inflateRawInto,
    inflateWithLimit,
    gunzipWithLimit,
    gzip,
    gunzip,
    deflate,
    inflate,
    deflateRaw,
    inflateRaw,
    Gzip,
    Gunzip,
    DeflateCodec,
    InflateCodec,
    DeflateRawCodec,
    GzipStream,
    GunzipStream,
    DeflateStream,
    InflateStream,
    RleCodec,
} from "../../lib/pack/gzip.cp";

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

// ============================================================================
// Real Node.js v26 zlib / gzip reference vectors (0-byte-diff ground truth)
// ============================================================================

const NODE_RAW_L1: [141]u8 = [0x73, 0x0e, 0x50, 0xf0, 0x49, 0xcc, 0x4b, 0x2f, 0x4d, 0x4c, 0x4f, 0x55, 0x08, 0x72, 0x73, 0x56, 0x30, 0xb4, 0x34, 0x35, 0xd4, 0x37, 0xb4, 0x34, 0x35, 0x00, 0x11, 0x46, 0x0a, 0x01, 0xa5, 0x45, 0xa9, 0xba, 0xce, 0x01, 0x0a, 0x2e, 0xa9, 0x69, 0x39, 0x89, 0x25, 0xa9, 0xfa, 0x51, 0x39, 0x99, 0x49, 0xfa, 0xee, 0x55, 0x99, 0x05, 0x0a, 0xae, 0x79, 0xe9, 0x99, 0x79, 0xa9, 0x8a, 0x0a, 0xce, 0xa3, 0x06, 0x0c, 0x87, 0x30, 0x30, 0x30, 0x34, 0x32, 0x36, 0x31, 0x35, 0x33, 0xb7, 0xb0, 0x4c, 0x4c, 0x4a, 0x4e, 0x49, 0x4d, 0x4b, 0xcf, 0xc8, 0xcc, 0xca, 0xce, 0xc9, 0xcd, 0xcb, 0x2f, 0x28, 0x2c, 0x2a, 0x2e, 0x29, 0x2d, 0x2b, 0xaf, 0xa8, 0xac, 0x72, 0x74, 0x72, 0x76, 0x71, 0x75, 0x73, 0xf7, 0xf0, 0xf4, 0xf2, 0xf6, 0xf1, 0xf5, 0xf3, 0x0f, 0x08, 0x0c, 0x0a, 0x0e, 0x09, 0x0d, 0x0b, 0x8f, 0x88, 0x8c, 0xe2, 0x72, 0x24, 0x03, 0x44, 0x44, 0x46, 0x11, 0x44, 0x00];

const NODE_RAW_L6: [141]u8 = [0x73, 0x0e, 0x50, 0xf0, 0x49, 0xcc, 0x4b, 0x2f, 0x4d, 0x4c, 0x4f, 0x55, 0x08, 0x72, 0x73, 0x56, 0x30, 0xb4, 0x34, 0x35, 0xd4, 0x37, 0xb4, 0x34, 0x35, 0x00, 0x11, 0x46, 0x0a, 0x01, 0xa5, 0x45, 0xa9, 0xba, 0xce, 0x01, 0x0a, 0x2e, 0xa9, 0x69, 0x39, 0x89, 0x25, 0xa9, 0xfa, 0x51, 0x39, 0x99, 0x49, 0xfa, 0xee, 0x55, 0x99, 0x05, 0x0a, 0xae, 0x79, 0xe9, 0x99, 0x79, 0xa9, 0x8a, 0x0a, 0xce, 0xa3, 0x06, 0x0c, 0x07, 0x03, 0x0c, 0x0c, 0x8d, 0x8c, 0x4d, 0x4c, 0xcd, 0xcc, 0x2d, 0x2c, 0x13, 0x93, 0x92, 0x53, 0x52, 0xd3, 0xd2, 0x33, 0x32, 0xb3, 0xb2, 0x73, 0x72, 0xf3, 0xf2, 0x0b, 0x0a, 0x8b, 0x8a, 0x4b, 0x4a, 0xcb, 0xca, 0x2b, 0x2a, 0xab, 0x1c, 0x9d, 0x9c, 0x5d, 0x5c, 0xdd, 0xdc, 0x3d, 0x3c, 0xbd, 0xbc, 0x7d, 0x7c, 0xfd, 0xfc, 0x03, 0x02, 0x83, 0x82, 0x43, 0x42, 0xc3, 0xc2, 0x23, 0x22, 0xa3, 0xb8, 0x1c, 0xc9, 0x00, 0x11, 0x91, 0x51, 0x04, 0x11, 0x00];

const NODE_ZLIB_L6: [147]u8 = [0x78, 0x9c, 0x73, 0x0e, 0x50, 0xf0, 0x49, 0xcc, 0x4b, 0x2f, 0x4d, 0x4c, 0x4f, 0x55, 0x08, 0x72, 0x73, 0x56, 0x30, 0xb4, 0x34, 0x35, 0xd4, 0x37, 0xb4, 0x34, 0x35, 0x00, 0x11, 0x46, 0x0a, 0x01, 0xa5, 0x45, 0xa9, 0xba, 0xce, 0x01, 0x0a, 0x2e, 0xa9, 0x69, 0x39, 0x89, 0x25, 0xa9, 0xfa, 0x51, 0x39, 0x99, 0x49, 0xfa, 0xee, 0x55, 0x99, 0x05, 0x0a, 0xae, 0x79, 0xe9, 0x99, 0x79, 0xa9, 0x8a, 0x0a, 0xce, 0xa3, 0x06, 0x0c, 0x07, 0x03, 0x0c, 0x0c, 0x8d, 0x8c, 0x4d, 0x4c, 0xcd, 0xcc, 0x2d, 0x2c, 0x13, 0x93, 0x92, 0x53, 0x52, 0xd3, 0xd2, 0x33, 0x32, 0xb3, 0xb2, 0x73, 0x72, 0xf3, 0xf2, 0x0b, 0x0a, 0x8b, 0x8a, 0x4b, 0x4a, 0xcb, 0xca, 0x2b, 0x2a, 0xab, 0x1c, 0x9d, 0x9c, 0x5d, 0x5c, 0xdd, 0xdc, 0x3d, 0x3c, 0xbd, 0xbc, 0x7d, 0x7c, 0xfd, 0xfc, 0x03, 0x02, 0x83, 0x82, 0x43, 0x42, 0xc3, 0xc2, 0x23, 0x22, 0xa3, 0xb8, 0x1c, 0xc9, 0x00, 0x11, 0x91, 0x51, 0x04, 0x11, 0x00, 0xb0, 0x3e, 0xc9, 0x56];

const NODE_ZLIB_L9: [147]u8 = [0x78, 0xda, 0x73, 0x0e, 0x50, 0xf0, 0x49, 0xcc, 0x4b, 0x2f, 0x4d, 0x4c, 0x4f, 0x55, 0x08, 0x72, 0x73, 0x56, 0x30, 0xb4, 0x34, 0x35, 0xd4, 0x37, 0xb4, 0x34, 0x35, 0x00, 0x11, 0x46, 0x0a, 0x01, 0xa5, 0x45, 0xa9, 0xba, 0xce, 0x01, 0x0a, 0x2e, 0xa9, 0x69, 0x39, 0x89, 0x25, 0xa9, 0xfa, 0x51, 0x39, 0x99, 0x49, 0xfa, 0xee, 0x55, 0x99, 0x05, 0x0a, 0xae, 0x79, 0xe9, 0x99, 0x79, 0xa9, 0x8a, 0x0a, 0xce, 0xa3, 0x06, 0x0c, 0x07, 0x03, 0x0c, 0x0c, 0x8d, 0x8c, 0x4d, 0x4c, 0xcd, 0xcc, 0x2d, 0x2c, 0x13, 0x93, 0x92, 0x53, 0x52, 0xd3, 0xd2, 0x33, 0x32, 0xb3, 0xb2, 0x73, 0x72, 0xf3, 0xf2, 0x0b, 0x0a, 0x8b, 0x8a, 0x4b, 0x4a, 0xcb, 0xca, 0x2b, 0x2a, 0xab, 0x1c, 0x9d, 0x9c, 0x5d, 0x5c, 0xdd, 0xdc, 0x3d, 0x3c, 0xbd, 0xbc, 0x7d, 0x7c, 0xfd, 0xfc, 0x03, 0x02, 0x83, 0x82, 0x43, 0x42, 0xc3, 0xc2, 0x23, 0x22, 0xa3, 0xb8, 0x1c, 0xc9, 0x00, 0x11, 0x91, 0x51, 0x04, 0x11, 0x00, 0xb0, 0x3e, 0xc9, 0x56];

const NODE_GZIP_L6: [159]u8 = [0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x73, 0x0e, 0x50, 0xf0, 0x49, 0xcc, 0x4b, 0x2f, 0x4d, 0x4c, 0x4f, 0x55, 0x08, 0x72, 0x73, 0x56, 0x30, 0xb4, 0x34, 0x35, 0xd4, 0x37, 0xb4, 0x34, 0x35, 0x00, 0x11, 0x46, 0x0a, 0x01, 0xa5, 0x45, 0xa9, 0xba, 0xce, 0x01, 0x0a, 0x2e, 0xa9, 0x69, 0x39, 0x89, 0x25, 0xa9, 0xfa, 0x51, 0x39, 0x99, 0x49, 0xfa, 0xee, 0x55, 0x99, 0x05, 0x0a, 0xae, 0x79, 0xe9, 0x99, 0x79, 0xa9, 0x8a, 0x0a, 0xce, 0xa3, 0x06, 0x0c, 0x07, 0x03, 0x0c, 0x0c, 0x8d, 0x8c, 0x4d, 0x4c, 0xcd, 0xcc, 0x2d, 0x2c, 0x13, 0x93, 0x92, 0x53, 0x52, 0xd3, 0xd2, 0x33, 0x32, 0xb3, 0xb2, 0x73, 0x72, 0xf3, 0xf2, 0x0b, 0x0a, 0x8b, 0x8a, 0x4b, 0x4a, 0xcb, 0xca, 0x2b, 0x2a, 0xab, 0x1c, 0x9d, 0x9c, 0x5d, 0x5c, 0xdd, 0xdc, 0x3d, 0x3c, 0xbd, 0xbc, 0x7d, 0x7c, 0xfd, 0xfc, 0x03, 0x02, 0x83, 0x82, 0x43, 0x42, 0xc3, 0xc2, 0x23, 0x22, 0xa3, 0xb8, 0x1c, 0xc9, 0x00, 0x11, 0x91, 0x51, 0x04, 0x11, 0x00, 0xd7, 0x7a, 0x72, 0x70, 0x9f, 0x02, 0x00, 0x00];

const NODE_GZIP_FULL_HDR: [113]u8 = [0x1f, 0x8b, 0x08, 0x1e, 0x78, 0x56, 0x34, 0x12, 0x02, 0x03, 0x06, 0x00, 0x41, 0x42, 0x02, 0x00, 0xca, 0xfe, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x74, 0x78, 0x74, 0x00, 0x6d, 0x69, 0x6e, 0x69, 0x7a, 0x5f, 0x6f, 0x78, 0x69, 0x64, 0x65, 0x20, 0x72, 0x65, 0x66, 0x00, 0x36, 0x9e, 0xf3, 0x4d, 0xcd, 0x4d, 0x4a, 0x2d, 0xd2, 0x35, 0xb4, 0x52, 0x08, 0x72, 0x73, 0x56, 0x30, 0xb4, 0x34, 0x35, 0x52, 0x70, 0x73, 0x8d, 0x08, 0x09, 0x72, 0xd4, 0x76, 0xf3, 0x73, 0xf4, 0x75, 0xd5, 0x76, 0x73, 0xf6, 0xf7, 0xf5, 0x75, 0xf5, 0x0b, 0xd1, 0x76, 0xf3, 0x70, 0x0e, 0x72, 0x56, 0x28, 0x4b, 0x2d, 0xca, 0x4c, 0xcb, 0x4c, 0x4d, 0x51, 0x04, 0x00, 0xd5, 0x67, 0x53, 0x8d, 0x38, 0x00, 0x00, 0x00];

const NODE_GZIP_CONCAT: [200]u8 = [0x1f, 0x8b, 0x08, 0x1e, 0x78, 0x56, 0x34, 0x12, 0x02, 0x03, 0x06, 0x00, 0x41, 0x42, 0x02, 0x00, 0xca, 0xfe, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x74, 0x78, 0x74, 0x00, 0x6d, 0x69, 0x6e, 0x69, 0x7a, 0x5f, 0x6f, 0x78, 0x69, 0x64, 0x65, 0x20, 0x72, 0x65, 0x66, 0x00, 0x36, 0x9e, 0xf3, 0x4d, 0xcd, 0x4d, 0x4a, 0x2d, 0xd2, 0x35, 0xb4, 0x52, 0x08, 0x72, 0x73, 0x56, 0x30, 0xb4, 0x34, 0x35, 0x52, 0x70, 0x73, 0x8d, 0x08, 0x09, 0x72, 0xd4, 0x76, 0xf3, 0x73, 0xf4, 0x75, 0xd5, 0x76, 0x73, 0xf6, 0xf7, 0xf5, 0x75, 0xf5, 0x0b, 0xd1, 0x76, 0xf3, 0x70, 0x0e, 0x72, 0x56, 0x28, 0x4b, 0x2d, 0xca, 0x4c, 0xcb, 0x4c, 0x4d, 0x51, 0x04, 0x00, 0xd5, 0x67, 0x53, 0x8d, 0x38, 0x00, 0x00, 0x00, 0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0xf3, 0x4d, 0xcd, 0x4d, 0x4a, 0x2d, 0xd2, 0x35, 0xb2, 0x52, 0x70, 0xce, 0xcf, 0x4b, 0x4e, 0x2c, 0x49, 0xcd, 0x4b, 0x2c, 0x49, 0x4d, 0x51, 0x70, 0xaf, 0xca, 0x2c, 0x50, 0x28, 0x2e, 0x29, 0x4a, 0x4d, 0xcc, 0x55, 0x28, 0xcf, 0x2c, 0xc9, 0x50, 0x70, 0xa9, 0xcc, 0x4b, 0xcc, 0xcd, 0x4c, 0x56, 0xf0, 0x28, 0x4d, 0x4b, 0xcb, 0x4d, 0xcc, 0x53, 0x50, 0x53, 0xf0, 0x89, 0x32, 0x37, 0x57, 0x54, 0xf0, 0x1d, 0xe2, 0xfa, 0x01, 0xdc, 0x63, 0xa9, 0x01, 0x00, 0x01, 0x00, 0x00];

fn buildExpectedNodePayload(buf: *[]u8): usize {
    const unit = "CP Language RFC 1951/1950/1952 Pure-CP Deflate/Zlib/Gzip Engine! ";
    let pos: usize = 0;
    let r: usize = 0;
    while (r < 8) {
        let i: usize = 0;
        while (i < unit.len) { buf[pos + i] = unit[i]; i += 1; }
        pos += unit.len;
        r += 1;
    }
    const tail = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZ";
    let j: usize = 0;
    while (j < tail.len) { buf[pos + j] = tail[j]; j += 1; }
    pos += tail.len;
    return pos;
}

// ============================================================================
// Test 1: Industrial Node.js zlib/gzip Vectors + Full Headers + Multi-Block Roundtrips
// ============================================================================

fn runTestNodeVectorsAndRoundtrips(): void! {
    let expected_arr: [1024]u8 = [0; 1024];
    let expected_s: []u8 = expected_arr[0..];
    const exp_len = buildExpectedNodePayload(expected_s);
    const expected: []const u8 = expected_arr[0..exp_len];
    expect_true(exp_len == 671)?;

    // 1.1 Decode Node.js Raw Deflate L1 / L6
    const raw1 = inflateRawWithLimit(NODE_RAW_L1[0..], 4096)?;
    expect_true(bytesEqual(raw1, expected))?;
    const raw6 = inflateRawWithLimit(NODE_RAW_L6[0..], 4096)?;
    expect_true(bytesEqual(raw6, expected))?;

    // 1.2 Zero-heap inflateRawInto
    let into_arr: [1024]u8 = [0; 1024];
    let into_s: []u8 = into_arr[0..];
    const into_n = inflateRawInto(NODE_RAW_L6[0..], into_s)?;
    expect_true(into_n == exp_len)?;
    expect_true(bytesEqual(into_arr[0..into_n], expected))?;

    // 1.3 Decode Node.js Zlib L6 / L9
    const z6 = inflateWithLimit(NODE_ZLIB_L6[0..], 4096)?;
    expect_true(bytesEqual(z6, expected))?;
    const z9 = inflateWithLimit(NODE_ZLIB_L9[0..], 4096)?;
    expect_true(bytesEqual(z9, expected))?;

    // 1.4 Decode Node.js Gzip L6
    const g6 = gunzipWithLimit(NODE_GZIP_L6[0..], 4096)?;
    expect_true(bytesEqual(g6, expected))?;

    // 1.5 Full RFC 1952 Header (FEXTRA + FNAME + FCOMMENT + FHCRC)
    const hinfo = parseGzipHeader(NODE_GZIP_FULL_HDR[0..])?;
    expect_true(hinfo.header.mtime == 0x12345678)?;
    expect_true((hinfo.header.flags & (GZIP_FEXTRA | GZIP_FNAME | GZIP_FCOMMENT | GZIP_FHCRC)) == 0x1E)?;
    expect_true(hinfo.has_hcrc)?;
    expect_true(hinfo.extra.len == 6 && hinfo.extra[4] == 0xCA && hinfo.extra[5] == 0xFE)?;
    expect_true(bytesEqual(hinfo.name, "sample.txt"))?;
    expect_true(bytesEqual(hinfo.comment, "miniz_oxide ref"))?;

    const m1_out = gunzipWithLimit(NODE_GZIP_FULL_HDR[0..], 4096)?;
    const p1 = "Member-1: RFC 1952 FEXTRA+FNAME+FCOMMENT+FHCRC verified!";
    expect_true(bytesEqual(m1_out, p1))?;

    // 1.6 Concatenated 2-Member Gzip stream
    const concat_out = gunzipWithLimit(NODE_GZIP_CONCAT[0..], 4096)?;
    const p2_unit = "Member-2: Concatenated Gzip stream with Dynamic Huffman & LZ77! ";
    expect_true(concat_out.len == p1.len + p2_unit.len * 4)?;
    expect_true(bytesEqual(concat_out[0..p1.len], p1))?;
    expect_true(bytesEqual(concat_out[p1.len..p1.len + p2_unit.len], p2_unit))?;

    // 1.7 Pure-CP Compress + Decompress across Levels 0, 1, 6, 9 + Empty + Large 70KB (>65535 & >32KB window)
    const levels: [4]i32 = [0, 1, 6, 9];
    let li: usize = 0;
    while (li < 4) {
        const lv = levels[li];
        const c_raw = deflateRaw(expected, lv);
        const d_raw = inflateRaw(c_raw).?;
        expect_true(bytesEqual(d_raw, expected))?;

        const c_z = deflate(expected, lv);
        const d_z = inflate(c_z).?;
        expect_true(bytesEqual(d_z, expected))?;

        const c_g = gzip(expected, lv);
        const d_g = gunzip(c_g).?;
        expect_true(bytesEqual(d_g, expected))?;
        li += 1;
    }

    // Empty payload roundtrip
    expect_true(gunzip(gzip("", 6)).?.len == 0)?;
    expect_true(inflate(deflate("", 6)).?.len == 0)?;
    expect_true(inflateRaw(deflateRaw("", 0)).?.len == 0)?;

    // Large 70KB payload (multi-block Stored at level 0 & sliding window > 32KB at level 6)
    const big_len: usize = 70000;
    let big_raw: [*]u8 = @alloc(big_len);
    let bi: usize = 0;
    while (bi < big_len) {
        unsafe { big_raw[bi] = ((bi * 7 + (bi / 257)) & 0xFF) as u8; }
        bi += 1;
    }
    const big_src: []const u8 = unsafe { big_raw[0..big_len] };
    const big_gz0 = gzip(big_src, 0);
    const big_out0 = gunzipWithLimit(big_gz0, big_len)?;
    expect_true(bytesEqual(big_out0, big_src))?;

    const big_gz6 = gzip(big_src, 6);
    const big_out6 = gunzipWithLimit(big_gz6, big_len)?;
    expect_true(bytesEqual(big_out6, big_src))?;

    // OO & Streaming API backward compatibility
    const oo_c = Gzip.compress(expected, 6);
    expect_true(bytesEqual(Gunzip.decompress(oo_c).?, expected))?;
    expect_true(bytesEqual(InflateCodec.decompress(DeflateCodec.compress(expected, 6)).?, expected))?;
    expect_true(bytesEqual(DeflateRawCodec.decompress(DeflateRawCodec.compress(expected, 6)).?, expected))?;

    let sc = GzipStream.create(6).?;
    const s1 = sc.write("Chunk-A: ");
    const s2 = sc.write("Chunk-B: Pure-CP streaming!");
    const s3 = sc.finish();
    let sd = GunzipStream.create().?;
    * = sd.write(s1);
    * = sd.write(s2);
    * = sd.write(s3);
    const s_out = sd.finish();
    expect_true(bytesEqual(s_out, "Chunk-A: Chunk-B: Pure-CP streaming!"))?;

    let rle_in = "AAAAABBBBBCCCCCDDDDD";
    let rle_c: [64]u8 = [0; 64];
    const rle_cn = RleCodec.compress(rle_in, rle_c[0..]);
    let rle_d: [64]u8 = [0; 64];
    const rle_dn = RleCodec.decompress(rle_c[0..rle_cn], rle_d[0..]);
    expect_true(bytesEqual(rle_d[0..rle_dn], rle_in))?;
}

// ============================================================================
// Test 2: 16-lane SIMD vs Scalar 0-Byte-Diff Parity
// ============================================================================

fn runTestSimdParity(): void! {
    let src_arr: [512]u8 = [0; 512];
    let i: usize = 0;
    while (i < 512) {
        src_arr[i] = ((i * 37 + 13) & 0xFF) as u8;
        i += 1;
    }

    // 2.1 Stored copy parity across lengths 0..128
    let len: usize = 0;
    while (len <= 128) {
        let a: [160]u8 = [0xAA; 160];
        let b: [160]u8 = [0xAA; 160];
        let as_s: []u8 = a[0..];
        let bs_s: []u8 = b[0..];
        inflateCopyStoredScalar(as_s, 7, src_arr[0..len], len);
        inflateCopyStoredSimd(bs_s, 7, src_arr[0..len], len);
        expect_true(bytesEqual(a[0..], b[0..]))?;
        len += 1;
    }

    // 2.2 LZ77 match copy parity: dist=1 (RLE @splat), overlapping dist=2..15, SIMD dist=16,17,32,96
    const dists: [8]usize = [1, 2, 3, 7, 15, 16, 17, 64];
    let di: usize = 0;
    while (di < 8) {
        const dist = dists[di];
        let mlen: usize = 1;
        while (mlen <= 100) {
            let a: [300]u8 = [0; 300];
            let b: [300]u8 = [0; 300];
            let k: usize = 0;
            while (k < 128) {
                a[k] = src_arr[k];
                b[k] = src_arr[k];
                k += 1;
            }
            let as_s: []u8 = a[0..];
            let bs_s: []u8 = b[0..];
            inflateCopyMatchScalar(as_s, 128, dist, mlen);
            inflateCopyMatchSimd(bs_s, 128, dist, mlen);
            expect_true(bytesEqual(a[0..], b[0..]))?;
            mlen += 7;
        }
        di += 1;
    }

    // 2.3 Deflate match-length extension SIMD vs Scalar across every mismatch position 0..258
    let buf: [600]u8 = [0x5A; 600];
    let mm: usize = 0;
    while (mm <= 258) {
        if (mm < 258) {
            buf[300 + mm] = 0xA5;
        }
        const s_len = deflateMatchLenScalar(buf[0..], 0, 300, 258);
        const v_len = deflateMatchLenSimd(buf[0..], 0, 300, 258);
        expect_true(s_len == v_len)?;
        expect_true(v_len == mm)?;
        if (mm < 258) {
            buf[300 + mm] = 0x5A;
        }
        mm += 1;
    }
}

// ============================================================================
// Test 3: Security & RFC 1950/1951/1952 Malformed Guards (miniz_oxide invariants)
// ============================================================================

fn runTestSecurityBoundaries(): void! {
    // 3.1 Bad Stored block LEN ^ NLEN != 0xFFFF
    const bad_stored: [5]u8 = [0x01, 0x05, 0x00, 0x00, 0x00];
    let got_bad_stored: bool = false;
    * = inflateRawWithLimit(bad_stored[0..], 1024) catch |e| {
        got_bad_stored = (e == GzipError.BadStoredLength);
    };
    expect_true(got_bad_stored)?;

    // 3.2 Invalid BTYPE == 3
    const bad_btype: [2]u8 = [0x07, 0x00];
    let got_bad_btype: bool = false;
    * = inflateRawWithLimit(bad_btype[0..], 1024) catch |e| {
        got_bad_btype = (e == GzipError.InvalidBlockType);
    };
    expect_true(got_bad_btype)?;

    // 3.3 RFC 1950 Bad Zlib Headers (from miniz_oxide core.rs tests)
    const bad_z1: [6]u8 = [0x77, 0x85, 0x03, 0x00, 0x00, 0x01]; // CM != 8
    const bad_z2: [6]u8 = [0x88, 0x98, 0x03, 0x00, 0x00, 0x01]; // CINFO > 7
    const bad_z3: [6]u8 = [0x78, 0x98, 0x03, 0x00, 0x00, 0x01]; // mod 31 != 0
    const bad_z4: [6]u8 = [0x78, 0xBB, 0x03, 0x00, 0x00, 0x01]; // FDICT set (0x78BB % 31 == 0)
    let bz1: bool = false;
    * = parseZlibHeader(bad_z1[0..]) catch |e| { bz1 = (e == GzipError.BadZlibHeader); };
    expect_true(bz1)?;
    let bz2: bool = false;
    * = parseZlibHeader(bad_z2[0..]) catch |e| { bz2 = (e == GzipError.BadZlibHeader); };
    expect_true(bz2)?;
    let bz3: bool = false;
    * = parseZlibHeader(bad_z3[0..]) catch |e| { bz3 = (e == GzipError.BadZlibHeader); };
    expect_true(bz3)?;
    let got_fdict: bool = false;
    * = inflateWithLimit(bad_z4[0..], 1024) catch |e| {
        got_fdict = (e == GzipError.PresetDictNotSupported);
    };
    expect_true(got_fdict)?;

    // 3.4 Single-bit checksum & ISIZE & FHCRC tamper detection
    let bad_crc_gz: [159]u8 = NODE_GZIP_L6;
    bad_crc_gz[151] = bad_crc_gz[151] ^ 0x01; // flip 1 bit in CRC32
    let got_bad_crc: bool = false;
    * = gunzipWithLimit(bad_crc_gz[0..], 4096) catch |e| {
        got_bad_crc = (e == GzipError.BadChecksum);
    };
    expect_true(got_bad_crc)?;

    let bad_isize_gz: [159]u8 = NODE_GZIP_L6;
    bad_isize_gz[155] = bad_isize_gz[155] ^ 0x01; // flip 1 bit in ISIZE
    let got_bad_isize: bool = false;
    * = gunzipWithLimit(bad_isize_gz[0..], 4096) catch |e| {
        got_bad_isize = (e == GzipError.BadIsize);
    };
    expect_true(got_bad_isize)?;

    let bad_hcrc_gz: [113]u8 = NODE_GZIP_FULL_HDR;
    bad_hcrc_gz[45] = bad_hcrc_gz[45] ^ 0x01; // flip 1 bit in FHCRC
    let got_bad_hcrc: bool = false;
    * = gunzipWithLimit(bad_hcrc_gz[0..], 4096) catch |e| {
        got_bad_hcrc = (e == GzipError.BadHeaderCrc);
    };
    expect_true(got_bad_hcrc)?;

    let bad_adler_z: [147]u8 = NODE_ZLIB_L6;
    bad_adler_z[146] = bad_adler_z[146] ^ 0x01; // flip 1 bit in Adler32
    let got_bad_adler: bool = false;
    * = inflateWithLimit(bad_adler_z[0..], 4096) catch |e| {
        got_bad_adler = (e == GzipError.BadChecksum);
    };
    expect_true(got_bad_adler)?;

    // 3.5 Output limit guard (resource exhaustion protection)
    const huge_len: usize = 65536;
    let huge_raw: [*]u8 = @alloc(huge_len);
    let hi: usize = 0;
    while (hi < huge_len) {
        unsafe { huge_raw[hi] = 0x41; } // 'A' repeated 64KB
        hi += 1;
    }
    const huge_src: []const u8 = unsafe { huge_raw[0..huge_len] };
    const huge_raw_def = deflateRaw(huge_src, 6);
    const huge_zlib = deflate(huge_src, 6);
    const huge_gz = gzip(huge_src, 6);
    expect_true(huge_raw_def.len < 1024)?;

    let lim_raw: bool = false;
    * = inflateRawWithLimit(huge_raw_def, 1024) catch |e| {
        lim_raw = (e == GzipError.OutputLimitExceeded);
    };
    expect_true(lim_raw)?;

    let lim_z: bool = false;
    * = inflateWithLimit(huge_zlib, 1024) catch |e| {
        lim_z = (e == GzipError.OutputLimitExceeded);
    };
    expect_true(lim_z)?;

    let lim_gz: bool = false;
    * = gunzipWithLimit(huge_gz, 1024) catch |e| {
        lim_gz = (e == GzipError.OutputLimitExceeded);
    };
    expect_true(lim_gz)?;

    // Verify full decode succeeds with adequate limit
    const full_ok = gunzipWithLimit(huge_gz, huge_len)?;
    expect_true(full_ok.len == huge_len)?;
}

// ============================================================================
// Test 4: 1,300 Deterministic PRNG Malformed Raw / Zlib / Gzip Fuzz Cases
// ============================================================================

fn nextPrng(state: *u64): u64 {
    let x: u64 = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn runTestMalformedFuzz(): void! {
    let rng: u64 = 0x5EED_1951_1952_CAFE;
    let buf: [256]u8 = [0; 256];
    let handled: usize = 0;

    let iter: usize = 0;
    while (iter < 1300) {
        const mode: u64 = nextPrng(&rng) % 5;
        let len: usize = 0;

        if (mode == 0) {
            // Mutate real Node.js Raw Deflate stream (dynamic Huffman + LZ77)
            len = NODE_RAW_L6.len;
            let i: usize = 0;
            while (i < len) { buf[i] = NODE_RAW_L6[i]; i += 1; }
            const pos: usize = (nextPrng(&rng) as usize) % len;
            buf[pos] = buf[pos] ^ (((nextPrng(&rng) & 0xFF) as u8) | 1);
            if ((nextPrng(&rng) & 3) == 0) {
                len = (nextPrng(&rng) as usize) % len;
            }
            * = inflateRawWithLimit(buf[0..len], 4096) catch |e| { * = e; };
        } else if (mode == 1) {
            // Mutate real Node.js Zlib stream
            len = NODE_ZLIB_L6.len;
            let i: usize = 0;
            while (i < len) { buf[i] = NODE_ZLIB_L6[i]; i += 1; }
            const pos: usize = (nextPrng(&rng) as usize) % len;
            buf[pos] = buf[pos] ^ (((nextPrng(&rng) & 0xFF) as u8) | 1);
            * = inflateWithLimit(buf[0..len], 4096) catch |e| { * = e; };
        } else if (mode == 2) {
            // Mutate real Node.js Concatenated Gzip stream (with FEXTRA/FNAME/FCOMMENT/FHCRC)
            len = NODE_GZIP_CONCAT.len;
            let i: usize = 0;
            while (i < len) { buf[i] = NODE_GZIP_CONCAT[i]; i += 1; }
            const pos: usize = (nextPrng(&rng) as usize) % len;
            buf[pos] = buf[pos] ^ (((nextPrng(&rng) & 0xFF) as u8) | 1);
            * = gunzipWithLimit(buf[0..len], 4096) catch |e| { * = e; };
        } else if (mode == 3) {
            // Synthetic Dynamic Huffman header fuzz (BTYPE=2)
            len = ((nextPrng(&rng) as usize) % 60) + 4;
            buf[0] = 0x05; // BFINAL=1, BTYPE=2 (Dynamic)
            let i: usize = 1;
            while (i < len) {
                buf[i] = (nextPrng(&rng) & 0xFF) as u8;
                i += 1;
            }
            * = inflateRawWithLimit(buf[0..len], 4096) catch |e| { * = e; };
        } else {
            // Pure random bytes across all 3 decoders
            len = (nextPrng(&rng) as usize) % 80;
            let i: usize = 0;
            while (i < len) {
                buf[i] = (nextPrng(&rng) & 0xFF) as u8;
                i += 1;
            }
            * = inflateRawWithLimit(buf[0..len], 4096) catch |e| { * = e; };
            * = inflateWithLimit(buf[0..len], 4096) catch |e| { * = e; };
            * = gunzipWithLimit(buf[0..len], 4096) catch |e| { * = e; };
        }

        handled += 1;
        iter += 1;
    }

    expect_true(handled == 1300)?;
}

fn main() {
    test("1. Node.js zlib/gzip L0/L1/L6/L9 + RFC 1952 full headers + concat + 70KB roundtrips", () => {
        runTestNodeVectorsAndRoundtrips()?;
    });
    test("2. 16-lane SIMD vs Scalar 0-byte-diff parity (Stored / LZ77 RLE+Block / MatchLen)", () => {
        runTestSimdParity()?;
    });
    test("3. miniz_oxide security boundaries (Stored/Huffman/Zlib/Gzip/CRC/Adler/OutputLimit)", () => {
        runTestSecurityBoundaries()?;
    });
    test("4. 1,300 deterministic PRNG malformed Raw/Zlib/Gzip fuzz cases", () => {
        runTestMalformedFuzz()?;
    });
}
