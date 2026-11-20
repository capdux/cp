// Source: cp/lib/ini/ini.cp (Pure CP INI / Systemd / Gitconfig Parser, Lossless AST & Incremental Editor)
// Zero external dependencies; preserves comments, blank lines, and section ordering on stringify().

const std = @import("std");

pub err IniError {
    UnterminatedSection,
    UnterminatedQuote,
    MissingEquals,
    InvalidInteger,
    InvalidBool,
    SectionNotFound,
    KeyNotFound,
    OutOfMemory,
}

pub struct IniOptions {
    allow_duplicate_keys: bool = true,
    case_sensitive_sections: bool = true,
    allow_hash_comments: bool = true,
    allow_semicolon_comments: bool = true,
}

pub enum IniLineKind {
    Blank,
    Comment,
    SectionHeader,
    KeyValue,
}

pub struct IniLine {
    kind: IniLineKind = .Blank,
    raw: []u8 = "",
    section: []u8 = "",
    key: []u8 = "",
    value: []u8 = "",
    inline_comment: []u8 = "",
    delim: u8 = '=',
    dirty: bool = false,

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.raw.len > 0) alloc.free(_.raw);
        if (_.section.len > 0) alloc.free(_.section);
        if (_.key.len > 0) alloc.free(_.key);
        if (_.value.len > 0) alloc.free(_.value);
        if (_.inline_comment.len > 0) alloc.free(_.inline_comment);
        _.raw = "";
        _.section = "";
        _.key = "";
        _.value = "";
        _.inline_comment = "";
    }
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
    }
    return true;
}

fn dupSlice(alloc: std.mem.Allocator, s: []const u8): ([]u8)! {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return IniError.OutOfMemory;
}

fn endsWithContinuation(line: []const u8): bool {
    if (line.len == 0) return false;
    let bs_count: usize = 0;
    let i: usize = line.len;
    while (i > 0 && line[i - 1] == '\\') {
        bs_count += 1;
        i -= 1;
    }
    return (bs_count % 2) == 1;
}

fn isCommentStart(c: u8, opts: IniOptions): bool {
    if (c == '#' && opts.allow_hash_comments) return true;
    if (c == ';' && opts.allow_semicolon_comments) return true;
    return false;
}

fn needsQuoting(val: []const u8): bool {
    if (val.len == 0) return false;
    if (isSpace(val[0]) || isSpace(val[val.len - 1])) return true;
    for (c of val) {
        if (c == '\n' || c == '\r' || c == '\t' || c == '#' || c == ';' || c == '"') {
            return true;
        }
    }
    return false;
}

fn appendJsonEscaped(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): ! {
    buf.append(alloc, '"') catch return IniError.OutOfMemory;
    for (c of s) {
        switch (c) {
            '"' => buf.appendSlice(alloc, "\\\"") catch return IniError.OutOfMemory,
            '\\' => buf.appendSlice(alloc, "\\\\") catch return IniError.OutOfMemory,
            '\n' => buf.appendSlice(alloc, "\\n") catch return IniError.OutOfMemory,
            '\r' => buf.appendSlice(alloc, "\\r") catch return IniError.OutOfMemory,
            '\t' => buf.appendSlice(alloc, "\\t") catch return IniError.OutOfMemory,
            0x08 => buf.appendSlice(alloc, "\\b") catch return IniError.OutOfMemory,
            0x0C => buf.appendSlice(alloc, "\\f") catch return IniError.OutOfMemory,
            else => {
                if (c < 0x20) {
                    const hex = "0123456789abcdef";
                    let esc: [6]u8 = ['\\', 'u', '0', '0', hex[(c >> 4) & 0x0F], hex[c & 0x0F]];
                    buf.appendSlice(alloc, esc[0..6]) catch return IniError.OutOfMemory;
                } else {
                    buf.append(alloc, c) catch return IniError.OutOfMemory;
                }
            },
        }
    }
    buf.append(alloc, '"') catch return IniError.OutOfMemory;
}

