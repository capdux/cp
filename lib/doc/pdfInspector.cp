// Source: cp/lib/pdf/inspector.cp (PDF 1.4-1.7 Object/XRef/FlateDecode/Page-Tree/Outlines & Text Extraction Engine)
// Reference-hardened against J-F-Liu/lopdf (MIT)
// Pure CP zero-dynamic-heap PDF structural validator, lexical scanner, XRef/XRef-Stream/ObjStm/incremental-update parser,
// FlateDecode/ASCIIHexDecode/PNG-TIFF-Predictor stream decoder, page-tree & outline walker, and BT..ET content stream text extractor.

const std = @import("std");
import { PdfError } from "./pdfWriter.cp";
import { inflate } from "cap:pack";
import { Simd } from "../simd/index.cp";

// ============================================================================
// 1. Legacy Backward-Compatible Structural Inspector (`PdfInfo` & `PdfInspector`)
// ============================================================================

/// Structural inspection summary and document metadata extracted from a PDF byte buffer.
pub struct PdfInfo {
    version: []const u8 = "",
    page_count: usize = 0,
    has_xref: bool = false,
    has_eof: bool = false,
    title: ([]const u8)? = null,
    author: ([]const u8)? = null,
    creator: ([]const u8)? = null,
}

fn isPdfWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0C || c == 0x00;
}

fn isPdfDelimiter(c: u8): bool {
    return isPdfWhitespace(c) || c == '/' || c == '<' || c == '>' || c == '[' || c == ']' || c == '(' || c == ')' || c == '{' || c == '}' || c == '%';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn skipWhitespace(bytes: []const u8, pos: *usize) {
    while (pos.* < bytes.len) {
        const adv: usize = Simd.skipWhitespace(bytes[pos.*..]);
        pos.* += adv;
        if (pos.* < bytes.len && (bytes[pos.*] == 0 || bytes[pos.*] == 0x0C)) {
            pos.* += 1;
        } else {
            break;
        }
    }
}

/// Checks if `bytes[pos..]` matches `stream` followed by `\r\n` or `\n` (PDF stream body start).
fn matchStreamKeyword(bytes: []const u8, pos: usize): usize? {
    if (pos + 6 > bytes.len) return null;
    if (!std.mem.eql(u8, bytes[pos..pos + 6], "stream")) return null;
    if (pos > 0 && !isPdfDelimiter(bytes[pos - 1])) return null;
    if (pos + 6 < bytes.len) {
        if (bytes[pos + 6] == '\r' && pos + 7 < bytes.len && bytes[pos + 7] == '\n') {
            return pos + 8;
        }
        if (bytes[pos + 6] == '\n') {
            return pos + 7;
        }
    }
    return null;
}

/// Finds the end of a `stream ... endstream` section starting after `stream\n` using 32-byte SIMD scan.
fn skipStreamPayload(bytes: []const u8, body_start: usize): usize {
    let p: usize = body_start;
    while (p + 9 <= bytes.len) {
        const rel: usize = Simd.indexOfByte(bytes[p..bytes.len], 'e') ?? return bytes.len;
        p += rel;
        if (p + 9 > bytes.len) return bytes.len;
        if (std.mem.eql(u8, bytes[p..p + 9], "endstream")) {
            return p + 9;
        }
        p += 1;
    }
    return bytes.len;
}

/// Parses a PDF literal string `(...)` starting at `open_idx` (where `bytes[open_idx] == '('`).
/// Uses 16-byte SIMD scan for `\`, `(`, `)` to skip plain runs at hardware speed.
fn parseLiteralStringSlice(bytes: []const u8, open_idx: usize): ([]const u8)? {
    if (open_idx >= bytes.len || bytes[open_idx] != '(') return null;
    let depth: usize = 1;
    let i: usize = open_idx + 1;
    while (i < bytes.len) {
        const rel: usize = Simd.indexOfAny4(bytes[i..bytes.len], '\\', '(', ')', ')') ?? return null;
        i += rel;
        const c = bytes[i];
        if (c == '\\') {
            const step: usize = if (i + 1 < bytes.len) 2 else 1;
            i += step;
            continue;
        }
        if (c == '(') {
            depth += 1;
        } else if (c == ')') {
            depth -= 1;
            if (depth == 0) {
                return bytes[open_idx + 1 .. i];
            }
        }
        i += 1;
    }
    return null;
}

/// Locates the last `startxref` marker and parses the decimal byte offset following it.
fn parseStartXrefOffset(bytes: []const u8): usize? {
    let search_end = bytes.len;
    const idx = std.mem.lastIndexOf(u8, bytes[0..search_end], "startxref") ?? return null;
    if (idx > 0 && !isPdfDelimiter(bytes[idx - 1])) return null;
    let p: usize = idx + 9;
    if (p >= bytes.len || !isPdfWhitespace(bytes[p])) return null;
    skipWhitespace(bytes, &p);
    if (p >= bytes.len || !isDigit(bytes[p])) return null;
    const num_start = p;
    while (p < bytes.len && isDigit(bytes[p])) {
        p += 1;
    }
    if (p < bytes.len && !isPdfDelimiter(bytes[p])) return null;
    return std.fmt.parseInt(usize, bytes[num_start..p], 10) catch return null;
}

/// Checks if `%%EOF` appears after `startxref` at the tail of the PDF.
fn hasValidEof(bytes: []const u8): bool {
    const startxref_idx = std.mem.lastIndexOf(u8, bytes, "startxref") ?? return false;
    const eof_idx = std.mem.lastIndexOf(u8, bytes, "%%EOF") ?? return false;
    if (eof_idx <= startxref_idx) return false;
    let tail = eof_idx + 5;
    while (tail < bytes.len) : (tail += 1) {
        if (!isPdfWhitespace(bytes[tail])) return false;
    }
    return true;
}

/// Validates that `startxref` offset points to a valid traditional `xref` table or PDF 1.5+ `/Type /XRef` stream object.
fn hasValidXref(bytes: []const u8): bool {
    const xref_off = parseStartXrefOffset(bytes) ?? return false;
    if (xref_off >= bytes.len) return false;
    let p = xref_off;
    skipWhitespace(bytes, &p);
    if (p + 4 <= bytes.len && std.mem.eql(u8, bytes[p..p + 4], "xref")) {
        if (p + 4 == bytes.len || isPdfWhitespace(bytes[p + 4])) return true;
    }
    let lex = PdfLexer.init(bytes, p);
    const t0 = lex.nextToken();
    const t1 = lex.nextToken();
    const t2 = lex.nextToken();
    if (t0.kind == PdfTokenKind.number_int && t1.kind == PdfTokenKind.number_int &&
        t2.kind == PdfTokenKind.keyword && std.mem.eql(u8, t2.slice(bytes), "obj"))
    {
        return true;
    }
    return false;
}

/// Counts `/Type /Page` entries outside `stream ... endstream` payloads (excluding `/Type /Pages`).
fn countPageObjects(bytes: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        if (bytes[i] == '(') {
            if (s := parseLiteralStringSlice(bytes, i)) {
                i += s.len + 2;
                continue;
            }
        }
        if (bytes[i] == 's') {
            if (body_start := matchStreamKeyword(bytes, i)) {
                i = skipStreamPayload(bytes, body_start);
                continue;
            }
        }
        if (i + 5 <= bytes.len && std.mem.eql(u8, bytes[i..i + 5], "/Type")) {
            let p = i + 5;
            if (p < bytes.len && isPdfDelimiter(bytes[p])) {
                skipWhitespace(bytes, &p);
                if (p + 5 <= bytes.len && std.mem.eql(u8, bytes[p..p + 5], "/Page")) {
                    const after = p + 5;
                    if (after == bytes.len || isPdfDelimiter(bytes[after])) {
                        count += 1;
                        i = after;
                        continue;
                    }
                }
            }
        }
        i += 1;
    }
    return count;
}

/// Extracts the string value associated with a metadata key (e.g. `/Title`) outside streams.
fn extractMetadataField(bytes: []const u8, key: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < bytes.len) {
        if (bytes[i] == '(') {
            if (s := parseLiteralStringSlice(bytes, i)) {
                i += s.len + 2;
                continue;
            }
        }
        if (bytes[i] == 's') {
            if (body_start := matchStreamKeyword(bytes, i)) {
                i = skipStreamPayload(bytes, body_start);
                continue;
            }
        }
        if (i + key.len <= bytes.len && std.mem.eql(u8, bytes[i..i + key.len], key)) {
            let p = i + key.len;
            if (p < bytes.len && isPdfDelimiter(bytes[p])) {
                skipWhitespace(bytes, &p);
                if (p < bytes.len && bytes[p] == '(') {
                    return parseLiteralStringSlice(bytes, p);
                }
            }
        }
        i += 1;
    }
    return null;
}

/// Static PDF 1.4 structural validator, version extractor, and metadata inspector.
pub static struct PdfInspector {
    /// Extracts the PDF header version string (e.g. `"1.4"`) if `bytes` begins with `%PDF-X.Y`.
    static fn extractVersion(bytes: []const u8): string? {
        if (bytes.len < 8) return null;
        if (!std.mem.eql(u8, bytes[0..5], "%PDF-")) return null;
        const maj = bytes[5];
        const dot = bytes[6];
        const min = bytes[7];
        if (!isDigit(maj) || dot != '.' || !isDigit(min)) return null;
        if (bytes.len > 8 && !isPdfWhitespace(bytes[8])) return null;
        return bytes[5..8];
    }

    /// Returns `true` if `bytes` has a valid `%PDF-` header, `xref`/`startxref`, and trailing `%%EOF`.
    static fn isValidPdf(bytes: []const u8): bool {
        if (PdfInspector.extractVersion(bytes) == null) return false;
        if (!hasValidXref(bytes)) return false;
        if (!hasValidEof(bytes)) return false;
        return true;
    }

    /// Validates PDF structural markers and extracts version, `/Type /Page` count,
    /// `xref`/`%%EOF` flags, and `/Title`, `/Author`, `/Creator` metadata.
    static fn inspect(bytes: []const u8): PdfInfo ! PdfError {
        const ver_opt = PdfInspector.extractVersion(bytes);
        if (ver_opt == null) {
            return PdfError.InvalidPdf;
        }
        const ver = ver_opt.?;
        if (!hasValidXref(bytes)) {
            return PdfError.MissingXRef;
        }
        if (!hasValidEof(bytes)) {
            return PdfError.MissingEof;
        }

        const page_count = countPageObjects(bytes);
        const title = extractMetadataField(bytes, "/Title");
        const author = extractMetadataField(bytes, "/Author");
        const creator = extractMetadataField(bytes, "/Creator");

        return PdfInfo{
            version: ver,
            page_count: page_count,
            has_xref: true,
            has_eof: true,
            title: title,
            author: author,
            creator: creator,
        };
    }
}

// ============================================================================
// 2. Production PDF 1.4-1.7 Lexer, Object Model & Error Set
// ============================================================================

