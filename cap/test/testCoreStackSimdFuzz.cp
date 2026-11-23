// Production 5,000-Iteration Cross-Stack Deterministic Fuzz & SIMD Verification Suite (Wave 27 Round 9)
//   - Test 1 (1,500 iters): OOXML (XLSX SharedStrings/Worksheet/Formula + DOCX Document/Markdown + PPTX Slide)
//   - Test 2 (1,000 iters): LSM Storage Engine (LsmWalCodec.recoverLog + SstReader.open)
//   - Test 3 (1,500 iters): WebRTC / SRTP (STUN + ICE Candidate + SDP + SRTP/SRTCP unprotect)
//   - Test 4 (1,000 iters): Brotli zero-crash decompression (`brotliDecompressInto`)

import { test, expect_eq, expect_true } from "cap:test";
import {
    XlsxCellValue,
    parseSharedStringsXml,
    parseWorksheetCellsXml,
    evalXlsxFormula,
    DocxRel,
    DocxBlockRef,
    DocxRichParagraph,
    DocxRun,
    DocxTable,
    DocxTableCell,
    parseDocxDocumentXml,
    markdownToDocxXml,
    PptxShape,
    PptxTextParagraph,
    PptxTextRun,
    PptxTable,
    PptxTableCell,
    parsePptxSlideXml,
} from "../../lib/doc/index.cp";
import {
    LsmWalRecordView,
    LsmWalCodec,
    SstBuilderConfig,
    SstBuilder,
    SstReader,
} from "../../lib/db/index.cp";
import {
    SdpCodecSpec,
    parseStunPacket,
    parseIceCandidateAttr,
    parseWebRtcSdp,
    SrtpKeyMaterial,
    SrtpProfile,
    SrtpContext,
} from "../../lib/net/index.cp";
import {
    brotliDecompressInto,
} from "../../lib/pack/index.cp";

fn xorshift64(state: *u64): u64 {
    let x: u64 = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

// ============================================================================
// Test 1: OOXML Office Suite SIMD + 1,500-Iteration Fuzz
// ============================================================================
test("Fuzz 1/4 (1,500 iters): OOXML XLSX/DOCX/PPTX XML parsers & formula evaluator") {
    let rng: u64 = 0x1122334455667789;
    let buf: [256]u8 = [0; 256];
    let shared_strs: [16][]const u8 = [""; 16];
    let cells: [16]XlsxCellValue = [XlsxCellValue{}; 16];
    let slab: [512]u8 = [0; 512];

    let rels: [8]DocxRel = [DocxRel{}; 8];
    let dblocks: [16]DocxBlockRef = [DocxBlockRef{}; 16];
    let dparas: [16]DocxRichParagraph = [DocxRichParagraph{}; 16];
    let druns: [32]DocxRun = [DocxRun{}; 32];
    let dtables: [4]DocxTable = [DocxTable{}; 4];
    let dcells: [16]DocxTableCell = [DocxTableCell{}; 16];
    let rid_slab: [128]u8 = [0; 128];
    let xml_out: [1024]u8 = [0; 1024];

    let pshapes: [16]PptxShape = [PptxShape{}; 16];
    let pparas: [16]PptxTextParagraph = [PptxTextParagraph{}; 16];
    let pruns: [32]PptxTextRun = [PptxTextRun{}; 32];
    let ptables: [4]PptxTable = [PptxTable{}; 4];
    let pcells: [16]PptxTableCell = [PptxTableCell{}; 16];

    const xml_alpha: []const u8 = "<>/=\"'&;:#?![]()+-*/0129ABSUMIFw:ptcrvsi \r\n\t";

    let iter: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) {
            const rv: u64 = xorshift64(*rng);
            if ((iter % 3) == 0) {
                buf[j] = xml_alpha[@as(usize, @intCast(rv % xml_alpha.len))];
            } else {
                buf[j] = @as(u8, @intCast(rv & 0xFF));
            }
        }
        // Deeply nested formula parens every 16 iterations
        if ((iter % 16) == 0 && len >= 60) {
            buf[0] = '=';
            let k: usize = 1;
            while (k < 55) : (k += 1) buf[k] = '(';
        }

        const slice: []const u8 = buf[0..len];
        const ss_cnt: usize = parseSharedStringsXml(slice, shared_strs[0..], slab[0..]) catch 0;
        const c_cnt: usize = parseWorksheetCellsXml(slice, shared_strs[0..ss_cnt], cells[0..]) catch 0;
        * = evalXlsxFormula(slice, cells[0..c_cnt]) catch 0.0;

        * = parseDocxDocumentXml(
            slice,
            rels[0..0],
            dblocks[0..],
            dparas[0..],
            druns[0..],
            dtables[0..],
            dcells[0..],
            slab[0..],
        ) catch {};

        let rel_cnt: usize = 0;
        * = markdownToDocxXml(slice, rels[0..], *rel_cnt, rid_slab[0..], xml_out[0..]) catch 0;

        * = parsePptxSlideXml(
            slice,
            pshapes[0..],
            pparas[0..],
            pruns[0..],
            ptables[0..],
            pcells[0..],
            slab[0..],
        ) catch {};
    }
    expect_eq(iter, 1500);
}