fn appendIniFormattedValue(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: []const u8): ! {
    if (!needsQuoting(val)) {
        buf.appendSlice(alloc, val) catch return IniError.OutOfMemory;
        return;
    }
    buf.append(alloc, '"') catch return IniError.OutOfMemory;
    for (c of val) {
        switch (c) {
            '"' => buf.appendSlice(alloc, "\\\"") catch return IniError.OutOfMemory,
            '\\' => buf.appendSlice(alloc, "\\\\") catch return IniError.OutOfMemory,
            '\n' => buf.appendSlice(alloc, "\\n") catch return IniError.OutOfMemory,
            '\r' => buf.appendSlice(alloc, "\\r") catch return IniError.OutOfMemory,
            '\t' => buf.appendSlice(alloc, "\\t") catch return IniError.OutOfMemory,
            else => buf.append(alloc, c) catch return IniError.OutOfMemory,
        }
    }
    buf.append(alloc, '"') catch return IniError.OutOfMemory;
}

struct ParsedVal {
    val: []u8 = "",
    comment: []u8 = "",
}

pub struct IniDoc {
    allocator: std.mem.Allocator,
    options: IniOptions = IniOptions{},
    lines: std.ArrayList(IniLine) = .empty,

    static fn init(alloc: std.mem.Allocator): IniDoc {
        return IniDoc{
            allocator: alloc,
            options: IniOptions{},
            lines: .empty,
        };
    }

    fn deinit(*_) {
        for (*line of _.lines.items) {
            line.deinit(_.allocator);
        }
        _.lines.deinit(_.allocator);
    }

    static fn parse(alloc: std.mem.Allocator, text: []const u8): IniDoc! {
        return IniDoc.parseWithOptions(alloc, text, IniOptions{});
    }

    static fn parseWithOptions(alloc: std.mem.Allocator, text: []const u8, opts: IniOptions): IniDoc! {
        let doc = IniDoc{
            allocator: alloc,
            options: opts,
            lines: .empty,
        };
        errdefer doc.deinit();

        let src: []const u8 = text;
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            src = src[3..];
        }

        let current_section: []const u8 = "";
        let pos: usize = 0;

