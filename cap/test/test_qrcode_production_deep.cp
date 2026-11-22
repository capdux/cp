// Source: cp/cap/test/test_qrcode_production_deep.cp
// Deep Production Verification Suite for cap:qrcode & cap.qrcode

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ReedSolomon,
    QrError,
    QrEccLevel,
    QrMatrix,
    QrCode,
    QrRenderer,
} from "cap:image";
import { Totp, TotpConfig } from "cap:security";

fn strEql(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn verifyFinderAt(mat: QrMatrix, ox: usize, oy: usize): !void {
    let dy: usize = 0;
    while (dy < 7) : (dy += 1) {
        let dx: usize = 0;
        while (dx < 7) : (dx += 1) {
            const expected = (dx == 0 || dx == 6 || dy == 0 || dy == 6) ||
                (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4);
            expect_eq(mat.get(ox + dx, oy + dy), expected);
        }
    }
}

fn verifyQrFunctionPatterns(mat: QrMatrix): !void {
    const s = mat.size;
    // 3 Finder patterns (7x7)
    verifyFinderAt(mat, 0, 0)?;
    verifyFinderAt(mat, s - 7, 0)?;
    verifyFinderAt(mat, 0, s - 7)?;

    // 1-module white separators around finders
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        expect_eq(mat.get(7, i), false);
        expect_eq(mat.get(i, 7), false);
        expect_eq(mat.get(s - 8, i), false);
        expect_eq(mat.get(s - 8 + i, 7), false);
        expect_eq(mat.get(7, s - 8 + i), false);
        expect_eq(mat.get(i, s - 8), false);
    }

    // Timing patterns on row 6 and column 6
    let t: usize = 8;
    while (t < s - 8) : (t += 1) {
        const expected = (t % 2) == 0;
        expect_eq(mat.get(t, 6), expected);
        expect_eq(mat.get(6, t), expected);
    }

    // Always-dark module at (8, size - 8)
    expect_eq(mat.get(8, s - 8), true);
}

fn countNewlines(s: []const u8): usize {
    let cnt: usize = 0;
    for (c of s) {
        if (c == '\n') cnt += 1;
    }
    return cnt;
}

fn testReedSolomonVectors(): !void {
    // 1. GF(256) arithmetic invariants under primitive polynomial 0x11D
    expect_eq(ReedSolomon.gfMul(0, 0x57), 0);
    expect_eq(ReedSolomon.gfMul(1, 0xAB), 0xAB);
    expect_eq(ReedSolomon.gfMul(0x8E, 2), 1); // (0x8E << 1) ^ 0x11D == 1
    expect_eq(ReedSolomon.gfMul(0x57, 0x83), 0x31); // QR GF(256) mod 0x11D == 0x31 (49)

    // 2. ISO/IEC 18004 standard Version 1-M ("HELLO WORLD" 16 data codewords -> 10 EC codewords)
    const data_v1m: [16]u8 = [32, 91, 11, 120, 209, 114, 220, 77, 67, 64, 236, 17, 236, 17, 236, 17];
    const expected_v1m: [10]u8 = [196, 35, 39, 119, 235, 215, 231, 226, 93, 23];
    let out_v1m: [10]u8 = [0; 10];
    ReedSolomon.computeEcc(&data_v1m, 10, &out_v1m);
    for (i of 0..10) {
        expect_eq(out_v1m[i], expected_v1m[i]);
    }

    // 3. Generator degree 7 test (Version 1-L: 19 data codewords -> 7 EC codewords)
    const data_v1l: [19]u8 = [
        0x10, 0x20, 0x0C, 0x56, 0x61, 0x80, 0xEC, 0x11, 0xEC, 0x11,
        0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC,
    ];
    let out_v1l: [7]u8 = [0; 7];
    ReedSolomon.computeEcc(&data_v1l, 7, &out_v1l);
    const expected_v1l: [7]u8 = [0x53, 0x55, 0x97, 0x67, 0x10, 0x05, 0x84];
    for (i of 0..7) {
        expect_eq(out_v1l[i], expected_v1l[i]);
    }
}

