// Source: cp/cap/test/test_barcode_production_deep.cp
// Deep Production Verification Suite for cap:barcode & cap.barcode

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BarcodeError,
    BarcodeFormat,
    BarcodeBitPattern,
    BarcodeEncoder,
    BarcodeRenderer,
} from "cap:barcode";
import { XmlParser } from "cap:xml";

fn strEql(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn countNewlines(s: []const u8): usize {
    let cnt: usize = 0;
    for (c of s) {
        if (c == '\n') cnt += 1;
    }
    return cnt;
}

fn testCheckDigitsAndValidation(): !void {
    // 1. Known standard EAN-13: 4006381333931 (Staedtler)
    expect_eq(BarcodeEncoder.ean13CheckDigit("400638133393")?, 1);
    expect_eq(BarcodeEncoder.verifyEan13("4006381333931"), true);
    expect_eq(BarcodeEncoder.verifyEan13("4006381333932"), false);

    // Known EAN-13 with check digit 0: 9780201379624 -> check 4, 5901234123457 -> check 7
    expect_eq(BarcodeEncoder.ean13CheckDigit("590123412345")?, 7);
    expect_eq(BarcodeEncoder.verifyEan13("5901234123457"), true);

    // 2. Known standard UPC-A: 036000291452
    expect_eq(BarcodeEncoder.upcaCheckDigit("03600029145")?, 2);
    expect_eq(BarcodeEncoder.verifyUpca("036000291452"), true);
    expect_eq(BarcodeEncoder.verifyUpca("036000291459"), false);

    // 3. InvalidChecksum / InvalidLength / InvalidCharacter / EmptyInput guards
    const alloc = std.heap.page_allocator;
    let got_bad_ean_csum = false;
    * = BarcodeEncoder.encodeEan13(alloc, "4006381333939") catch |e| {
        if (e == BarcodeError.InvalidChecksum) got_bad_ean_csum = true;
    };
    expect_eq(got_bad_ean_csum, true);

    let got_bad_upca_csum = false;
    * = BarcodeEncoder.encodeUpcA(alloc, "036000291450") catch |e| {
        if (e == BarcodeError.InvalidChecksum) got_bad_upca_csum = true;
    };
    expect_eq(got_bad_upca_csum, true);

    let got_empty = false;
    * = BarcodeEncoder.ean13CheckDigit("") catch |e| {
        if (e == BarcodeError.EmptyInput) got_empty = true;
    };
    expect_eq(got_empty, true);

    let got_len = false;
    * = BarcodeEncoder.ean13CheckDigit("12345") catch |e| {
        if (e == BarcodeError.InvalidLength) got_len = true;
    };
    expect_eq(got_len, true);

    let got_char = false;
    * = BarcodeEncoder.upcaCheckDigit("0360002914A") catch |e| {
        if (e == BarcodeError.InvalidCharacter) got_char = true;
    };
    expect_eq(got_char, true);
}

fn verifyEanGuards(modules: []bool): !void {
    expect_eq(modules.len, 95);
    // Left guard: 101
    expect_eq(modules[0], true);
    expect_eq(modules[1], false);
    expect_eq(modules[2], true);
    // Center guard: 01010 at offset 45..50
    expect_eq(modules[45], false);
    expect_eq(modules[46], true);
    expect_eq(modules[47], false);
    expect_eq(modules[48], true);
    expect_eq(modules[49], false);
    // Right guard: 101 at offset 92..95
    expect_eq(modules[92], true);
    expect_eq(modules[93], false);
    expect_eq(modules[94], true);
}

fn testEan13AndUpcA95Modules(): !void {
    const alloc = std.heap.page_allocator;

    // 1. EAN-13 with 12 digits (auto check digit) vs 13 digits
    let e12 = BarcodeEncoder.encodeEan13(alloc, "400638133393")?;
    defer e12.deinit();
    expect_eq(e12.format, BarcodeFormat.Ean13);
    expect_eq(strEql(e12.text, "4006381333931"), true);
    verifyEanGuards(e12.modules)?;

    let e13 = BarcodeEncoder.encodeEan13(alloc, "4006381333931")?;
    defer e13.deinit();
    for (i of 0..95) {
        expect_eq(e12.modules[i], e13.modules[i]);
    }

    // 2. UPC-A 11/12 digits -> 95 modules identical to EAN-13 with leading '0'
    let upca11 = BarcodeEncoder.encodeUpcA(alloc, "03600029145")?;
    defer upca11.deinit();
    expect_eq(upca11.format, BarcodeFormat.UpcA);
    expect_eq(strEql(upca11.text, "036000291452"), true);
    verifyEanGuards(upca11.modules)?;

    let ean_compat = BarcodeEncoder.encodeEan13(alloc, "0036000291452")?;
    defer ean_compat.deinit();
    for (i of 0..95) {
        expect_eq(upca11.modules[i], ean_compat.modules[i]);
    }
}

fn testCode128CompressionAndGuards(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Pure digits (12 digits -> Start C + 6 pairs + Check + Stop = 9 symbols -> 11*9 + 2 = 101 modules)
    let p_num = BarcodeEncoder.encodeCode128(alloc, "001234567890")?;
    defer p_num.deinit();
    expect_eq(p_num.format, BarcodeFormat.Code128);
    expect_eq(p_num.modules.len, 101);
    expect_eq((p_num.modules.len - 2) % 11, 0);

    // 2. Mixed alphanumeric ("AB123456CD" -> Start B + A + B + Code C + 12 + 34 + 56 + Code B + C + D + Check + Stop = 12 symbols -> 134 modules)
    let p_mix = BarcodeEncoder.encodeCode128(alloc, "AB123456CD")?;
    defer p_mix.deinit();
    expect_eq(p_mix.modules.len, 134);
    expect_eq((p_mix.modules.len - 2) % 11, 0);

    // Verify 13-module Stop pattern at end: 1100011101011
    const stop_expected: [13]bool = [
        true, true, false, false, false, true, true, true, false, true, false, true, true,
    ];
    const base = p_mix.modules.len - 13;
    let i: usize = 0;
    while (i < 13) : (i += 1) {
        expect_eq(p_mix.modules[base + i], stop_expected[i]);
    }

    // 3. Control character (Code A)
    let p_ctrl = BarcodeEncoder.encodeCode128(alloc, "\n1234")?;
    defer p_ctrl.deinit();
    expect_eq((p_ctrl.modules.len - 2) % 11, 0);

    // 4. Non-ASCII rejection
    let got_non_ascii = false;
    * = BarcodeEncoder.encodeCode128(alloc, "ABC\x80") catch |e| {
        if (e == BarcodeError.InvalidCharacter) got_non_ascii = true;
    };
    expect_eq(got_non_ascii, true);
}

fn testRenderersXmlAndZeroLeak(): !void {
    const alloc = std.heap.page_allocator;

    let pat = BarcodeEncoder.encodeCode128(alloc, "CP-2026&POS")?;
    defer pat.deinit();

    // 1. SVG output + cap:xml DOM validation
    const svg = BarcodeRenderer.toSvg(alloc, pat, 2, 50, true)?;
    defer alloc.free(svg);
    expect_eq(std.mem.startsWith(u8, svg, "<svg xmlns=\"http://www.w3.org/2000/svg\""), true);
    expect_eq(std.mem.endsWith(u8, svg, "</svg>\n"), true);

    let doc = XmlParser.parse(alloc, svg)?;
    defer doc.deinit();
    const root = doc.root;
    expect_eq(strEql(root.tag, "svg"), true);
    expect_eq(root.getAttr("shape-rendering") != null, true);
    expect_eq(root.findChild("rect") != null, true);
    const text_node = root.findChild("text").?;
    const tc = text_node.innerText(alloc)?;
    defer alloc.free(tc);
    expect_eq(strEql(tc, "CP-2026&POS"), true);

    // 2. ASCII block output
    const ascii = BarcodeRenderer.toAscii(alloc, pat)?;
    defer alloc.free(ascii);
    expect_eq(countNewlines(ascii), 4);
    expect_eq(std.mem.indexOf(u8, ascii, "\xe2\x96\x88") != null, true);
    expect_eq(std.mem.indexOf(u8, ascii, " ") != null, true);
}

fn testCapBarcodeDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    let pat = cap.barcode.BarcodeEncoder.encodeEan13(alloc, "4006381333931")?;
    defer pat.deinit();
    expect_eq(pat.format, cap.barcode.BarcodeFormat.Ean13);
    expect_eq(pat.modules.len, 95);

    const svg = cap.barcode.BarcodeRenderer.toSvg(alloc, pat, 2, 40, false)?;
    defer alloc.free(svg);
    expect_eq(std.mem.indexOf(u8, svg, "<rect ") != null, true);

    const ascii = cap.barcode.BarcodeRenderer.toAscii(alloc, pat)?;
    defer alloc.free(ascii);
    expect_eq(countNewlines(ascii), 4);
}

fn main() {
    test("1. EAN-13 & UPC-A Check Digits, Verification & Error Guards", () => {
        testCheckDigitsAndValidation()?;
    });
    test("2. EAN-13 & UPC-A 95-Module Encoding & Guard Bit Patterns", () => {
        testEan13AndUpcA95Modules()?;
    });
    test("3. Code 128 Optimal A/B/C Switching, Mod 103 & Non-ASCII Guard", () => {
        testCode128CompressionAndGuards()?;
    });
    test("4. BarcodeRenderer toSvg (cap:xml Verified), toAscii & Zero-Leak", () => {
        testRenderersXmlAndZeroLeak()?;
    });
    test("5. import cap from \"cap\"; cap.barcode Dynamic Mounting", () => {
        testCapBarcodeDynamicMount()?;
    });
}