        while (pos < src.len) {
            let line_start: usize = pos;
            while (pos < src.len && src[pos] != '\n') {
                pos += 1;
            }
            let line_end: usize = pos;
            if (line_end > line_start && src[line_end - 1] == '\r') {
                line_end -= 1;
            }
            if (pos < src.len && src[pos] == '\n') {
                pos += 1;
            }

            let first_line: []const u8 = src[line_start..line_end];
            let trimmed: []const u8 = trimSpaces(first_line);

            // 1. Blank line
            if (trimmed.len == 0) {
                let raw_dup = dupSlice(alloc, first_line)?;
                doc.lines.append(alloc, IniLine{
                    kind: .Blank,
                    raw: raw_dup,
                }) catch return IniError.OutOfMemory;
                continue;
            }

            // 2. Full-line comment
            if (isCommentStart(trimmed[0], opts)) {
                let raw_dup = dupSlice(alloc, first_line)?;
                doc.lines.append(alloc, IniLine{
                    kind: .Comment,
                    raw: raw_dup,
                }) catch return IniError.OutOfMemory;
                continue;
            }

            // 3. Section header: [section] or [section "sub"]
            if (trimmed[0] == '[') {
                let close_idx: usize? = null;
                let in_quotes: bool = false;
                let escaped: bool = false;
                let idx: usize = 1;
                while (idx < trimmed.len) : (idx += 1) {
                    let ch = trimmed[idx];
                    if (escaped) {
                        escaped = false;
                        continue;
                    }
                    if (in_quotes && ch == '\\') {
                        escaped = true;
                        continue;
                    }
                    if (ch == '"') {
                        in_quotes = !in_quotes;
                        continue;
                    }
                    if (!in_quotes && ch == ']') {
                        close_idx = idx;
                        break;
                    }
                }

                let cidx = close_idx ?? return IniError.UnterminatedSection;
                let sec_name = trimSpaces(trimmed[1..cidx]);
                let after = trimSpaces(trimmed[(cidx + 1)..]);
                let inline_cmt: []const u8 = "";
                if (after.len > 0) {
                    if (isCommentStart(after[0], opts)) {
                        inline_cmt = after;
                    } else {
                        return IniError.UnterminatedSection;
                    }
                }

                let raw_dup = dupSlice(alloc, first_line)?;
                let sec_dup = dupSlice(alloc, sec_name)?;
                let cmt_dup = dupSlice(alloc, inline_cmt)?;
                doc.lines.append(alloc, IniLine{
                    kind: .SectionHeader,
                    raw: raw_dup,
                    section: sec_dup,
                    inline_comment: cmt_dup,
                }) catch return IniError.OutOfMemory;
                current_section = doc.lines.items[doc.lines.items.len - 1].section;
                continue;
            }

            // 4. Key-Value line (with optional trailing `\` continuations)
            let delim_pos: usize? = null;
            let k_idx: usize = 0;
            while (k_idx < first_line.len) : (k_idx += 1) {
                let ch = first_line[k_idx];
                if (ch == '=' || ch == ':') {
                    delim_pos = k_idx;
                    break;
                }
            }

            let dpos = delim_pos ?? return IniError.MissingEquals;
            let key_slice = trimSpaces(first_line[0..dpos]);
            if (key_slice.len == 0) {
                return IniError.MissingEquals;
            }
            let delim_ch: u8 = first_line[dpos];

            // Collect physical raw lines and logical value parts if `\` continuation is present
            let raw_buf: std.ArrayList(u8) = .empty;
            defer raw_buf.deinit(alloc);
            let val_buf: std.ArrayList(u8) = .empty;
            defer val_buf.deinit(alloc);

            raw_buf.appendSlice(alloc, first_line) catch return IniError.OutOfMemory;

            let cur_val_part: []const u8 = first_line[(dpos + 1)..];
            let has_cont: bool = endsWithContinuation(cur_val_part);

            if (has_cont) {
                let without_bs = trimSpaces(cur_val_part[0..(cur_val_part.len - 1)]);
                val_buf.appendSlice(alloc, without_bs) catch return IniError.OutOfMemory;

                while (has_cont && pos < src.len) {
                    let next_start: usize = pos;
                    while (pos < src.len && src[pos] != '\n') {
                        pos += 1;
                    }
                    let next_end: usize = pos;
                    if (next_end > next_start && src[next_end - 1] == '\r') {
                        next_end -= 1;
                    }
                    if (pos < src.len && src[pos] == '\n') {
                        pos += 1;
                    }
                    let next_line = src[next_start..next_end];
                    raw_buf.append(alloc, '\n') catch return IniError.OutOfMemory;
                    raw_buf.appendSlice(alloc, next_line) catch return IniError.OutOfMemory;

                    has_cont = endsWithContinuation(next_line);
                    let next_part: []const u8 = if (has_cont) next_line[0..(next_line.len - 1)] else next_line;
                    let next_trimmed = trimSpaces(next_part);
                    if (next_trimmed.len > 0) {
                        if (val_buf.items.len > 0) {
                            val_buf.append(alloc, ' ') catch return IniError.OutOfMemory;
                        }
                        val_buf.appendSlice(alloc, next_trimmed) catch return IniError.OutOfMemory;
                    }
                }
            } else {
                val_buf.appendSlice(alloc, cur_val_part) catch return IniError.OutOfMemory;
            }

            let parsed_val_res = IniDoc.parseValueAndComment(alloc, val_buf.items, opts)?;
            let parsed_val = parsed_val_res.val;
            let parsed_cmt = parsed_val_res.comment;
            errdefer {
                if (parsed_val.len > 0) alloc.free(parsed_val);
                if (parsed_cmt.len > 0) alloc.free(parsed_cmt);
            }

            if (!opts.allow_duplicate_keys) {
                if (existing_idx := doc.findLastKeyIndex(current_section, key_slice)) {
                    let existing = &doc.lines.items[existing_idx];
                    if (existing.value.len > 0) alloc.free(existing.value);
                    existing.value = parsed_val;
                    if (existing.inline_comment.len > 0) alloc.free(existing.inline_comment);
                    existing.inline_comment = parsed_cmt;
                    existing.dirty = true;
                    continue;
                }
            }

            let raw_owned = dupSlice(alloc, raw_buf.items)?;
            let sec_owned = dupSlice(alloc, current_section)?;
            let key_owned = dupSlice(alloc, key_slice)?;

            doc.lines.append(alloc, IniLine{
                kind: .KeyValue,
                raw: raw_owned,
                section: sec_owned,
                key: key_owned,
                value: parsed_val,
                inline_comment: parsed_cmt,
                delim: delim_ch,
                dirty: false,
            }) catch return IniError.OutOfMemory;
        }