/// Error set for full PDF 1.4-1.7 object, XRef, stream filter, page-tree, and text extraction operations.
pub err PdfParseError {
    InvalidPdf,
    MissingXRef,
    InvalidXRef,
    MissingTrailer,
    ObjectNotFound,
    InvalidObject,
    InvalidStream,
    UnsupportedFilter,
    DecompressFailed,
    BufferTooSmall,
    OutputSliceTooSmall,
    EncryptedDocument,
    StackOverflow,
}

/// Lexical token kinds emitted by `PdfLexer`.
pub enum PdfTokenKind {
    eof,
    number_int,
    number_real,
    boolean,
    null_val,
    name,
    literal_string,
    hex_string,
    dict_start,
    dict_end,
    array_start,
    array_end,
    keyword,
}

/// Zero-copy token span pointing into the source PDF buffer.
pub struct PdfToken {
    kind: PdfTokenKind = PdfTokenKind.eof;
    start: usize = 0;
    end: usize = 0;

    fn slice(bytes: []const u8): []const u8 {
        if (start >= end || end > bytes.len) return "";
        return bytes[start..end];
    }
}

/// Zero-allocation PDF 1.4-1.7 lexical scanner supporting comments, nested literal strings,
/// hex strings, names, dictionaries, arrays, numbers, booleans, null, and keywords.
pub struct PdfLexer {
    bytes: []const u8 = [];
    pos: usize = 0;

    static fn init(bytes: []const u8, start_pos: usize): PdfLexer {
        const p: usize = if (start_pos <= bytes.len) start_pos else bytes.len;
        return PdfLexer{ bytes: bytes, pos: p };
    }

    /// Skips PDF whitespace and `% ... \r?\n` comments using 16-byte SIMD vector scanners.
    fn skipWsAndComments(*_) {
        while (pos < bytes.len) {
            const adv: usize = Simd.skipWhitespace(bytes[pos..bytes.len]);
            pos += adv;
            if (pos >= bytes.len) break;
            if (bytes[pos] == 0 || bytes[pos] == 0x0C) {
                pos += 1;
            } else if (bytes[pos] == '%') {
                pos += 1;
                if (rel := Simd.indexOfAny2(bytes[pos..bytes.len], '\n', '\r')) {
                    pos += rel;
                } else {
                    pos = bytes.len;
                }
            } else {
                break;
            }
        }
    }

    /// Reads the next lexical token and advances `pos`.
    fn nextToken(*_): PdfToken {
        skipWsAndComments();
        if (pos >= bytes.len) {
            return PdfToken{ kind: PdfTokenKind.eof, start: pos, end: pos };
        }

        const start = pos;
        const c = bytes[pos];

        // Dictionary delimiters `<<` / `>>` vs Hex string `<...>`
        if (c == '<') {
            if (pos + 1 < bytes.len && bytes[pos + 1] == '<') {
                pos += 2;
                return PdfToken{ kind: PdfTokenKind.dict_start, start: start, end: pos };
            }
            pos += 1;
            if (rel := Simd.indexOfByte(bytes[pos..bytes.len], '>')) {
                pos += rel + 1;
            } else {
                pos = bytes.len;
            }
            return PdfToken{ kind: PdfTokenKind.hex_string, start: start, end: pos };
        }
        if (c == '>') {
            if (pos + 1 < bytes.len && bytes[pos + 1] == '>') {
                pos += 2;
                return PdfToken{ kind: PdfTokenKind.dict_end, start: start, end: pos };
            }
            pos += 1;
            return PdfToken{ kind: PdfTokenKind.keyword, start: start, end: pos };
        }
        if (c == '[') {
            pos += 1;
            return PdfToken{ kind: PdfTokenKind.array_start, start: start, end: pos };
        }
        if (c == ']') {
            pos += 1;
            return PdfToken{ kind: PdfTokenKind.array_end, start: start, end: pos };
        }

        // Literal string `(...)` with balanced parentheses and backslash escapes (SIMD accelerated)
        if (c == '(') {
            let depth: usize = 1;
            pos += 1;
            while (pos < bytes.len && depth > 0) {
                if (rel := Simd.indexOfAny4(bytes[pos..bytes.len], '\\', '(', ')', ')')) {
                    pos += rel;
                    const ch = bytes[pos];
                    if (ch == '\\') {
                        const step: usize = if (pos + 1 < bytes.len) 2 else 1;
                        pos += step;
                        continue;
                    }
                    if (ch == '(') {
                        depth += 1;
                    } else if (ch == ')') {
                        depth -= 1;
                    }
                    pos += 1;
                } else {
                    pos = bytes.len;
                    break;
                }
            }
            return PdfToken{ kind: PdfTokenKind.literal_string, start: start, end: pos };
        }

        // Name `/...`
        if (c == '/') {
            pos += 1;
            while (pos < bytes.len && !isPdfDelimiter(bytes[pos])) {
                pos += 1;
            }
            return PdfToken{ kind: PdfTokenKind.name, start: start, end: pos };
        }

        // Regular token (number, boolean, null, or keyword)
        while (pos < bytes.len && !isPdfDelimiter(bytes[pos])) {
            pos += 1;
        }
        const tok = bytes[start..pos];
        if (std.mem.eql(u8, tok, "true") || std.mem.eql(u8, tok, "false")) {
            return PdfToken{ kind: PdfTokenKind.boolean, start: start, end: pos };
        }
        if (std.mem.eql(u8, tok, "null")) {
            return PdfToken{ kind: PdfTokenKind.null_val, start: start, end: pos };
        }

        // Classify numeric token
        let idx: usize = 0;
        if (tok.len > 0 && (tok[0] == '+' || tok[0] == '-')) idx = 1;
        let has_digit: bool = false;
        let has_dot: bool = false;
        let all_num: bool = idx < tok.len;
        while (idx < tok.len) : (idx += 1) {
            if (isDigit(tok[idx])) {
                has_digit = true;
            } else if (tok[idx] == '.' && !has_dot) {
                has_dot = true;
            } else {
                all_num = false;
                break;
            }
        }
        if (all_num && has_digit) {
            const nk = if (has_dot) PdfTokenKind.number_real else PdfTokenKind.number_int;
            return PdfToken{ kind: nk, start: start, end: pos };
        }

        return PdfToken{ kind: PdfTokenKind.keyword, start: start, end: pos };
    }

    /// Skips one complete PDF value (scalar, balanced `[...]`, or balanced `<<...>>`).
    fn skipValue(*_) {
        const tok = nextToken();
        if (tok.kind == PdfTokenKind.dict_start) {
            let depth: usize = 1;
            while (depth > 0) {
                const t = nextToken();
                if (t.kind == PdfTokenKind.eof) break;
                if (t.kind == PdfTokenKind.dict_start) depth += 1;
                if (t.kind == PdfTokenKind.dict_end) depth -= 1;
            }
        } else if (tok.kind == PdfTokenKind.array_start) {
            let depth: usize = 1;
            while (depth > 0) {
                const t = nextToken();
                if (t.kind == PdfTokenKind.eof) break;
                if (t.kind == PdfTokenKind.array_start) depth += 1;
                if (t.kind == PdfTokenKind.array_end) depth -= 1;
            }
        } else if (tok.kind == PdfTokenKind.number_int) {
            // Check for indirect reference `N G R`
            const saved = pos;
            const t2 = nextToken();
            if (t2.kind == PdfTokenKind.number_int) {
                const t3 = nextToken();
                if (t3.kind == PdfTokenKind.keyword && std.mem.eql(u8, t3.slice(bytes), "R")) {
                    return;
                }
            }
            pos = saved;
        }
    }
}

/// Indirect PDF object reference (`obj_num gen_num R`).
pub struct PdfObjRef {
    obj_num: u32 = 0;
    gen_num: u16 = 0;
}

/// Single entry in a parsed PDF cross-reference table or PDF 1.5+ cross-reference stream.
pub struct PdfXrefEntry {
    obj_num: u32 = 0;
    gen_num: u16 = 0;
    offset: usize = 0;
    in_use: bool = false;
    is_compressed: bool = false;
    obj_stm_num: u32 = 0;
    obj_stm_index: u16 = 0;
}

/// Metadata parsed from PDF `trailer << ... >>` dictionaries or `/Type /XRef` stream dictionaries
/// (including incremental `/Prev` chains and hybrid `/XRefStm`).
pub struct PdfTrailerInfo {
    size: u32 = 0;
    has_root: bool = false;
    root: PdfObjRef = PdfObjRef{};
    has_info: bool = false;
    info: PdfObjRef = PdfObjRef{};
    has_encrypt: bool = false;
    has_prev: bool = false;
    prev_xref_offset: usize = 0;
    xref_sections_parsed: usize = 0;
    is_xref_stream: bool = false;
    has_xref_stm: bool = false;
    xref_stm_offset: usize = 0;
}

fn parseU32Tok(bytes: []const u8, tok: PdfToken): u32? {
    if (tok.kind != PdfTokenKind.number_int) return null;
    return std.fmt.parseInt(u32, tok.slice(bytes), 10) catch return null;
}

fn parseI32Tok(bytes: []const u8, tok: PdfToken): i32? {
    if (tok.kind != PdfTokenKind.number_int) return null;
    return std.fmt.parseInt(i32, tok.slice(bytes), 10) catch return null;
}

fn parseF64Tok(bytes: []const u8, tok: PdfToken): f64? {
    if (tok.kind != PdfTokenKind.number_int && tok.kind != PdfTokenKind.number_real) return null;
    return std.fmt.parseFloat(f64, tok.slice(bytes)) catch return null;
}

/// Searches a PDF dictionary slice (starting at `<<` or inside it) for key `key_name` (e.g. `"/Root"`)
/// and returns a `PdfLexer` positioned right at the start of the key's value.
fn findDictValuePos(dict_bytes: []const u8, key_name: []const u8): usize? {
    let lex = PdfLexer.init(dict_bytes, 0);
    const first = lex.nextToken();
    if (first.kind != PdfTokenKind.dict_start) {
        lex.pos = 0;
    }
    while (true) {
        const k = lex.nextToken();
        if (k.kind == PdfTokenKind.eof || k.kind == PdfTokenKind.dict_end) return null;
        if (k.kind != PdfTokenKind.name) return null;
        if (std.mem.eql(u8, k.slice(dict_bytes), key_name)) {
            lex.skipWsAndComments();
            return lex.pos;
        }
        lex.skipValue();
    }
}

fn parseObjRefAt(bytes: []const u8, start_pos: usize): PdfObjRef? {
    let lex = PdfLexer.init(bytes, start_pos);
    const t1 = lex.nextToken();
    const t2 = lex.nextToken();
    const t3 = lex.nextToken();
    if (t1.kind != PdfTokenKind.number_int || t2.kind != PdfTokenKind.number_int) return null;
    if (t3.kind != PdfTokenKind.keyword || !std.mem.eql(u8, t3.slice(bytes), "R")) return null;
    const obj_n = parseU32Tok(bytes, t1) ?? return null;
    const gen_n = parseU32Tok(bytes, t2) ?? return null;
    return PdfObjRef{ obj_num: obj_n, gen_num: @as(u16, @intCast(gen_n & 0xFFFF)) };
}

