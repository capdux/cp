// Source: cp/lib/pdf/inspector.cp (PDF Header, XRef, Page Count & Info Metadata Inspector)
// Pure CP zero-allocation PDF structural validator and metadata/page-count inspector.

const std = @import("std");
import { PdfError } from "./pdfWriter.cp";

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

fn skipWhitespace(bytes: []const u8, pos: *usize) {
    while (pos.* < bytes.len && isPdfWhitespace(bytes[pos.*])) {
        pos.* += 1;
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

/// Finds the end of a `stream ... endstream` section starting after `stream\n`.
fn skipStreamPayload(bytes: []const u8, body_start: usize): usize {
    const end_idx = std.mem.indexOfPos(u8, bytes, body_start, "endstream") ?? return bytes.len;
    return end_idx + 9;
}

/// Parses a PDF literal string `(...)` starting at `open_idx` (where `bytes[open_idx] == '('`).
/// Respects nested parentheses and `\` escape sequences, returning the inner raw slice.
fn parseLiteralStringSlice(bytes: []const u8, open_idx: usize): ([]const u8)? {
    if (open_idx >= bytes.len || bytes[open_idx] != '(') return null;
    let depth: usize = 1;
    let i: usize = open_idx + 1;
    while (i < bytes.len) : (i += 1) {
        const c = bytes[i];
        if (c == '\\') {
            if (i + 1 < bytes.len) {
                i += 1;
            }
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

/// Validates that `startxref` offset points to a valid `xref` table inside `bytes`.
fn hasValidXref(bytes: []const u8): bool {
    const xref_off = parseStartXrefOffset(bytes) ?? return false;
    if (xref_off >= bytes.len) return false;
    let p = xref_off;
    skipWhitespace(bytes, &p);
    if (p + 4 > bytes.len) return false;
    if (!std.mem.eql(u8, bytes[p..p + 4], "xref")) return false;
    if (p + 4 < bytes.len && !isPdfWhitespace(bytes[p + 4])) return false;
    return true;
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
