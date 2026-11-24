// Reference-hardened against RazrFalcon/roxmltree & ECMA-376 OOXML (MIT OR Apache-2.0)
// Wave 37 R10 Verification Suite: cap:doc DOCX & PPTX OOXML Security, SIMD 0-Diff & 1600-Case PRNG Fuzz

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    DOCX_MAIN_CONTENT_TYPE,
    OOXML_MAX_PART_SIZE,
    scalarOoxmlFindTagOrAmp,
    simdOoxmlFindTagOrAmp,
    scalarOoxmlBytesEqual,
    simdOoxmlBytesEqual,
    isSafeOoxmlPartPath,
    DocxError,
    DocxParagraphIter,
    extractDocxText,
    summarizeDocx,
    validateDocxStrict,
} from "../../lib/doc/docx.cp";
import {
    PPTX_MAIN_CONTENT_TYPE,
    PptxError,
    PptxSlideRefIter,
    parsePptxSlideSize,
    extractSlideText,
    summarizeSlide,
    validatePptxStrict,
} from "../../lib/doc/pptx.cp";

// ---------------------------------------------------------------------------
// Deterministic PRNG & Minimal Zero-Alloc Stored-ZIP Builder for OOXML Tests
// ---------------------------------------------------------------------------

struct OoxmlFuzzRng {
    state: u64,

    pub static fn init(seed: u64): OoxmlFuzzRng {
        return OoxmlFuzzRng { state: if (seed == 0) 1 else seed };
    }

    pub fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

struct ZipPartSpec {
    name: []const u8,
    data: []const u8,
    override_uncomp: u32 = 0,
}

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn copySlice(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[off + i] = src[i];
    }
    return off + src.len;
}

fn buildStoredZip(parts: []const ZipPartSpec, out: []u8): usize {
    let pos: usize = 0;
    let lfh_offsets: [16]u32 = [0; 16];
    let p: usize = 0;
    while (p < parts.len) : (p += 1) {
        lfh_offsets[p] = pos as u32;
        const spec = parts[p];
        const comp_sz: u32 = spec.data.len as u32;
        const uncomp_sz: u32 = if (spec.override_uncomp != 0) spec.override_uncomp else comp_sz;
        // LFH signature PK\x03\x04
        out[pos] = 0x50;
        out[pos + 1] = 0x4B;
        out[pos + 2] = 0x03;
        out[pos + 3] = 0x04;
        putU16Le(out, pos + 4, 20);
        putU16Le(out, pos + 6, 0);
        putU16Le(out, pos + 8, 0); // STORE
        putU16Le(out, pos + 10, 0);
        putU16Le(out, pos + 12, 0);
        putU32Le(out, pos + 14, 0);
        putU32Le(out, pos + 18, comp_sz);
        putU32Le(out, pos + 22, uncomp_sz);
        putU16Le(out, pos + 26, spec.name.len as u16);
        putU16Le(out, pos + 28, 0);
        pos += 30;
        pos = copySlice(out, pos, spec.name);
        pos = copySlice(out, pos, spec.data);
    }

    const cd_start: u32 = pos as u32;
    p = 0;
    while (p < parts.len) : (p += 1) {
        const spec = parts[p];
        const comp_sz: u32 = spec.data.len as u32;
        const uncomp_sz: u32 = if (spec.override_uncomp != 0) spec.override_uncomp else comp_sz;
        // CD signature PK\x01\x02
        out[pos] = 0x50;
        out[pos + 1] = 0x4B;
        out[pos + 2] = 0x01;
        out[pos + 3] = 0x02;
        putU16Le(out, pos + 4, 20);
        putU16Le(out, pos + 6, 20);
        putU16Le(out, pos + 8, 0);
        putU16Le(out, pos + 10, 0);
        putU16Le(out, pos + 12, 0);
        putU16Le(out, pos + 14, 0);
        putU32Le(out, pos + 16, 0);
        putU32Le(out, pos + 20, comp_sz);
        putU32Le(out, pos + 24, uncomp_sz);
        putU16Le(out, pos + 28, spec.name.len as u16);
        putU16Le(out, pos + 30, 0);
        putU16Le(out, pos + 32, 0);
        putU16Le(out, pos + 34, 0);
        putU16Le(out, pos + 36, 0);
        putU32Le(out, pos + 38, 0);
        putU32Le(out, pos + 42, lfh_offsets[p]);
        pos += 46;
        pos = copySlice(out, pos, spec.name);
    }
    const cd_size: u32 = (pos as u32) - cd_start;

    // EOCD signature PK\x05\x06
    out[pos] = 0x50;
    out[pos + 1] = 0x4B;
    out[pos + 2] = 0x05;
    out[pos + 3] = 0x06;
    putU16Le(out, pos + 4, 0);
    putU16Le(out, pos + 6, 0);
    putU16Le(out, pos + 8, parts.len as u16);
    putU16Le(out, pos + 10, parts.len as u16);
    putU32Le(out, pos + 12, cd_size);
    putU32Le(out, pos + 16, cd_start);
    putU16Le(out, pos + 20, 0);
    return pos + 22;
}