fn locateObjectBodyAtOffset(pdf_bytes: []const u8, start_offset: usize): ([]const u8)? {
    if (start_offset >= pdf_bytes.len) return null;
    let lex = PdfLexer.init(pdf_bytes, start_offset);
    const t_num = lex.nextToken();
    const t_gen = lex.nextToken();
    const t_obj = lex.nextToken();
    if (t_num.kind != PdfTokenKind.number_int || t_gen.kind != PdfTokenKind.number_int) return null;
    if (t_obj.kind != PdfTokenKind.keyword || !std.mem.eql(u8, t_obj.slice(pdf_bytes), "obj")) return null;

    lex.skipWsAndComments();
    const body_start = lex.pos;

    let i: usize = body_start;
    while (i < pdf_bytes.len) {
        if (pdf_bytes[i] == '(') {
            if (s := parseLiteralStringSlice(pdf_bytes, i)) {
                i += s.len + 2;
                continue;
            }
        }
        if (pdf_bytes[i] == 's') {
            if (sbody := matchStreamKeyword(pdf_bytes, i)) {
                i = skipStreamPayload(pdf_bytes, sbody);
                continue;
            }
        }
        if (i + 6 <= pdf_bytes.len && std.mem.eql(u8, pdf_bytes[i..i + 6], "endobj")) {
            const before_ok: bool = (i == body_start) || isPdfDelimiter(pdf_bytes[i - 1]);
            const after_ok: bool = (i + 6 == pdf_bytes.len) || isPdfDelimiter(pdf_bytes[i + 6]);
            if (before_ok && after_ok) {
                return pdf_bytes[body_start..i];
            }
        }
        i += 1;
    }
    return null;
}

fn readBeU64(bytes: []const u8, off: usize, width: usize): u64 {
    let v: u64 = 0;
    let i: usize = 0;
    while (i < width) : (i += 1) {
        v = (v << 8) | @as(u64, bytes[off + i]);
    }
    return v;
}

fn mergeTrailerDict(tr_slice: []const u8, trailer: *PdfTrailerInfo) {
    if (trailer.size == 0) {
        if (sp := findDictValuePos(tr_slice, "/Size")) {
            let sl = PdfLexer.init(tr_slice, sp);
            if (sz := parseU32Tok(tr_slice, sl.nextToken())) {
                trailer.size = sz;
            }
        }
    }
    if (!trailer.has_root) {
        if (rp := findDictValuePos(tr_slice, "/Root")) {
            if (rref := parseObjRefAt(tr_slice, rp)) {
                trailer.has_root = true;
                trailer.root = rref;
            }
        }
    }
    if (!trailer.has_info) {
        if (ip := findDictValuePos(tr_slice, "/Info")) {
            if (iref := parseObjRefAt(tr_slice, ip)) {
                trailer.has_info = true;
                trailer.info = iref;
            }
        }
    }
    if (findDictValuePos(tr_slice, "/Encrypt") != null) {
        trailer.has_encrypt = true;
    }
}

/// Parses a PDF 1.5+ Cross-Reference Stream (`N G obj << /Type /XRef ... >> stream ... endstream endobj`)
/// at `stm_off` into `tbl`, returning optional `/Prev` byte offset.
fn parseXrefStreamAt(
    pdf_bytes: []const u8,
    stm_off: usize,
    tbl: *PdfXrefTable,
): (usize?) ! PdfParseError {
    const obj_body = locateObjectBodyAtOffset(pdf_bytes, stm_off) ?? return PdfParseError.InvalidXRef;
    const sv = extractStreamFromObject(pdf_bytes, tbl.*, obj_body) catch return PdfParseError.InvalidXRef;
    const dict = sv.dict_slice;

    // Verify `/Type /XRef`
    const tp = findDictValuePos(dict, "/Type") ?? return PdfParseError.InvalidXRef;
    let tlex = PdfLexer.init(dict, tp);
    const ttok = tlex.nextToken();
    if (ttok.kind != PdfTokenKind.name || !std.mem.eql(u8, ttok.slice(dict), "/XRef")) {
        return PdfParseError.InvalidXRef;
    }

    // Required `/Size` and `/W [w0 w1 w2]`
    const size_pos = findDictValuePos(dict, "/Size") ?? return PdfParseError.InvalidXRef;
    let slex = PdfLexer.init(dict, size_pos);
    const size_val: u32 = parseU32Tok(dict, slex.nextToken()) ?? return PdfParseError.InvalidXRef;

    const w_pos = findDictValuePos(dict, "/W") ?? return PdfParseError.InvalidXRef;
    let wlex = PdfLexer.init(dict, w_pos);
    if (wlex.nextToken().kind != PdfTokenKind.array_start) return PdfParseError.InvalidXRef;
    let widths: [3]usize = [0; 3];
    let wi: usize = 0;
    while (wi < 3) : (wi += 1) {
        const wt = wlex.nextToken();
        const wv: u32 = parseU32Tok(dict, wt) ?? return PdfParseError.InvalidXRef;
        if (wv > 8) return PdfParseError.InvalidXRef;
        widths[wi] = @as(usize, wv);
    }
    const entry_width: usize = widths[0] + widths[1] + widths[2];
    if (entry_width == 0) return PdfParseError.InvalidXRef;

    // Parse `/Index [...]` (defaults to `[0 size_val]`)
    let idx_starts: [64]u32 = [0; 64];
    let idx_counts: [64]u32 = [0; 64];
    let num_sections: usize = 0;

    if (ip := findDictValuePos(dict, "/Index")) {
        let ilex = PdfLexer.init(dict, ip);
        if (ilex.nextToken().kind != PdfTokenKind.array_start) return PdfParseError.InvalidXRef;
        while (true) {
            const st_tok = ilex.nextToken();
            if (st_tok.kind == PdfTokenKind.array_end || st_tok.kind == PdfTokenKind.eof) break;
            const ct_tok = ilex.nextToken();
            const st_v: u32 = parseU32Tok(dict, st_tok) ?? return PdfParseError.InvalidXRef;
            const ct_v: u32 = parseU32Tok(dict, ct_tok) ?? return PdfParseError.InvalidXRef;
            if (num_sections >= 64) return PdfParseError.InvalidXRef;
            idx_starts[num_sections] = st_v;
            idx_counts[num_sections] = ct_v;
            num_sections += 1;
        }
    } else {
        idx_starts[0] = 0;
        idx_counts[0] = size_val;
        num_sections = 1;
    }

    // Decode XRef stream (automatically handles /FlateDecode + /DecodeParms PNG/TIFF predictor!)
    let stm_buf: [65536]u8 = [0; 65536];
    const dec_len: usize = decodePdfStream(sv, stm_buf[0..]) catch return PdfParseError.InvalidXRef;
    const dec_data = stm_buf[0..dec_len];

    // Pre-validate total entries cannot overrun decoded stream (anti-amplification guard per lopdf)
    let total_entries: usize = 0;
    let si: usize = 0;
    while (si < num_sections) : (si += 1) {
        total_entries += @as(usize, idx_counts[si]);
        if (total_entries > dec_len / entry_width) {
            return PdfParseError.InvalidXRef;
        }
    }

    let pos: usize = 0;
    si = 0;
    while (si < num_sections) : (si += 1) {
        const start_id: u32 = idx_starts[si];
        const sec_cnt: u32 = idx_counts[si];
        let j: u32 = 0;
        while (j < sec_cnt) : (j += 1) {
            if (pos + entry_width > dec_len) return PdfParseError.InvalidXRef;
            const f0: u64 = if (widths[0] > 0) readBeU64(dec_data, pos, widths[0]) else 1;
            const f1: u64 = if (widths[1] > 0) readBeU64(dec_data, pos + widths[0], widths[1]) else 0;
            const f2: u64 = if (widths[2] > 0) readBeU64(dec_data, pos + widths[0] + widths[1], widths[2]) else 0;
            pos += entry_width;

            const obj_num: u32 = start_id + j;
            if (tbl.findIndex(obj_num) == null) {
                if (tbl.count >= tbl.entries.len) {
                    return PdfParseError.OutputSliceTooSmall;
                }
                if (f0 == 0) {
                    tbl.entries[tbl.count] = PdfXrefEntry{
                        obj_num: obj_num,
                        gen_num: @as(u16, @intCast(f2 & 0xFFFF)),
                        offset: 0,
                        in_use: false,
                        is_compressed: false,
                    };
                } else if (f0 == 1) {
                    tbl.entries[tbl.count] = PdfXrefEntry{
                        obj_num: obj_num,
                        gen_num: @as(u16, @intCast(f2 & 0xFFFF)),
                        offset: @as(usize, @intCast(f1)),
                        in_use: true,
                        is_compressed: false,
                    };
                } else if (f0 == 2) {
                    tbl.entries[tbl.count] = PdfXrefEntry{
                        obj_num: obj_num,
                        gen_num: 0,
                        offset: 0,
                        in_use: true,
                        is_compressed: true,
                        obj_stm_num: @as(u32, @intCast(f1 & 0xFFFF_FFFF)),
                        obj_stm_index: @as(u16, @intCast(f2 & 0xFFFF)),
                    };
                } else {
                    // Unknown future entry type treated as free/null per ISO 32000-1 §7.5.8.3
                    tbl.entries[tbl.count] = PdfXrefEntry{
                        obj_num: obj_num,
                        in_use: false,
                    };
                }
                tbl.count += 1;
            }
        }
    }

    tbl.trailer.is_xref_stream = true;
    tbl.trailer.xref_sections_parsed += 1;
    mergeTrailerDict(dict, &tbl.trailer);

    if (pp := findDictValuePos(dict, "/Prev")) {
        let pl = PdfLexer.init(dict, pp);
        const pt = pl.nextToken();
        if (pt.kind == PdfTokenKind.number_int) {
            const prev_off: usize = std.fmt.parseInt(usize, pt.slice(dict), 10) catch return PdfParseError.InvalidXRef;
            if (!tbl.trailer.has_prev) {
                tbl.trailer.has_prev = true;
                tbl.trailer.prev_xref_offset = prev_off;
            }
            return prev_off;
        }
    }
    return null;
}

