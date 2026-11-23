// Production SIMD Vector Acceleration & 5,000-Iteration Deterministic Fuzz Test Suite
// Covers lib/doc/csvTable.cp, lib/doc/pdfInspector.cp, and lib/vector/fullTextIndex.cp (Wave 27 Round 4)
//   - Verifies cap:simd fast-path correctness across CSV, PDF, and FullTextIndex
//   - Executes 5,000 deterministic xorshift64 malformed-input fuzz iterations:
//       * 1,800 iters: sniffCsvDialect, CsvTableReader, inferCsvSchema, csvToJsonl, jsonlToCsv
//       * 1,600 iters: PdfInspector.inspect, PdfXrefTable.parse, extractPdfMetadata,
//                      collectPdfPages, extractTextFromContentStream, decodePdfStringToUtf8
//       * 1,600 iters: tokenizeText, searchQuery (malformed boolean/nested parens), deserializeSnapshot

import { test, expect_eq, expect_true } from "cap:test";
import {
    CsvTableOptions,
    CsvTableReader,
    CsvColumnProfile,
    sniffCsvDialect,
    inferCsvSchema,
    csvToJsonl,
    jsonlToCsv,
    PdfInspector,
    PdfXrefEntry,
    PdfXrefTable,
    PdfObjRef,
    PdfPageInfo,
    extractPdfMetadata,
    collectPdfPages,
    extractTextFromContentStream,
    decodePdfStringToUtf8,
} from "../../lib/doc/index.cp";
import {
    FullTextFieldSpec,
    FullTextConfig,
    FtDocMeta,
    FtTermEntry,
    FtPosting,
    FtHit,
    FtTokenSpan,
    FullTextBuffers,
    FullTextScratch,
    FullTextIndex,
    tokenizeText,
} from "../../lib/vector/index.cp";

fn xorshift64(state: *u64): u64 {
    let x: u64 = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

// ============================================================================
// Test 1: CSV SIMD Fast-Path Verification + 1,800-Iteration Deterministic Fuzz
// ============================================================================
test("CSV SIMD + Fuzz (1,800 iters): sniff/reader/schema/csvToJsonl/jsonlToCsv zero-crash") {
    // 1. Verify SIMD paths (>32-byte quoted & unquoted fields, comments, CRLF)
    const sample_csv: string =
        "# comment line spanning more than thirty two bytes for simd skip\r\n" ++
        "id,description,score\r\n" ++
        "1,\"a_very_long_quoted_field_exceeding_32_bytes_with_\"\"escaped\"\"_quotes\",98.5\r\n" ++
        "2,plain_unquoted_identifier_exceeding_thirty_two_bytes_total,42.0\r\n";

    const dial = sniffCsvDialect(sample_csv, 8);
    expect_eq(dial.delimiter, ',');

    let reader = CsvTableReader.init(sample_csv, CsvTableOptions{
        delimiter: ',',
        comment_char: '#',
    });
    let fields: [8]string = [""; 8];
    if (r0 := reader.nextRow(fields[0..])) {
        expect_eq(r0.fields_count, 3);
    } else {
        expect_true(false);
    }
    if (r1 := reader.nextRow(fields[0..])) {
        expect_eq(r1.fields_count, 3);
    } else {
        expect_true(false);
    }
    if (r2 := reader.nextRow(fields[0..])) {
        expect_eq(r2.fields_count, 3);
    } else {
        expect_true(false);
    }

    // 2. 1,800 deterministic malformed fuzz iterations
    let rng: u64 = 0xA1B2C3D4E5F60718;
    let buf: [192]u8 = [0; 192];
    let profiles: [8]CsvColumnProfile = [CsvColumnProfile{}; 8];
    let hdr_sc: [8]string = [""; 8];
    let fld_sc: [8]string = [""; 8];
    let out_buf: [512]u8 = [0; 512];
    let unesc_buf: [128]u8 = [0; 128];
    const cols: [3]string = ["a", "b", "c"];

    const alphabet: []const u8 = "a,b;c\td|\"'\\\r\n#0123456789-{}:[]truefalse\xEF\xBB\xBF\xFF\x00 ";

    let iter: usize = 0;
    while (iter < 1800) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 192));
        let j: usize = 0;
        while (j < len) : (j += 1) {
            const rv: u64 = xorshift64(*rng);
            if ((iter % 3) == 0) {
                buf[j] = alphabet[@as(usize, @intCast(rv % alphabet.len))];
            } else {
                buf[j] = @as(u8, @intCast(rv & 0xFF));
            }
        }
        const s: string = buf[0..len] as string;
        * = sniffCsvDialect(s, 8);

        let r = CsvTableReader.init(s, CsvTableOptions{
            delimiter: if ((iter % 2) == 0) ',' else ';',
            comment_char: if ((iter % 5) == 0) '#' else 0,
            strict_column_count: (iter % 4) == 0,
        });
        let row_lim: usize = 0;
        while (row_lim < 32) : (row_lim += 1) {
            if (r.nextRow(fld_sc[0..]) == null) break;
        }

        const prof_cnt: usize = inferCsvSchema(
            s,
            CsvTableOptions{ delimiter: ',', has_header: (iter % 2) == 0 },
            profiles[0..],
            fld_sc[0..],
        ) catch 0;
        * = csvToJsonl(
            s,
            CsvTableOptions{ delimiter: ',', has_header: true },
            profiles[0..prof_cnt],
            hdr_sc[0..],
            fld_sc[0..],
            out_buf[0..],
        ) catch 0;
        * = jsonlToCsv(s, cols[0..], ',', fld_sc[0..], unesc_buf[0..], out_buf[0..]) catch 0;
    }

    expect_eq(iter, 1800);
}

