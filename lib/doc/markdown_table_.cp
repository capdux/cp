// Reference Only: Functional overlap with lib/md/table.cp | Kept for comparison/analysis
// Source: Absorbed from /data/dev/bun/src/md/parser.zig | Synced: 2026-11-21T23:53:53Z (GFM Pipe Table Parser & Alignment Detector)
// Pure-CP zero-allocation GitHub Flavored Markdown pipe-table row parser, delimiter alignment detector, and cell escaper.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

export enum ColAlign {
    none,
    left,
    center,
    right,
}

/// Parse a single GFM delimiter cell (e.g. `---`, `:---`, `:---:`, `---:`) into `ColAlign`.
/// Returns null if the cell is not a valid GFM delimiter cell.
export fn parseDelimiterCell(raw_cell: []const u8): ColAlign? {
    const s = trimWs(raw_cell);
    if (s.len == 0) return null;

    let left_colon = false;
    let right_colon = false;
    let start: usize = 0;
    let end: usize = s.len;

    if (s[start] == ':') {
        left_colon = true;
        start += 1;
    }
    if (end > start && s[end - 1] == ':') {
        right_colon = true;
        end -= 1;
    }
    if (start >= end) return null; // must have at least one '-'
    let i = start;
    while (i < end) {
        if (s[i] != '-') return null;
        i += 1;
    }

    if (left_colon && right_colon) return ColAlign.center;
    if (left_colon) return ColAlign.left;
    if (right_colon) return ColAlign.right;
    return ColAlign.none;
}

/// Zero-copy iterator over cells of a GFM pipe-table row (handles leading/trailing `|` and escaped `\|`).
export struct TableRowIter {
    row: []const u8,
    pos: usize,

    pub static fn init(raw_line: []const u8): TableRowIter {
        let s = trimWs(raw_line);
        if (s.len > 0 && s[0] == '|') {
            s = s[1..];
        }
        // Strip unescaped trailing '|'
        if (s.len > 0 && s[s.len - 1] == '|') {
            let bs_count: usize = 0;
            let k = s.len - 1;
            while (k > 0 && s[k - 1] == '\\') {
                bs_count += 1;
                k -= 1;
            }
            if ((bs_count % 2) == 0) {
                s = s[0..s.len - 1];
            }
        }
        return TableRowIter { row: s, pos: 0 };
    }

    pub fn next(*_): ([]const u8)? {
        if (_.pos > _.row.len) return null;
        if (_.row.len == 0) {
            _.pos = 1;
            return null;
        }
        const start = _.pos;
        while (_.pos < _.row.len) {
            if (_.row[_.pos] == '\\' && _.pos + 1 < _.row.len) {
                _.pos += 2;
                continue;
            }
            if (_.row[_.pos] == '|') {
                const cell = trimWs(_.row[start.._.pos]);
                _.pos += 1;
                return cell;
            }
            _.pos += 1;
        }
        const cell = trimWs(_.row[start.._.row.len]);
        _.pos = _.row.len + 1;
        return cell;
    }
}

/// Check if `raw_line` is a valid GFM table delimiter row and return its column count, or null if invalid.
export fn parseDelimiterRow(raw_line: []const u8, out_aligns: []ColAlign): usize? {
    const trimmed = trimWs(raw_line);
    if (trimmed.len == 0) return null;
    // Must contain at least one '-'
    let has_dash = false;
    let i: usize = 0;
    while (i < trimmed.len) {
        if (trimmed[i] == '-') {
            has_dash = true;
            break;
        }
        i += 1;
    }
    if (!has_dash) return null;

    let it = TableRowIter.init(trimmed);
    let count: usize = 0;
    while (cell := it.next()) {
        const col_align = parseDelimiterCell(cell) ?? return null;
        if (count < out_aligns.len) {
            out_aligns[count] = col_align;
        }
        count += 1;
    }
    if (count == 0) return null;
    return count;
}

/// Unescape `\|` -> `|` in a GFM table cell into `out`.
export fn unescapeTableCell(raw_cell: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw_cell.len) {
        if (raw_cell[i] == '\\' && i + 1 < raw_cell.len && raw_cell[i + 1] == '|') {
            if (pos >= out.len) return null;
            out[pos] = '|';
            pos += 1;
            i += 2;
            continue;
        }
        if (pos >= out.len) return null;
        out[pos] = raw_cell[i];
        pos += 1;
        i += 1;
    }
    return out[0..pos];
}

/// Escape `|` -> `\|` and newlines -> space in a GFM table cell into `out`.
export fn escapeTableCell(text: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        const c = text[i];
        if (c == '|') {
            if (pos + 2 > out.len) return null;
            out[pos] = '\\';
            out[pos + 1] = '|';
            pos += 2;
        } else if (c == '\n' || c == '\r') {
            if (pos >= out.len) return null;
            out[pos] = ' ';
            pos += 1;
        } else {
            if (pos >= out.len) return null;
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return out[0..pos];
}