/// Extracts a compressed indirect object's body slice from a PDF 1.5+ `/Type /ObjStm` stream
/// (`obj_stm_num`) into caller-provided `out_buf`. Returns byte length written.
pub fn locateObjectBodyFromObjStm(
    pdf_bytes: []const u8,
    xref: PdfXrefTable,
    obj_stm_num: u32,
    target_obj_num: u32,
    out_buf: []u8,
): usize ! PdfParseError {
    const stm_body = xref.locateObjectBody(pdf_bytes, PdfObjRef{ obj_num: obj_stm_num, gen_num: 0 }) ?? return PdfParseError.ObjectNotFound;
    const sv = extractStreamFromObject(pdf_bytes, xref, stm_body)?;
    const dict = sv.dict_slice;

    // Validate `/Type /ObjStm`, `/N`, and `/First`
    const tp = findDictValuePos(dict, "/Type") ?? return PdfParseError.InvalidObject;
    let tlex = PdfLexer.init(dict, tp);
    const ttok = tlex.nextToken();
    if (ttok.kind != PdfTokenKind.name || !std.mem.eql(u8, ttok.slice(dict), "/ObjStm")) {
        return PdfParseError.InvalidObject;
    }

    const np = findDictValuePos(dict, "/N") ?? return PdfParseError.InvalidObject;
    let nlex = PdfLexer.init(dict, np);
    const n_objs: usize = @as(usize, parseU32Tok(dict, nlex.nextToken()) ?? return PdfParseError.InvalidObject);

    const fp = findDictValuePos(dict, "/First") ?? return PdfParseError.InvalidObject;
    let flex = PdfLexer.init(dict, fp);
    const first_off: usize = @as(usize, parseU32Tok(dict, flex.nextToken()) ?? return PdfParseError.InvalidObject);

    let stm_scratch: [65536]u8 = [0; 65536];
    const dec_len: usize = decodePdfStream(sv, stm_scratch[0..])?;
    if (first_off > dec_len) return PdfParseError.InvalidObject;

    // Parse `[0..first_off)` pairs of `<obj_num> <rel_offset>`
    const header = stm_scratch[0..first_off];
    let hlex = PdfLexer.init(header, 0);

    let found: bool = false;
    let target_start: usize = 0;
    let target_end: usize = dec_len;

    let i: usize = 0;
    while (i < n_objs) : (i += 1) {
        const id_tok = hlex.nextToken();
        const off_tok = hlex.nextToken();
        if (id_tok.kind != PdfTokenKind.number_int || off_tok.kind != PdfTokenKind.number_int) break;
        const oid: u32 = parseU32Tok(header, id_tok) ?? break;
        const rel_off: usize = @as(usize, parseU32Tok(header, off_tok) ?? break);
        const abs_off: usize = first_off + rel_off;
        if (abs_off > dec_len) continue;

        if (found) {
            if (abs_off >= target_start && abs_off < target_end) {
                target_end = abs_off;
            }
            break;
        }
        if (oid == target_obj_num) {
            found = true;
            target_start = abs_off;
        }
    }

    if (!found || target_start >= target_end || target_end > dec_len) {
        return PdfParseError.ObjectNotFound;
    }

    // Trim leading whitespace before object value
    while (target_start < target_end && isPdfWhitespace(stm_scratch[target_start])) : (target_start += 1) {}
    const slice_len: usize = target_end - target_start;
    if (slice_len > out_buf.len) return PdfParseError.BufferTooSmall;
    let k: usize = 0;
    while (k < slice_len) : (k += 1) {
        out_buf[k] = stm_scratch[target_start + k];
    }
    return slice_len;
}

// ============================================================================
// 3. XRef Table + PDF 1.5+ XRef Stream + Incremental `/Prev` Chain (`PdfXrefTable`)
// ============================================================================

/// Caller-slice-backed PDF cross-reference table supporting multi-subsection `xref` tables,
/// PDF 1.5+ `/Type /XRef` streams, hybrid `/XRefStm`, and ISO 32000-1 §7.5.6 `/Prev` update chains.
pub struct PdfXrefTable {
    entries: []PdfXrefEntry = [],
    count: usize = 0;
    trailer: PdfTrailerInfo = PdfTrailerInfo{};

    fn findIndex(obj_num: u32): usize? {
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (entries[i].obj_num == obj_num) return i;
        }
        return null;
    }

    /// Looks up the active `PdfXrefEntry` for `obj_num`, or returns `null` if absent.
    fn findEntry(obj_num: u32): PdfXrefEntry? {
        if (idx := findIndex(obj_num)) {
            return entries[idx];
        }
        return null;
    }

    /// Parses the PDF `startxref` -> `xref` / `/Type /XRef` stream -> `trailer` chain
    /// (following `/Prev` incremental updates and hybrid `/XRefStm`) into caller-provided `out_entries`.
    static fn parse(pdf_bytes: []const u8, out_entries: []PdfXrefEntry): PdfXrefTable ! PdfParseError {
        if (PdfInspector.extractVersion(pdf_bytes) == null) {
            return PdfParseError.InvalidPdf;
        }
        const start_off = parseStartXrefOffset(pdf_bytes) ?? return PdfParseError.MissingXRef;

        let tbl = PdfXrefTable{
            entries: out_entries,
            count: 0,
            trailer: PdfTrailerInfo{},
        };

        let cur_off: usize = start_off;
        let hops: usize = 0;
        while (hops < 32) : (hops += 1) {
            if (cur_off >= pdf_bytes.len) return PdfParseError.InvalidXRef;
            let lex = PdfLexer.init(pdf_bytes, cur_off);
            const kw = lex.nextToken();

            // PDF 1.5+ Cross-Reference Stream (`N G obj << /Type /XRef ... >> stream`)
            if (kw.kind == PdfTokenKind.number_int) {
                const next_prev = parseXrefStreamAt(pdf_bytes, cur_off, &tbl)?;
                if (po := next_prev) {
                    if (po == cur_off) break;
                    cur_off = po;
                    continue;
                }
                break;
            }

            if (kw.kind != PdfTokenKind.keyword || !std.mem.eql(u8, kw.slice(pdf_bytes), "xref")) {
                return PdfParseError.InvalidXRef;
            }

            // Parse one or more subsections: `<first_obj> <num_entries>` followed by `<offset> <gen> <n|f>`
            while (true) {
                const tok = lex.nextToken();
                if (tok.kind == PdfTokenKind.keyword && std.mem.eql(u8, tok.slice(pdf_bytes), "trailer")) {
                    break;
                }
                if (tok.kind != PdfTokenKind.number_int) {
                    return PdfParseError.InvalidXRef;
                }
                const first_obj: u32 = parseU32Tok(pdf_bytes, tok) ?? return PdfParseError.InvalidXRef;
                const cnt_tok = lex.nextToken();
                const sub_cnt: u32 = parseU32Tok(pdf_bytes, cnt_tok) ?? return PdfParseError.InvalidXRef;

                let j: u32 = 0;
                while (j < sub_cnt) : (j += 1) {
                    const off_t = lex.nextToken();
                    const gen_t = lex.nextToken();
                    const flag_t = lex.nextToken();
                    if (off_t.kind != PdfTokenKind.number_int || gen_t.kind != PdfTokenKind.number_int || flag_t.kind != PdfTokenKind.keyword) {
                        return PdfParseError.InvalidXRef;
                    }
                    const entry_off: usize = std.fmt.parseInt(usize, off_t.slice(pdf_bytes), 10) catch return PdfParseError.InvalidXRef;
                    const entry_gen: u32 = parseU32Tok(pdf_bytes, gen_t) ?? return PdfParseError.InvalidXRef;
                    const flag_s = flag_t.slice(pdf_bytes);
                    const in_use: bool = std.mem.eql(u8, flag_s, "n");
                    if (!in_use && !std.mem.eql(u8, flag_s, "f")) {
                        return PdfParseError.InvalidXRef;
                    }

                    const obj_num: u32 = first_obj + j;
                    if (tbl.findIndex(obj_num) == null) {
                        if (tbl.count >= tbl.entries.len) {
                            return PdfParseError.OutputSliceTooSmall;
                        }
                        tbl.entries[tbl.count] = PdfXrefEntry{
                            obj_num: obj_num,
                            gen_num: @as(u16, @intCast(entry_gen & 0xFFFF)),
                            offset: entry_off,
                            in_use: in_use,
                        };
                        tbl.count += 1;
                    }
                }
            }

            // Parse `trailer << ... >>` dictionary
            lex.skipWsAndComments();
            const tr_start = lex.pos;
            const tr_open = lex.nextToken();
            if (tr_open.kind != PdfTokenKind.dict_start) {
                return PdfParseError.MissingTrailer;
            }
            lex.pos = tr_start;
            lex.skipValue();
            const tr_slice = pdf_bytes[tr_start..lex.pos];

            tbl.trailer.xref_sections_parsed += 1;
            mergeTrailerDict(tr_slice, &tbl.trailer);

            // Hybrid PDF 1.5+: merge optional `/XRefStm <offset>` entries
            if (xsp := findDictValuePos(tr_slice, "/XRefStm")) {
                let xsl = PdfLexer.init(tr_slice, xsp);
                const xst = xsl.nextToken();
                if (xst.kind == PdfTokenKind.number_int) {
                    const xstm_off: usize = std.fmt.parseInt(usize, xst.slice(tr_slice), 10) catch 0;
                    if (xstm_off > 0 && xstm_off < pdf_bytes.len) {
                        tbl.trailer.has_xref_stm = true;
                        tbl.trailer.xref_stm_offset = xstm_off;
                        * = parseXrefStreamAt(pdf_bytes, xstm_off, &tbl) catch null;
                    }
                }
            }

            // Follow `/Prev` incremental xref link if present
            if (pp := findDictValuePos(tr_slice, "/Prev")) {
                let pl = PdfLexer.init(tr_slice, pp);
                const pt = pl.nextToken();
                if (pt.kind == PdfTokenKind.number_int) {
                    const prev_off: usize = std.fmt.parseInt(usize, pt.slice(tr_slice), 10) catch return PdfParseError.InvalidXRef;
                    if (!tbl.trailer.has_prev) {
                        tbl.trailer.has_prev = true;
                        tbl.trailer.prev_xref_offset = prev_off;
                    }
                    if (prev_off == cur_off) break;
                    cur_off = prev_off;
                    continue;
                }
            }
            break;
        }

        return tbl;
    }

    /// Resolves uncompressed indirect object `obj_ref` via `xref` and returns the zero-copy body slice
    /// between `N G obj` and `endobj`.
    fn locateObjectBody(pdf_bytes: []const u8, obj_ref: PdfObjRef): ([]const u8)? {
        const entry = findEntry(obj_ref.obj_num) ?? return null;
        if (!entry.in_use || entry.is_compressed || entry.offset >= pdf_bytes.len) return null;
        return locateObjectBodyAtOffset(pdf_bytes, entry.offset);
    }

    /// Transparently resolves both uncompressed (`type 1`) and compressed `/Type /ObjStm` (`type 2`)
    /// indirect objects. Uncompressed objects return a zero-copy slice of `pdf_bytes`; compressed
    /// objects are extracted into `out_buf` and returned as `out_buf[0..n]`.
    fn resolveObjectBody(pdf_bytes: []const u8, obj_ref: PdfObjRef, out_buf: []u8): ([]const u8)? {
        const entry = findEntry(obj_ref.obj_num) ?? return null;
        if (!entry.in_use) return null;
        if (!entry.is_compressed) {
            if (entry.offset >= pdf_bytes.len) return null;
            return locateObjectBodyAtOffset(pdf_bytes, entry.offset);
        }
        const n = locateObjectBodyFromObjStm(pdf_bytes, _, entry.obj_stm_num, obj_ref.obj_num, out_buf) catch return null;
        return out_buf[0..n];
    }
}