fn docxCheckOk(data: []const u8): bool {
    validateDocxStrict(data) catch {
        return false;
    };
    return true;
}

fn pptxCheckOk(data: []const u8): bool {
    validatePptxStrict(data) catch {
        return false;
    };
    return true;
}

// ---------------------------------------------------------------------------
// Test 1: Synthetic DOCX Roundtrip + ZIP Traversal / Entity / Unclosed Guard
// ---------------------------------------------------------------------------

fn runTestDocxOoxmlHardening(): void! {
    const ct_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/></Types>";
    const doc_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><w:body><w:p><w:pPr><w:pStyle w:val=\"Heading1\"/></w:pPr><w:r><w:t>Hello &amp; &lt;DOCX&gt; &#65;&#x4E2D;</w:t></w:r></w:p><w:p><w:r><w:t>&quot;Quote&quot; &apos;OK&apos;</w:t><w:tab/><w:t>Col2</w:t><w:br/><w:t>Line2</w:t></w:r></w:p></w:body></w:document>";

    let zip_buf: [4096]u8 = [0; 4096];
    const parts: [2]ZipPartSpec = [
        ZipPartSpec { name: "[Content_Types].xml", data: ct_xml },
        ZipPartSpec { name: "word/document.xml", data: doc_xml },
    ];
    const zlen = buildStoredZip(parts[0..], zip_buf[0..]);
    expect_eq(true, docxCheckOk(zip_buf[0..zlen]));
    expect_eq(true, docxCheckOk(doc_xml));
    expect_eq(true, DOCX_MAIN_CONTENT_TYPE.len > 0);

    // Roundtrip text & paragraph extraction
    let text_buf: [512]u8 = [0; 512];
    const tlen = extractDocxText(doc_xml, text_buf[0..]) orelse return error.TestFailed;
    const expected = "Hello & <DOCX> A中\n\"Quote\" 'OK'\tCol2\nLine2";
    expect_eq(true, simdOoxmlBytesEqual(text_buf[0..tlen], expected));

    const stats = summarizeDocx(doc_xml);
    expect_eq(stats.paragraph_count, 2 as usize);
    expect_eq(stats.heading_count, 1 as usize);
    expect_eq(stats.run_count, 2 as usize);

    let pit = DocxParagraphIter.init(doc_xml);
    const p1 = pit.next() orelse return error.TestFailed;
    expect_eq(true, p1.is_heading);
    const p2 = pit.next() orelse return error.TestFailed;
    expect_eq(true, !p2.is_heading);
    expect_eq(true, pit.next() == null);

    // Safe / unsafe path validation
    expect_eq(true, isSafeOoxmlPartPath("word/document.xml"));
    expect_eq(true, isSafeOoxmlPartPath("[Content_Types].xml"));
    expect_eq(true, !isSafeOoxmlPartPath(""));
    expect_eq(true, !isSafeOoxmlPartPath("../word/document.xml"));
    expect_eq(true, !isSafeOoxmlPartPath("word/../../etc/passwd"));
    expect_eq(true, !isSafeOoxmlPartPath("/word/document.xml"));
    expect_eq(true, !isSafeOoxmlPartPath("C:/word/document.xml"));
    expect_eq(true, !isSafeOoxmlPartPath("word\\document.xml"));
    expect_eq(true, !isSafeOoxmlPartPath("word/doc\x00.xml"));

    // Path traversal inside ZIP must fail
    const bad_trav: [2]ZipPartSpec = [
        ZipPartSpec { name: "[Content_Types].xml", data: ct_xml },
        ZipPartSpec { name: "../word/document.xml", data: doc_xml },
    ];
    const trav_len = buildStoredZip(bad_trav[0..], zip_buf[0..]);
    expect_eq(true, !docxCheckOk(zip_buf[0..trav_len]));

    // Missing word/document.xml must fail
    const miss_parts: [1]ZipPartSpec = [
        ZipPartSpec { name: "[Content_Types].xml", data: ct_xml },
    ];
    const miss_len = buildStoredZip(miss_parts[0..], zip_buf[0..]);
    expect_eq(true, !docxCheckOk(zip_buf[0..miss_len]));

    // Truncated ZIP (missing EOCD) must fail
    expect_eq(true, !docxCheckOk(zip_buf[0..(zlen - 10)]));

    // Oversized uncompressed size (> 64 MiB) must fail
    const big_parts: [2]ZipPartSpec = [
        ZipPartSpec { name: "[Content_Types].xml", data: ct_xml },
        ZipPartSpec { name: "word/document.xml", data: doc_xml, override_uncomp: (OOXML_MAX_PART_SIZE as u32) + 1 },
    ];
    const big_len = buildStoredZip(big_parts[0..], zip_buf[0..]);
    expect_eq(true, !docxCheckOk(zip_buf[0..big_len]));

    // Invalid XML entities (surrogate, control char, unknown, bare &) & unclosed tag
    expect_eq(true, !docxCheckOk("<w:document><w:body><w:p><w:r><w:t>&#xD800;</w:t></w:r></w:p></w:body></w:document>"));
    expect_eq(true, !docxCheckOk("<w:document><w:body><w:p><w:r><w:t>&#x01;</w:t></w:r></w:p></w:body></w:document>"));
    expect_eq(true, !docxCheckOk("<w:document><w:body><w:p><w:r><w:t>&#0;</w:t></w:r></w:p></w:body></w:document>"));
    expect_eq(true, !docxCheckOk("<w:document><w:body><w:p><w:r><w:t>&unknown;</w:t></w:r></w:p></w:body></w:document>"));
    expect_eq(true, !docxCheckOk("<w:document><w:body><w:p><w:r><w:t>a & b</w:t></w:r></w:p></w:body></w:document>"));
    expect_eq(true, !docxCheckOk("<w:document><w:body><w:p></w:document>"));
}

