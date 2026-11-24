// Source: Absorbed from /data/dev/bun/src/logger/logger.cp | Synced: 2026-11-23T22:00:12Z
// lib/logger/logger.cp
// Structured Compiler & Runtime Diagnostic Logger (cap:logger)
// Absorbed from Bun src/logger/logger.cp
// Supports LogLevel filtering, max_errors circuit breaker, O(log N) LineIndex cache,
// multi-note DiagNote with source pointers, multi-line Range context window,
// Plain / ANSI / JSON diagnostic array output, addErrorFmt, and Log.merge.

const std = @import("std");

fn appendBytes(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        out_buf[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendUsize(out_buf: []u8, pos: usize, val: usize): usize {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{val}) catch "";
    return appendBytes(out_buf, pos, s);
}

fn appendI32(out_buf: []u8, pos: usize, val: i32): usize {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{val}) catch "";
    return appendBytes(out_buf, pos, s);
}

fn appendRepeat(out_buf: []u8, pos: usize, ch: u8, count: usize): usize {
    let p = pos;
    let i: usize = 0;
    while (i < count && p < out_buf.len) {
        out_buf[p] = ch;
        p += 1;
        i += 1;
    }
    return p;
}

fn appendJsonString(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = appendBytes(out_buf, pos, "\"");
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        const c = s[i];
        if (c == '"') {
            p = appendBytes(out_buf, p, "\\\"");
        } else if (c == '\\') {
            p = appendBytes(out_buf, p, "\\\\");
        } else if (c == '\n') {
            p = appendBytes(out_buf, p, "\\n");
        } else if (c == '\r') {
            p = appendBytes(out_buf, p, "\\r");
        } else if (c == '\t') {
            p = appendBytes(out_buf, p, "\\t");
        } else if (c < 0x20) {
            let tmp: [8]u8 = [0; 8];
            const esc = std.fmt.bufPrint(tmp[0..8], "\\u{x:0>4}", .{c}) catch "";
            p = appendBytes(out_buf, p, esc);
        } else {
            p = appendBytes(out_buf, p, s[i .. i + 1]);
        }
        i += 1;
    }
    return appendBytes(out_buf, p, "\"");
}

fn countDigits(val: usize): usize {
    if (val < 10) return 1;
    let n = val;
    let d: usize = 0;
    while (n > 0) {
        n = n / 10;
        d += 1;
    }
    return d;
}

pub enum LogLevel: u8 {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Err = 3,
    Verbose = 4,

    pub fn rank(): u8 {
        return switch (_) {
            .Verbose => 0,
            .Debug => 1,
            .Info => 2,
            .Warn => 3,
            .Err => 4,
        };
    }

    pub fn label(): []const u8 {
        return switch (_) {
            .Debug => "debug",
            .Info => "info",
            .Warn => "warn",
            .Err => "error",
            .Verbose => "verbose",
        };
    }

    pub fn ansiColor(): []const u8 {
        return switch (_) {
            .Debug => "\x1b[36m",
            .Info => "\x1b[34m",
            .Warn => "\x1b[1;33m",
            .Err => "\x1b[1;31m",
            .Verbose => "\x1b[90m",
        };
    }

    pub fn atLeast(other: LogLevel): bool {
        return _.rank() >= other.rank();
    }

    /// Determines whether a message of this level should be emitted under `min_level`.
    pub fn shouldEmit(min_level: LogLevel): bool {
        return _.rank() >= min_level.rank();
    }

    pub static fn fromString(level_name: []const u8): LogLevel? {
        if (std.mem.eql(u8, level_name, "verbose")) return LogLevel.Verbose;
        if (std.mem.eql(u8, level_name, "debug")) return LogLevel.Debug;
        if (std.mem.eql(u8, level_name, "info")) return LogLevel.Info;
        if (std.mem.eql(u8, level_name, "warn")) return LogLevel.Warn;
        if (std.mem.eql(u8, level_name, "error") || std.mem.eql(u8, level_name, "err")) return LogLevel.Err;
        return null;
    }
}