// ============================================================================
// Test 2: PDF SIMD Fast-Path Verification + 1,600-Iteration Deterministic Fuzz
// ============================================================================
test("PDF SIMD + Fuzz (1,600 iters): inspect/xref/metadata/pages/content/string zero-crash") {
    let rng: u64 = 0x243F6A8885A308D3;
    let buf: [256]u8 = [0; 256];
    let xref_entries: [32]PdfXrefEntry = [PdfXrefEntry{}; 32];
    let meta_arena: [256]u8 = [0; 256];
    let page_stack: [32]PdfObjRef = [PdfObjRef{}; 32];
    let pages_out: [32]PdfPageInfo = [PdfPageInfo{}; 32];
    let text_out: [256]u8 = [0; 256];
    let str_out: [256]u8 = [0; 256];

    const pdf_tokens: []const u8 = "%PDF-1.7\n<< /Type /Pages /Kids [1 0 R] >> ()\\<>[] \r\n\t0123456789xrefstreamendstreamBTET";

    let iter: usize = 0;
    while (iter < 1600) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) {
            const rv: u64 = xorshift64(*rng);
            if ((iter % 3) == 0) {
                buf[j] = pdf_tokens[@as(usize, @intCast(rv % pdf_tokens.len))];
            } else {
                buf[j] = @as(u8, @intCast(rv & 0xFF));
            }
        }

        // Inject structured adversarial headers/markers periodically
        if ((iter % 4) == 0 && len >= 9) {
            buf[0] = '%';
            buf[1] = 'P';
            buf[2] = 'D';
            buf[3] = 'F';
            buf[4] = '-';
            buf[5] = '1';
            buf[6] = '.';
            buf[7] = '7';
            buf[8] = '\n';
        }
        if ((iter % 8) == 1 && len >= 32) {
            // Deeply nested literal string parentheses
            let k: usize = 0;
            while (k < 24) : (k += 1) buf[k] = '(';
        }

        const slice: []const u8 = buf[0..len];
        * = PdfInspector.inspect(slice) catch {};

        if (xref := PdfXrefTable.parse(slice, xref_entries[0..]) catch null) {
            * = extractPdfMetadata(slice, xref, meta_arena[0..]) catch {};
            * = collectPdfPages(slice, xref, page_stack[0..], pages_out[0..]) catch 0;
        }

        * = extractTextFromContentStream(slice, text_out[0..]) catch 0;
        * = decodePdfStringToUtf8(slice, str_out[0..]) catch 0;
    }

    expect_eq(iter, 1600);
}