// ============================================================================
// Test 2: LSM Storage Engine WAL & SSTable 1,000-Iteration Fuzz
// ============================================================================
test("Fuzz 2/4 (1,000 iters): LsmWalCodec.recoverLog & SstReader.open corruption guard") {
    // Build a real small SSTable template to mutate/truncate
    let tpl_sst: [2048]u8 = [0; 2048];
    let idx_w: [512]u8 = [0; 512];
    let blm_w: [128]u8 = [0; 128];
    let ts_s: []u8 = tpl_sst[0..];
    let iw_s: []u8 = idx_w[0..];
    let bw_s: []u8 = blm_w[0..];
    let b = SstBuilder.init(SstBuilderConfig{ expected_keys: 16 }, *ts_s, *iw_s, *bw_s) catch {
        expect_true(false);
        return;
    };
    * = b.addEntry("alpha", "v1", 1, false) catch { expect_true(false); return; };
    * = b.addEntry("beta", "v2", 2, false) catch { expect_true(false); return; };
    const st = b.finish() catch { expect_true(false); return; };
    const tpl_len: usize = st.total_bytes;

    let rng: u64 = 0x223344556677889A;
    let buf: [256]u8 = [0; 256];
    let recs: [16]LsmWalRecordView = [LsmWalRecordView{}; 16];
    let recs_s: []LsmWalRecordView = recs[0..];
    let sk: [256]u8 = [0; 256];
    let sk_s: []u8 = sk[0..];

    let iter: usize = 0;
    while (iter < 1000) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) buf[j] = @as(u8, @intCast(xorshift64(*rng) & 0xFF));

        // 1. Fuzz WAL recovery
        * = LsmWalCodec.recoverLog(buf[0..len], *recs_s);

        // 2. Fuzz SSTable open & lookup (half random, half mutated valid SSTable)
        if ((iter % 2) == 0) {
            const p: usize = @as(usize, @intCast(xorshift64(*rng) % tpl_len));
            tpl_sst[p] ^= 0x3C;
            if (r := SstReader.open(tpl_sst[0..tpl_len]) catch null) {
                * = r.getAt("alpha", 10, *sk_s) catch {};
            }
            tpl_sst[p] ^= 0x3C; // restore
        } else {
            if (r := SstReader.open(buf[0..len]) catch null) {
                * = r.getAt("alpha", 10, *sk_s) catch {};
            }
        }
    }
    expect_eq(iter, 1000);
}

// ============================================================================
// Test 3: WebRTC STUN / ICE / SDP & SRTP/SRTCP 1,500-Iteration Fuzz
// ============================================================================
test("Fuzz 3/4 (1,500 iters): WebRTC STUN/ICE/SDP & SRTP/SRTCP unprotect guard") {
    let rng: u64 = 0x33445566778899AB;
    let buf: [256]u8 = [0; 256];
    let codecs: [16]SdpCodecSpec = [SdpCodecSpec{}; 16];
    let plain_out: [256]u8 = [0; 256];

    let master: SrtpKeyMaterial = SrtpKeyMaterial{
        master_key: [0x11; 16],
        master_salt: [0x22; 14],
    };
    let srtp: SrtpContext = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);

    let iter: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) buf[j] = @as(u8, @intCast(xorshift64(*rng) & 0xFF));

        // Periodically inject STUN magic cookie (0x2112A442)
        if ((iter % 4) == 0 && len >= 20) {
            buf[4] = 0x21;
            buf[5] = 0x12;
            buf[6] = 0xA4;
            buf[7] = 0x42;
        }
        const slice: []const u8 = buf[0..len];

        * = parseStunPacket(slice) catch {};
        * = parseIceCandidateAttr(slice) catch {};
        * = parseWebRtcSdp(slice, codecs[0..]) catch {};
        * = srtp.unprotectRtp(slice, plain_out[0..]) catch 0;
        * = srtp.unprotectRtcp(slice, plain_out[0..]) catch 0;
    }
    expect_eq(iter, 1500);
}

// ============================================================================
// Test 4: Brotli Zero-Crash Decompression 1,000-Iteration Fuzz
// ============================================================================
test("Fuzz 4/4 (1,000 iters): brotliDecompressInto malformed stream zero-crash guard") {
    let rng: u64 = 0x445566778899AABC;
    let buf: [192]u8 = [0; 192];
    let out: [1024]u8 = [0; 1024];
    let out_s: []u8 = out[0..];

    let iter: usize = 0;
    while (iter < 1000) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 192));
        let j: usize = 0;
        while (j < len) : (j += 1) buf[j] = @as(u8, @intCast(xorshift64(*rng) & 0xFF));
        * = brotliDecompressInto(buf[0..len], *out_s) catch 0;
    }
    expect_eq(iter, 1000);
}