/// Zero-overhead 4-byte source byte-offset location (`-1` denotes empty / no location).
pub struct Loc {
    pub start: i32 = -1,

    pub static fn init(start: i32): Loc {
        return Loc{ start: start };
    }

    pub static fn empty(): Loc {
        return Loc{ start: -1 };
    }

    pub fn isEmpty(): bool {
        return _.start < 0;
    }

    pub fn toUsize(): usize {
        if (_.start <= 0) {
            return 0;
        }
        return _.start as usize;
    }

    pub fn eql(other: Loc): bool {
        return _.start == other.start;
    }
}

/// Byte span inside a source buffer (`loc.start .. loc.start + len`).
pub struct Range {
    pub loc: Loc = Loc{},
    pub len: i32 = 0,

    pub static fn init(start: i32, len: i32): Range {
        return Range{
            loc: Loc.init(start),
            len: if (len < 0) 0 else len,
        };
    }

    pub static fn empty(): Range {
        return Range{ loc: Loc.empty(), len: 0 };
    }

    pub fn isEmpty(): bool {
        return _.loc.isEmpty() || _.len <= 0;
    }

    pub fn contains(offset: i32): bool {
        return !_.loc.isEmpty() && offset >= _.loc.start && offset < _.loc.start + _.len;
    }

    pub fn end(): i32 {
        if (_.loc.isEmpty()) {
            return -1;
        }
        return _.loc.start + _.len;
    }

    pub fn sliceOf(contents: []const u8): []const u8 {
        if (_.isEmpty()) {
            return "";
        }
        const s = _.loc.toUsize();
        if (s >= contents.len) {
            return "";
        }
        const e_raw = s + (_.len as usize);
        const e = if (e_raw > contents.len) contents.len else e_raw;
        return contents[s..e];
    }
}

/// 1-based line and column coordinate along with the enclosing line byte boundaries.
pub struct LineCol {
    pub line: usize = 1,
    pub col: usize = 1,
    pub line_start: usize = 0,
    pub line_end: usize = 0,
}

pub const LINE_INDEX_MAX_LINES: usize = 2048;

/// High-performance precomputed line-start byte offset cache for O(log N) lookup.
pub struct LineIndex {
    pub starts: [LINE_INDEX_MAX_LINES]usize = undefined,
    pub count: usize = 0,
    pub truncated: bool = false,

    pub static fn build(contents: []const u8): LineIndex {
        let idx = LineIndex{
            starts: undefined,
            count: 1,
            truncated: false,
        };
        idx.starts[0] = 0;
        let i: usize = 0;
        while (i < contents.len) {
            if (contents[i] == '\n') {
                if (idx.count < LINE_INDEX_MAX_LINES) {
                    idx.starts[idx.count] = i + 1;
                    idx.count += 1;
                } else {
                    idx.truncated = true;
                    break;
                }
            }
            i += 1;
        }
        return idx;
    }

    pub fn lineCount(): usize {
        return _.count;
    }

    pub fn lineStart(line_1based: usize): usize {
        if (line_1based <= 1 || _.count == 0) {
            return 0;
        }
        const idx = if (line_1based - 1 < _.count) (line_1based - 1) else (_.count - 1);
        return _.starts[idx];
    }

    pub fn lineEnd(contents: []const u8, line_1based: usize): usize {
        const start = _.lineStart(line_1based);
        let end = start;
        if (line_1based < _.count) {
            end = _.starts[line_1based];
            if (end > start && contents[end - 1] == '\n') {
                end -= 1;
            }
            if (end > start && contents[end - 1] == '\r') {
                end -= 1;
            }
            return end;
        }
        while (end < contents.len && contents[end] != '\n' && contents[end] != '\r') {
            end += 1;
        }
        return end;
    }

    pub fn lineText(contents: []const u8, line_1based: usize): []const u8 {
        const s = _.lineStart(line_1based);
        const e = _.lineEnd(contents, line_1based);
        if (s > e || e > contents.len) {
            return "";
        }
        return contents[s..e];
    }

