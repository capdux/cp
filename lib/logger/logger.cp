// Source: /data/dev/bun/src/logger/logger.cp
// lib/logger/logger.cp
// Structured Compiler & Runtime Diagnostic Logger (cap:logger)
// Absorbed from Bun src/logger/logger.cp

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

pub enum LogLevel: u8 {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Err = 3,
    Verbose = 4,

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

    /// Determines whether a message of this level should be emitted under `min_level`.
    pub fn shouldEmit(min_level: LogLevel): bool {
        return switch (min_level) {
            .Err => _ == LogLevel.Err,
            .Warn => _ == LogLevel.Err || _ == LogLevel.Warn,
            .Info => _ == LogLevel.Err || _ == LogLevel.Warn || _ == LogLevel.Info,
            .Debug => _ != LogLevel.Verbose,
            .Verbose => true,
        };
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

/// Immutable source file view with line/column coordinate lookup.
pub struct Source {
    pub path: []const u8 = "",
    pub contents: []const u8 = "",

    pub static fn init(path: []const u8, contents: []const u8): Source {
        return Source{
            path: path,
            contents: contents,
        };
    }

    /// Computes 1-based line and column number for byte `offset` in `contents`.
    pub fn lineColOf(offset: usize): LineCol {
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
}

/// Structured diagnostic message with optional source location, snippet, and note.
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
            };
        }
        const off = range.loc.toUsize();
        const lc = src.lineColOf(off);
        const snippet = if (lc.line_end >= lc.line_start && lc.line_end <= src.contents.len)
            src.contents[lc.line_start..lc.line_end]
        else
            "";
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
        };
    }

    /// Formats the diagnostic as plain text (`error: msg\n  --> file:line:col\n  | code\n  |   ^~~\n  = note: ...`).
    pub fn formatPlain(out_buf: []u8): []const u8 {
        return _.formatInternal(out_buf, false);
    }

    /// Formats the diagnostic with ANSI terminal highlight codes.
    pub fn formatAnsi(out_buf: []u8): []const u8 {
        return _.formatInternal(out_buf, true);
    }

    fn formatInternal(out_buf: []u8, use_ansi: bool): []const u8 {
        let pos: usize = 0;

        if (use_ansi) {
            pos = appendBytes(out_buf, pos, _.level.ansiColor());
        }
        pos = appendBytes(out_buf, pos, _.level.label());
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
                pos = appendBytes(out_buf, pos, "   | ");
                pos = appendBytes(out_buf, pos, _.line_text);
                pos = appendBytes(out_buf, pos, "\n");

                pos = appendBytes(out_buf, pos, "   | ");
                const pad: usize = if (_.col > 1) (_.col - 1) else 0;
                pos = appendRepeat(out_buf, pos, ' ', pad);
                if (use_ansi) {
                    pos = appendBytes(out_buf, pos, _.level.ansiColor());
                }
                pos = appendBytes(out_buf, pos, "^");
                if (_.range_len > 1) {
                    pos = appendRepeat(out_buf, pos, '~', (_.range_len - 1) as usize);
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

        return out_buf[0..pos];
    }
}

pub const LOG_MAX_MSGS: usize = 64;

/// Fixed-capacity zero-heap diagnostic log accumulator tracking errors, warnings, and structured messages.
pub struct Log {
    pub level: LogLevel = LogLevel.Info,
    pub errors: usize = 0,
    pub warnings: usize = 0,
    pub msgs: [64]DiagMsg = undefined,
    pub count: usize = 0,

    pub static fn init(level: LogLevel): Log {
        return Log{
            level: level,
            errors: 0,
            warnings: 0,
            count: 0,
        };
    }

    pub fn addMsg(*_, msg: DiagMsg) {
        if (msg.level == LogLevel.Err) {
            _.errors += 1;
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

    pub fn addInfo(*_, text: []const u8) {
        _.addMsg(DiagMsg{ level: LogLevel.Info, text: text });
    }

    pub fn addDebug(*_, text: []const u8) {
        _.addMsg(DiagMsg{ level: LogLevel.Debug, text: text });
    }

    pub fn hasErrors(): bool {
        return _.errors > 0;
    }

    pub fn clear(*_) {
        _.errors = 0;
        _.warnings = 0;
        _.count = 0;
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
}