        return doc;
    }

    static fn parseValueAndComment(
        alloc: std.mem.Allocator,
        raw_val: []const u8,
        opts: IniOptions,
    ): ParsedVal! {
        let s = trimSpaces(raw_val);
        if (s.len == 0) {
            return ParsedVal{};
        }

        let first = s[0];
        if (first == '"' || first == '\'') {
            let quote = first;
            let out: std.ArrayList(u8) = .empty;
            errdefer out.deinit(alloc);

            let i: usize = 1;
            let closed: bool = false;
            while (i < s.len) : (i += 1) {
                let ch = s[i];
                if (quote == '"' && ch == '\\') {
                    if (i + 1 >= s.len) {
                        return IniError.UnterminatedQuote;
                    }
                    i += 1;
                    let esc = s[i];
                    switch (esc) {
                        'n' => out.append(alloc, '\n') catch return IniError.OutOfMemory,
                        'r' => out.append(alloc, '\r') catch return IniError.OutOfMemory,
                        't' => out.append(alloc, '\t') catch return IniError.OutOfMemory,
                        '\\' => out.append(alloc, '\\') catch return IniError.OutOfMemory,
                        '"' => out.append(alloc, '"') catch return IniError.OutOfMemory,
                        else => {
                            out.append(alloc, '\\') catch return IniError.OutOfMemory;
                            out.append(alloc, esc) catch return IniError.OutOfMemory;
                        },
                    }
                    continue;
                }
                if (ch == quote) {
                    closed = true;
                    i += 1;
                    break;
                }
                out.append(alloc, ch) catch return IniError.OutOfMemory;
            }

            if (!closed) {
                return IniError.UnterminatedQuote;
            }

            let rest = trimSpaces(s[i..]);
            let cmt: []const u8 = "";
            if (rest.len > 0 && isCommentStart(rest[0], opts)) {
                cmt = rest;
            }

            let val_owned = out.toOwnedSlice(alloc) catch return IniError.OutOfMemory;
            errdefer if (val_owned.len > 0) alloc.free(val_owned);
            let cmt_owned = dupSlice(alloc, cmt)?;
            return ParsedVal{ val: val_owned, comment: cmt_owned };
        }

        // Unquoted value: check for inline comment preceded by whitespace (or at index 0)
        let val_end: usize = s.len;
        let cmt_slice: []const u8 = "";
        let idx: usize = 0;
        while (idx < s.len) : (idx += 1) {
            if (isCommentStart(s[idx], opts) && (idx == 0 || isSpace(s[idx - 1]))) {
                val_end = idx;
                cmt_slice = s[idx..];
                break;
            }
        }

        let val_trimmed = trimSpaces(s[0..val_end]);
        let val_owned = dupSlice(alloc, val_trimmed)?;
        errdefer if (val_owned.len > 0) alloc.free(val_owned);
        let cmt_owned = dupSlice(alloc, cmt_slice)?;
        return ParsedVal{ val: val_owned, comment: cmt_owned };
    }

    fn sectionEq(a: []const u8, b: []const u8): bool {
        if (_.options.case_sensitive_sections) {
            return std.mem.eql(u8, a, b);
        }
        return asciiEqlIgnoreCase(a, b);
    }

    fn findLastKeyIndex(sec: []const u8, key: []const u8): usize? {
        let found: usize? = null;
        let i: usize = 0;
        while (i < _.lines.items.len) : (i += 1) {
            let line = _.lines.items[i];
            if (line.kind == .KeyValue && _.sectionEq(line.section, sec) && std.mem.eql(u8, line.key, key)) {
                found = i;
            }
        }
        return found;
    }

    fn hasSection(sec: []const u8): bool {
        for (line of _.lines.items) {
            if (sec.len == 0) {
                if (line.kind == .KeyValue && line.section.len == 0) return true;
            } else {
                if (line.kind == .SectionHeader && _.sectionEq(line.section, sec)) return true;
            }
        }
        return false;
    }

    fn hasKey(sec: []const u8, key: []const u8): bool {
        return _.findLastKeyIndex(sec, key) != null;
    }

    fn get(sec: []const u8, key: []const u8): ([]const u8)? {
        if (idx := _.findLastKeyIndex(sec, key)) {
            return _.lines.items[idx].value;
        }
        return null;
    }

    fn getAll(alloc: std.mem.Allocator, sec: []const u8, key: []const u8): ([][]const u8)! {
        let out: std.ArrayList(string) = .empty;
        errdefer out.deinit(alloc);
        for (line of _.lines.items) {
            if (line.kind == .KeyValue && _.sectionEq(line.section, sec) && std.mem.eql(u8, line.key, key)) {
                out.append(alloc, line.value) catch return IniError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return IniError.OutOfMemory;
    }

    fn getInt(sec: []const u8, key: []const u8): i64! {
        if (!_.hasSection(sec)) return IniError.SectionNotFound;
        let raw = _.get(sec, key) ?? return IniError.KeyNotFound;
        let trimmed = trimSpaces(raw);
        return std.fmt.parseInt(i64, trimmed, 0) catch return IniError.InvalidInteger;
    }

    fn getFloat(sec: []const u8, key: []const u8): f64! {
        if (!_.hasSection(sec)) return IniError.SectionNotFound;
        let raw = _.get(sec, key) ?? return IniError.KeyNotFound;
        let trimmed = trimSpaces(raw);
        return std.fmt.parseFloat(f64, trimmed);
    }

    fn getBool(sec: []const u8, key: []const u8): bool! {
        if (!_.hasSection(sec)) return IniError.SectionNotFound;
        let raw = _.get(sec, key) ?? return IniError.KeyNotFound;
        let trimmed = trimSpaces(raw);
        if (asciiEqlIgnoreCase(trimmed, "true") ||
            asciiEqlIgnoreCase(trimmed, "yes") ||
            asciiEqlIgnoreCase(trimmed, "on") ||
            std.mem.eql(u8, trimmed, "1")) {
            return true;
        }
        if (asciiEqlIgnoreCase(trimmed, "false") ||
            asciiEqlIgnoreCase(trimmed, "no") ||
            asciiEqlIgnoreCase(trimmed, "off") ||
            std.mem.eql(u8, trimmed, "0")) {
            return false;
        }
        return IniError.InvalidBool;
    }

    fn sections(alloc: std.mem.Allocator): ([][]const u8)! {
        let out: std.ArrayList(string) = .empty;
        errdefer out.deinit(alloc);
        for (line of _.lines.items) {
            if (line.kind == .SectionHeader && line.section.len > 0) {
                let exists: bool = false;
                for (prev of out.items) {
                    if (_.sectionEq(prev, line.section)) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    out.append(alloc, line.section) catch return IniError.OutOfMemory;
                }
            }
        }
        return out.toOwnedSlice(alloc) catch return IniError.OutOfMemory;
    }

    fn keys(alloc: std.mem.Allocator, sec: []const u8): ([][]const u8)! {
        let out: std.ArrayList(string) = .empty;
        errdefer out.deinit(alloc);
        for (line of _.lines.items) {
            if (line.kind == .KeyValue && _.sectionEq(line.section, sec)) {
                let exists: bool = false;
                for (prev of out.items) {
                    if (std.mem.eql(u8, prev, line.key)) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    out.append(alloc, line.key) catch return IniError.OutOfMemory;
                }
            }
        }
        return out.toOwnedSlice(alloc) catch return IniError.OutOfMemory;
    }

    fn set(*_, sec: []const u8, key: []const u8, val: []const u8): ! {
        if (idx := _.findLastKeyIndex(sec, key)) {
            let new_val = dupSlice(_.allocator, val)?;
            let line = &_.lines.items[idx];
            if (line.value.len > 0) _.allocator.free(line.value);
            line.value = new_val;
            line.dirty = true;
            return;
        }

        // Find insertion point inside existing section (after its last KeyValue or SectionHeader)
        let insert_idx: usize? = null;
        if (sec.len == 0) {
            let i: usize = 0;
            while (i < _.lines.items.len) : (i += 1) {
                if (_.lines.items[i].kind == .SectionHeader) {
                    insert_idx = i;
                    break;
                }
                if (_.lines.items[i].kind == .KeyValue) {
                    insert_idx = i + 1;
                }
            }
            if (insert_idx == null) {
                insert_idx = 0;
            }
        } else {
            let in_sec: bool = false;
            let last_sec_kv: usize? = null;
            let i: usize = 0;
            while (i < _.lines.items.len) : (i += 1) {
                let line = _.lines.items[i];
                if (line.kind == .SectionHeader) {
                    if (_.sectionEq(line.section, sec)) {
                        in_sec = true;
                        last_sec_kv = i + 1;
                    } else if (in_sec) {
                        break;
                    }
                } else if (in_sec && line.kind == .KeyValue) {
                    last_sec_kv = i + 1;
                }
            }
            insert_idx = last_sec_kv;
        }

        if (ins := insert_idx) {
            let sec_dup = dupSlice(_.allocator, sec)?;
            errdefer if (sec_dup.len > 0) _.allocator.free(sec_dup);
            let key_dup = dupSlice(_.allocator, key)?;
            errdefer if (key_dup.len > 0) _.allocator.free(key_dup);
            let val_dup = dupSlice(_.allocator, val)?;
            errdefer if (val_dup.len > 0) _.allocator.free(val_dup);

            _.lines.insert(_.allocator, ins, IniLine{
                kind: .KeyValue,
                section: sec_dup,
                key: key_dup,
                value: val_dup,
                delim: '=',
                dirty: true,
            }) catch return IniError.OutOfMemory;
            return;
        }

        // Section does not exist: append blank separator (if needed), SectionHeader, and KeyValue
        if (_.lines.items.len > 0 && _.lines.items[_.lines.items.len - 1].kind != .Blank) {
            _.lines.append(_.allocator, IniLine{
                kind: .Blank,
                dirty: false,
            }) catch return IniError.OutOfMemory;
        }

        let hdr_sec = dupSlice(_.allocator, sec)?;
        errdefer if (hdr_sec.len > 0) _.allocator.free(hdr_sec);
        _.lines.append(_.allocator, IniLine{
            kind: .SectionHeader,
            section: hdr_sec,
            dirty: true,
        }) catch return IniError.OutOfMemory;

        let kv_sec = dupSlice(_.allocator, sec)?;
        errdefer if (kv_sec.len > 0) _.allocator.free(kv_sec);
        let key_dup = dupSlice(_.allocator, key)?;
        errdefer if (key_dup.len > 0) _.allocator.free(key_dup);
        let val_dup = dupSlice(_.allocator, val)?;
        errdefer if (val_dup.len > 0) _.allocator.free(val_dup);

        _.lines.append(_.allocator, IniLine{
            kind: .KeyValue,
            section: kv_sec,
            key: key_dup,
            value: val_dup,
            delim: '=',
            dirty: true,
        }) catch return IniError.OutOfMemory;
    }

    fn removeKey(*_, sec: []const u8, key: []const u8): bool {
        let removed: bool = false;
        let i: usize = _.lines.items.len;
        while (i > 0) {
            i -= 1;
            let line = _.lines.items[i];
            if (line.kind == .KeyValue && _.sectionEq(line.section, sec) && std.mem.eql(u8, line.key, key)) {
                let old = _.lines.orderedRemove(i);
                old.deinit(_.allocator);
                removed = true;
            }
        }
        return removed;
    }

    fn removeSection(*_, sec: []const u8): bool {
        let removed: bool = false;
        let i: usize = _.lines.items.len;
        while (i > 0) {
            i -= 1;
            let line = _.lines.items[i];
            let match_hdr = (line.kind == .SectionHeader && _.sectionEq(line.section, sec));
            let match_kv = (line.kind == .KeyValue && _.sectionEq(line.section, sec));
            if (match_hdr || match_kv) {
                let old = _.lines.orderedRemove(i);
                old.deinit(_.allocator);
                removed = true;
            }
        }
        return removed;
    }

    fn stringify(): ([]u8)! {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(_.allocator);

        let i: usize = 0;
        while (i < _.lines.items.len) : (i += 1) {
            let line = _.lines.items[i];
            if (i > 0) {
                buf.append(_.allocator, '\n') catch return IniError.OutOfMemory;
            }
            if (!line.dirty) {
                buf.appendSlice(_.allocator, line.raw) catch return IniError.OutOfMemory;
                continue;
            }
            switch (line.kind) {
                .Blank => {},
                .Comment => {
                    buf.appendSlice(_.allocator, line.raw) catch return IniError.OutOfMemory;
                },
                .SectionHeader => {
                    buf.append(_.allocator, '[') catch return IniError.OutOfMemory;
                    buf.appendSlice(_.allocator, line.section) catch return IniError.OutOfMemory;
                    buf.append(_.allocator, ']') catch return IniError.OutOfMemory;
                    if (line.inline_comment.len > 0) {
                        buf.append(_.allocator, ' ') catch return IniError.OutOfMemory;
                        buf.appendSlice(_.allocator, line.inline_comment) catch return IniError.OutOfMemory;
                    }
                },
                .KeyValue => {
                    buf.appendSlice(_.allocator, line.key) catch return IniError.OutOfMemory;
                    buf.append(_.allocator, ' ') catch return IniError.OutOfMemory;
                    buf.append(_.allocator, line.delim) catch return IniError.OutOfMemory;
                    buf.append(_.allocator, ' ') catch return IniError.OutOfMemory;
                    appendIniFormattedValue(&buf, _.allocator, line.value)?;
                    if (line.inline_comment.len > 0) {
                        buf.append(_.allocator, ' ') catch return IniError.OutOfMemory;
                        buf.appendSlice(_.allocator, line.inline_comment) catch return IniError.OutOfMemory;
                    }
                },
            }
        }
        if (_.lines.items.len > 0) {
            buf.append(_.allocator, '\n') catch return IniError.OutOfMemory;
        }
        return buf.toOwnedSlice(_.allocator) catch return IniError.OutOfMemory;
    }

    fn toJson(): ([]u8)! {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(_.allocator);

        buf.append(_.allocator, '{') catch return IniError.OutOfMemory;
        let first_top: bool = true;

        // 1. Global keys (section == "")
        if (_.hasSection("")) {
            let gkeys = _.keys(_.allocator, "")?;
            defer _.allocator.free(gkeys);
            for (k of gkeys) {
                if (!first_top) {
                    buf.append(_.allocator, ',') catch return IniError.OutOfMemory;
                }
                first_top = false;
                appendJsonEscaped(&buf, _.allocator, k)?;
                buf.append(_.allocator, ':') catch return IniError.OutOfMemory;
                let vals = _.getAll(_.allocator, "", k)?;
                defer _.allocator.free(vals);
                if (vals.len > 1) {
                    buf.append(_.allocator, '[') catch return IniError.OutOfMemory;
                    let vi: usize = 0;
                    while (vi < vals.len) : (vi += 1) {
                        if (vi > 0) buf.append(_.allocator, ',') catch return IniError.OutOfMemory;
                        appendJsonEscaped(&buf, _.allocator, vals[vi])?;
                    }
                    buf.append(_.allocator, ']') catch return IniError.OutOfMemory;
                } else if (vals.len == 1) {
                    appendJsonEscaped(&buf, _.allocator, vals[0])?;
                } else {
                    buf.appendSlice(_.allocator, "null") catch return IniError.OutOfMemory;
                }
            }
        }

        // 2. Named sections
        let secs = _.sections(_.allocator)?;
        defer _.allocator.free(secs);
        for (sec of secs) {
            if (!first_top) {
                buf.append(_.allocator, ',') catch return IniError.OutOfMemory;
            }
            first_top = false;
            appendJsonEscaped(&buf, _.allocator, sec)?;
            buf.appendSlice(_.allocator, ":{") catch return IniError.OutOfMemory;

            let skeys = _.keys(_.allocator, sec)?;
            defer _.allocator.free(skeys);
            let first_key: bool = true;
            for (k of skeys) {
                if (!first_key) {
                    buf.append(_.allocator, ',') catch return IniError.OutOfMemory;
                }
                first_key = false;
                appendJsonEscaped(&buf, _.allocator, k)?;
                buf.append(_.allocator, ':') catch return IniError.OutOfMemory;

                let vals = _.getAll(_.allocator, sec, k)?;
                defer _.allocator.free(vals);
                if (vals.len > 1) {
                    buf.append(_.allocator, '[') catch return IniError.OutOfMemory;
                    let vi: usize = 0;
                    while (vi < vals.len) : (vi += 1) {
                        if (vi > 0) buf.append(_.allocator, ',') catch return IniError.OutOfMemory;
                        appendJsonEscaped(&buf, _.allocator, vals[vi])?;
                    }
                    buf.append(_.allocator, ']') catch return IniError.OutOfMemory;
                } else if (vals.len == 1) {
                    appendJsonEscaped(&buf, _.allocator, vals[0])?;
                } else {
                    buf.appendSlice(_.allocator, "null") catch return IniError.OutOfMemory;
                }
            }
            buf.append(_.allocator, '}') catch return IniError.OutOfMemory;
        }

        buf.append(_.allocator, '}') catch return IniError.OutOfMemory;
        return buf.toOwnedSlice(_.allocator) catch return IniError.OutOfMemory;
    }
}