    /// Binary search O(log N) lookup for 1-based line and column of `offset`.
    pub fn lineColOf(contents: []const u8, offset: usize): LineCol {
        const clamped = if (offset > contents.len) contents.len else offset;
        if (_.count == 0) {
            return LineCol{};
        }
        // If truncated beyond indexed region, scan from last indexed start
        if (_.truncated && clamped >= _.starts[_.count - 1]) {
            let line = _.count;
            let line_start = _.starts[_.count - 1];
            let i = line_start;
            while (i < clamped) {
                if (contents[i] == '\n') {
                    line += 1;
                    line_start = i + 1;
                }
                i += 1;
            }
            let le = line_start;
            while (le < contents.len && contents[le] != '\n' && contents[le] != '\r') {
                le += 1;
            }
            return LineCol{
                line: line,
                col: (clamped - line_start) + 1,
                line_start: line_start,
                line_end: le,
            };
        }

        let lo: usize = 0;
        let hi: usize = _.count;
        while (lo + 1 < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.starts[mid] <= clamped) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        const line_num = lo + 1;
        const line_start = _.starts[lo];
        const le = _.lineEnd(contents, line_num);
        return LineCol{
            line: line_num,
            col: (clamped - line_start) + 1,
            line_start: line_start,
            line_end: le,
        };
    }
}

/// Immutable source file view with optional O(log N) line index cache.
pub struct Source {
    pub path: []const u8 = "",
    pub contents: []const u8 = "",
    pub has_index: bool = false,
    pub cached_index: LineIndex = LineIndex{ starts: undefined, count: 0, truncated: false },

    pub static fn init(path: []const u8, contents: []const u8): Source {
        return Source{
            path: path,
            contents: contents,
            has_index: false,
            cached_index: LineIndex{ starts: undefined, count: 0, truncated: false },
        };
    }

    pub static fn initIndexed(path: []const u8, contents: []const u8): Source {
        return Source{
            path: path,
            contents: contents,
            has_index: true,
            cached_index: LineIndex.build(contents),
        };
    }

    pub fn buildLineIndex(*_) {
        if (!_.has_index) {
            _.cached_index = LineIndex.build(_.contents);
            _.has_index = true;
        }
    }

    /// Computes 1-based line and column number for byte `offset` in `contents`.
    pub fn lineColOf(offset: usize): LineCol {
        if (_.has_index) {
            return _.cached_index.lineColOf(_.contents, offset);
        }
        const clamped = if (offset > _.contents.len) _.contents.len else offset;
        let line: usize = 1;
        let line_start: usize = 0;
        let i: usize = 0;

        while (i < clamped) {
            if (_.contents[i] == '\n') {
                line += 1;
                line_start = i + 1;
            }
            i += 1;
        }

        let line_end = line_start;
        while (line_end < _.contents.len && _.contents[line_end] != '\n' && _.contents[line_end] != '\r') {
            line_end += 1;
        }

        return LineCol{
            line: line,
            col: (clamped - line_start) + 1,
            line_start: line_start,
            line_end: line_end,
        };
    }

    /// Returns the single line slice containing byte `offset`.
    pub fn lineTextOf(offset: usize): []const u8 {
        const lc = _.lineColOf(offset);
        if (lc.line_start > lc.line_end || lc.line_end > _.contents.len) {
            return "";
        }
        return _.contents[lc.line_start..lc.line_end];
    }

    /// Returns the 1-based line text slice.
    pub fn lineTextByNumber(line_1based: usize): []const u8 {
        if (line_1based == 0) {
            return "";
        }
        if (_.has_index) {
            return _.cached_index.lineText(_.contents, line_1based);
        }
        let cur_line: usize = 1;
        let line_start: usize = 0;
        let i: usize = 0;
        while (i < _.contents.len && cur_line < line_1based) {
            if (_.contents[i] == '\n') {
                cur_line += 1;
                line_start = i + 1;
            }
            i += 1;
        }
        if (cur_line != line_1based || line_start > _.contents.len) {
            return "";
        }
        let line_end = line_start;
        while (line_end < _.contents.len && _.contents[line_end] != '\n' && _.contents[line_end] != '\r') {
            line_end += 1;
        }
        return _.contents[line_start..line_end];
    }
}

