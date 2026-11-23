// Origin: Pure-CP Stdlib (Zero-Alloc OOXML SpreadsheetML .xlsx Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation OOXML SpreadsheetML parser:
// A1 cell reference codec, xl/workbook.xml sheet iterator, xl/sharedStrings.xml iterator
// with rich-text <r><t> concatenation & XML entity unescaping, and xl/worksheets/sheet*.xml
// streaming cell iterator.

const std = @import("std");
import { Simd } from "../simd/index.cp";

pub const MAX_XLSX_COL: u32 = 16383;    // Column XFD (16384 columns, 0-based)
pub const MAX_XLSX_ROW: u32 = 1048575;  // Row 1048576 (1048576 rows, 0-based)

/// 0-based Excel cell coordinate (`col` in 0..=16383, `row` in 0..=1048575).
pub struct XlsxCellRef {
    col: u32 = 0,
    row: u32 = 0,
}

/// Workbook sheet metadata parsed from `<sheet name="..." sheetId="..." r:id="..." state="..."/>`.
pub struct XlsxSheetInfo {
    name: []const u8 = "",
    sheet_id: u32 = 0,
    r_id: []const u8 = "",
    is_hidden: bool = false,
}

/// OOXML SpreadsheetML cell value type (`t` attribute on `<c>`).
pub enum XlsxCellType {
    number,
    shared_string,
    inline_string,
    formula_str,
    bool_val,
    error_val,
    unknown,
}

/// Zero-copy parsed worksheet cell from `<c r="..." t="..." s="...">...</c>`.
pub struct XlsxCell {
    ref: XlsxCellRef = XlsxCellRef {},
    cell_type: XlsxCellType = XlsxCellType.number,
    style_idx: u32 = 0,
    raw_value: []const u8 = "",
    formula: []const u8 = "",

    /// Returns the shared string table index if `cell_type == .shared_string`.
    fn asSharedStringIdx(): u32? {
        if (_.cell_type != XlsxCellType.shared_string || _.raw_value.len == 0) {
            return null;
        }
        return parseU32Strict(_.raw_value);
    }

    /// Returns the boolean value if `cell_type == .bool_val`.
    fn asBool(): bool? {
        if (_.cell_type != XlsxCellType.bool_val) {
            return null;
        }
        if (std.mem.eql(u8, _.raw_value, "1") || std.mem.eql(u8, _.raw_value, "true") || std.mem.eql(u8, _.raw_value, "TRUE")) {
            return true;
        }
        if (std.mem.eql(u8, _.raw_value, "0") || std.mem.eql(u8, _.raw_value, "false") || std.mem.eql(u8, _.raw_value, "FALSE")) {
            return false;
        }
        return null;
    }

    /// Parses `raw_value` as a finite `f64` for numeric cells (`number` or `unknown`).
    fn asF64(): f64? {
        if (_.raw_value.len == 0) {
            return null;
        }
        if (_.cell_type != XlsxCellType.number && _.cell_type != XlsxCellType.unknown) {
            return null;
        }
        const val = std.fmt.parseFloat(f64, _.raw_value) catch return null;
        if (!std.math.isFinite(val)) {
            return null;
        }
        return val;
    }
}

struct CloseTagMatch {
    close_start: usize,
    after_close: usize,
}

// ---------------------------------------------------------------------------
// A1 Cell Reference & Column Name Codec
// ---------------------------------------------------------------------------

/// Parses an Excel A1-style cell reference (e.g. `"A1"`, `"Z10"`, `"AA1"`, `"XFD1048576"`)
/// into 0-based `(col, row)`. Strictly validates uppercase ASCII letters, non-zero row
/// without leading zeros, and Excel OOXML bounds (`XFD` / `1048576`).
pub fn parseCellRef(cell_ref: []const u8): XlsxCellRef? {
    if (cell_ref.len < 2 || cell_ref.len > 10) {
        return null;
    }
    let i: usize = 0;
    let col1: u32 = 0;
    let letter_count: usize = 0;

    while (i < cell_ref.len && cell_ref[i] >= 'A' && cell_ref[i] <= 'Z') {
        letter_count += 1;
        if (letter_count > 3) {
            return null;
        }
        col1 = col1 * 26 + ((cell_ref[i] - 'A') as u32) + 1;
        i += 1;
    }
    if (letter_count == 0 || col1 == 0 || col1 > MAX_XLSX_COL + 1) {
        return null;
    }
    if (i >= cell_ref.len || cell_ref[i] < '1' || cell_ref[i] > '9') {
        return null;
    }

    let row1: u32 = 0;
    let digit_count: usize = 0;
    while (i < cell_ref.len) {
        const b = cell_ref[i];
        if (b < '0' || b > '9') {
            return null;
        }
        digit_count += 1;
        if (digit_count > 7) {
            return null;
        }
        row1 = row1 * 10 + ((b - '0') as u32);
        if (row1 > MAX_XLSX_ROW + 1) {
            return null;
        }
        i += 1;
    }
    if (row1 == 0) {
        return null;
    }

    return XlsxCellRef {
        col: col1 - 1,
        row: row1 - 1,
    };
}

/// Formats a 0-based column index (`0..=16383`) into Excel column letters (`"A"`..`"XFD"`).
/// Returns the number of bytes written to `out`, or `null` if `col > MAX_XLSX_COL` or `out` is too small.
pub fn formatColName(col: u32, out: []u8): usize? {
    if (col > MAX_XLSX_COL) {
        return null;
    }
    let tmp: [3]u8 = [0; 3];
    let len: usize = 0;
    let n: u32 = col;
    while (true) {
        const rem: u32 = n % 26;
        tmp[len] = (('A' as u32) + rem) as u8;
        len += 1;
        if (n < 26) {
            break;
        }
        n = (n / 26) - 1;
    }
    if (out.len < len) {
        return null;
    }
    let k: usize = 0;
    while (k < len) {
        out[k] = tmp[len - 1 - k];
        k += 1;
    }
    return len;
}

// ---------------------------------------------------------------------------
// Internal Zero-Alloc XML Scanning & Entity Unescaping Primitives
// ---------------------------------------------------------------------------

fn isXmlWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn isTagNameEnd(b: u8): bool {
    return isXmlWs(b) || b == '/' || b == '>' || b == '=';
}

fn parseU32Strict(s: []const u8): u32? {
    if (s.len == 0 || s.len > 10) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < '0' || b > '9') {
            return null;
        }
        acc = acc * 10 + ((b - '0') as u64);
        if (acc > 4294967295) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

/// Returns the local name after the last `':'` namespace prefix (e.g. `"x:sheet"` -> `"sheet"`).
fn localTagName(name: []const u8): []const u8 {
    let i: usize = name.len;
    while (i > 0) {
        i -= 1;
        if (name[i] == ':') {
            return name[i + 1 ..];
        }
    }
    return name;
}

/// Finds the closing `'>'` of an XML tag starting at `xml[start] == '<'`, respecting quoted attributes
/// using 16/32-byte SIMD hardware vector scanning.
fn findTagClose(xml: []const u8, start: usize): usize? {
    if (start >= xml.len || xml[start] != '<') {
        return null;
    }
    let i = start + 1;
    let quote: u8 = 0;
    while (i < xml.len) {
        if (quote != 0) {
            const rel: usize = Simd.indexOfByte(xml[i..xml.len], quote) ?? return null;
            i += rel + 1;
            quote = 0;
        } else {
            const rel: usize = Simd.indexOfAny4(xml[i..xml.len], '>', '"', '\'', '\'') ?? return null;
            i += rel;
            const b = xml[i];
            if (b == '>') {
                return i;
            }
            quote = b;
            i += 1;
        }
    }
    return null;
}

/// Skips an XML comment `<!-- ... -->` or processing instruction `<? ... ?>` or `<! ... >` starting at `start`.
fn skipSpecialTag(xml: []const u8, start: usize): usize? {
    if (start + 4 <= xml.len && std.mem.eql(u8, xml[start .. start + 4], "<!--")) {
        let i = start + 4;
        while (i + 2 < xml.len) {
            if (xml[i] == '-' && xml[i + 1] == '-' && xml[i + 2] == '>') {
                return i + 3;
            }
            i += 1;
        }
        return null;
    }
    if (start + 2 <= xml.len && xml[start + 1] == '?') {
        let i = start + 2;
        while (i + 1 < xml.len) {
            if (xml[i] == '?' && xml[i + 1] == '>') {
                return i + 2;
            }
            i += 1;
        }
        return null;
    }
    const end = findTagClose(xml, start) ?? return null;
    return end + 1;
}