// ---------------------------------------------------------------------------
// Test 2: Synthetic PPTX Roundtrip + ZIP Traversal / Missing Slide / Entity Guard
// ---------------------------------------------------------------------------

fn runTestPptxOoxmlHardening(): void! {
    const ct_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/></Types>";
    const pres_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><p:presentation xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><p:sldIdLst><p:sldId id=\"256\" r:id=\"rId2\"/></p:sldIdLst><p:sldSz cx=\"12192000\" cy=\"6858000\" type=\"screen16x9\"/></p:presentation>";
    const slide_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><p:sld xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\"><p:cSld><p:spTree><p:sp><p:txBody><a:p><a:r><a:t>Slide 1 &amp; &#x4E2D;&#65;</a:t></a:r></a:p><a:p><a:r><a:t>Item</a:t></a:r><a:br/><a:r><a:t>Next</a:t></a:r></a:p></p:txBody></p:sp></p:spTree></p:cSld></p:sld>";

    let zip_buf: [4096]u8 = [0; 4096];
    const parts: [3]ZipPartSpec = [
        ZipPartSpec { name: "[Content_Types].xml", data: ct_xml },
        ZipPartSpec { name: "ppt/presentation.xml", data: pres_xml },
        ZipPartSpec { name: "ppt/slides/slide1.xml", data: slide_xml },
    ];
    const zlen = buildStoredZip(parts[0..], zip_buf[0..]);
    expect_eq(true, pptxCheckOk(zip_buf[0..zlen]));
    expect_eq(true, pptxCheckOk(pres_xml));
    expect_eq(true, pptxCheckOk(slide_xml));
    expect_eq(true, PPTX_MAIN_CONTENT_TYPE.len > 0);

    // Roundtrip presentation & slide parsing
    const sz = parsePptxSlideSize(pres_xml) orelse return error.TestFailed;
    expect_eq(sz.cx, 12192000 as u64);
    expect_eq(sz.cy, 6858000 as u64);

    let rit = PptxSlideRefIter.init(pres_xml);
    const sref = rit.next() orelse return error.TestFailed;
    expect_eq(sref.id, 256 as u32);
    expect_eq(true, simdOoxmlBytesEqual(sref.r_id, "rId2"));
    expect_eq(true, rit.next() == null);

    let out: [512]u8 = [0; 512];
    const slen = extractSlideText(slide_xml, out[0..]) orelse return error.TestFailed;
    expect_eq(true, simdOoxmlBytesEqual(out[0..slen], "Slide 1 & 中A\nItem\nNext"));

    const sst = summarizeSlide(slide_xml);
    expect_eq(sst.shape_count, 1 as usize);
    expect_eq(sst.paragraph_count, 2 as usize);

    // Missing ppt/slides/slide*.xml must fail
    const no_slide: [2]ZipPartSpec = [
        ZipPartSpec { name: "[Content_Types].xml", data: ct_xml },
        ZipPartSpec { name: "ppt/presentation.xml", data: pres_xml },
    ];
    const ns_len = buildStoredZip(no_slide[0..], zip_buf[0..]);
    expect_eq(true, !pptxCheckOk(zip_buf[0..ns_len]));

    // Path traversal inside PPTX ZIP must fail
    const bad_trav: [3]ZipPartSpec = [
        ZipPartSpec { name: "[Content_Types].xml", data: ct_xml },
        ZipPartSpec { name: "ppt/presentation.xml", data: pres_xml },
        ZipPartSpec { name: "ppt/slides/../../evil.xml", data: slide_xml },
    ];
    const bt_len = buildStoredZip(bad_trav[0..], zip_buf[0..]);
    expect_eq(true, !pptxCheckOk(zip_buf[0..bt_len]));

    // Surrogate entity, control char & unclosed tag in slide XML must fail
    expect_eq(true, !pptxCheckOk("<p:sld><a:p><a:t>&#xDFFF;</a:t></a:p></p:sld>"));
    expect_eq(true, !pptxCheckOk("<p:sld><a:p><a:t>&#7;</a:t></a:p></p:sld>"));
    expect_eq(true, !pptxCheckOk("<p:sld><a:p><a:t>unclosed</p:sld>"));
}