pub const MAX_DIAG_NOTES: usize = 8;

/// Secondary diagnostic annotation, optionally pointing to a separate source location.
pub struct DiagNote {
    pub text: []const u8 = "",
    pub file: []const u8 = "",
    pub loc: Loc = Loc{},
    pub range_len: i32 = 0,
    pub line: usize = 0,
    pub col: usize = 0,
    pub line_text: []const u8 = "",

    pub static fn textOnly(text: []const u8): DiagNote {
        return DiagNote{ text: text };
    }

    pub static fn fromSource(src: Source?, range: Range, text: []const u8): DiagNote {
        if (s := src) {
            if (range.loc.isEmpty()) {
                return DiagNote{ text: text, file: s.path };
            }
            const off = range.loc.toUsize();
            const lc = s.lineColOf(off);
            const snippet = if (lc.line_end >= lc.line_start && lc.line_end <= s.contents.len)
                s.contents[lc.line_start..lc.line_end]
            else
                "";
            return DiagNote{
                text: text,
                file: s.path,
                loc: range.loc,
                range_len: range.len,
                line: lc.line,
                col: lc.col,
                line_text: snippet,
            };
        }
        return DiagNote{ text: text };
    }
}

/// Structured diagnostic message with optional source location, context window, multi-notes, and JSON rendering.
pub struct DiagMsg {
    pub level: LogLevel = LogLevel.Info,
    pub text: []const u8 = "",
    pub file: []const u8 = "",
    pub loc: Loc = Loc{},
    pub range_len: i32 = 0,
    pub line: usize = 0,
    pub col: usize = 0,
    pub line_text: []const u8 = "",
    pub note: []const u8 = "",
    pub code: []const u8 = "",
    pub prev_line_text: []const u8 = "",
    pub has_prev_line: bool = false,
    pub notes: [MAX_DIAG_NOTES]DiagNote = undefined,
    pub notes_len: usize = 0,

    pub static fn fromSource(
        level: LogLevel,
        src: Source,
        range: Range,
        text: []const u8,
        note: []const u8,
    ): DiagMsg {
        if (range.loc.isEmpty()) {
            return DiagMsg{
                level: level,
                text: text,
                file: src.path,
                loc: range.loc,
                range_len: 0,
                line: 0,
                col: 0,
                line_text: "",
                note: note,
                code: "",
                prev_line_text: "",
                has_prev_line: false,
                notes: undefined,
                notes_len: 0,
            };
        }
        const off = range.loc.toUsize();
        const lc = src.lineColOf(off);
        const snippet = if (lc.line_end >= lc.line_start && lc.line_end <= src.contents.len)
            src.contents[lc.line_start..lc.line_end]
        else
            "";
        let prev_txt: []const u8 = "";
        let has_prev = false;
        if (lc.line > 1) {
            prev_txt = src.lineTextByNumber(lc.line - 1);
            has_prev = true;
        }
        return DiagMsg{
            level: level,
            text: text,
            file: src.path,
            loc: range.loc,
            range_len: range.len,
            line: lc.line,
            col: lc.col,
            line_text: snippet,
            note: note,
            code: "",
            prev_line_text: prev_txt,
            has_prev_line: has_prev,
            notes: undefined,
            notes_len: 0,
        };
    }

    pub static fn fromSourceWithNotes(
        level: LogLevel,
        src: Source,
        range: Range,
        text: []const u8,
        note_list: []const DiagNote,
    ): DiagMsg {
        let msg = DiagMsg.fromSource(level, src, range, text, "");
        let i: usize = 0;
        while (i < note_list.len && i < MAX_DIAG_NOTES) {
            * = msg.addNote(note_list[i]);
            i += 1;
        }
        return msg;
    }

    pub fn addNote(*_, n: DiagNote): bool {
        if (_.notes_len >= MAX_DIAG_NOTES) {
            return false;
        }
        _.notes[_.notes_len] = n;
        _.notes_len += 1;
        return true;
    }