/// Checks if a tag slice `xml[open_start..tag_close + 1]` is self-closing (`<.../>`).
fn isSelfClosing(xml: []const u8, open_start: usize, tag_close: usize): bool {
    if (tag_close <= open_start + 1) {
        return false;
    }
    let i = tag_close;
    while (i > open_start + 1) {
        i -= 1;
        if (!isXmlWs(xml[i])) {
            return xml[i] == '/';
        }
    }
    return false;
}

/// Extracts an attribute value from the attribute region `attrs` (between tag name and `>` or `/>`).
fn getAttr(attrs: []const u8, target: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < attrs.len) {
        while (i < attrs.len && (isXmlWs(attrs[i]) || attrs[i] == '/')) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const key_start = i;
        while (i < attrs.len && !isXmlWs(attrs[i]) && attrs[i] != '=' && attrs[i] != '/' && attrs[i] != '>') {
            i += 1;
        }
        const key = attrs[key_start..i];
        while (i < attrs.len && isXmlWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || attrs[i] != '=') {
            continue;
        }
        i += 1;
        while (i < attrs.len && isXmlWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const q = attrs[i];
        if (q != '"' && q != '\'') {
            return null;
        }
        i += 1;
        const val_start = i;
        while (i < attrs.len && attrs[i] != q) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const val = attrs[val_start..i];
        i += 1;
        if (std.mem.eql(u8, key, target)) {
            return val;
        }
    }
    return null;
}

/// Finds the matching closing tag `</tag_local>` (respecting nesting of same local tag name).
fn findMatchingClose(xml: []const u8, start_pos: usize, limit: usize, tag_local: []const u8): CloseTagMatch? {
    let i = start_pos;
    let depth: usize = 1;

    while (i < limit) {
        if (xml[i] != '<') {
            const rel: usize = Simd.indexOfByte(xml[i..limit], '<') ?? return null;
            i += rel;
        }
        if (i + 1 >= limit) {
            return null;
        }
        const next_b = xml[i + 1];
        if (next_b == '!' || next_b == '?') {
            const after = skipSpecialTag(xml, i) ?? return null;
            i = after;
            continue;
        }
        const close_gt = findTagClose(xml, i) ?? return null;
        if (close_gt >= limit) {
            return null;
        }
        if (next_b == '/') {
            let nstart = i + 2;
            while (nstart < close_gt && isXmlWs(xml[nstart])) {
                nstart += 1;
            }
            let nend = nstart;
            while (nend < close_gt && !isTagNameEnd(xml[nend])) {
                nend += 1;
            }
            const lname = localTagName(xml[nstart..nend]);
            if (std.mem.eql(u8, lname, tag_local)) {
                depth -= 1;
                if (depth == 0) {
                    return CloseTagMatch {
                        close_start: i,
                        after_close: close_gt + 1,
                    };
                }
            }
            i = close_gt + 1;
        } else {
            let nend = i + 1;
            while (nend < close_gt && !isTagNameEnd(xml[nend])) {
                nend += 1;
            }
            const lname = localTagName(xml[i + 1 .. nend]);
            if (std.mem.eql(u8, lname, tag_local) && !isSelfClosing(xml, i, close_gt)) {
                depth += 1;
            }
            i = close_gt + 1;
        }
    }
    return null;
}

fn emitByte(out: []u8, out_len: *usize, b: u8): bool {
    if (out_len.* >= out.len) {
        return false;
    }
    out[out_len.*] = b;
    out_len.* += 1;
    return true;
}