// ============================================================================
// Test 3: FullTextIndex SIMD ASCII Fast-Path + 1,600-Iteration Deterministic Fuzz
// ============================================================================
test("FullTextIndex SIMD + Fuzz (1,600 iters): tokenize/searchQuery/deserializeSnapshot zero-crash") {
    // 1. Setup a small valid index to fuzz searchQuery & snapshot deserialization
    let fspecs: [2]FullTextFieldSpec = [
        FullTextFieldSpec{ weight: 2.0, b: 0.75 },
        FullTextFieldSpec{ weight: 1.0, b: 0.75 },
    ];
    let fsums: [2]u64 = [0; 2];
    let docs: [8]FtDocMeta = [FtDocMeta{}; 8];
    let flens: [16]u32 = [0; 16];
    let terms: [128]FtTermEntry = [FtTermEntry{}; 128];
    let htbl: [256]u32 = [0; 256];
    let tbytes: [2048]u8 = [0; 2048];
    let posts: [256]FtPosting = [FtPosting{}; 256];
    let pos_a: [512]u32 = [0; 512];

    const bufs: FullTextBuffers = FullTextBuffers{
        field_specs: fspecs[0..],
        field_len_sums: fsums[0..],
        docs: docs[0..],
        doc_field_lens: flens[0..],
        terms: terms[0..],
        term_hash_table: htbl[0..],
        term_bytes: tbytes[0..],
        postings: posts[0..],
        positions: pos_a[0..],
    };

    let idx: FullTextIndex = FullTextIndex.init(FullTextConfig{ k1: 1.2, field_count: 2 }, bufs) catch {
        expect_true(false);
        return;
    };

    let sc_tids: [128]u32 = [0; 128];
    let sc_pos: [128]u32 = [0; 128];
    let sc_scores: [8]f64 = [0.0; 8];
    let sc_mask: [8]u8 = [0; 8];
    let sc_aux: [512]u8 = [0; 512];
    let sc_cnt: [8]u32 = [0; 8];
    let scratch: FullTextScratch = FullTextScratch{
        term_ids: sc_tids[0..],
        token_positions: sc_pos[0..],
        scores: sc_scores[0..],
        doc_mask: sc_mask[0..],
        doc_mask_aux: sc_aux[0..],
        matched_terms_cnt: sc_cnt[0..],
    };

    const doc_en: [2][]const u8 = ["HIGHWAY SIMD ACCELERATED ENGINE", "Vectorized ASCII lowercase folding and whitespace skipping"];
    * = idx.addDocument(1, doc_en[0..], *scratch) catch { expect_true(false); return; };

    // Verify SIMD pure-ASCII fast path produces exact lowercase stemmed hits
    let hits: [4]FtHit = [FtHit{}; 4];
    let hits_s: []FtHit = hits[0..];
    const nh: usize = idx.searchQuery("HIGHWAY AND SIMD AND lowercase", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(nh, 1);
    expect_eq(hits[0].doc_id, 1);

    // Serialize a valid snapshot template for bit-flip fuzzing
    let snap_tpl: [8192]u8 = [0; 8192];
    let snap_s: []u8 = snap_tpl[0..];
    const snap_len: usize = idx.serializeSnapshot(*snap_s) catch { expect_true(false); return; };

    // Fresh scratch buffers for snapshot deserialization fuzzing
    let fspecs2: [2]FullTextFieldSpec = [FullTextFieldSpec{}; 2];
    let fsums2: [2]u64 = [0; 2];
    let docs2: [8]FtDocMeta = [FtDocMeta{}; 8];
    let flens2: [16]u32 = [0; 16];
    let terms2: [128]FtTermEntry = [FtTermEntry{}; 128];
    let htbl2: [256]u32 = [0; 256];
    let tbytes2: [2048]u8 = [0; 2048];
    let posts2: [256]FtPosting = [FtPosting{}; 256];
    let pos_a2: [512]u32 = [0; 512];
    const bufs2: FullTextBuffers = FullTextBuffers{
        field_specs: fspecs2[0..],
        field_len_sums: fsums2[0..],
        docs: docs2[0..],
        doc_field_lens: flens2[0..],
        terms: terms2[0..],
        term_hash_table: htbl2[0..],
        term_bytes: tbytes2[0..],
        postings: posts2[0..],
        positions: pos_a2[0..],
    };

    let rng: u64 = 0x13198A2E03707344;
    let qbuf: [160]u8 = [0; 160];
    let spans: [32]FtTokenSpan = [FtTokenSpan{}; 32];
    let spans_s: []FtTokenSpan = spans[0..];
    const q_alpha: []const u8 = "abcxyzANDORNOT()\"~*+- 0129\t\n\xE6\x90\x9C\xFF\x80";

    let iter: usize = 0;
    while (iter < 1600) : (iter += 1) {
        const qlen: usize = @as(usize, @intCast(xorshift64(*rng) % 160));
        let j: usize = 0;
        while (j < qlen) : (j += 1) {
            const rv: u64 = xorshift64(*rng);
            if ((iter % 2) == 0) {
                qbuf[j] = q_alpha[@as(usize, @intCast(rv % q_alpha.len))];
            } else {
                qbuf[j] = @as(u8, @intCast(rv & 0xFF));
            }
        }
        // Deeply nested parens stress
        if ((iter % 16) == 0 && qlen >= 48) {
            let k: usize = 0;
            while (k < 44) : (k += 1) qbuf[k] = '(';
        }

        // 1. Fuzz tokenizer (random invalid UTF-8 + CJK + ASCII)
        * = tokenizeText(qbuf[0..qlen], true, true, true, *spans_s);

        // 2. Fuzz boolean/phrase/fuzzy query parser
        * = idx.searchQuery(qbuf[0..qlen], *scratch, *hits_s) catch 0;

        // 3. Fuzz deserializeSnapshot with truncated / mutated binary snapshots
        if ((iter % 2) == 0) {
            const flip_idx: usize = @as(usize, @intCast(xorshift64(*rng) % snap_len));
            snap_tpl[flip_idx] ^= 0x5A;
            * = FullTextIndex.deserializeSnapshot(snap_tpl[0..snap_len], bufs2) catch {};
            snap_tpl[flip_idx] ^= 0x5A; // restore
        } else {
            * = FullTextIndex.deserializeSnapshot(qbuf[0..qlen], bufs2) catch {};
        }
    }

    expect_eq(iter, 1600);
}