    pub fn addNoteFromSource(*_, src: Source, range: Range, text: []const u8): bool {
        return _.addNote(DiagNote.fromSource(src, range, text));
    }

    /// Formats the diagnostic as plain text.
    pub fn formatPlain(out_buf: []u8): []const u8 {
        return _.formatInternal(out_buf, false);
    }

    /// Formats the diagnostic with ANSI terminal highlight codes.
    pub fn formatAnsi(out_buf: []u8): []const u8 {
        return _.formatInternal(out_buf, true);
    }

    fn renderUnderlineLen(): usize {
        if (_.range_len <= 1) {
            return 1;
        }
        const raw = _.range_len as usize;
        if (_.line_text.len > 0 && _.col > 0) {
            const avail = if (_.col - 1 < _.line_text.len) (_.line_text.len - (_.col - 1)) else 1;
            if (raw > avail && avail > 0) {
                return avail;
            }
        }
        return raw;
    }

    static fn formatNoteBlock(out_buf: []u8, start_pos: usize, n: DiagNote, use_ansi: bool): usize {
        let pos = start_pos;
        pos = appendBytes(out_buf, pos, "   = ");
        if (use_ansi) {
            pos = appendBytes(out_buf, pos, "\x1b[36m");
        }
        pos = appendBytes(out_buf, pos, "note");
        if (use_ansi) {
            pos = appendBytes(out_buf, pos, "\x1b[0m");
        }
        pos = appendBytes(out_buf, pos, ": ");
        pos = appendBytes(out_buf, pos, n.text);
        pos = appendBytes(out_buf, pos, "\n");

        if (n.file.len > 0 && n.line > 0) {
            pos = appendBytes(out_buf, pos, "  --> ");
            pos = appendBytes(out_buf, pos, n.file);
            pos = appendBytes(out_buf, pos, ":");
            pos = appendUsize(out_buf, pos, n.line);
            pos = appendBytes(out_buf, pos, ":");
            pos = appendUsize(out_buf, pos, n.col);
            pos = appendBytes(out_buf, pos, "\n");

            if (n.line_text.len > 0) {
                const gw = countDigits(n.line);
                const gutter_pad = if (gw < 2) 2 else gw;
                const ldigits = countDigits(n.line);
                pos = appendRepeat(out_buf, pos, ' ', gutter_pad - ldigits);
                pos = appendUsize(out_buf, pos, n.line);
                pos = appendBytes(out_buf, pos, " | ");
                pos = appendBytes(out_buf, pos, n.line_text);
                pos = appendBytes(out_buf, pos, "\n");

                pos = appendRepeat(out_buf, pos, ' ', gutter_pad);
                pos = appendBytes(out_buf, pos, " | ");
                const pad: usize = if (n.col > 1) (n.col - 1) else 0;
                pos = appendRepeat(out_buf, pos, ' ', pad);
                if (use_ansi) {
                    pos = appendBytes(out_buf, pos, "\x1b[36m");
                }
                pos = appendBytes(out_buf, pos, "^");
                if (n.range_len > 1) {
                    pos = appendRepeat(out_buf, pos, '~', (n.range_len - 1) as usize);
                }
                if (use_ansi) {
                    pos = appendBytes(out_buf, pos, "\x1b[0m");
                }
                pos = appendBytes(out_buf, pos, "\n");
            }
        }
        return pos;
    }

