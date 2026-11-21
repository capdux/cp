// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc IUCr CIF 1.1 / mmCIF Parser)
// Pure CP zero-heap-allocation IUCr CIF 1.1 & mmCIF (PDBx) lexer and parser:
// data_ blocks, save_ frames, _tag value pairs, single/double quoted strings,
// column-0 semicolon multiline ;...; text fields, and loop_ table/row iterators.

/// CIF lexical token classification.
pub enum CifTokenKind: u8 {
    data_block = 0,
    save_frame = 1,
    loop_kw = 2,
    tag = 3,
    value = 4,
}

/// Zero-copy CIF lexical token.
pub struct CifToken {
    kind: CifTokenKind = CifTokenKind.value,
    text: []const u8 = "",
    is_quoted: bool = false,

    /// Returns `true` if this token is an unquoted CIF missing value sentinel `"."`.
    fn isMissing(): bool {
        return _.kind == CifTokenKind.value && !_.is_quoted && bytesEqual(_.text, ".");
    }

    /// Returns `true` if this token is an unquoted CIF unknown value sentinel `"?"`.
    fn isUnknown(): bool {
        return _.kind == CifTokenKind.value && !_.is_quoted && bytesEqual(_.text, "?");
    }

    /// For `data_<name>` or `save_<name>`, returns `<name>`; otherwise returns `text`.
    fn blockName(): []const u8 {
        if ((_.kind == CifTokenKind.data_block || _.kind == CifTokenKind.save_frame) && _.text.len >= 5) {
            return _.text[5..];
        }
        return _.text;
    }
}

/// Zero-copy top-level `_tag value` entry outside `loop_` tables.
pub struct CifEntry {
    tag: []const u8 = "",
    value: []const u8 = "",
    is_missing: bool = false,
    is_unknown: bool = false,
}

/// Aggregate statistics from a single-pass scan of a CIF / mmCIF document.
pub struct CifStats {
    data_block_count: u32 = 0,
    tag_pair_count: u32 = 0,
    loop_count: u32 = 0,
    loop_row_count: u32 = 0,
}

// ---------------------------------------------------------------------------
// Internal ASCII & Case-Insensitive Helpers
// ---------------------------------------------------------------------------

