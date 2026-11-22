// Origin: Pure-CP Stdlib (Zero-Alloc OOXML SpreadsheetML .xlsx Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation OOXML SpreadsheetML parser:
// A1 cell reference codec, xl/workbook.xml sheet iterator, xl/sharedStrings.xml iterator
// with rich-text <r><t> concatenation & XML entity unescaping, and xl/worksheets/sheet*.xml
// streaming cell iterator.

const std = @import("std");

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

/// Finds the closing `'>'` of an XML tag starting at `xml[start] == '<'`, respecting quoted attributes.
fn findTagClose(xml: []const u8, start: usize): usize? {
    if (start >= xml.len || xml[start] != '<') {
        return null;
    }
    let i = start + 1;
    let quote: u8 = 0;
    while (i < xml.len) {
        const b = xml[i];
        if (quote != 0) {
            if (b == quote) {
                quote = 0;
            }
        } else {
            if (b == '"' || b == '\'') {
                quote = b;
            } else if (b == '>') {
                return i;
            }
        }
        i += 1;
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
            i += 1;
            continue;
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