    fn formatInternal(out_buf: []u8, use_ansi: bool): []const u8 {
        let pos: usize = 0;

        if (use_ansi) {
            pos = appendBytes(out_buf, pos, _.level.ansiColor());
        }
        pos = appendBytes(out_buf, pos, _.level.label());
        if (_.code.len > 0) {
            pos = appendBytes(out_buf, pos, "[");
            pos = appendBytes(out_buf, pos, _.code);
            pos = appendBytes(out_buf, pos, "]");
        }
        if (use_ansi) {
            pos = appendBytes(out_buf, pos, "\x1b[0m");
        }
        pos = appendBytes(out_buf, pos, ": ");
        pos = appendBytes(out_buf, pos, _.text);
        pos = appendBytes(out_buf, pos, "\n");

        if (_.file.len > 0 && _.line > 0) {
            pos = appendBytes(out_buf, pos, "  --> ");
            pos = appendBytes(out_buf, pos, _.file);
            pos = appendBytes(out_buf, pos, ":");
            pos = appendUsize(out_buf, pos, _.line);
            pos = appendBytes(out_buf, pos, ":");
            pos = appendUsize(out_buf, pos, _.col);
            pos = appendBytes(out_buf, pos, "\n");

            if (_.line_text.len > 0) {
                const ldigits = countDigits(_.line);
                const gutter_w: usize = if (ldigits < 2) 2 else ldigits;

                // Optional previous context line (1-line before window)
                if (_.has_prev_line && _.line > 1 && _.prev_line_text.len > 0) {
                    const prev_digits = countDigits(_.line - 1);
                    if (gutter_w > prev_digits) {
                        pos = appendRepeat(out_buf, pos, ' ', gutter_w - prev_digits);
                    }
                    pos = appendUsize(out_buf, pos, _.line - 1);
                    pos = appendBytes(out_buf, pos, " | ");
                    pos = appendBytes(out_buf, pos, _.prev_line_text);
                    pos = appendBytes(out_buf, pos, "\n");
                }

                if (gutter_w > ldigits) {
                    pos = appendRepeat(out_buf, pos, ' ', gutter_w - ldigits);
                }
                pos = appendUsize(out_buf, pos, _.line);
                pos = appendBytes(out_buf, pos, " | ");
                pos = appendBytes(out_buf, pos, _.line_text);
                pos = appendBytes(out_buf, pos, "\n");

                pos = appendRepeat(out_buf, pos, ' ', gutter_w);
                pos = appendBytes(out_buf, pos, " | ");
                const pad: usize = if (_.col > 1) (_.col - 1) else 0;
                pos = appendRepeat(out_buf, pos, ' ', pad);
                if (use_ansi) {
                    pos = appendBytes(out_buf, pos, _.level.ansiColor());
                }
                pos = appendBytes(out_buf, pos, "^");
                const ulen = _.renderUnderlineLen();
                if (ulen > 1) {
                    pos = appendRepeat(out_buf, pos, '~', ulen - 1);
                }
                if (use_ansi) {
                    pos = appendBytes(out_buf, pos, "\x1b[0m");
                }
                pos = appendBytes(out_buf, pos, "\n");
            }
        }

        if (_.note.len > 0) {
            pos = appendBytes(out_buf, pos, "   = note: ");
            pos = appendBytes(out_buf, pos, _.note);
            pos = appendBytes(out_buf, pos, "\n");
        }

        let ni: usize = 0;
        while (ni < _.notes_len) {
            pos = DiagMsg.formatNoteBlock(out_buf, pos, _.notes[ni], use_ansi);
            ni += 1;
        }

        return out_buf[0..pos];
    }