fn isWhitespace(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

fn toLowerAscii(ch: u8): u8 {
    if (ch >= 'A' && ch <= 'Z') {
        return ch + 32;
    }
    return ch;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn bytesEqualIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (toLowerAscii(s[i]) != toLowerAscii(prefix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn skipWsAndComments(data: []const u8, start: usize): usize {
    let pos = start;
    while (pos < data.len) {
        const ch = data[pos];
        if (isWhitespace(ch)) {
            pos += 1;
        } else if (ch == '#') {
            pos += 1;
            while (pos < data.len && data[pos] != '\n') {
                pos += 1;
            }
        } else {
            break;
        }
    }
    return pos;
}

// ---------------------------------------------------------------------------
// Public Lexer & Detection API
// ---------------------------------------------------------------------------

/// Checks whether `data` starts with a `data_` block header (case-insensitive)
/// after skipping leading whitespace and `#` comment lines.
pub fn isCif(data: []const u8): bool {
    const pos = skipWsAndComments(data, 0);
    if (pos >= data.len) {
        return false;
    }
    return startsWithIgnoreCase(data[pos..], "data_");
}

/// Zero-allocation CIF 1.1 / mmCIF streaming lexer.
pub struct CifLexer {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(data: []const u8): CifLexer {
        return CifLexer {
            data: data,
            pos: 0,
            failed: false,
        };
    }

    /// Returns `true` if the lexer encountered a syntax error (e.g. unterminated quote or `;...;`).
    fn hasError(): bool {
        return _.failed;
    }

    /// Advances past whitespace and `#` comments and returns the next `CifToken`, or `null` at EOF/error.
    fn next(*_): CifToken? {
        if (_.failed) {
            return null;
        }
        _.pos = skipWsAndComments(_.data, _.pos);
        if (_.pos >= _.data.len) {
            return null;
        }

        const ch = _.data[_.pos];

        // 1. Column-0 semicolon multiline text field `;...;`
        if (ch == ';' && (_.pos == 0 || _.data[_.pos - 1] == '\n' || _.data[_.pos - 1] == '\r')) {
            let content_start = _.pos + 1;
            // Skip single optional line break immediately following opening ';'
            if (content_start < _.data.len && _.data[content_start] == '\r') {
                content_start += 1;
            }
            if (content_start < _.data.len && _.data[content_start] == '\n') {
                content_start += 1;
            }

            let scan = _.pos + 1;
            while (scan < _.data.len) {
                if (_.data[scan] == ';' && (_.data[scan - 1] == '\n' || _.data[scan - 1] == '\r')) {
                    let content_end = scan;
                    // Strip the single line terminator preceding closing ';'
                    if (content_end > content_start && _.data[content_end - 1] == '\n') {
                        content_end -= 1;
                    }
                    if (content_end > content_start && _.data[content_end - 1] == '\r') {
                        content_end -= 1;
                    }
                    _.pos = scan + 1;
                    return CifToken {
                        kind: CifTokenKind.value,
                        text: _.data[content_start..content_end],
                        is_quoted: true,
                    };
                }
                scan += 1;
            }
            // Unterminated multiline semicolon block
            _.failed = true;
            return null;
        }

        // 2. Single-quoted '...' or double-quoted "..." string
        // Per CIF 1.1: closes ONLY when quote is immediately followed by whitespace or EOF.
        if (ch == '\'' || ch == '"') {
            const quote = ch;
            const start = _.pos + 1;
            let scan = start;
            while (scan < _.data.len) {
                const c = _.data[scan];
                if (c == '\n' || c == '\r') {
                    _.failed = true;
                    return null;
                }
                if (c == quote && (scan + 1 >= _.data.len || isWhitespace(_.data[scan + 1]))) {
                    _.pos = scan + 1;
                    return CifToken {
                        kind: CifTokenKind.value,
                        text: _.data[start..scan],
                        is_quoted: true,
                    };
                }
                scan += 1;
            }
            _.failed = true;
            return null;
        }

        // 3. Unquoted token (terminated by whitespace or EOF)
        const start = _.pos;
        while (_.pos < _.data.len && !isWhitespace(_.data[_.pos])) {
            _.pos += 1;
        }
        const raw = _.data[start.._.pos];

        if (startsWithIgnoreCase(raw, "data_")) {
            return CifToken { kind: CifTokenKind.data_block, text: raw, is_quoted: false };
        }
        if (startsWithIgnoreCase(raw, "save_")) {
            return CifToken { kind: CifTokenKind.save_frame, text: raw, is_quoted: false };
        }
        if (bytesEqualIgnoreCase(raw, "loop_")) {
            return CifToken { kind: CifTokenKind.loop_kw, text: raw, is_quoted: false };
        }
        if (raw.len >= 2 && raw[0] == '_') {
            return CifToken { kind: CifTokenKind.tag, text: raw, is_quoted: false };
        }
        return CifToken { kind: CifTokenKind.value, text: raw, is_quoted: false };
    }
}

// ---------------------------------------------------------------------------
// Top-Level Tag-Value Pair Iterator & Lookup
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over top-level `_tag value` pairs (automatically skips `loop_` tables).
pub struct CifPairIter {
    lexer: CifLexer = CifLexer {},
    failed: bool = false,

    pub fn init(data: []const u8): CifPairIter {
        return CifPairIter {
            lexer: CifLexer.init(data),
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed || _.lexer.hasError();
    }

    fn next(*_): CifEntry? {
        if (_.failed || _.lexer.hasError()) {
            return null;
        }

        while (tok := _.lexer.next()) {
            if (tok.kind == CifTokenKind.data_block || tok.kind == CifTokenKind.save_frame) {
                continue;
            }
            if (tok.kind == CifTokenKind.loop_kw) {
                // Skip loop column header tags
                let col_count: u32 = 0;
                while (true) {
                    const saved = _.lexer.pos;
                    if (t := _.lexer.next()) {
                        if (t.kind == CifTokenKind.tag) {
                            col_count += 1;
                        } else {
                            _.lexer.pos = saved;
                            break;
                        }
                    } else {
                        break;
                    }
                }
                if (col_count == 0 || _.lexer.hasError()) {
                    _.failed = true;
                    return null;
                }
                // Skip loop value cells
                let val_count: u32 = 0;
                while (true) {
                    const saved = _.lexer.pos;
                    if (v := _.lexer.next()) {
                        if (v.kind == CifTokenKind.value) {
                            val_count += 1;
                        } else {
                            _.lexer.pos = saved;
                            break;
                        }
                    } else {
                        break;
                    }
                }
                if (_.lexer.hasError() || (val_count % col_count) != 0) {
                    _.failed = true;
                    return null;
                }
                continue;
            }
            if (tok.kind == CifTokenKind.tag) {
                if (val := _.lexer.next()) {
                    if (val.kind != CifTokenKind.value) {
                        _.failed = true;
                        return null;
                    }
                    return CifEntry {
                        tag: tok.text,
                        value: val.text,
                        is_missing: val.isMissing(),
                        is_unknown: val.isUnknown(),
                    };
                }
                _.failed = true;
                return null;
            }
            // Stray value outside tag or loop
            _.failed = true;
            return null;
        }
        return null;
    }
}

/// Finds the first top-level value matching `tag_name` (case-insensitive) in a CIF document.
pub fn findCifTag(data: []const u8, tag_name: []const u8): ([]const u8)? {
    let it = CifPairIter.init(data);
    while (entry := it.next()) {
        if (bytesEqualIgnoreCase(entry.tag, tag_name)) {
            return entry.value;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// Loop Table & Row Iterators (Zero-Alloc)
// ---------------------------------------------------------------------------

/// Zero-copy view of a single row within a `loop_` table.
pub struct CifRow {
    row_slice: []const u8 = "",
    col_count: u32 = 0,

    fn colCount(): u32 {
        return _.col_count;
    }

    /// Returns the `col_idx`-th value string in this row, or `null` if out of range.
    fn get(col_idx: u32): ([]const u8)? {
        if (tok := _.getToken(col_idx)) {
            return tok.text;
        }
        return null;
    }

    /// Returns the full `CifToken` at `col_idx` (preserving `isMissing()` / `isUnknown()`).
    fn getToken(col_idx: u32): CifToken? {
        if (col_idx >= _.col_count) {
            return null;
        }
        let lex = CifLexer.init(_.row_slice);
        let idx: u32 = 0;
        while (tok := lex.next()) {
            if (idx == col_idx) {
                return tok;
            }
            idx += 1;
        }
        return null;
    }
}

/// Zero-allocation iterator yielding grouped `CifRow` records from a `loop_` table.
pub struct CifRowIter {
    data: []const u8 = "",
    lexer: CifLexer = CifLexer {},
    col_count: u32 = 0,
    failed: bool = false,

    pub fn init(values_slice: []const u8, col_count: u32): CifRowIter {
        return CifRowIter {
            data: values_slice,
            lexer: CifLexer.init(values_slice),
            col_count: col_count,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed || _.lexer.hasError();
    }

    fn next(*_): CifRow? {
        if (_.failed || _.lexer.hasError() || _.col_count == 0) {
            return null;
        }
        _.lexer.pos = skipWsAndComments(_.data, _.lexer.pos);
        if (_.lexer.pos >= _.data.len) {
            return null;
        }

        const row_start = _.lexer.pos;
        let i: u32 = 0;
        while (i < _.col_count) {
            if (tok := _.lexer.next()) {
                if (tok.kind != CifTokenKind.value) {
                    _.failed = true;
                    return null;
                }
            } else {
                _.failed = true;
                return null;
            }
            i += 1;
        }
        const row_end = _.lexer.pos;
        return CifRow {
            row_slice: _.data[row_start..row_end],
            col_count: _.col_count,
        };
    }
}

/// Zero-copy view of a CIF `loop_` table with column lookup and row iteration.
pub struct CifLoop {
    tags_slice: []const u8 = "",
    values_slice: []const u8 = "",
    col_count: u32 = 0,

    /// Returns the number of columns declared in this `loop_` header.
    fn colCount(): u32 {
        return _.col_count;
    }

    /// Finds the 0-based column index for `tag_name` (case-insensitive), or `null`.
    fn colIndex(tag_name: []const u8): u32? {
        let lex = CifLexer.init(_.tags_slice);
        let idx: u32 = 0;
        while (tok := lex.next()) {
            if (tok.kind == CifTokenKind.tag) {
                if (bytesEqualIgnoreCase(tok.text, tag_name)) {
                    return idx;
                }
                idx += 1;
            }
        }
        return null;
    }

    /// Returns a zero-allocation `CifRowIter` over all rows in this `loop_` table.
    fn rows(): CifRowIter {
        return CifRowIter.init(_.values_slice, _.col_count);
    }
}

/// Zero-allocation iterator over all `loop_` tables in a CIF / mmCIF document.
pub struct CifLoopIter {
    data: []const u8 = "",
    lexer: CifLexer = CifLexer {},
    failed: bool = false,

    pub fn init(data: []const u8): CifLoopIter {
        return CifLoopIter {
            data: data,
            lexer: CifLexer.init(data),
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed || _.lexer.hasError();
    }

    fn next(*_): CifLoop? {
        if (_.failed || _.lexer.hasError()) {
            return null;
        }

        while (tok := _.lexer.next()) {
            if (tok.kind != CifTokenKind.loop_kw) {
                continue;
            }

            const tags_start = _.lexer.pos;
            let tags_end = tags_start;
            let col_count: u32 = 0;

            while (true) {
                const saved = _.lexer.pos;
                if (t := _.lexer.next()) {
                    if (t.kind == CifTokenKind.tag) {
                        col_count += 1;
                        tags_end = _.lexer.pos;
                    } else {
                        _.lexer.pos = saved;
                        break;
                    }
                } else {
                    break;
                }
            }

            if (col_count == 0 || _.lexer.hasError()) {
                _.failed = true;
                return null;
            }

            const values_start = tags_end;
            let values_end = values_start;
            let val_count: u32 = 0;

            while (true) {
                const saved = _.lexer.pos;
                if (v := _.lexer.next()) {
                    if (v.kind == CifTokenKind.value) {
                        val_count += 1;
                        values_end = _.lexer.pos;
                    } else {
                        _.lexer.pos = saved;
                        break;
                    }
                } else {
                    break;
                }
            }

            if (_.lexer.hasError() || (val_count % col_count) != 0) {
                _.failed = true;
                return null;
            }

            return CifLoop {
                tags_slice: _.data[tags_start..tags_end],
                values_slice: _.data[values_start..values_end],
                col_count: col_count,
            };
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// Single-Pass Summary
// ---------------------------------------------------------------------------

/// Scans a CIF / mmCIF document in a single zero-allocation pass and collects structural stats.
/// Returns `null` if not a CIF document or if any syntax / incomplete `loop_` error is found.
pub fn summarizeCif(data: []const u8): CifStats? {
    if (!isCif(data)) {
        return null;
    }
    let lex = CifLexer.init(data);
    let stats = CifStats {};

    while (tok := lex.next()) {
        if (tok.kind == CifTokenKind.data_block) {
            stats.data_block_count += 1;
        } else if (tok.kind == CifTokenKind.save_frame) {
            continue;
        } else if (tok.kind == CifTokenKind.tag) {
            if (val := lex.next()) {
                if (val.kind != CifTokenKind.value) {
                    return null;
                }
                stats.tag_pair_count += 1;
            } else {
                return null;
            }
        } else if (tok.kind == CifTokenKind.loop_kw) {
            let cols: u32 = 0;
            while (true) {
                const saved = lex.pos;
                if (t := lex.next()) {
                    if (t.kind == CifTokenKind.tag) {
                        cols += 1;
                    } else {
                        lex.pos = saved;
                        break;
                    }
                } else {
                    break;
                }
            }
            if (cols == 0 || lex.hasError()) {
                return null;
            }
            let vals: u32 = 0;
            while (true) {
                const saved = lex.pos;
                if (v := lex.next()) {
                    if (v.kind == CifTokenKind.value) {
                        vals += 1;
                    } else {
                        lex.pos = saved;
                        break;
                    }
                } else {
                    break;
                }
            }
            if (lex.hasError() || (vals % cols) != 0) {
                return null;
            }
            stats.loop_count += 1;
            stats.loop_row_count += vals / cols;
        } else {
            return null;
        }
    }

    if (lex.hasError()) {
        return null;
    }
    return stats;
}