// ============================================================================
// 4. Name, Literal/Hex/UTF-16BE String Decoders & Stream Filter Engine
// ============================================================================

fn appendUtf8Codepoint(out: []u8, pos: *usize, cp: u32): void ! PdfParseError {
    if (cp < 0x80) {
        if (pos.* + 1 > out.len) return PdfParseError.BufferTooSmall;
        out[pos.*] = cp as u8;
        pos.* += 1;
    } else if (cp < 0x800) {
        if (pos.* + 2 > out.len) return PdfParseError.BufferTooSmall;
        out[pos.*] = (0xC0 | (cp >> 6)) as u8;
        out[pos.* + 1] = (0x80 | (cp & 0x3F)) as u8;
        pos.* += 2;
    } else if (cp < 0x10000) {
        if (pos.* + 3 > out.len) return PdfParseError.BufferTooSmall;
        out[pos.*] = (0xE0 | (cp >> 12)) as u8;
        out[pos.* + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[pos.* + 2] = (0x80 | (cp & 0x3F)) as u8;
        pos.* += 3;
    } else {
        if (pos.* + 4 > out.len) return PdfParseError.BufferTooSmall;
        out[pos.*] = (0xF0 | (cp >> 18)) as u8;
        out[pos.* + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
        out[pos.* + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[pos.* + 3] = (0x80 | (cp & 0x3F)) as u8;
        pos.* += 4;
    }
}

/// Decodes a PDF Name token (e.g. `"/Flate#44ecode"`) stripping the leading `/` and resolving `#XX` escapes.
pub fn decodePdfName(raw_name: []const u8, out_buf: []u8): usize ! PdfParseError {
    let i: usize = if (raw_name.len > 0 && raw_name[0] == '/') 1 else 0;
    let out_pos: usize = 0;
    while (i < raw_name.len) {
        if (raw_name[i] == '#' && i + 2 < raw_name.len) {
            if (hi := hexNibble(raw_name[i + 1])) {
                if (lo := hexNibble(raw_name[i + 2])) {
                    if (out_pos >= out_buf.len) return PdfParseError.BufferTooSmall;
                    out_buf[out_pos] = (hi << 4) | lo;
                    out_pos += 1;
                    i += 3;
                    continue;
                }
            }
        }
        if (out_pos >= out_buf.len) return PdfParseError.BufferTooSmall;
        out_buf[out_pos] = raw_name[i];
        out_pos += 1;
        i += 1;
    }
    return out_pos;
}

fn decodeRawPdfStringBytes(raw_token: []const u8, out_raw: []u8): usize ! PdfParseError {
    if (raw_token.len < 2) return 0;
    let out_pos: usize = 0;

    if (raw_token[0] == '(' && raw_token[raw_token.len - 1] == ')') {
        const inner = raw_token[1 .. raw_token.len - 1];
        let i: usize = 0;
        while (i < inner.len) {
            const c = inner[i];
            if (c == '\\' && i + 1 < inner.len) {
                const nxt = inner[i + 1];
                if (nxt == 'n') {
                    if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
                    out_raw[out_pos] = '\n';
                    out_pos += 1;
                    i += 2;
                    continue;
                } else if (nxt == 'r') {
                    if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
                    out_raw[out_pos] = '\r';
                    out_pos += 1;
                    i += 2;
                    continue;
                } else if (nxt == 't') {
                    if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
                    out_raw[out_pos] = '\t';
                    out_pos += 1;
                    i += 2;
                    continue;
                } else if (nxt == 'b') {
                    if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
                    out_raw[out_pos] = 0x08;
                    out_pos += 1;
                    i += 2;
                    continue;
                } else if (nxt == 'f') {
                    if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
                    out_raw[out_pos] = 0x0C;
                    out_pos += 1;
                    i += 2;
                    continue;
                } else if (nxt == '\r') {
                    i += 2;
                    if (i < inner.len && inner[i] == '\n') i += 1;
                    continue;
                } else if (nxt == '\n') {
                    i += 2;
                    continue;
                } else if (nxt >= '0' && nxt <= '7') {
                    let val: u32 = (nxt - '0') as u32;
                    let consumed: usize = 2;
                    if (i + 2 < inner.len && inner[i + 2] >= '0' && inner[i + 2] <= '7') {
                        val = (val << 3) | ((inner[i + 2] - '0') as u32);
                        consumed = 3;
                        if (i + 3 < inner.len && inner[i + 3] >= '0' && inner[i + 3] <= '7') {
                            val = (val << 3) | ((inner[i + 3] - '0') as u32);
                            consumed = 4;
                        }
                    }
                    if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
                    out_raw[out_pos] = (val & 0xFF) as u8;
                    out_pos += 1;
                    i += consumed;
                    continue;
                } else {
                    if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
                    out_raw[out_pos] = nxt;
                    out_pos += 1;
                    i += 2;
                    continue;
                }
            }
            if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
            out_raw[out_pos] = c;
            out_pos += 1;
            i += 1;
        }
        return out_pos;
    }

    if (raw_token[0] == '<' && raw_token[raw_token.len - 1] == '>') {
        const inner = raw_token[1 .. raw_token.len - 1];
        let have_hi: bool = false;
        let hi_val: u8 = 0;
        let i: usize = 0;
        while (i < inner.len) : (i += 1) {
            if (isPdfWhitespace(inner[i])) continue;
            if (nib := hexNibble(inner[i])) {
                if (!have_hi) {
                    hi_val = nib;
                    have_hi = true;
                } else {
                    if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
                    out_raw[out_pos] = (hi_val << 4) | nib;
                    out_pos += 1;
                    have_hi = false;
                }
            }
        }
        if (have_hi) {
            if (out_pos >= out_raw.len) return PdfParseError.BufferTooSmall;
            out_raw[out_pos] = hi_val << 4;
            out_pos += 1;
        }
        return out_pos;
    }

    return 0;
}

/// Decodes a PDF literal string `(...)` or hex string `<...>` (handling escapes, octal codes,
/// UTF-16BE BOM `FE FF` with surrogate pairs, and PDFDocEncoding) into UTF-8 in `out_buf`.
pub fn decodePdfStringToUtf8(raw_token: []const u8, out_buf: []u8): usize ! PdfParseError {
    let raw_len: usize = decodeRawPdfStringBytes(raw_token, out_buf)?;
    // Detect UTF-16BE BOM (0xFE 0xFF) and transcode in place via temporary conversion
    if (raw_len >= 2 && out_buf[0] == 0xFE && out_buf[1] == 0xFF) {
        let tmp: [512]u8 = [0; 512];
        const copy_n: usize = if (raw_len <= tmp.len) raw_len else tmp.len;
        let ci: usize = 0;
        while (ci < copy_n) : (ci += 1) {
            tmp[ci] = out_buf[ci];
        }
        let out_pos: usize = 0;
        let i: usize = 2;
        while (i + 1 < copy_n) {
            const cu1: u32 = ((tmp[i] as u32) << 8) | (tmp[i + 1] as u32);
            i += 2;
            if (cu1 >= 0xD800 && cu1 <= 0xDBFF && i + 1 < copy_n) {
                const cu2: u32 = ((tmp[i] as u32) << 8) | (tmp[i + 1] as u32);
                if (cu2 >= 0xDC00 && cu2 <= 0xDFFF) {
                    const cp: u32 = ((cu1 - 0xD800) << 10) + (cu2 - 0xDC00) + 0x10000;
                    appendUtf8Codepoint(out_buf, *out_pos, cp)?;
                    i += 2;
                    continue;
                }
            }
            appendUtf8Codepoint(out_buf, *out_pos, cu1)?;
        }
        return out_pos;
    }
    return raw_len;
}

/// Supported PDF stream compression / encoding filters.
pub enum PdfStreamFilter {
    none,
    flate_decode,
    ascii_hex_decode,
    unsupported,
}

/// PDF `/DecodeParms` predictor parameters (ISO 32000-1 §7.4.4.4: TIFF Predictor 2 & PNG Predictors 10..15).
pub struct PdfDecodeParms {
    predictor: u32 = 1;
    columns: u32 = 1;
    colors: u32 = 1;
    bits_per_component: u32 = 8;
}

/// Zero-copy view of a PDF stream object's dictionary, filter, decode parameters, and raw encoded bytes.
pub struct PdfStreamView {
    dict_slice: []const u8 = "";
    raw_data: []const u8 = "";
    filter: PdfStreamFilter = PdfStreamFilter.none;
    decode_parms: PdfDecodeParms = PdfDecodeParms{};
}

fn parseDecodeParms(dict_slice: []const u8): PdfDecodeParms {
    let parms = PdfDecodeParms{};
    const dp = findDictValuePos(dict_slice, "/DecodeParms") ?? return parms;
    let lex = PdfLexer.init(dict_slice, dp);
    let tok = lex.nextToken();
    if (tok.kind == PdfTokenKind.array_start) {
        tok = lex.nextToken();
    }
    if (tok.kind != PdfTokenKind.dict_start) return parms;

    const sub_start = tok.start;
    lex.pos = sub_start;
    lex.skipValue();
    const sub_dict = dict_slice[sub_start..lex.pos];

    if (pp := findDictValuePos(sub_dict, "/Predictor")) {
        let pl = PdfLexer.init(sub_dict, pp);
        if (v := parseU32Tok(sub_dict, pl.nextToken())) parms.predictor = v;
    }
    if (cp := findDictValuePos(sub_dict, "/Columns")) {
        let cl = PdfLexer.init(sub_dict, cp);
        if (v := parseU32Tok(sub_dict, cl.nextToken())) {
            if (v > 0) parms.columns = v;
        }
    }
    if (colp := findDictValuePos(sub_dict, "/Colors")) {
        let col_l = PdfLexer.init(sub_dict, colp);
        if (v := parseU32Tok(sub_dict, col_l.nextToken())) {
            if (v > 0) parms.colors = v;
        }
    }
    if (bp := findDictValuePos(sub_dict, "/BitsPerComponent")) {
        let bl = PdfLexer.init(sub_dict, bp);
        if (v := parseU32Tok(sub_dict, bl.nextToken())) {
            if (v > 0) parms.bits_per_component = v;
        }
    }
    return parms;
}

fn pdfPaethPredict(left: u8, above: u8, upper_left: u8): u8 {
    const a: i32 = @as(i32, left);
    const b: i32 = @as(i32, above);
    const c: i32 = @as(i32, upper_left);
    const p: i32 = a + b - c;
    let pa: i32 = p - a;
    if (pa < 0) pa = -pa;
    let pb: i32 = p - b;
    if (pb < 0) pb = -pb;
    let pc: i32 = p - c;
    if (pc < 0) pc = -pc;
    if (pa <= pb && pa <= pc) return left;
    if (pb <= pc) return above;
    return upper_left;
}

/// Applies ISO 32000-1 §7.4.4.4 PNG Predictor (10..15: None/Sub/Up/Average/Paeth) or
/// TIFF Predictor 2 reverse filtering on `raw` into caller-provided `out_buf` with zero heap allocation.
pub fn applyPdfPredictorDecode(raw: []const u8, parms: PdfDecodeParms, out_buf: []u8): usize ! PdfParseError {
    if (parms.predictor <= 1) {
        if (raw.len > out_buf.len) return PdfParseError.BufferTooSmall;
        let i: usize = 0;
        while (i < raw.len) : (i += 1) {
            out_buf[i] = raw[i];
        }
        return raw.len;
    }

    if (parms.columns == 0 || parms.colors == 0 || parms.bits_per_component == 0) {
        return PdfParseError.InvalidStream;
    }

    const bpp_calc: usize = (@as(usize, parms.colors) * @as(usize, parms.bits_per_component) + 7) / 8;
    const bpp: usize = if (bpp_calc == 0) 1 else bpp_calc;
    const row_bytes: usize = (@as(usize, parms.columns) * @as(usize, parms.colors) * @as(usize, parms.bits_per_component) + 7) / 8;
    if (row_bytes == 0) return PdfParseError.InvalidStream;

    // TIFF Predictor 2 (horizontal differencing, 8-bit components)
    if (parms.predictor == 2) {
        if (raw.len > out_buf.len) return PdfParseError.BufferTooSmall;
        if (parms.bits_per_component != 8) return PdfParseError.UnsupportedFilter;
        let pos: usize = 0;
        while (pos < raw.len) {
            const avail: usize = raw.len - pos;
            const rlen: usize = if (avail < row_bytes) avail else row_bytes;
            let i: usize = 0;
            while (i < rlen) : (i += 1) {
                if (i < bpp) {
                    out_buf[pos + i] = raw[pos + i];
                } else {
                    out_buf[pos + i] = raw[pos + i] +% out_buf[pos + i - bpp];
                }
            }
            pos += rlen;
        }
        return raw.len;
    }

    // PNG Predictors 10..15 (each row prefixed with 1-byte filter type 0..4)
    if (parms.predictor >= 10 && parms.predictor <= 15) {
        if (raw.len == 0) return 0;
        const src_stride: usize = row_bytes + 1;
        if (raw.len < src_stride) return PdfParseError.InvalidStream;

        let in_pos: usize = 0;
        let out_pos: usize = 0;
        let has_prev: bool = false;
        let prev_off: usize = 0;

        while (in_pos < raw.len) {
            if (in_pos + src_stride > raw.len) return PdfParseError.InvalidStream;
            if (out_pos + row_bytes > out_buf.len) return PdfParseError.BufferTooSmall;

            const filter_type: u8 = raw[in_pos];
            in_pos += 1;
            const eff_bpp: usize = if (bpp < row_bytes) bpp else row_bytes;

            if (filter_type == 0) {
                // None
                let i: usize = 0;
                while (i < row_bytes) : (i += 1) {
                    out_buf[out_pos + i] = raw[in_pos + i];
                }
            } else if (filter_type == 1) {
                // Sub
                let i: usize = 0;
                while (i < row_bytes) : (i += 1) {
                    const left: u8 = if (i >= eff_bpp) out_buf[out_pos + i - eff_bpp] else 0;
                    out_buf[out_pos + i] = raw[in_pos + i] +% left;
                }
            } else if (filter_type == 2) {
                // Up
                let i: usize = 0;
                while (i < row_bytes) : (i += 1) {
                    const up: u8 = if (has_prev) out_buf[prev_off + i] else 0;
                    out_buf[out_pos + i] = raw[in_pos + i] +% up;
                }
            } else if (filter_type == 3) {
                // Average
                let i: usize = 0;
                while (i < row_bytes) : (i += 1) {
                    const left: u16 = if (i >= eff_bpp) @as(u16, out_buf[out_pos + i - eff_bpp]) else 0;
                    const up: u16 = if (has_prev) @as(u16, out_buf[prev_off + i]) else 0;
                    out_buf[out_pos + i] = raw[in_pos + i] +% @as(u8, @intCast((left + up) >> 1));
                }
            } else if (filter_type == 4) {
                // Paeth
                let i: usize = 0;
                while (i < row_bytes) : (i += 1) {
                    const left: u8 = if (i >= eff_bpp) out_buf[out_pos + i - eff_bpp] else 0;
                    const up: u8 = if (has_prev) out_buf[prev_off + i] else 0;
                    const ul: u8 = if (has_prev && i >= eff_bpp) out_buf[prev_off + i - eff_bpp] else 0;
                    out_buf[out_pos + i] = raw[in_pos + i] +% pdfPaethPredict(left, up, ul);
                }
            } else {
                return PdfParseError.InvalidStream;
            }

            prev_off = out_pos;
            has_prev = true;
            in_pos += row_bytes;
            out_pos += row_bytes;
        }
        return out_pos;
    }

    return PdfParseError.UnsupportedFilter;
}

fn parseFilterKind(dict_slice: []const u8): PdfStreamFilter {
    const fp = findDictValuePos(dict_slice, "/Filter") ?? return PdfStreamFilter.none;
    let lex = PdfLexer.init(dict_slice, fp);
    let tok = lex.nextToken();
    if (tok.kind == PdfTokenKind.array_start) {
        tok = lex.nextToken();
    }
    if (tok.kind != PdfTokenKind.name) return PdfStreamFilter.none;
    const name = tok.slice(dict_slice);
    if (std.mem.eql(u8, name, "/FlateDecode") || std.mem.eql(u8, name, "/Fl")) {
        return PdfStreamFilter.flate_decode;
    }
    if (std.mem.eql(u8, name, "/ASCIIHexDecode") || std.mem.eql(u8, name, "/AHx")) {
        return PdfStreamFilter.ascii_hex_decode;
    }
    return PdfStreamFilter.unsupported;
}

/// Extracts the dictionary, filter, and exact encoded payload slice from an indirect stream object body
/// (supporting both direct `/Length N` and indirect `/Length N G R` resolved through `xref`).
pub fn extractStreamFromObject(
    pdf_bytes: []const u8,
    xref: PdfXrefTable,
    obj_body: []const u8,
): PdfStreamView ! PdfParseError {
    let lex = PdfLexer.init(obj_body, 0);
    lex.skipWsAndComments();
    const dict_start = lex.pos;
    const first = lex.nextToken();
    if (first.kind != PdfTokenKind.dict_start) {
        return PdfParseError.InvalidStream;
    }
    lex.pos = dict_start;
    lex.skipValue();
    const dict_slice = obj_body[dict_start..lex.pos];

    // Locate `stream\r?\n` immediately after dictionary
    lex.skipWsAndComments();
    const body_start = matchStreamKeyword(obj_body, lex.pos) ?? return PdfParseError.InvalidStream;

    // Resolve `/Length` (direct integer or indirect object ref)
    let declared_len: usize? = null;
    if (lp := findDictValuePos(dict_slice, "/Length")) {
        if (lref := parseObjRefAt(dict_slice, lp)) {
            if (len_body := xref.locateObjectBody(pdf_bytes, lref)) {
                let ll = PdfLexer.init(len_body, 0);
                if (lv := parseU32Tok(len_body, ll.nextToken())) {
                    declared_len = lv as usize;
                }
            }
        } else {
            let ll = PdfLexer.init(dict_slice, lp);
            if (lv := parseU32Tok(dict_slice, ll.nextToken())) {
                declared_len = lv as usize;
            }
        }
    }

    let raw_end: usize = body_start;
    if (dlen := declared_len) {
        if (body_start + dlen > obj_body.len) return PdfParseError.InvalidStream;
        raw_end = body_start + dlen;
    } else {
        raw_end = std.mem.indexOfPos(u8, obj_body, body_start, "endstream") ?? return PdfParseError.InvalidStream;
        while (raw_end > body_start && (obj_body[raw_end - 1] == '\n' || obj_body[raw_end - 1] == '\r')) {
            raw_end -= 1;
        }
    }

    return PdfStreamView{
        dict_slice: dict_slice,
        raw_data: obj_body[body_start..raw_end],
        filter: parseFilterKind(dict_slice),
        decode_parms: parseDecodeParms(dict_slice),
    };
}

/// Decodes a `PdfStreamView` (raw uncompressed, `/FlateDecode` via `cap:pack` `inflate`, or `/ASCIIHexDecode`,
/// automatically applying `/DecodeParms` PNG 10..15 / TIFF 2 Predictor reverse filtering) into `out_buf`.
pub fn decodePdfStream(stream: PdfStreamView, out_buf: []u8): usize ! PdfParseError {
    if (stream.filter == PdfStreamFilter.none) {
        if (stream.decode_parms.predictor > 1) {
            return applyPdfPredictorDecode(stream.raw_data, stream.decode_parms, out_buf);
        }
        if (stream.raw_data.len > out_buf.len) return PdfParseError.BufferTooSmall;
        let i: usize = 0;
        while (i < stream.raw_data.len) : (i += 1) {
            out_buf[i] = stream.raw_data[i];
        }
        return stream.raw_data.len;
    }

    if (stream.filter == PdfStreamFilter.ascii_hex_decode) {
        let out_pos: usize = 0;
        let have_hi: bool = false;
        let hi_val: u8 = 0;
        let i: usize = 0;
        while (i < stream.raw_data.len) : (i += 1) {
            const c = stream.raw_data[i];
            if (c == '>') break;
            if (isPdfWhitespace(c)) continue;
            if (nib := hexNibble(c)) {
                if (!have_hi) {
                    hi_val = nib;
                    have_hi = true;
                } else {
                    if (out_pos >= out_buf.len) return PdfParseError.BufferTooSmall;
                    out_buf[out_pos] = (hi_val << 4) | nib;
                    out_pos += 1;
                    have_hi = false;
                }
            }
        }
        if (have_hi) {
            if (out_pos >= out_buf.len) return PdfParseError.BufferTooSmall;
            out_buf[out_pos] = hi_val << 4;
            out_pos += 1;
        }
        return out_pos;
    }

    if (stream.filter == PdfStreamFilter.flate_decode) {
        const decomp_opt = inflate(stream.raw_data);
        if (decomp := decomp_opt) {
            if (stream.decode_parms.predictor > 1) {
                return applyPdfPredictorDecode(decomp, stream.decode_parms, out_buf);
            }
            if (decomp.len > out_buf.len) return PdfParseError.BufferTooSmall;
            let i: usize = 0;
            while (i < decomp.len) : (i += 1) {
                out_buf[i] = decomp[i];
            }
            return decomp.len;
        }
        return PdfParseError.DecompressFailed;
    }

    return PdfParseError.UnsupportedFilter;
}

// ============================================================================
// 5. Document Metadata, Page Tree Traversal & Outlines (External Slices)
// ============================================================================

/// Offset/length span inside a caller-provided UTF-8 arena buffer.
pub struct PdfMetaSpan {
    off: usize = 0;
    len: usize = 0;

    fn slice(arena: []const u8): []const u8 {
        if (len == 0 || off + len > arena.len) return "";
        return arena[off .. off + len];
    }
}

/// Rich UTF-8 decoded document metadata extracted from trailer `/Info` & `/Encrypt`.
pub struct PdfDocMetadata {
    version: []const u8 = "";
    is_encrypted: bool = false;
    title: PdfMetaSpan = PdfMetaSpan{};
    author: PdfMetaSpan = PdfMetaSpan{};
    subject: PdfMetaSpan = PdfMetaSpan{};
    keywords: PdfMetaSpan = PdfMetaSpan{};
    creator: PdfMetaSpan = PdfMetaSpan{};
    producer: PdfMetaSpan = PdfMetaSpan{};
    creation_date: PdfMetaSpan = PdfMetaSpan{};
    mod_date: PdfMetaSpan = PdfMetaSpan{};
    arena_used: usize = 0;
}

fn decodeDictStringField(
    dict_slice: []const u8,
    key: []const u8,
    arena: []u8,
    arena_pos: *usize,
): PdfMetaSpan ! PdfParseError {
    const vp = findDictValuePos(dict_slice, key) ?? return PdfMetaSpan{};
    let lex = PdfLexer.init(dict_slice, vp);
    const tok = lex.nextToken();
    if (tok.kind != PdfTokenKind.literal_string && tok.kind != PdfTokenKind.hex_string) {
        return PdfMetaSpan{};
    }
    const start_off = arena_pos.*;
    const written = decodePdfStringToUtf8(tok.slice(dict_slice), arena[start_off..])?;
    arena_pos.* += written;
    return PdfMetaSpan{ off: start_off, len: written };
}

/// Extracts all standard `/Info` metadata fields (decoding literal/hex/UTF-16BE strings into `out_arena`).
pub fn extractPdfMetadata(
    pdf_bytes: []const u8,
    xref: PdfXrefTable,
    out_arena: []u8,
): PdfDocMetadata ! PdfParseError {
    let meta = PdfDocMetadata{
        version: PdfInspector.extractVersion(pdf_bytes) ?? "",
        is_encrypted: xref.trailer.has_encrypt,
    };
    if (!xref.trailer.has_info) return meta;

    let info_scratch: [8192]u8 = [0; 8192];
    const info_body = xref.resolveObjectBody(pdf_bytes, xref.trailer.info, info_scratch[0..]) ?? return meta;
    let pos: usize = 0;
    meta.title = decodeDictStringField(info_body, "/Title", out_arena, *pos)?;
    meta.author = decodeDictStringField(info_body, "/Author", out_arena, *pos)?;
    meta.subject = decodeDictStringField(info_body, "/Subject", out_arena, *pos)?;
    meta.keywords = decodeDictStringField(info_body, "/Keywords", out_arena, *pos)?;
    meta.creator = decodeDictStringField(info_body, "/Creator", out_arena, *pos)?;
    meta.producer = decodeDictStringField(info_body, "/Producer", out_arena, *pos)?;
    meta.creation_date = decodeDictStringField(info_body, "/CreationDate", out_arena, *pos)?;
    meta.mod_date = decodeDictStringField(info_body, "/ModDate", out_arena, *pos)?;
    meta.arena_used = pos;
    return meta;
}

/// PDF rectangle `[llx lly urx ury]` in PDF points (1/72 inch).
pub struct PdfRect {
    llx: f64 = 0.0;
    lly: f64 = 0.0;
    urx: f64 = 0.0;
    ury: f64 = 0.0;

    fn width(): f64 {
        return if (urx >= llx) (urx - llx) else (llx - urx);
    }

    fn height(): f64 {
        return if (ury >= lly) (ury - lly) else (lly - ury);
    }
}

fn parseRectAt(bytes: []const u8, start_pos: usize): PdfRect? {
    let lex = PdfLexer.init(bytes, start_pos);
    if (lex.nextToken().kind != PdfTokenKind.array_start) return null;
    const x0 = parseF64Tok(bytes, lex.nextToken()) ?? return null;
    const y0 = parseF64Tok(bytes, lex.nextToken()) ?? return null;
    const x1 = parseF64Tok(bytes, lex.nextToken()) ?? return null;
    const y1 = parseF64Tok(bytes, lex.nextToken()) ?? return null;
    return PdfRect{ llx: x0, lly: y0, urx: x1, ury: y1 };
}

/// Structural descriptor for a single PDF page collected from `/Pages` -> `/Kids`.
pub struct PdfPageInfo {
    page_index: usize = 0;
    obj_ref: PdfObjRef = PdfObjRef{};
    media_box: PdfRect = PdfRect{};
    has_crop_box: bool = false;
    crop_box: PdfRect = PdfRect{};
    rotate: i32 = 0;
    has_contents_ref: bool = false;
    contents_ref: PdfObjRef = PdfObjRef{};
    has_contents_array: bool = false;
    font_count: usize = 0;
}

fn countFontsInResources(pdf_bytes: []const u8, xref: PdfXrefTable, page_body: []const u8): usize {
    const rp = findDictValuePos(page_body, "/Resources") ?? return 0;
    let res_slice: []const u8 = page_body;
    if (rref := parseObjRefAt(page_body, rp)) {
        res_slice = xref.locateObjectBody(pdf_bytes, rref) ?? return 0;
    } else {
        let rl = PdfLexer.init(page_body, rp);
        const rstart = rl.pos;
        rl.skipValue();
        res_slice = page_body[rstart..rl.pos];
    }

    const fp = findDictValuePos(res_slice, "/Font") ?? return 0;
    let font_slice: []const u8 = res_slice;
    if (fref := parseObjRefAt(res_slice, fp)) {
        font_slice = xref.locateObjectBody(pdf_bytes, fref) ?? return 0;
    } else {
        let fl = PdfLexer.init(res_slice, fp);
        const fstart = fl.pos;
        fl.skipValue();
        font_slice = res_slice[fstart..fl.pos];
    }

    let lex = PdfLexer.init(font_slice, 0);
    if (lex.nextToken().kind != PdfTokenKind.dict_start) return 0;
    let cnt: usize = 0;
    while (true) {
        const k = lex.nextToken();
        if (k.kind == PdfTokenKind.eof || k.kind == PdfTokenKind.dict_end) break;
        if (k.kind != PdfTokenKind.name) break;
        cnt += 1;
        lex.skipValue();
    }
    return cnt;
}

/// Iteratively walks `/Root -> /Pages -> /Kids` using caller-provided `stack_work` (zero recursion,
/// arbitrary page count) and populates `out_pages` with MediaBox, CropBox, Rotate, Contents & font counts.
pub fn collectPdfPages(
    pdf_bytes: []const u8,
    xref: PdfXrefTable,
    stack_work: []PdfObjRef,
    out_pages: []PdfPageInfo,
): usize ! PdfParseError {
    if (!xref.trailer.has_root) return PdfParseError.InvalidObject;
    if (stack_work.len == 0) return PdfParseError.BufferTooSmall;

    let obj_scratch: [16384]u8 = [0; 16384];
    const root_body = xref.resolveObjectBody(pdf_bytes, xref.trailer.root, obj_scratch[0..]) ?? return PdfParseError.ObjectNotFound;
    const pages_pos = findDictValuePos(root_body, "/Pages") ?? return PdfParseError.InvalidObject;
    const pages_ref = parseObjRefAt(root_body, pages_pos) ?? return PdfParseError.InvalidObject;

    // Inherit root `/MediaBox` & `/Rotate` if present on root `/Pages`
    let default_mb = PdfRect{};
    let default_rot: i32 = 0;
    if (root_pages_body := xref.resolveObjectBody(pdf_bytes, pages_ref, obj_scratch[0..])) {
        if (mbp := findDictValuePos(root_pages_body, "/MediaBox")) {
            if (mb := parseRectAt(root_pages_body, mbp)) default_mb = mb;
        }
        if (rotp := findDictValuePos(root_pages_body, "/Rotate")) {
            let rl = PdfLexer.init(root_pages_body, rotp);
            if (rv := parseI32Tok(root_pages_body, rl.nextToken())) default_rot = rv;
        }
    }

    let sp: usize = 0;
    stack_work[sp] = pages_ref;
    sp += 1;

    let page_count: usize = 0;
    while (sp > 0) {
        sp -= 1;
        const cur_ref = stack_work[sp];
        const body = xref.resolveObjectBody(pdf_bytes, cur_ref, obj_scratch[0..]) ?? continue;

        const tp = findDictValuePos(body, "/Type") ?? continue;
        let tlex = PdfLexer.init(body, tp);
        const ttok = tlex.nextToken();
        if (ttok.kind != PdfTokenKind.name) continue;
        const tname = ttok.slice(body);

        if (std.mem.eql(u8, tname, "/Pages")) {
            // Collect `/Kids [...]` and push in reverse order so DFS preserves document page order
            const kp = findDictValuePos(body, "/Kids") ?? continue;
            let klex = PdfLexer.init(body, kp);
            if (klex.nextToken().kind != PdfTokenKind.array_start) continue;

            const push_base = sp;
            while (true) {
                const saved = klex.pos;
                const nt = klex.nextToken();
                if (nt.kind == PdfTokenKind.eof || nt.kind == PdfTokenKind.array_end) break;
                if (kref := parseObjRefAt(body, saved)) {
                    // Advance past `N G R`
                    klex.pos = saved;
                    klex.skipValue();
                    if (sp >= stack_work.len) return PdfParseError.StackOverflow;
                    stack_work[sp] = kref;
                    sp += 1;
                }
            }
            // Reverse `[push_base .. sp)` in place so leftmost kid is popped first
            let lo: usize = push_base;
            let hi: usize = sp;
            while (lo + 1 < hi) {
                hi -= 1;
                const tmp = stack_work[lo];
                stack_work[lo] = stack_work[hi];
                stack_work[hi] = tmp;
                lo += 1;
            }
        } else if (std.mem.eql(u8, tname, "/Page")) {
            if (page_count >= out_pages.len) return PdfParseError.OutputSliceTooSmall;
            let pi = PdfPageInfo{
                page_index: page_count,
                obj_ref: cur_ref,
                media_box: default_mb,
                rotate: default_rot,
            };
            if (mbp := findDictValuePos(body, "/MediaBox")) {
                if (mb := parseRectAt(body, mbp)) pi.media_box = mb;
            }
            if (cbp := findDictValuePos(body, "/CropBox")) {
                if (cb := parseRectAt(body, cbp)) {
                    pi.has_crop_box = true;
                    pi.crop_box = cb;
                }
            }
            if (rotp := findDictValuePos(body, "/Rotate")) {
                let rl = PdfLexer.init(body, rotp);
                if (rv := parseI32Tok(body, rl.nextToken())) pi.rotate = rv;
            }
            if (cp := findDictValuePos(body, "/Contents")) {
                if (cref := parseObjRefAt(body, cp)) {
                    pi.has_contents_ref = true;
                    pi.contents_ref = cref;
                } else {
                    let cl = PdfLexer.init(body, cp);
                    if (cl.nextToken().kind == PdfTokenKind.array_start) {
                        pi.has_contents_array = true;
                    }
                }
            }
            pi.font_count = countFontsInResources(pdf_bytes, xref, body);
            out_pages[page_count] = pi;
            page_count += 1;
        }
    }

    return page_count;
}

/// Bookmark / outline entry extracted from `/Root -> /Outlines`.
pub struct PdfOutlineItem {
    title_off: usize = 0;
    title_len: usize = 0;
    depth: u16 = 0;
    obj_ref: PdfObjRef = PdfObjRef{};

    fn titleSlice(arena: []const u8): []const u8 {
        if (title_len == 0 || title_off + title_len > arena.len) return "";
        return arena[title_off .. title_off + title_len];
    }
}

/// Walks `/Root -> /Outlines` (`/First` / `/Next` linked chain) and extracts UTF-8 bookmark titles.
pub fn extractPdfOutlines(
    pdf_bytes: []const u8,
    xref: PdfXrefTable,
    out_items: []PdfOutlineItem,
    out_arena: []u8,
    arena_used: *usize,
): usize ! PdfParseError {
    if (!xref.trailer.has_root) return 0;
    const root_body = xref.locateObjectBody(pdf_bytes, xref.trailer.root) ?? return 0;
    const op = findDictValuePos(root_body, "/Outlines") ?? return 0;
    const outlines_ref = parseObjRefAt(root_body, op) ?? return 0;
    const outlines_body = xref.locateObjectBody(pdf_bytes, outlines_ref) ?? return 0;

    const fp = findDictValuePos(outlines_body, "/First") ?? return 0;
    let cur_opt: PdfObjRef? = parseObjRefAt(outlines_body, fp);
    let count: usize = 0;
    let guard: usize = 0;

    while (guard < 4096) : (guard += 1) {
        const cur_ref = cur_opt ?? break;
        const item_body = xref.locateObjectBody(pdf_bytes, cur_ref) ?? break;
        if (count >= out_items.len) return PdfParseError.OutputSliceTooSmall;

        const span = decodeDictStringField(item_body, "/Title", out_arena, arena_used)?;
        out_items[count] = PdfOutlineItem{
            title_off: span.off,
            title_len: span.len,
            depth: 0,
            obj_ref: cur_ref,
        };
        count += 1;

        if (np := findDictValuePos(item_body, "/Next")) {
            cur_opt = parseObjRefAt(item_body, np);
        } else {
            cur_opt = null;
        }
    }

    return count;
}

// ============================================================================
// 6. Production PDF Content Stream Text Extractor (`BT ... ET`)
// ============================================================================

fn appendCharSafe(out: []u8, pos: *usize, c: u8): void ! PdfParseError {
    if (pos.* >= out.len) return PdfParseError.BufferTooSmall;
    out[pos.*] = c;
    pos.* += 1;
}

fn emitSeparator(out: []u8, pos: *usize, sep: u8): void ! PdfParseError {
    if (pos.* == 0) return;
    const prev = out[pos.* - 1];
    if (sep == '\n') {
        if (prev != '\n') appendCharSafe(out, pos, '\n')?;
    } else if (sep == ' ') {
        if (prev != ' ' && prev != '\n' && prev != '\t') appendCharSafe(out, pos, ' ')?;
    }
}

/// Extracts plain UTF-8 text from a decoded PDF content stream (`BT ... ET`), supporting
/// `Tj`, `TJ` (with automatic kerning word-spacing), `'`, `"`, `T*`, `Td`, `TD`, and `Tm`.
pub fn extractTextFromContentStream(
    content_stream: []const u8,
    out_text: []u8,
): usize ! PdfParseError {
    let lex = PdfLexer.init(content_stream, 0);
    let in_text: bool = false;
    let out_pos: usize = 0;
    let has_y: bool = false;
    let cur_y: f64 = 0.0;

    let ops: [8]PdfToken = [PdfToken{}; 8];
    let op_cnt: usize = 0;

    while (true) {
        const tok = lex.nextToken();
        if (tok.kind == PdfTokenKind.eof) break;

        if (tok.kind == PdfTokenKind.keyword) {
            const kw = tok.slice(content_stream);
            if (std.mem.eql(u8, kw, "BT")) {
                in_text = true;
                has_y = false;
                op_cnt = 0;
                continue;
            }
            if (std.mem.eql(u8, kw, "ET")) {
                if (in_text && out_pos > 0) {
                    emitSeparator(out_text, *out_pos, '\n')?;
                }
                in_text = false;
                op_cnt = 0;
                continue;
            }

            if (in_text) {
                if (std.mem.eql(u8, kw, "Tj")) {
                    if (op_cnt >= 1) {
                        const st = ops[op_cnt - 1];
                        if (st.kind == PdfTokenKind.literal_string || st.kind == PdfTokenKind.hex_string) {
                            const w = decodePdfStringToUtf8(st.slice(content_stream), out_text[out_pos..])?;
                            out_pos += w;
                        }
                    }
                } else if (std.mem.eql(u8, kw, "'") || std.mem.eql(u8, kw, "\"")) {
                    emitSeparator(out_text, *out_pos, '\n')?;
                    if (op_cnt >= 1) {
                        const st = ops[op_cnt - 1];
                        if (st.kind == PdfTokenKind.literal_string || st.kind == PdfTokenKind.hex_string) {
                            const w = decodePdfStringToUtf8(st.slice(content_stream), out_text[out_pos..])?;
                            out_pos += w;
                        }
                    }
                } else if (std.mem.eql(u8, kw, "TJ")) {
                    if (op_cnt >= 1 && ops[op_cnt - 1].kind == PdfTokenKind.array_start) {
                        const arr_tok = ops[op_cnt - 1];
                        let alex = PdfLexer.init(content_stream, arr_tok.start + 1);
                        while (true) {
                            const el = alex.nextToken();
                            if (el.kind == PdfTokenKind.eof || el.kind == PdfTokenKind.array_end) break;
                            if (el.kind == PdfTokenKind.literal_string || el.kind == PdfTokenKind.hex_string) {
                                const w = decodePdfStringToUtf8(el.slice(content_stream), out_text[out_pos..])?;
                                out_pos += w;
                            } else if (el.kind == PdfTokenKind.number_int || el.kind == PdfTokenKind.number_real) {
                                if (kern := parseF64Tok(content_stream, el)) {
                                    // Negative kerning < -120 thousandths of em indicates word boundary
                                    if (kern < -120.0) {
                                        emitSeparator(out_text, *out_pos, ' ')?;
                                    }
                                }
                            }
                        }
                    }
                } else if (std.mem.eql(u8, kw, "T*")) {
                    emitSeparator(out_text, *out_pos, '\n')?;
                } else if (std.mem.eql(u8, kw, "Td") || std.mem.eql(u8, kw, "TD")) {
                    if (op_cnt >= 2) {
                        const tx = parseF64Tok(content_stream, ops[op_cnt - 2]) ?? 0.0;
                        const ty = parseF64Tok(content_stream, ops[op_cnt - 1]) ?? 0.0;
                        const abs_ty = if (ty < 0.0) -ty else ty;
                        if (abs_ty > 0.5) {
                            emitSeparator(out_text, *out_pos, '\n')?;
                        } else if (tx > 1.0) {
                            emitSeparator(out_text, *out_pos, ' ')?;
                        }
                        if (has_y) cur_y += ty;
                    }
                } else if (std.mem.eql(u8, kw, "Tm")) {
                    if (op_cnt >= 6) {
                        const new_y = parseF64Tok(content_stream, ops[op_cnt - 1]) ?? cur_y;
                        if (has_y) {
                            const dy = if (new_y > cur_y) (new_y - cur_y) else (cur_y - new_y);
                            if (dy > 0.5) {
                                emitSeparator(out_text, *out_pos, '\n')?;
                            }
                        }
                        cur_y = new_y;
                        has_y = true;
                    }
                }
            }
            op_cnt = 0;
            continue;
        }

        // Capture full `[...]` array span as a single operand token for `TJ`
        if (tok.kind == PdfTokenKind.array_start) {
            const arr_start = tok.start;
            let depth: usize = 1;
            while (depth > 0) {
                const nt = lex.nextToken();
                if (nt.kind == PdfTokenKind.eof) break;
                if (nt.kind == PdfTokenKind.array_start) depth += 1;
                if (nt.kind == PdfTokenKind.array_end) depth -= 1;
            }
            const arr_span = PdfToken{ kind: PdfTokenKind.array_start, start: arr_start, end: lex.pos };
            if (op_cnt < 8) {
                ops[op_cnt] = arr_span;
                op_cnt += 1;
            }
            continue;
        }

        if (op_cnt < 8) {
            ops[op_cnt] = tok;
            op_cnt += 1;
        } else {
            let s: usize = 1;
            while (s < 8) : (s += 1) {
                ops[s - 1] = ops[s];
            }
            ops[7] = tok;
        }
    }

    // Trim single trailing newline if present
    if (out_pos > 0 && out_text[out_pos - 1] == '\n') {
        out_pos -= 1;
    }
    return out_pos;
}

/// Decodes a page's `/Contents` stream (single indirect stream or array of streams, supporting
/// uncompressed, `/FlateDecode`, and `/ASCIIHexDecode`) and extracts plain UTF-8 text into `out_text`.
pub fn extractPageText(
    pdf_bytes: []const u8,
    xref: PdfXrefTable,
    page: PdfPageInfo,
    stream_scratch: []u8,
    out_text: []u8,
): usize ! PdfParseError {
    if (page.has_contents_ref) {
        const s_body = xref.locateObjectBody(pdf_bytes, page.contents_ref) ?? return PdfParseError.ObjectNotFound;
        const sv = extractStreamFromObject(pdf_bytes, xref, s_body)?;
        const dec_len = decodePdfStream(sv, stream_scratch)?;
        return extractTextFromContentStream(stream_scratch[0..dec_len], out_text);
    }

    if (page.has_contents_array) {
        let page_scratch: [8192]u8 = [0; 8192];
        const p_body = xref.resolveObjectBody(pdf_bytes, page.obj_ref, page_scratch[0..]) ?? return PdfParseError.ObjectNotFound;
        const cp = findDictValuePos(p_body, "/Contents") ?? return 0;
        let lex = PdfLexer.init(p_body, cp);
        if (lex.nextToken().kind != PdfTokenKind.array_start) return 0;

        let total_text: usize = 0;
        while (true) {
            const saved = lex.pos;
            const nt = lex.nextToken();
            if (nt.kind == PdfTokenKind.eof || nt.kind == PdfTokenKind.array_end) break;
            if (cref := parseObjRefAt(p_body, saved)) {
                lex.pos = saved;
                lex.skipValue();
                const s_body = xref.locateObjectBody(pdf_bytes, cref) ?? continue;
                const sv = extractStreamFromObject(pdf_bytes, xref, s_body)?;
                const dec_len = decodePdfStream(sv, stream_scratch)?;
                if (total_text > 0) {
                    emitSeparator(out_text, *total_text, '\n')?;
                }
                const wrote = extractTextFromContentStream(stream_scratch[0..dec_len], out_text[total_text..])?;
                total_text += wrote;
            }
        }
        return total_text;
    }

    return 0;
}