    /// Formats a single diagnostic as a machine-readable JSON object.
    pub fn formatJson(out_buf: []u8): []const u8 {
        let pos: usize = 0;
        pos = appendBytes(out_buf, pos, "{\"level\":");
        pos = appendJsonString(out_buf, pos, _.level.label());
        pos = appendBytes(out_buf, pos, ",\"message\":");
        pos = appendJsonString(out_buf, pos, _.text);
        if (_.code.len > 0) {
            pos = appendBytes(out_buf, pos, ",\"code\":");
            pos = appendJsonString(out_buf, pos, _.code);
        }
        if (_.file.len > 0 || _.line > 0) {
            pos = appendBytes(out_buf, pos, ",\"location\":{\"file\":");
            pos = appendJsonString(out_buf, pos, _.file);
            pos = appendBytes(out_buf, pos, ",\"line\":");
            pos = appendUsize(out_buf, pos, _.line);
            pos = appendBytes(out_buf, pos, ",\"column\":");
            pos = appendUsize(out_buf, pos, _.col);
            pos = appendBytes(out_buf, pos, ",\"offset\":");
            pos = appendI32(out_buf, pos, _.loc.start);
            pos = appendBytes(out_buf, pos, ",\"length\":");
            pos = appendI32(out_buf, pos, _.range_len);
            pos = appendBytes(out_buf, pos, ",\"line_text\":");
            pos = appendJsonString(out_buf, pos, _.line_text);
            pos = appendBytes(out_buf, pos, "}");
        }
        pos = appendBytes(out_buf, pos, ",\"notes\":[");
        let wrote_note = false;
        if (_.note.len > 0) {
            pos = appendBytes(out_buf, pos, "{\"message\":");
            pos = appendJsonString(out_buf, pos, _.note);
            pos = appendBytes(out_buf, pos, "}");
            wrote_note = true;
        }
        let ni: usize = 0;
        while (ni < _.notes_len) {
            if (wrote_note) {
                pos = appendBytes(out_buf, pos, ",");
            }
            const n = _.notes[ni];
            pos = appendBytes(out_buf, pos, "{\"message\":");
            pos = appendJsonString(out_buf, pos, n.text);
            if (n.file.len > 0 && n.line > 0) {
                pos = appendBytes(out_buf, pos, ",\"location\":{\"file\":");
                pos = appendJsonString(out_buf, pos, n.file);
                pos = appendBytes(out_buf, pos, ",\"line\":");
                pos = appendUsize(out_buf, pos, n.line);
                pos = appendBytes(out_buf, pos, ",\"column\":");
                pos = appendUsize(out_buf, pos, n.col);
                pos = appendBytes(out_buf, pos, "}");
            }
            pos = appendBytes(out_buf, pos, "}");
            wrote_note = true;
            ni += 1;
        }
        pos = appendBytes(out_buf, pos, "]}");
        return out_buf[0..pos];
    }
}

pub const LOG_MAX_MSGS: usize = 64;

/// Fixed-capacity zero-heap diagnostic log accumulator tracking errors, warnings, circuit breaker, and multi-format rendering.
pub struct Log {
    pub level: LogLevel = LogLevel.Info,
    pub errors: usize = 0,
    pub warnings: usize = 0,
    pub msgs: [LOG_MAX_MSGS]DiagMsg = undefined,
    pub count: usize = 0,
    pub max_errors: usize = 0,
    pub suppressed_errors: usize = 0,
    pub limit_reached: bool = false,

    pub static fn init(level: LogLevel): Log {
        return Log{
            level: level,
            errors: 0,
            warnings: 0,
            count: 0,
            max_errors: 0,
            suppressed_errors: 0,
            limit_reached: false,
        };
    }

    pub static fn initWithMaxErrors(level: LogLevel, max_errors: usize): Log {
        return Log{
            level: level,
            errors: 0,
            warnings: 0,
            count: 0,
            max_errors: max_errors,
            suppressed_errors: 0,
            limit_reached: false,
        };
    }

    pub fn setMaxErrors(*_, max_errors: usize) {
        _.max_errors = max_errors;
    }

    pub fn isErrorLimitReached(): bool {
        return _.limit_reached;
    }

    pub fn addMsg(*_, msg: DiagMsg) {
        if (msg.level == LogLevel.Err) {
            if (_.max_errors > 0 && _.errors >= _.max_errors) {
                _.limit_reached = true;
                _.suppressed_errors += 1;
                return;
            }
            _.errors += 1;
            if (_.max_errors > 0 && _.errors >= _.max_errors) {
                _.limit_reached = true;
            }
        } else if (msg.level == LogLevel.Warn) {
            _.warnings += 1;
        }
        if (!msg.level.shouldEmit(_.level)) {
            return;
        }
        if (_.count < LOG_MAX_MSGS) {
            _.msgs[_.count] = msg;
            _.count += 1;
        }
    }

    pub fn addError(*_, src: Source?, loc: Loc, text: []const u8) {
        if (s := src) {
            _.addMsg(DiagMsg.fromSource(LogLevel.Err, s, Range{ loc: loc, len: 1 }, text, ""));
        } else {
            _.addMsg(DiagMsg{ level: LogLevel.Err, text: text, loc: loc });
        }
    }