// ---------------------------------------------------------------------------
// Test 3: 16-lane SIMD vs Scalar 0-Diff Differential Verification
// ---------------------------------------------------------------------------

fn runTestOoxmlSimdZeroDiff(): void! {
    let a: [64]u8 = [0; 64];
    let b: [64]u8 = [0; 64];
    let len: usize = 0;
    while (len <= 64) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            a[i] = (97 + (i % 26)) as u8;
            b[i] = a[i];
        }
        // No-hit find
        expect_eq(simdOoxmlFindTagOrAmp(a[0..len]), scalarOoxmlFindTagOrAmp(a[0..len]));
        // Equal compare
        expect_eq(simdOoxmlBytesEqual(a[0..len], b[0..len]), scalarOoxmlBytesEqual(a[0..len], b[0..len]));
        expect_eq(true, simdOoxmlBytesEqual(a[0..len], b[0..len]));

        let pos: usize = 0;
        while (pos < len) : (pos += 1) {
            const orig = a[pos];
            // Test '<', '>', '&' at every index
            a[pos] = '<';
            expect_eq(simdOoxmlFindTagOrAmp(a[0..len]), scalarOoxmlFindTagOrAmp(a[0..len]));
            a[pos] = '>';
            expect_eq(simdOoxmlFindTagOrAmp(a[0..len]), scalarOoxmlFindTagOrAmp(a[0..len]));
            a[pos] = '&';
            expect_eq(simdOoxmlFindTagOrAmp(a[0..len]), scalarOoxmlFindTagOrAmp(a[0..len]));
            a[pos] = orig;

            // Single-byte mismatch at every index
            b[pos] = orig ^ 0x01;
            expect_eq(simdOoxmlBytesEqual(a[0..len], b[0..len]), scalarOoxmlBytesEqual(a[0..len], b[0..len]));
            expect_eq(true, !simdOoxmlBytesEqual(a[0..len], b[0..len]));
            b[pos] = orig;
        }
    }
}