fn emitUtf8(out: []u8, out_len: *usize, cp: u32): bool {
    if (cp <= 0x7F) {
        return emitByte(out, out_len, cp as u8);
    }
    if (cp <= 0x7FF) {
        if (!emitByte(out, out_len, (0xC0 | (cp >> 6)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    if (cp <= 0xFFFF) {
        if (!emitByte(out, out_len, (0xE0 | (cp >> 12)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 6) & 0x3F)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    if (cp <= 0x10FFFF) {
        if (!emitByte(out, out_len, (0xF0 | (cp >> 18)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 12) & 0x3F)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 6) & 0x3F)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    return false;
}

fn parseHexU32(s: []const u8): u32? {
    if (s.len == 0 || s.len > 6) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        let d: u32 = 0;
        if (b >= '0' && b <= '9') {
            d = (b - '0') as u32;
        } else if (b >= 'a' && b <= 'f') {
            d = (b - 'a' + 10) as u32;
        } else if (b >= 'A' && b <= 'F') {
            d = (b - 'A' + 10) as u32;
        } else {
            return null;
        }
        acc = (acc << 4) | d;
        i += 1;
    }
    return acc;
}

/// Unescapes standard XML entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`, `&#N;`, `&#xH;`)
/// from `src` into `out`, advancing `out_len`. Returns `false` if `out` overflows.
fn unescapeXmlInto(src: []const u8, out: []u8, out_len: *usize): bool {
    let i: usize = 0;
    while (i < src.len) {
        if (src[i] != '&') {
            const rel: usize = Simd.indexOfByte(src[i..src.len], '&') ?? (src.len - i);
            if (out_len.* + rel > out.len) return false;
            @memcpy(out[out_len.*..out_len.* + rel], src[i..i + rel]);
            out_len.* += rel;
            i += rel;
            if (i >= src.len) break;
        }
        if (src[i] == '&') {
            let semi = i + 1;
            const max_semi = if (i + 12 < src.len) i + 12 else src.len;
            while (semi < max_semi && src[semi] != ';') {
                semi += 1;
            }
            if (semi < max_semi && src[semi] == ';') {
                const ent = src[i + 1 .. semi];
                if (std.mem.eql(u8, ent, "amp")) {
                    if (!emitByte(out, out_len, '&')) return false;
                    i = semi + 1;
                    continue;
                } else if (std.mem.eql(u8, ent, "lt")) {
                    if (!emitByte(out, out_len, '<')) return false;
                    i = semi + 1;
                    continue;
                } else if (std.mem.eql(u8, ent, "gt")) {
                    if (!emitByte(out, out_len, '>')) return false;
                    i = semi + 1;
                    continue;
                } else if (std.mem.eql(u8, ent, "quot")) {
                    if (!emitByte(out, out_len, '"')) return false;
                    i = semi + 1;
                    continue;
                } else if (std.mem.eql(u8, ent, "apos")) {
                    if (!emitByte(out, out_len, '\'')) return false;
                    i = semi + 1;
                    continue;
                } else if (ent.len >= 2 && ent[0] == '#') {
                    let cp_opt: u32? = null;
                    if (ent.len >= 3 && (ent[1] == 'x' || ent[1] == 'X')) {
                        cp_opt = parseHexU32(ent[2..]);
                    } else {
                        cp_opt = parseU32Strict(ent[1..]);
                    }
                    if (cp := cp_opt) {
                        if (cp > 0 && cp <= 0x10FFFF) {
                            if (!emitUtf8(out, out_len, cp)) return false;
                            i = semi + 1;
                            continue;
                        }
                    }
                }
            }
        }
        if (!emitByte(out, out_len, src[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 1. xl/workbook.xml Sheet Iterator
// ---------------------------------------------------------------------------

/// Zero-copy iterator over `<sheet name="..." sheetId="..." r:id="..." state="..."/>`
/// entries inside `xl/workbook.xml`.
pub struct XlsxSheetIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    fn init(workbook_xml: []const u8): XlsxSheetIter {
        return XlsxSheetIter {
            src: workbook_xml,
            pos: 0,
            done: workbook_xml.len == 0,
        };
    }

    fn next(*_): XlsxSheetInfo? {
        if (_.done) {
            return null;
        }
        let i = _.pos;
        while (i < _.src.len) {
            if (_.src[i] != '<') {
                i += 1;
                continue;
            }
            if (i + 1 >= _.src.len) {
                _.done = true;
                return null;
            }
            const nb = _.src[i + 1];
            if (nb == '!' || nb == '?') {
                const after_opt = skipSpecialTag(_.src, i);
                if (after_opt == null) {
                    _.done = true;
                    return null;
                }
                i = after_opt.?;
                continue;
            }
            const gt_opt = findTagClose(_.src, i);
            if (gt_opt == null) {
                _.done = true;
                return null;
            }
            const gt = gt_opt.?;
            if (nb == '/') {
                i = gt + 1;
                continue;
            }

            let nend = i + 1;
            while (nend < gt && !isTagNameEnd(_.src[nend])) {
                nend += 1;
            }
            const lname = localTagName(_.src[i + 1 .. nend]);
            if (std.mem.eql(u8, lname, "sheet")) {
                const attrs = _.src[nend..gt];
                const name_opt = getAttr(attrs, "name");
                _.pos = gt + 1;
                if (name := name_opt) {
                    let sheet_id: u32 = 0;
                    if (sid_str := getAttr(attrs, "sheetId")) {
                        if (sid := parseU32Strict(sid_str)) {
                            sheet_id = sid;
                        }
                    }
                    let r_id: []const u8 = "";
                    if (rid := getAttr(attrs, "r:id")) {
                        r_id = rid;
                    } else if (id_fallback := getAttr(attrs, "id")) {
                        r_id = id_fallback;
                    }
                    let is_hidden = false;
                    if (st := getAttr(attrs, "state")) {
                        if (std.mem.eql(u8, st, "hidden") || std.mem.eql(u8, st, "veryHidden")) {
                            is_hidden = true;
                        }
                    }
                    return XlsxSheetInfo {
                        name: name,
                        sheet_id: sheet_id,
                        r_id: r_id,
                        is_hidden: is_hidden,
                    };
                }
                i = gt + 1;
                continue;
            }
            i = gt + 1;
        }
        _.done = true;
        return null;
    }
}

// ---------------------------------------------------------------------------
// 2. xl/sharedStrings.xml Iterator & Rich-Text Extractor
// ---------------------------------------------------------------------------

/// Extracts and unescapes plain text from a single `<si>...</si>` XML fragment
/// into `out`. Supports both plain `<t>...</t>` and rich-text `<r><t>...</t></r>`,
/// skipping phonetic guide `<rPh>...</rPh>` blocks.
/// Returns total bytes written, or `null` if `out` buffer is too small.
pub fn extractSharedString(si_xml: []const u8, out: []u8): usize? {
    let out_len: usize = 0;
    let i: usize = 0;

    while (i < si_xml.len) {
        if (si_xml[i] != '<') {
            i += 1;
            continue;
        }
        if (i + 1 >= si_xml.len) {
            break;
        }
        const nb = si_xml[i + 1];
        if (nb == '!' || nb == '?') {
            const after = skipSpecialTag(si_xml, i) ?? break;
            i = after;
            continue;
        }
        const gt = findTagClose(si_xml, i) ?? break;
        if (nb == '/') {
            i = gt + 1;
            continue;
        }

        let nend = i + 1;
        while (nend < gt && !isTagNameEnd(si_xml[nend])) {
            nend += 1;
        }
        const lname = localTagName(si_xml[i + 1 .. nend]);

        // Skip phonetic runs <rPh>...</rPh>
        if (std.mem.eql(u8, lname, "rPh")) {
            if (isSelfClosing(si_xml, i, gt)) {
                i = gt + 1;
            } else if (m := findMatchingClose(si_xml, gt + 1, si_xml.len, "rPh")) {
                i = m.after_close;
            } else {
                break;
            }
            continue;
        }

        if (std.mem.eql(u8, lname, "t")) {
            if (isSelfClosing(si_xml, i, gt)) {
                i = gt + 1;
                continue;
            }
            const m = findMatchingClose(si_xml, gt + 1, si_xml.len, "t") ?? break;
            const raw_t = si_xml[gt + 1 .. m.close_start];
            if (!unescapeXmlInto(raw_t, out, *out_len)) {
                return null;
            }
            i = m.after_close;
            continue;
        }

        i = gt + 1;
    }

    return out_len;
}

/// Zero-copy iterator over `<si>...</si>` items in `xl/sharedStrings.xml`.
/// Each call to `next()` yields the inner XML slice of `<si>`, which can be passed
/// directly to `extractSharedString(si_xml, out)`.
pub struct XlsxSharedStringIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    fn init(sst_xml: []const u8): XlsxSharedStringIter {
        return XlsxSharedStringIter {
            src: sst_xml,
            pos: 0,
            done: sst_xml.len == 0,
        };
    }

    fn next(*_): ([]const u8)? {
        if (_.done) {
            return null;
        }
        let i = _.pos;
        while (i < _.src.len) {
            if (_.src[i] != '<') {
                i += 1;
                continue;
            }
            if (i + 1 >= _.src.len) {
                _.done = true;
                return null;
            }
            const nb = _.src[i + 1];
            if (nb == '!' || nb == '?') {
                const after_opt = skipSpecialTag(_.src, i);
                if (after_opt == null) {
                    _.done = true;
                    return null;
                }
                i = after_opt.?;
                continue;
            }
            const gt_opt = findTagClose(_.src, i);
            if (gt_opt == null) {
                _.done = true;
                return null;
            }
            const gt = gt_opt.?;
            if (nb == '/') {
                i = gt + 1;
                continue;
            }
            let nend = i + 1;
            while (nend < gt && !isTagNameEnd(_.src[nend])) {
                nend += 1;
            }
            const lname = localTagName(_.src[i + 1 .. nend]);
            if (std.mem.eql(u8, lname, "si")) {
                if (isSelfClosing(_.src, i, gt)) {
                    _.pos = gt + 1;
                    return "";
                }
                const m_opt = findMatchingClose(_.src, gt + 1, _.src.len, "si");
                if (m_opt == null) {
                    _.done = true;
                    return null;
                }
                const m = m_opt.?;
                _.pos = m.after_close;
                return _.src[gt + 1 .. m.close_start];
            }
            i = gt + 1;
        }
        _.done = true;
        return null;
    }
}

// ---------------------------------------------------------------------------
// 3. xl/worksheets/sheet*.xml Streaming Cell Iterator
// ---------------------------------------------------------------------------

fn parseCellTypeAttr(t_val: []const u8): XlsxCellType {
    if (t_val.len == 0 || std.mem.eql(u8, t_val, "n")) {
        return XlsxCellType.number;
    }
    if (std.mem.eql(u8, t_val, "s")) {
        return XlsxCellType.shared_string;
    }
    if (std.mem.eql(u8, t_val, "inlineStr")) {
        return XlsxCellType.inline_string;
    }
    if (std.mem.eql(u8, t_val, "str")) {
        return XlsxCellType.formula_str;
    }
    if (std.mem.eql(u8, t_val, "b")) {
        return XlsxCellType.bool_val;
    }
    if (std.mem.eql(u8, t_val, "e")) {
        return XlsxCellType.error_val;
    }
    return XlsxCellType.unknown;
}

/// Extracts the inner text of the first `<t>...</t>` inside an `<is>...</is>` inline string element.
fn extractFirstTText(is_body: []const u8): []const u8 {
    let i: usize = 0;
    while (i < is_body.len) {
        if (is_body[i] != '<') {
            i += 1;
            continue;
        }
        if (i + 1 >= is_body.len) {
            break;
        }
        const nb = is_body[i + 1];
        if (nb == '!' || nb == '?') {
            const after = skipSpecialTag(is_body, i) ?? break;
            i = after;
            continue;
        }
        const gt = findTagClose(is_body, i) ?? break;
        if (nb == '/') {
            i = gt + 1;
            continue;
        }
        let nend = i + 1;
        while (nend < gt && !isTagNameEnd(is_body[nend])) {
            nend += 1;
        }
        const lname = localTagName(is_body[i + 1 .. nend]);
        if (std.mem.eql(u8, lname, "t")) {
            if (isSelfClosing(is_body, i, gt)) {
                return "";
            }
            if (m := findMatchingClose(is_body, gt + 1, is_body.len, "t")) {
                return is_body[gt + 1 .. m.close_start];
            }
            return "";
        }
        i = gt + 1;
    }
    return "";
}

/// Zero-allocation streaming iterator over all `<c ...>` cells inside `<sheetData>...</sheetData>`
/// of an `xl/worksheets/sheet*.xml` document.
pub struct XlsxCellIter {
    src: []const u8 = "",
    pos: usize = 0,
    end: usize = 0,
    done: bool = true,

    fn init(sheet_xml: []const u8): XlsxCellIter {
        let i: usize = 0;
        while (i < sheet_xml.len) {
            if (sheet_xml[i] != '<') {
                i += 1;
                continue;
            }
            if (i + 1 >= sheet_xml.len) {
                break;
            }
            const nb = sheet_xml[i + 1];
            if (nb == '!' || nb == '?') {
                const after = skipSpecialTag(sheet_xml, i) ?? break;
                i = after;
                continue;
            }
            const gt = findTagClose(sheet_xml, i) ?? break;
            if (nb == '/') {
                i = gt + 1;
                continue;
            }
            let nend = i + 1;
            while (nend < gt && !isTagNameEnd(sheet_xml[nend])) {
                nend += 1;
            }
            const lname = localTagName(sheet_xml[i + 1 .. nend]);
            if (std.mem.eql(u8, lname, "sheetData")) {
                if (isSelfClosing(sheet_xml, i, gt)) {
                    return XlsxCellIter { src: sheet_xml, pos: gt + 1, end: gt + 1, done: true };
                }
                if (m := findMatchingClose(sheet_xml, gt + 1, sheet_xml.len, "sheetData")) {
                    return XlsxCellIter {
                        src: sheet_xml,
                        pos: gt + 1,
                        end: m.close_start,
                        done: false,
                    };
                }
                break;
            }
            i = gt + 1;
        }
        return XlsxCellIter { src: sheet_xml, pos: sheet_xml.len, end: sheet_xml.len, done: true };
    }

    fn next(*_): XlsxCell? {
        if (_.done) {
            return null;
        }
        let i = _.pos;
        while (i < _.end) {
            if (_.src[i] != '<') {
                i += 1;
                continue;
            }
            if (i + 1 >= _.end) {
                _.done = true;
                return null;
            }
            const nb = _.src[i + 1];
            if (nb == '!' || nb == '?') {
                const after_opt = skipSpecialTag(_.src, i);
                if (after_opt == null || after_opt.? > _.end) {
                    _.done = true;
                    return null;
                }
                i = after_opt.?;
                continue;
            }
            const gt_opt = findTagClose(_.src, i);
            if (gt_opt == null || gt_opt.? >= _.end) {
                _.done = true;
                return null;
            }
            const gt = gt_opt.?;
            if (nb == '/') {
                i = gt + 1;
                continue;
            }

            let nend = i + 1;
            while (nend < gt && !isTagNameEnd(_.src[nend])) {
                nend += 1;
            }
            const lname = localTagName(_.src[i + 1 .. nend]);
            if (!std.mem.eql(u8, lname, "c")) {
                i = gt + 1;
                continue;
            }

            const attrs = _.src[nend..gt];
            const self_closed = isSelfClosing(_.src, i, gt);

            let body: []const u8 = "";
            if (self_closed) {
                _.pos = gt + 1;
            } else {
                const close_opt = findMatchingClose(_.src, gt + 1, _.end, "c");
                if (close_opt == null) {
                    _.done = true;
                    return null;
                }
                const close_m = close_opt.?;
                body = _.src[gt + 1 .. close_m.close_start];
                _.pos = close_m.after_close;
            }

            // Parse required cell reference `r="A1"`
            const ref_str_opt = getAttr(attrs, "r");
            if (ref_str_opt == null) {
                i = _.pos;
                continue;
            }
            const cell_ref_opt = parseCellRef(ref_str_opt.?);
            if (cell_ref_opt == null) {
                i = _.pos;
                continue;
            }

            let cell = XlsxCell {
                ref: cell_ref_opt.?,
                cell_type: XlsxCellType.number,
                style_idx: 0,
                raw_value: "",
                formula: "",
            };

            if (t_val := getAttr(attrs, "t")) {
                cell.cell_type = parseCellTypeAttr(t_val);
            }
            if (s_val := getAttr(attrs, "s")) {
                if (s_idx := parseU32Strict(s_val)) {
                    cell.style_idx = s_idx;
                }
            }

            // Scan child tags inside <c>...</c>: <f>, <v>, <is>
            let bi: usize = 0;
            while (bi < body.len) {
                if (body[bi] != '<') {
                    bi += 1;
                    continue;
                }
                if (bi + 1 >= body.len) {
                    break;
                }
                const bnb = body[bi + 1];
                if (bnb == '!' || bnb == '?') {
                    const bafter = skipSpecialTag(body, bi) ?? break;
                    bi = bafter;
                    continue;
                }
                const bgt = findTagClose(body, bi) ?? break;
                if (bnb == '/') {
                    bi = bgt + 1;
                    continue;
                }
                let bnend = bi + 1;
                while (bnend < bgt && !isTagNameEnd(body[bnend])) {
                    bnend += 1;
                }
                const blname = localTagName(body[bi + 1 .. bnend]);

                if (std.mem.eql(u8, blname, "f")) {
                    if (isSelfClosing(body, bi, bgt)) {
                        bi = bgt + 1;
                    } else if (fm := findMatchingClose(body, bgt + 1, body.len, "f")) {
                        cell.formula = body[bgt + 1 .. fm.close_start];
                        bi = fm.after_close;
                    } else {
                        break;
                    }
                    continue;
                }
                if (std.mem.eql(u8, blname, "v")) {
                    if (isSelfClosing(body, bi, bgt)) {
                        bi = bgt + 1;
                    } else if (vm := findMatchingClose(body, bgt + 1, body.len, "v")) {
                        cell.raw_value = body[bgt + 1 .. vm.close_start];
                        bi = vm.after_close;
                    } else {
                        break;
                    }
                    continue;
                }
                if (std.mem.eql(u8, blname, "is")) {
                    if (isSelfClosing(body, bi, bgt)) {
                        bi = bgt + 1;
                    } else if (im := findMatchingClose(body, bgt + 1, body.len, "is")) {
                        const is_inner = body[bgt + 1 .. im.close_start];
                        cell.raw_value = extractFirstTText(is_inner);
                        bi = im.after_close;
                    } else {
                        break;
                    }
                    continue;
                }

                bi = bgt + 1;
            }

            return cell;
        }
        _.done = true;
        return null;
    }
}


// ============================================================================
// 5. Production Extension: Error Set, Range Codec & Excel 1900/1904 Date Serials
// ============================================================================

/// Error set returned by extended XLSX coordinate, date, XML builder/parser, and formula operations.
pub err XlsxError {
    BufferTooSmall,
    InvalidCellRef,
    InvalidRange,
    InvalidDate,
    InvalidXml,
    SlotOverflow,
    FormulaSyntax,
    DivisionByZero,
    InvalidReference,
    CycleDetected,
    InvalidArgument,
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize ! XlsxError {
    if (pos + src.len > out.len) return XlsxError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos + i] = src[i];
    }
    return pos + src.len;
}

fn appendU32Ascii(out: []u8, pos: usize, v: u32): usize ! XlsxError {
    let digits: [10]u8 = [0; 10];
    let n: usize = 0;
    let x: u32 = v;
    if (x == 0) {
        digits[0] = '0';
        n = 1;
    } else {
        while (x > 0) {
            digits[n] = ('0' as u8) + ((x % 10) as u8);
            x /= 10;
            n += 1;
        }
    }
    if (pos + n > out.len) return XlsxError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos + i] = digits[n - 1 - i];
    }
    return pos + n;
}

fn appendXmlEscaped(out: []u8, pos: usize, text: []const u8): usize ! XlsxError {
    let cur: usize = pos;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const b = text[i];
        if (b == '&') {
            cur = appendBytes(out, cur, "&amp;")?;
        } else if (b == '<') {
            cur = appendBytes(out, cur, "&lt;")?;
        } else if (b == '>') {
            cur = appendBytes(out, cur, "&gt;")?;
        } else if (b == '"') {
            cur = appendBytes(out, cur, "&quot;")?;
        } else if (b == '\'') {
            cur = appendBytes(out, cur, "&apos;")?;
        } else {
            if (cur >= out.len) return XlsxError.BufferTooSmall;
            out[cur] = b;
            cur += 1;
        }
    }
    return cur;
}

fn appendF64Ascii(out: []u8, pos: usize, v: f64): usize ! XlsxError {
    let cur: usize = pos;
    let val: f64 = v;
    if (val < 0.0) {
        cur = appendBytes(out, cur, "-")?;
        val = -val;
    }
    const int_part: u64 = val as u64;
    const frac_scaled: u64 = (((val - (int_part as f64)) * 1000000.0) + 0.5) as u64;
    let ip: u64 = int_part;
    let fp: u64 = frac_scaled;
    if (fp >= 1000000) {
        ip += 1;
        fp -= 1000000;
    }
    // Emit integer part
    let idig: [20]u8 = [0; 20];
    let in_len: usize = 0;
    if (ip == 0) {
        idig[0] = '0';
        in_len = 1;
    } else {
        let tmp: u64 = ip;
        while (tmp > 0) {
            idig[in_len] = ('0' as u8) + ((tmp % 10) as u8);
            tmp /= 10;
            in_len += 1;
        }
    }
    if (cur + in_len > out.len) return XlsxError.BufferTooSmall;
    let k: usize = 0;
    while (k < in_len) : (k += 1) {
        out[cur + k] = idig[in_len - 1 - k];
    }
    cur += in_len;

    if (fp > 0) {
        cur = appendBytes(out, cur, ".")?;
        let fdig: [6]u8 = ['0'; 6];
        let tmp: u64 = fp;
        let fi: usize = 6;
        while (fi > 0) {
            fi -= 1;
            fdig[fi] = ('0' as u8) + ((tmp % 10) as u8);
            tmp /= 10;
        }
        let fend: usize = 6;
        while (fend > 1 && fdig[fend - 1] == '0') : (fend -= 1) {}
        cur = appendBytes(out, cur, fdig[0..fend])?;
    }
    return cur;
}

/// Encodes a 0-based `XlsxCellRef` into an A1-style string (`"A1"`..`"XFD1048576"`).
pub fn encodeCellRef(coord: XlsxCellRef, out: []u8): usize ! XlsxError {
    if (coord.col > MAX_XLSX_COL || coord.row > MAX_XLSX_ROW) return XlsxError.InvalidCellRef;
    const col_len = formatColName(coord.col, out) orelse return XlsxError.BufferTooSmall;
    return appendU32Ascii(out, col_len, coord.row + 1);
}

/// Decodes an A1 cell reference supporting optional `$` absolute markers (`$A$1`, `$B2`, `C$3`) and case insensitivity.
pub fn decodeCellRef(raw: []const u8): XlsxCellRef ! XlsxError {
    if (raw.len < 2 || raw.len > 12) return XlsxError.InvalidCellRef;
    let cleaned: [12]u8 = [0; 12];
    let clen: usize = 0;
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        let b = raw[i];
        if (b == '$') continue;
        if (b >= 'a' && b <= 'z') b -= 32;
        if (clen >= 12) return XlsxError.InvalidCellRef;
        cleaned[clen] = b;
        clen += 1;
    }
    return parseCellRef(cleaned[0..clen]) orelse return XlsxError.InvalidCellRef;
}

/// Normalized rectangular Excel range (`start.col <= end.col && start.row <= end.row`).
pub struct XlsxRangeRef {
    start: XlsxCellRef = XlsxCellRef{},
    end: XlsxCellRef = XlsxCellRef{},

    fn contains(coord: XlsxCellRef): bool {
        return coord.col >= start.col && coord.col <= end.col && coord.row >= start.row && coord.row <= end.row;
    }

    fn cellCount(): u64 {
        const w: u64 = ((end.col - start.col) as u64) + 1;
        const h: u64 = ((end.row - start.row) as u64) + 1;
        return w * h;
    }
}

/// Parses an Excel range string such as `"B2:F100"` or `"$A$1:$C$5"`.
pub fn parseCellRange(raw: []const u8): XlsxRangeRef ! XlsxError {
    let colon: usize = raw.len;
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        if (raw[i] == ':') {
            colon = i;
            break;
        }
    }
    if (colon == 0 || colon + 1 >= raw.len) return XlsxError.InvalidRange;
    const a = decodeCellRef(raw[0..colon])?;
    const b = decodeCellRef(raw[(colon + 1)..raw.len])?;
    return XlsxRangeRef{
        start: XlsxCellRef{
            col: if (a.col < b.col) a.col else b.col,
            row: if (a.row < b.row) a.row else b.row,
        },
        end: XlsxCellRef{
            col: if (a.col > b.col) a.col else b.col,
            row: if (a.row > b.row) a.row else b.row,
        },
    };
}

/// Calendar date + time-of-day decoded from or encoded into an Excel date serial number.
pub struct XlsxDateTime {
    year: i32 = 1900;
    month: u8 = 1;
    day: u8 = 1;
    hour: u8 = 0;
    minute: u8 = 0;
    second: u8 = 0;
}

// Howard Hinnant's O(1) Gregorian civil-calendar conversions (days relative to 1970-01-01).
fn civilToDays(y_in: i32, m_in: u8, d_in: u8): i64 {
    const y: i64 = (y_in as i64) - (if (m_in <= 2) @as(i64, 1) else @as(i64, 0));
    const era: i64 = @divFloor(y, 400);
    const yoe: i64 = y - era * 400;
    const m_adj: i64 = (m_in as i64) + (if (m_in > 2) @as(i64, -3) else @as(i64, 9));
    const doy: i64 = @divTrunc(153 * m_adj + 2, 5) + (d_in as i64) - 1;
    const doe: i64 = yoe * 365 + @divTrunc(yoe, 4) - @divTrunc(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

fn daysToCivil(z_in: i64): XlsxDateTime {
    const z: i64 = z_in + 719468;
    const era: i64 = @divFloor(z, 146097);
    const doe: i64 = z - era * 146097;
    const yoe: i64 = @divTrunc(doe - @divTrunc(doe, 1460) + @divTrunc(doe, 36524) - @divTrunc(doe, 146096), 365);
    const y0: i64 = yoe + era * 400;
    const doy: i64 = doe - (365 * yoe + @divTrunc(yoe, 4) - @divTrunc(yoe, 100));
    const mp: i64 = @divTrunc(5 * doy + 2, 153);
    const d: i64 = doy - @divTrunc(153 * mp + 2, 5) + 1;
    const m: i64 = mp + (if (mp < 10) @as(i64, 3) else @as(i64, -9));
    const y: i64 = y0 + (if (m <= 2) @as(i64, 1) else @as(i64, 0));
    return XlsxDateTime{
        year: y as i32,
        month: m as u8,
        day: d as u8,
    };
}

/// Converts an Excel date serial number to calendar date & time-of-day.
/// Faithfully handles the 1900 leap-year bug (`serial 60 == 1900-02-29`) and the 1904 Mac system.
pub fn excelSerialToYmd(serial: f64, use_1904: bool): XlsxDateTime ! XlsxError {
    if (serial < 0.0 || serial > 2958466.0) return XlsxError.InvalidDate;
    let day_int: i64 = serial as i64;
    const frac: f64 = serial - (day_int as f64);
    let secs: i64 = (frac * 86400.0 + 0.5) as i64;
    if (secs >= 86400) {
        secs -= 86400;
        day_int += 1;
    }

    let dt = XlsxDateTime{};
    if (!use_1904) {
        if (day_int == 60) {
            dt = XlsxDateTime{ year: 1900, month: 2, day: 29 };
        } else {
            // 1899-12-31 is civilToDays(1899, 12, 31) = -25568
            const adj: i64 = if (day_int > 60) (day_int - 1) else day_int;
            dt = daysToCivil(-25568 + adj);
        }
    } else {
        // 1904-01-01 is civilToDays(1904, 1, 1) = -24107
        dt = daysToCivil(-24107 + day_int);
    }

    dt.hour = (@divTrunc(secs, 3600)) as u8;
    dt.minute = (@divTrunc(@mod(secs, 3600), 60)) as u8;
    dt.second = (@mod(secs, 60)) as u8;
    return dt;
}

/// Converts a calendar date & time-of-day to an Excel date serial number (1900 or 1904 system).
pub fn ymdToExcelSerial(dt: XlsxDateTime, use_1904: bool): f64 ! XlsxError {
    if (dt.month < 1 || dt.month > 12 || dt.day < 1 || dt.day > 31) return XlsxError.InvalidDate;
    if (dt.hour > 23 || dt.minute > 59 || dt.second > 59) return XlsxError.InvalidDate;
    const time_frac: f64 = ((dt.hour as f64) * 3600.0 + (dt.minute as f64) * 60.0 + (dt.second as f64)) / 86400.0;

    if (!use_1904) {
        if (dt.year == 1900 && dt.month == 2 && dt.day == 29) {
            return 60.0 + time_frac;
        }
        const days: i64 = civilToDays(dt.year, dt.month, dt.day);
        let serial_days: i64 = days + 25568; // 1900-01-01 -> 1
        if (serial_days >= 60) {
            serial_days += 1; // account for Excel's non-existent 1900-02-29
        }
        if (serial_days < 0) return XlsxError.InvalidDate;
        return (serial_days as f64) + time_frac;
    }

    const days: i64 = civilToDays(dt.year, dt.month, dt.day);
    const serial_days: i64 = days + 24107; // 1904-01-01 -> 0
    if (serial_days < 0) return XlsxError.InvalidDate;
    return (serial_days as f64) + time_frac;
}

// ============================================================================
// 6. Zero-Heap OOXML Parsers & Builders (SharedStrings, Workbook, Styles, Sheet)
// ============================================================================

/// Parses all `<si>` entries from `sharedStrings.xml` (rich-text `<r><t>` + XML entity unescaping)
/// into caller-provided `text_slab` and `out_strings`, returning total string count.
pub fn parseSharedStringsXml(
    xml: []const u8,
    out_strings: [][]const u8,
    text_slab: []u8,
): usize ! XlsxError {
    let it = XlsxSharedStringIter.init(xml);
    let count: usize = 0;
    let slab_pos: usize = 0;
    while (true) {
        if (count >= out_strings.len) {
            // Check if another string exists
            let probe: [1]u8 = [0];
            if (it.next(probe[0..]) != null) return XlsxError.SlotOverflow;
            break;
        }
        const opt = it.next(text_slab[slab_pos..text_slab.len]);
        if (s := opt) {
            out_strings[count] = s;
            slab_pos += s.len;
            count += 1;
        } else {
            break;
        }
    }
    return count;
}

/// Parses all `<sheet>` entries from `workbook.xml` into caller-provided `out_sheets`.
pub fn parseWorkbookSheetsXml(xml: []const u8, out_sheets: []XlsxSheetInfo): usize ! XlsxError {
    let it = XlsxSheetIter.init(xml);
    let count: usize = 0;
    while (sh := it.next()) {
        if (count >= out_sheets.len) return XlsxError.SlotOverflow;
        out_sheets[count] = sh;
        count += 1;
    }
    return count;
}

/// Semantic category of an Excel `numFmtId` / custom format code.
pub enum XlsxNumFmtKind {
    general,
    integer,
    decimal,
    currency,
    percent,
    date,
    time,
    datetime,
    scientific,
    text,
    custom,
}

/// Parsed `<cellXfs><xf>` style record linked to its `numFmtId` and semantic category.
pub struct XlsxStyleInfo {
    xf_index: u32 = 0;
    num_fmt_id: u32 = 0;
    kind: XlsxNumFmtKind = XlsxNumFmtKind.general;
    format_code: []const u8 = "";
}

fn containsChar(s: []const u8, ch: u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return true;
    }
    return false;
}

/// Classifies an OOXML `numFmtId` and optional custom `format_code` into `XlsxNumFmtKind`.
pub fn classifyExcelNumFmt(num_fmt_id: u32, format_code: []const u8): XlsxNumFmtKind {
    if (num_fmt_id == 0) return XlsxNumFmtKind.general;
    if (num_fmt_id == 1 || num_fmt_id == 3) return XlsxNumFmtKind.integer;
    if (num_fmt_id == 2 || num_fmt_id == 4) return XlsxNumFmtKind.decimal;
    if (num_fmt_id >= 5 && num_fmt_id <= 8) return XlsxNumFmtKind.currency;
    if (num_fmt_id == 9 || num_fmt_id == 10) return XlsxNumFmtKind.percent;
    if (num_fmt_id == 11) return XlsxNumFmtKind.scientific;
    if (num_fmt_id >= 14 && num_fmt_id <= 17) return XlsxNumFmtKind.date;
    if (num_fmt_id >= 18 && num_fmt_id <= 21) return XlsxNumFmtKind.time;
    if (num_fmt_id == 22) return XlsxNumFmtKind.datetime;
    if (num_fmt_id == 49) return XlsxNumFmtKind.text;

    if (format_code.len > 0) {
        const has_date: bool = containsChar(format_code, 'y') || containsChar(format_code, 'Y')
            || containsChar(format_code, 'd') || containsChar(format_code, 'D');
        const has_time: bool = containsChar(format_code, 'h') || containsChar(format_code, 'H')
            || containsChar(format_code, 's') || containsChar(format_code, 'S');
        if (has_date && has_time) return XlsxNumFmtKind.datetime;
        if (has_date) return XlsxNumFmtKind.date;
        if (has_time) return XlsxNumFmtKind.time;
        if (containsChar(format_code, '%')) return XlsxNumFmtKind.percent;
        if (containsChar(format_code, '$')) return XlsxNumFmtKind.currency;
    }
    return XlsxNumFmtKind.custom;
}

struct CustomNumFmt {
    id: u32 = 0;
    code: []const u8 = "";
}

/// Parses `xl/styles.xml` `<numFmts>` and `<cellXfs>` entries into caller-provided `out_styles`.
pub fn parseStylesNumFmtsXml(styles_xml: []const u8, out_styles: []XlsxStyleInfo): usize ! XlsxError {
    let custom_fmts: [64]CustomNumFmt = [CustomNumFmt{}; 64];
    let custom_cnt: usize = 0;

    // Pass 1: collect custom <numFmt numFmtId="..." formatCode="..."/>
    let i: usize = 0;
    while (i < styles_xml.len) {
        if (styles_xml[i] != '<') { i += 1; continue; }
        if (i + 1 >= styles_xml.len) break;
        const nb = styles_xml[i + 1];
        if (nb == '!' || nb == '?') {
            if (after := skipSpecialTag(styles_xml, i)) {
                i = after;
                continue;
            }
            break;
        }
        const gt: usize = findTagClose(styles_xml, i) ?? break;
        if (nb != '/') {
            let nend = i + 1;
            while (nend < gt && !isTagNameEnd(styles_xml[nend])) : (nend += 1) {}
            const lname = localTagName(styles_xml[(i + 1)..nend]);
            if (std.mem.eql(u8, lname, "numFmt") && custom_cnt < 64) {
                const attrs = styles_xml[nend..gt];
                if (id_s := getAttr(attrs, "numFmtId")) {
                    if (id_u := parseU32Strict(id_s)) {
                        const code_s = getAttr(attrs, "formatCode") ?? "";
                        custom_fmts[custom_cnt] = CustomNumFmt{ id: id_u, code: code_s };
                        custom_cnt += 1;
                    }
                }
            }
        }
        i = gt + 1;
    }

    // Pass 2: locate <cellXfs> and collect <xf numFmtId="..."/>
    let count: usize = 0;
    let in_cell_xfs: bool = false;
    let j: usize = 0;
    while (j < styles_xml.len) {
        if (styles_xml[j] != '<') { j += 1; continue; }
        if (j + 1 >= styles_xml.len) break;
        const nb = styles_xml[j + 1];
        if (nb == '!' || nb == '?') {
            if (after := skipSpecialTag(styles_xml, j)) {
                j = after;
                continue;
            }
            break;
        }
        const gt: usize = findTagClose(styles_xml, j) ?? break;
        if (nb == '/') {
            let nstart = j + 2;
            while (nstart < gt && isXmlWs(styles_xml[nstart])) : (nstart += 1) {}
            let nend = nstart;
            while (nend < gt && !isTagNameEnd(styles_xml[nend])) : (nend += 1) {}
            if (std.mem.eql(u8, localTagName(styles_xml[nstart..nend]), "cellXfs")) {
                in_cell_xfs = false;
            }
        } else {
            let nend = j + 1;
            while (nend < gt && !isTagNameEnd(styles_xml[nend])) : (nend += 1) {}
            const lname = localTagName(styles_xml[(j + 1)..nend]);
            if (std.mem.eql(u8, lname, "cellXfs")) {
                in_cell_xfs = !isSelfClosing(styles_xml, j, gt);
            } else if (in_cell_xfs && std.mem.eql(u8, lname, "xf")) {
                if (count >= out_styles.len) return XlsxError.SlotOverflow;
                const attrs = styles_xml[nend..gt];
                let nfid: u32 = 0;
                if (ns := getAttr(attrs, "numFmtId")) {
                    nfid = parseU32Strict(ns) ?? 0;
                }
                let fcode: []const u8 = "";
                let ci: usize = 0;
                while (ci < custom_cnt) : (ci += 1) {
                    if (custom_fmts[ci].id == nfid) {
                        fcode = custom_fmts[ci].code;
                        break;
                    }
                }
                out_styles[count] = XlsxStyleInfo{
                    xf_index: count as u32,
                    num_fmt_id: nfid,
                    kind: classifyExcelNumFmt(nfid, fcode),
                    format_code: fcode,
                };
                count += 1;
            }
        }
        j = gt + 1;
    }
    return count;
}

/// High-level resolved worksheet cell value (shared strings already resolved, formula & style preserved).
pub struct XlsxCellValue {
    ref: XlsxCellRef = XlsxCellRef{},
    cell_type: XlsxCellType = XlsxCellType.number,
    style_idx: u32 = 0,
    num_val: f64 = 0.0,
    has_num: bool = false,
    bool_val: bool = false,
    text: []const u8 = "",
    formula: []const u8 = "",
}

/// Parses all cells from a worksheet XML into caller-provided `out_cells`, resolving `shared_strings`.
pub fn parseWorksheetCellsXml(
    sheet_xml: []const u8,
    shared_strings: []const []const u8,
    out_cells: []XlsxCellValue,
): usize ! XlsxError {
    let it = XlsxCellIter.init(sheet_xml);
    let count: usize = 0;
    while (c := it.next()) {
        if (count >= out_cells.len) return XlsxError.SlotOverflow;
        let cv = XlsxCellValue{
            ref: c.ref,
            cell_type: c.cell_type,
            style_idx: c.style_idx,
            formula: c.formula,
        };
        if (c.cell_type == XlsxCellType.shared_string) {
            if (idx := c.asSharedStringIdx()) {
                if ((idx as usize) < shared_strings.len) {
                    cv.text = shared_strings[idx as usize];
                }
            }
        } else if (c.cell_type == XlsxCellType.bool_val) {
            cv.bool_val = c.asBool() ?? false;
        } else if (c.cell_type == XlsxCellType.number || c.cell_type == XlsxCellType.unknown) {
            if (fv := c.asF64()) {
                cv.num_val = fv;
                cv.has_num = true;
            }
        } else {
            cv.text = c.raw_value;
        }
        out_cells[count] = cv;
        count += 1;
    }
    return count;
}

/// Builds a deduplicated OOXML `xl/sharedStrings.xml`, populating `out_unique` and per-input `out_indices`.
pub fn buildSharedStringsXml(
    raw_strings: []const []const u8,
    out_unique: [][]const u8,
    out_indices: []u32,
    out_xml: []u8,
): usize ! XlsxError {
    if (out_indices.len < raw_strings.len) return XlsxError.SlotOverflow;
    let unique_cnt: usize = 0;
    let i: usize = 0;
    while (i < raw_strings.len) : (i += 1) {
        const s = raw_strings[i];
        let found: usize = unique_cnt;
        let u: usize = 0;
        while (u < unique_cnt) : (u += 1) {
            if (std.mem.eql(u8, out_unique[u], s)) {
                found = u;
                break;
            }
        }
        if (found == unique_cnt) {
            if (unique_cnt >= out_unique.len) return XlsxError.SlotOverflow;
            out_unique[unique_cnt] = s;
            unique_cnt += 1;
        }
        out_indices[i] = found as u32;
    }

    let pos: usize = 0;
    pos = appendBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendBytes(out_xml, pos, "<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" count=\"")?;
    pos = appendU32Ascii(out_xml, pos, raw_strings.len as u32)?;
    pos = appendBytes(out_xml, pos, "\" uniqueCount=\"")?;
    pos = appendU32Ascii(out_xml, pos, unique_cnt as u32)?;
    pos = appendBytes(out_xml, pos, "\">")?;

    let u: usize = 0;
    while (u < unique_cnt) : (u += 1) {
        pos = appendBytes(out_xml, pos, "<si><t>")?;
        pos = appendXmlEscaped(out_xml, pos, out_unique[u])?;
        pos = appendBytes(out_xml, pos, "</t></si>")?;
    }
    pos = appendBytes(out_xml, pos, "</sst>")?;
    return pos;
}

/// Builds a multi-sheet OOXML `xl/workbook.xml` from `sheet_names`.
pub fn buildWorkbookXml(sheet_names: []const []const u8, out_xml: []u8): usize ! XlsxError {
    let pos: usize = 0;
    pos = appendBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendBytes(out_xml, pos, "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheets>")?;
    let i: usize = 0;
    while (i < sheet_names.len) : (i += 1) {
        pos = appendBytes(out_xml, pos, "<sheet name=\"")?;
        pos = appendXmlEscaped(out_xml, pos, sheet_names[i])?;
        pos = appendBytes(out_xml, pos, "\" sheetId=\"")?;
        pos = appendU32Ascii(out_xml, pos, (i + 1) as u32)?;
        pos = appendBytes(out_xml, pos, "\" r:id=\"rId")?;
        pos = appendU32Ascii(out_xml, pos, (i + 1) as u32)?;
        pos = appendBytes(out_xml, pos, "\"/>")?;
    }
    pos = appendBytes(out_xml, pos, "</sheets></workbook>")?;
    return pos;
}

/// Builds an OOXML `xl/worksheets/sheetN.xml` from `cells` (sorted by row) and optional `shared_strings`.
pub fn buildWorksheetXml(
    cells: []const XlsxCellValue,
    shared_strings: []const []const u8,
    out_xml: []u8,
): usize ! XlsxError {
    let pos: usize = 0;
    pos = appendBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendBytes(out_xml, pos, "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetData>")?;

    let row_open: bool = false;
    let cur_row: u32 = 0;
    let ref_buf: [12]u8 = [0; 12];

    let i: usize = 0;
    while (i < cells.len) : (i += 1) {
        const c = cells[i];
        if (!row_open || c.ref.row != cur_row) {
            if (row_open) {
                pos = appendBytes(out_xml, pos, "</row>")?;
            }
            pos = appendBytes(out_xml, pos, "<row r=\"")?;
            pos = appendU32Ascii(out_xml, pos, c.ref.row + 1)?;
            pos = appendBytes(out_xml, pos, "\">")?;
            row_open = true;
            cur_row = c.ref.row;
        }

        const rlen = encodeCellRef(c.ref, ref_buf[0..])?;
        pos = appendBytes(out_xml, pos, "<c r=\"")?;
        pos = appendBytes(out_xml, pos, ref_buf[0..rlen])?;
        pos = appendBytes(out_xml, pos, "\"")?;
        if (c.style_idx > 0) {
            pos = appendBytes(out_xml, pos, " s=\"")?;
            pos = appendU32Ascii(out_xml, pos, c.style_idx)?;
            pos = appendBytes(out_xml, pos, "\"")?;
        }

        if (c.cell_type == XlsxCellType.shared_string) {
            // Look up index in shared_strings; fall back to inlineStr if absent
            let found_idx: u32? = null;
            let si: usize = 0;
            while (si < shared_strings.len) : (si += 1) {
                if (std.mem.eql(u8, shared_strings[si], c.text)) {
                    found_idx = si as u32;
                    break;
                }
            }
            if (idx := found_idx) {
                pos = appendBytes(out_xml, pos, " t=\"s\"><v>")?;
                pos = appendU32Ascii(out_xml, pos, idx)?;
                pos = appendBytes(out_xml, pos, "</v></c>")?;
            } else {
                pos = appendBytes(out_xml, pos, " t=\"inlineStr\"><is><t>")?;
                pos = appendXmlEscaped(out_xml, pos, c.text)?;
                pos = appendBytes(out_xml, pos, "</t></is></c>")?;
            }
        } else if (c.cell_type == XlsxCellType.inline_string) {
            pos = appendBytes(out_xml, pos, " t=\"inlineStr\"><is><t>")?;
            pos = appendXmlEscaped(out_xml, pos, c.text)?;
            pos = appendBytes(out_xml, pos, "</t></is></c>")?;
        } else if (c.cell_type == XlsxCellType.bool_val) {
            pos = appendBytes(out_xml, pos, " t=\"b\"><v>")?;
            pos = appendBytes(out_xml, pos, if (c.bool_val) "1" else "0")?;
            pos = appendBytes(out_xml, pos, "</v></c>")?;
        } else {
            pos = appendBytes(out_xml, pos, ">")?;
            if (c.formula.len > 0) {
                pos = appendBytes(out_xml, pos, "<f>")?;
                pos = appendXmlEscaped(out_xml, pos, c.formula)?;
                pos = appendBytes(out_xml, pos, "</f>")?;
            }
            if (c.has_num) {
                pos = appendBytes(out_xml, pos, "<v>")?;
                pos = appendF64Ascii(out_xml, pos, c.num_val)?;
                pos = appendBytes(out_xml, pos, "</v>")?;
            }
            pos = appendBytes(out_xml, pos, "</c>")?;
        }
    }

    if (row_open) {
        pos = appendBytes(out_xml, pos, "</row>")?;
    }
    pos = appendBytes(out_xml, pos, "</sheetData></worksheet>")?;
    return pos;
}

// ============================================================================
// 7. Zero-Heap Spreadsheet Formula Evaluator (`evalXlsxFormula`)
// ============================================================================

struct FormulaParser {
    src: []const u8 = "";
    pos: usize = 0;
    cells: []const XlsxCellValue = [];
    visited: [16]XlsxCellRef = [XlsxCellRef{}; 16];
    depth: usize = 0;

    fn skipWs(*_) {
        while (pos < src.len && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n')) : (pos += 1) {}
    }

    fn peek(*_): u8 {
        skipWs();
        if (pos >= src.len) return 0;
        return src[pos];
    }

    fn evalCellValue(*_, coord: XlsxCellRef): f64 ! XlsxError {
        let vi: usize = 0;
        while (vi < depth) : (vi += 1) {
            if (visited[vi].col == coord.col && visited[vi].row == coord.row) {
                return XlsxError.CycleDetected;
            }
        }
        let ci: usize = 0;
        while (ci < cells.len) : (ci += 1) {
            const c = cells[ci];
            if (c.ref.col == coord.col && c.ref.row == coord.row) {
                if (c.formula.len > 0) {
                    if (depth >= 16) return XlsxError.CycleDetected;
                    let sub = FormulaParser{
                        src: c.formula,
                        pos: 0,
                        cells: cells,
                        visited: visited,
                        depth: depth + 1,
                    };
                    sub.visited[depth] = coord;
                    if (sub.peek() == '=') sub.pos += 1;
                    return sub.parseComparison();
                }
                if (c.has_num) return c.num_val;
                if (c.cell_type == XlsxCellType.bool_val) return if (c.bool_val) 1.0 else 0.0;
                return 0.0;
            }
        }
        return 0.0; // Empty cell evaluates to 0.0 in Excel
    }

    fn parseComparison(*_): f64 ! XlsxError {
        const lhs = parseAddSub()?;
        skipWs();
        if (pos >= src.len) return lhs;
        const c0 = src[pos];
        if (c0 == '=') {
            pos += 1;
            const rhs = parseAddSub()?;
            return if (lhs == rhs) 1.0 else 0.0;
        }
        if (c0 == '<') {
            if (pos + 1 < src.len && src[pos + 1] == '>') {
                pos += 2;
                const rhs = parseAddSub()?;
                return if (lhs != rhs) 1.0 else 0.0;
            }
            if (pos + 1 < src.len && src[pos + 1] == '=') {
                pos += 2;
                const rhs = parseAddSub()?;
                return if (lhs <= rhs) 1.0 else 0.0;
            }
            pos += 1;
            const rhs = parseAddSub()?;
            return if (lhs < rhs) 1.0 else 0.0;
        }
        if (c0 == '>') {
            if (pos + 1 < src.len && src[pos + 1] == '=') {
                pos += 2;
                const rhs = parseAddSub()?;
                return if (lhs >= rhs) 1.0 else 0.0;
            }
            pos += 1;
            const rhs = parseAddSub()?;
            return if (lhs > rhs) 1.0 else 0.0;
        }
        return lhs;
    }

    fn parseAddSub(*_): f64 ! XlsxError {
        let acc = parseMulDiv()?;
        while (true) {
            const ch = peek();
            if (ch == '+') {
                pos += 1;
                acc += parseMulDiv()?;
            } else if (ch == '-') {
                pos += 1;
                acc -= parseMulDiv()?;
            } else {
                break;
            }
        }
        return acc;
    }

    fn parseMulDiv(*_): f64 ! XlsxError {
        let acc = parseUnary()?;
        while (true) {
            const ch = peek();
            if (ch == '*') {
                pos += 1;
                acc *= parseUnary()?;
            } else if (ch == '/') {
                pos += 1;
                const denom = parseUnary()?;
                if (denom == 0.0) return XlsxError.DivisionByZero;
                acc /= denom;
            } else {
                break;
            }
        }
        return acc;
    }

    fn parseUnary(*_): f64 ! XlsxError {
        const ch = peek();
        if (ch == '-') {
            pos += 1;
            return -(parseUnary()?);
        }
        if (ch == '+') {
            pos += 1;
            return parseUnary();
        }
        return parsePrimary();
    }

    fn parseNumber(*_): f64 ! XlsxError {
        const start: usize = pos;
        let has_dot: bool = false;
        while (pos < src.len) {
            const b = src[pos];
            if (b >= '0' && b <= '9') {
                pos += 1;
            } else if (b == '.' && !has_dot) {
                has_dot = true;
                pos += 1;
            } else {
                break;
            }
        }
        return std.fmt.parseFloat(f64, src[start..pos]) catch return XlsxError.FormulaSyntax;
    }

    fn eqIdentUpper(ident: []const u8, upper: []const u8): bool {
        if (ident.len != upper.len) return false;
        let i: usize = 0;
        while (i < ident.len) : (i += 1) {
            let c = ident[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            if (c != upper[i]) return false;
        }
        return true;
    }

    fn evalAggFunc(*_, name: []const u8): f64 ! XlsxError {
        // Expects '(' already consumed
        let sum: f64 = 0.0;
        let cnt: u64 = 0;
        let min_v: f64 = 0.0;
        let max_v: f64 = 0.0;

        while (true) {
            if (peek() == ')') {
                pos += 1;
                break;
            }
            // Check if next token is a range A1:B5
            const arg_start: usize = pos;
            let scan: usize = pos;
            let has_colon: bool = false;
            while (scan < src.len && src[scan] != ',' && src[scan] != ')') : (scan += 1) {
                if (src[scan] == ':') has_colon = true;
            }
            if (has_colon) {
                let end_trim: usize = scan;
                while (end_trim > arg_start && isXmlWs(src[end_trim - 1])) : (end_trim -= 1) {}
                const rg = parseCellRange(src[arg_start..end_trim])?;
                pos = scan;
                // Iterate over cells inside range
                let ci: usize = 0;
                while (ci < cells.len) : (ci += 1) {
                    const c = cells[ci];
                    if (rg.contains(c.ref)) {
                        if (c.has_num || c.formula.len > 0 || c.cell_type == XlsxCellType.bool_val) {
                            const v = evalCellValue(c.ref)?;
                            if (cnt == 0) {
                                min_v = v;
                                max_v = v;
                            } else {
                                if (v < min_v) min_v = v;
                                if (v > max_v) max_v = v;
                            }
                            sum += v;
                            cnt += 1;
                        }
                    }
                }
            } else {
                const v = parseComparison()?;
                if (cnt == 0) {
                    min_v = v;
                    max_v = v;
                } else {
                    if (v < min_v) min_v = v;
                    if (v > max_v) max_v = v;
                }
                sum += v;
                cnt += 1;
            }

            const sep = peek();
            if (sep == ',') {
                pos += 1;
                continue;
            }
            if (sep == ')') {
                pos += 1;
                break;
            }
            return XlsxError.FormulaSyntax;
        }

        if (eqIdentUpper(name, "SUM")) return sum;
        if (eqIdentUpper(name, "COUNT")) return cnt as f64;
        if (eqIdentUpper(name, "AVERAGE")) {
            if (cnt == 0) return XlsxError.DivisionByZero;
            return sum / (cnt as f64);
        }
        if (eqIdentUpper(name, "MIN")) return if (cnt == 0) 0.0 else min_v;
        if (eqIdentUpper(name, "MAX")) return if (cnt == 0) 0.0 else max_v;
        return XlsxError.FormulaSyntax;
    }

    fn parsePrimary(*_): f64 ! XlsxError {
        const ch = peek();
        if (ch == '(') {
            pos += 1;
            const val = parseComparison()?;
            if (peek() != ')') return XlsxError.FormulaSyntax;
            pos += 1;
            return val;
        }
        if ((ch >= '0' && ch <= '9') || ch == '.') {
            return parseNumber();
        }
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '$') {
            const start: usize = pos;
            while (pos < src.len) {
                const b = src[pos];
                if ((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9') || b == '$' || b == '_') {
                    pos += 1;
                } else {
                    break;
                }
            }
            const ident = src[start..pos];
            if (peek() == '(') {
                pos += 1;
                if (eqIdentUpper(ident, "IF")) {
                    const cond = parseComparison()?;
                    if (peek() != ',') return XlsxError.FormulaSyntax;
                    pos += 1;
                    const t_val = parseComparison()?;
                    if (peek() != ',') return XlsxError.FormulaSyntax;
                    pos += 1;
                    const f_val = parseComparison()?;
                    if (peek() != ')') return XlsxError.FormulaSyntax;
                    pos += 1;
                    return if (cond != 0.0) t_val else f_val;
                }
                if (eqIdentUpper(ident, "ABS")) {
                    const v = parseComparison()?;
                    if (peek() != ')') return XlsxError.FormulaSyntax;
                    pos += 1;
                    return if (v < 0.0) -v else v;
                }
                if (eqIdentUpper(ident, "ROUND")) {
                    const v = parseComparison()?;
                    let digits: i32 = 0;
                    if (peek() == ',') {
                        pos += 1;
                        digits = (parseComparison()?) as i32;
                    }
                    if (peek() != ')') return XlsxError.FormulaSyntax;
                    pos += 1;
                    let scale: f64 = 1.0;
                    let d: i32 = 0;
                    while (d < digits && d < 12) : (d += 1) {
                        scale *= 10.0;
                    }
                    const scaled = v * scale;
                    const rounded: f64 = if (scaled >= 0.0) (((scaled + 0.5) as i64) as f64) else (((scaled - 0.5) as i64) as f64);
                    return rounded / scale;
                }
                return evalAggFunc(ident);
            }
            const coord = decodeCellRef(ident) catch return XlsxError.InvalidReference;
            return evalCellValue(coord);
        }
        return XlsxError.FormulaSyntax;
    }
}

/// Evaluates an Excel formula (e.g. `"=SUM(A1:A5) * 1.1"`, `"=IF(A1>=10, MAX(B1:B3), ABS(-5))"`)
/// over caller-provided `cells` with zero dynamic heap allocation and `#DIV/0!`/`#REF!`/`#CYCLE!` guards.
pub fn evalXlsxFormula(formula: []const u8, cells: []const XlsxCellValue): f64 ! XlsxError {
    if (formula.len == 0) return XlsxError.FormulaSyntax;
    let parser = FormulaParser{
        src: formula,
        pos: 0,
        cells: cells,
        depth: 0,
    };
    if (parser.peek() == '=') parser.pos += 1;
    const val = parser.parseComparison()?;
    parser.skipWs();
    if (parser.pos != parser.src.len) return XlsxError.FormulaSyntax;
    return val;
}