    pub fn addErrorRange(*_, src: Source, range: Range, text: []const u8, note: []const u8) {
        _.addMsg(DiagMsg.fromSource(LogLevel.Err, src, range, text, note));
    }

    pub fn addErrorWithNotes(*_, src: Source, range: Range, text: []const u8, notes: []const DiagNote) {
        _.addMsg(DiagMsg.fromSourceWithNotes(LogLevel.Err, src, range, text, notes));
    }

    pub fn addErrorFmt(*_, src: Source?, loc: Loc, buf: []u8, comptime fmt: []const u8, args: auto) {
        const text = std.fmt.bufPrint(buf, fmt, args) catch fmt;
        _.addError(src, loc, text);
    }

    pub fn addErrorRangeFmt(*_, src: Source, range: Range, buf: []u8, comptime fmt: []const u8, args: auto, note: []const u8) {
        const text = std.fmt.bufPrint(buf, fmt, args) catch fmt;
        _.addErrorRange(src, range, text, note);
    }

    pub fn addWarning(*_, src: Source?, loc: Loc, text: []const u8) {
        if (s := src) {
            _.addMsg(DiagMsg.fromSource(LogLevel.Warn, s, Range{ loc: loc, len: 1 }, text, ""));
        } else {
            _.addMsg(DiagMsg{ level: LogLevel.Warn, text: text, loc: loc });
        }
    }

    pub fn addWarningRange(*_, src: Source, range: Range, text: []const u8, note: []const u8) {
        _.addMsg(DiagMsg.fromSource(LogLevel.Warn, src, range, text, note));
    }

    pub fn addWarningFmt(*_, src: Source?, loc: Loc, buf: []u8, comptime fmt: []const u8, args: auto) {
        const text = std.fmt.bufPrint(buf, fmt, args) catch fmt;
        _.addWarning(src, loc, text);
    }

    pub fn addInfo(*_, text: []const u8) {
        _.addMsg(DiagMsg{ level: LogLevel.Info, text: text });
    }

    pub fn addDebug(*_, text: []const u8) {
        _.addMsg(DiagMsg{ level: LogLevel.Debug, text: text });
    }

    pub fn addVerbose(*_, text: []const u8) {
        _.addMsg(DiagMsg{ level: LogLevel.Verbose, text: text });
    }

    /// Merges all messages from `other` into this `Log`, respecting level filtering and `max_errors`.
    pub fn merge(*_, other: *const Log): usize {
        let merged: usize = 0;
        let i: usize = 0;
        while (i < other.count) {
            _.addMsg(other.msgs[i]);
            merged += 1;
            i += 1;
        }
        _.suppressed_errors += other.suppressed_errors;
        return merged;
    }

    pub fn hasErrors(): bool {
        return _.errors > 0;
    }

    pub fn clear(*_) {
        _.errors = 0;
        _.warnings = 0;
        _.count = 0;
        _.suppressed_errors = 0;
        _.limit_reached = false;
    }

    /// Formats all recorded diagnostic messages into `out_buf` (plain text or ANSI colored).
    pub fn formatAll(out_buf: []u8, use_ansi: bool): []const u8 {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.count && pos < out_buf.len) {
            const chunk = if (use_ansi)
                _.msgs[i].formatAnsi(out_buf[pos..])
            else
                _.msgs[i].formatPlain(out_buf[pos..]);
            pos += chunk.len;
            i += 1;
        }
        return out_buf[0..pos];
    }

    /// Formats all recorded diagnostic messages as a machine-readable JSON array.
    pub fn formatJson(out_buf: []u8): []const u8 {
        let pos: usize = 0;
        pos = appendBytes(out_buf, pos, "[");
        let i: usize = 0;
        while (i < _.count && pos < out_buf.len) {
            if (i > 0) {
                pos = appendBytes(out_buf, pos, ",");
            }
            const chunk = _.msgs[i].formatJson(out_buf[pos..]);
            pos += chunk.len;
            i += 1;
        }
        pos = appendBytes(out_buf, pos, "]");
        return out_buf[0..pos];
    }
}