fn testQrEncodeVersionsAndTotp(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Version 1 ("HELLO", Medium -> 21x21)
    let m1 = QrCode.encode(alloc, "HELLO", .Medium)?;
    defer m1.deinit();
    expect_eq(m1.version, 1);
    expect_eq(m1.size, 21);
    expect_eq(m1.modules.len, 441);
    expect_eq(m1.mask < 8, true);
    verifyQrFunctionPatterns(m1)?;

    // 2. Integration with cap:security TOTP otpauth:// URI -> automatic version upscaling
    const totp_uri = Totp.buildOtpAuthUri(
        alloc,
        "CPAuth",
        "alice@cp-lang.org",
        "JBSWY3DPEHPK3PXP",
        TotpConfig.initDefault(),
    )?;
    defer alloc.free(totp_uri);

    let m_totp = QrCode.encode(alloc, totp_uri, .Medium)?;
    defer m_totp.deinit();
    expect_eq(m_totp.version >= 5, true);
    expect_eq(m_totp.size, 17 + 4 * usize(m_totp.version));
    verifyQrFunctionPatterns(m_totp)?;

    // 3. Version 7..10 multi-block interleaving + alignment + version info patterns
    let big_payload: [220]u8 = ['A'; 220];
    let m_big = QrCode.encode(alloc, &big_payload, .Low)?;
    defer m_big.deinit();
    expect_eq(m_big.version >= 9, true);
    expect_eq(m_big.size, 17 + 4 * usize(m_big.version));
    verifyQrFunctionPatterns(m_big)?;
}

fn testEccLevelsAndBoundaries(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Exact capacities
    expect_eq(QrCode.maxByteCapacity(1, .Low), 17);
    expect_eq(QrCode.maxByteCapacity(1, .Medium), 14);
    expect_eq(QrCode.maxByteCapacity(1, .Quartile), 11);
    expect_eq(QrCode.maxByteCapacity(1, .High), 7);
    expect_eq(QrCode.maxByteCapacity(10, .Low), 271);
    expect_eq(QrCode.maxByteCapacity(10, .High), 119);
    expect_eq(QrCode.maxByteCapacity(0, .Low), 0);
    expect_eq(QrCode.maxByteCapacity(11, .Low), 0);

    // 2. Same payload across L, M, Q, H scales version monotonically
    const url = "https://cp-lang.org/docs/qrcode?v=2&mode=byte";
    let ml = QrCode.encode(alloc, url, .Low)?;
    defer ml.deinit();
    let mm = QrCode.encode(alloc, url, .Medium)?;
    defer mm.deinit();
    let mq = QrCode.encode(alloc, url, .Quartile)?;
    defer mq.deinit();
    let mh = QrCode.encode(alloc, url, .High)?;
    defer mh.deinit();

    expect_eq(ml.version <= mm.version, true);
    expect_eq(mm.version <= mq.version, true);
    expect_eq(mq.version <= mh.version, true);
    expect_eq(mh.version > ml.version, true);

    // 3. DataTooLong boundary guard (>271 bytes for Low, >119 bytes for High)
    let too_long_low: [272]u8 = ['Z'; 272];
    let got_too_long_low = false;
    * = QrCode.encode(alloc, &too_long_low, .Low) catch |e| {
        if (e == QrError.DataTooLong) got_too_long_low = true;
    };
    expect_eq(got_too_long_low, true);

    let too_long_high: [120]u8 = ['Z'; 120];
    let got_too_long_high = false;
    * = QrCode.encode(alloc, &too_long_high, .High) catch |e| {
        if (e == QrError.DataTooLong) got_too_long_high = true;
    };
    expect_eq(got_too_long_high, true);

    // 4. InvalidVersion boundary guard
    let got_invalid_v0 = false;
    * = QrCode.encodeVersion(alloc, "hi", 0, .Low) catch |e| {
        if (e == QrError.InvalidVersion) got_invalid_v0 = true;
    };
    expect_eq(got_invalid_v0, true);

    let got_invalid_v11 = false;
    * = QrCode.encodeVersion(alloc, "hi", 11, .Low) catch |e| {
        if (e == QrError.InvalidVersion) got_invalid_v11 = true;
    };
    expect_eq(got_invalid_v11, true);
}