// ---------------------------------------------------------------------------
// Test 4: 1,600-Case Deterministic PRNG Fuzz (800 DOCX + 800 PPTX)
// ---------------------------------------------------------------------------

fn runTestOoxmlFuzz1600(): void! {
    let rng = OoxmlFuzzRng.init(0x50C5_7788_1234_5678);
    let buf: [160]u8 = [0; 160];
    let buf2: [160]u8 = [0; 160];
    let text_out: [256]u8 = [0; 256];

    // 800 DOCX fuzz cases
    let c: usize = 0;
    while (c < 800) : (c += 1) {
        const flen = ((rng.next() % 140) as usize) + 1;
        let i: usize = 0;
        while (i < flen) : (i += 1) {
            const r = rng.next() % 12;
            if (r == 0) {
                buf[i] = '<';
            } else if (r == 1) {
                buf[i] = '>';
            } else if (r == 2) {
                buf[i] = '&';
            } else if (r == 3) {
                buf[i] = ';';
            } else if (r == 4) {
                buf[i] = '/';
            } else if (r == 5) {
                buf[i] = '.';
            } else if (r == 6) {
                buf[i] = '#';
            } else {
                buf[i] = (rng.next() & 0x7F) as u8;
            }
            buf2[i] = buf[i];
        }
        if ((c % 4) == 0 && flen >= 4) {
            buf[0] = 0x50;
            buf[1] = 0x4B;
            buf[2] = 0x03;
            buf[3] = 0x04;
        }
        const sl = buf[0..flen];
        expect_eq(simdOoxmlFindTagOrAmp(sl), scalarOoxmlFindTagOrAmp(sl));
        expect_eq(simdOoxmlBytesEqual(sl, buf2[0..flen]), scalarOoxmlBytesEqual(sl, buf2[0..flen]));
        * = docxCheckOk(sl);
        * = extractDocxText(sl, text_out[0..]);
        * = isSafeOoxmlPartPath(sl);
    }

    // 800 PPTX fuzz cases
    let p: usize = 0;
    while (p < 800) : (p += 1) {
        const flen = ((rng.next() % 140) as usize) + 1;
        let i: usize = 0;
        while (i < flen) : (i += 1) {
            const r = rng.next() % 12;
            if (r == 0) {
                buf[i] = '<';
            } else if (r == 1) {
                buf[i] = '>';
            } else if (r == 2) {
                buf[i] = '&';
            } else if (r == 3) {
                buf[i] = ':';
            } else if (r == 4) {
                buf[i] = '/';
            } else if (r == 5) {
                buf[i] = 'p';
            } else if (r == 6) {
                buf[i] = 'a';
            } else {
                buf[i] = (rng.next() & 0x7F) as u8;
            }
            buf2[i] = buf[i];
        }
        if ((p % 4) == 0 && flen >= 4) {
            buf[0] = 0x50;
            buf[1] = 0x4B;
            buf[2] = 0x03;
            buf[3] = 0x04;
        }
        if ((p % 3) == 0 && flen > 0) {
            buf2[flen / 2] ^= 0x01;
        }
        const sl = buf[0..flen];
        expect_eq(simdOoxmlFindTagOrAmp(sl), scalarOoxmlFindTagOrAmp(sl));
        expect_eq(simdOoxmlBytesEqual(sl, buf2[0..flen]), scalarOoxmlBytesEqual(sl, buf2[0..flen]));
        * = pptxCheckOk(sl);
        * = extractSlideText(sl, text_out[0..]);
    }
}

fn main() {
    test("DOCX OOXML synthetic ZIP roundtrip, path traversal, limits & entity guard", () => {
        runTestDocxOoxmlHardening()?;
    });
    test("PPTX OOXML synthetic ZIP roundtrip, missing slide, traversal & entity guard", () => {
        runTestPptxOoxmlHardening()?;
    });
    test("16-lane OOXML SIMD vs Scalar 0-diff differential verification", () => {
        runTestOoxmlSimdZeroDiff()?;
    });
    test("1600-case deterministic PRNG Fuzz (800 DOCX + 800 PPTX)", () => {
        runTestOoxmlFuzz1600()?;
    });
}