fn testRenderers(): !void {
    const alloc = std.heap.page_allocator;

    let mat = QrCode.encode(alloc, "HELLO", .Medium)?;
    defer mat.deinit();

    // 1. UTF-8 Half-block terminal output
    const term = QrRenderer.toTerminal(alloc, mat, 2)?;
    defer alloc.free(term);
    // 21 + 2*2 = 25 rows -> ceil(25/2) = 13 lines
    expect_eq(countNewlines(term), 13);
    expect_eq(std.mem.indexOf(u8, term, "\xe2\x96\x88") != null, true);
    expect_eq(std.mem.indexOf(u8, term, "\xe2\x96\x80") != null, true);
    expect_eq(std.mem.indexOf(u8, term, "\xe2\x96\x84") != null, true);

    // 2. ASCII grid output
    const ascii = QrRenderer.toAscii(alloc, mat, 1)?;
    defer alloc.free(ascii);
    expect_eq(countNewlines(ascii), mat.size + 2);
    expect_eq(std.mem.indexOf(u8, ascii, "##") != null, true);
    expect_eq(std.mem.indexOf(u8, ascii, "  ") != null, true);

    // 3. SVG vector output
    const svg = QrRenderer.toSvg(alloc, mat, 4, 2)?;
    defer alloc.free(svg);
    expect_eq(std.mem.startsWith(u8, svg, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\""), true);
    expect_eq(std.mem.indexOf(u8, svg, "<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>") != null, true);
    expect_eq(std.mem.indexOf(u8, svg, "<path d=\"M") != null, true);
    expect_eq(std.mem.endsWith(u8, svg, "</svg>\n"), true);
}

fn testCapQrcodeDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    expect_eq(cap.image.ReedSolomon.gfMul(0x8E, 2), 1);

    let mat = cap.image.QrCode.encode(alloc, "https://cp-lang.org", .Quartile)?;
    defer mat.deinit();
    expect_eq(mat.version >= 1, true);
    verifyQrFunctionPatterns(mat)?;

    const term = cap.image.QrRenderer.toTerminal(alloc, mat, 1)?;
    defer alloc.free(term);
    expect_eq(countNewlines(term), (mat.size + 3) / 2);

    const svg = cap.image.QrRenderer.toSvg(alloc, mat, 2, 1)?;
    defer alloc.free(svg);
    expect_eq(std.mem.startsWith(u8, svg, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\""), true);
    expect_eq(strEql(svg[svg.len - 7 ..], "</svg>\n"), true);
}

fn main() {
    test("1. GF(256) Arithmetic & ReedSolomon Standard Vectors", () => {
        testReedSolomonVectors()?;
    });
    test("2. QrCode.encode V1 (HELLO), TOTP otpauth:// Auto-Scaling & Function Patterns", () => {
        testQrEncodeVersionsAndTotp()?;
    });
    test("3. ECC Levels (L/M/Q/H), Capacities & DataTooLong/InvalidVersion Guards", () => {
        testEccLevelsAndBoundaries()?;
    });
    test("4. QrRenderer toTerminal (UTF-8 Half-Block), toAscii & toSvg Vector Output", () => {
        testRenderers()?;
    });
    test("5. import cap from \"cap\"; cap.qrcode Automatic Dynamic Mounting", () => {
        testCapQrcodeDynamicMount()?;
    });
}
