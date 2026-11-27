// @cap/pretty-format — Pure CP zero-allocation structured value pretty-printer.
// Clean-room MIT implementation inspired by npm `pretty-format`.

import { strEq } from "cap:ascii";
import { writeStr, writeU64, writeI64 } from "cap:fmt";

pub const PRETTY_BUF_CAP: usize = 2048;
pub const PRETTY_MAX_NEST: usize = 32;

fn pfmtWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn pfmtWriteSpaces(out: []u8, pos: usize, count: usize): usize {
    if (pos >= out.len || count == 0) {
        return pos;
    }
    let avail = out.len - pos;
    let n = if (count < avail) { count } else { avail };
    @memset(out[pos..pos + n], ' ' as u8);
    return pos + n;
}

fn pfmtWriteF64(out: []u8, pos: usize, val: f64): usize {
    if (val != val) {
        return writeStr("NaN", out, pos);
    }
    if (val > 1.0e308) {
        return writeStr("Infinity", out, pos);
    }
    if (val < -1.0e308) {
        return writeStr("-Infinity", out, pos);
    }
    let w: usize = pos;
    let x: f64 = val;
    if (x < 0.0) {
        w = pfmtWriteByte(out, w, '-' as u8);
        x = -x;
    }
    let int_part: u64 = x as u64;
    w = writeU64(int_part, out, w);

    let frac: f64 = x - (int_part as f64);
    let scaled: u64 = ((frac * 1000000.0) + 0.5) as u64;
    if (scaled >= (1000000 as u64)) {
        // Re-emit rounded integer part when fractional rounds up to 1.0
        w = pos;
        if (val < 0.0) {
            w = pfmtWriteByte(out, w, '-' as u8);
        }
        return writeU64(int_part + (1 as u64), out, w);
    }
    if (scaled == (0 as u64)) {
        return w;
    }

    let frac_buf: [6]u8 = undefined;
    let rem: u64 = scaled;
    let idx: usize = 6;
    while (idx > 0) {
        idx = idx - 1;
        frac_buf[idx] = ((rem % (10 as u64)) as u8) + ('0' as u8);
        rem = rem / (10 as u64);
    }
    let trimmed: usize = 6;
    while (trimmed > 0 && frac_buf[trimmed - 1] == ('0' as u8)) {
        trimmed = trimmed - 1;
    }
    if (trimmed > 0) {
        w = pfmtWriteByte(out, w, '.' as u8);
        w = writeStr(frac_buf[0..trimmed], out, w);
    }
    return w;
}

fn pfmtWriteStringLiteral(out: []u8, pos: usize, s: string, escape_string: bool): usize {
    let w: usize = pos;
    w = pfmtWriteByte(out, w, '"' as u8);
    if (!escape_string) {
        if (w < out.len && s.len > 0) {
            let avail = out.len - w;
            let n = if (s.len < avail) { s.len } else { avail };
            @memcpy(out[w..w + n], s[0..n]);
            w = w + n;
        }
        w = pfmtWriteByte(out, w, '"' as u8);
        return w;
    }
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        let ch: u8 = s[i];
        if (ch == ('\\' as u8)) {
            w = writeStr("\\\\", out, w);
        } else if (ch == ('"' as u8)) {
            w = writeStr("\\\"", out, w);
        } else if (ch == ('\n' as u8)) {
            w = writeStr("\\n", out, w);
        } else if (ch == ('\r' as u8)) {
            w = writeStr("\\r", out, w);
        } else if (ch == ('\t' as u8)) {
            w = writeStr("\\t", out, w);
        } else {
            w = pfmtWriteByte(out, w, ch);
        }
        i = i + 1;
    }
    w = pfmtWriteByte(out, w, '"' as u8);
    return w;
}

/// Configuration options for `PrettyWriter`.
pub struct PrettyOptions {
    pub indent: usize,
    pub max_depth: usize,
    pub min: bool,
    pub escape_string: bool,
    pub print_basic_prototype: bool,

    pub static fn defaults(): PrettyOptions {
        return PrettyOptions {
            indent: 2,
            max_depth: 8,
            min: false,
            escape_string: true,
            print_basic_prototype: true
        };
    }

    pub static fn compact(): PrettyOptions {
        return PrettyOptions {
            indent: 0,
            max_depth: 8,
            min: true,
            escape_string: true,
            print_basic_prototype: false
        };
    }
}

/// Zero-heap fixed-capacity structured value pretty-printer.
pub struct PrettyWriter {
    pub opts: PrettyOptions,
    pub buf: [2048]u8,
    pub len: usize,
    pub depth: usize,
    pub item_counts: [32]usize,
    pub truncated_depth: usize,

    pub static fn new(opts: PrettyOptions): PrettyWriter {
        return PrettyWriter {
            opts: opts,
            buf: undefined,
            len: 0,
            depth: 0,
            item_counts: undefined,
            truncated_depth: 0
        };
    }

    pub static fn defaultWriter(): PrettyWriter {
        return PrettyWriter.new(PrettyOptions.defaults());
    }

    pub fn clear(*_) {
        _.len = 0;
        _.depth = 0;
        _.truncated_depth = 0;
    }

    pub fn isTruncated(): bool {
        return _.truncated_depth > 0;
    }

    pub fn writeNull(*_) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = writeStr("null", _.buf[0..PRETTY_BUF_CAP], _.len);
    }

    pub fn writeUndefined(*_) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = writeStr("undefined", _.buf[0..PRETTY_BUF_CAP], _.len);
    }

    pub fn writeBool(*_, val: bool) {
        if (_.truncated_depth > 0) {
            return;
        }
        if (val) {
            _.len = writeStr("true", _.buf[0..PRETTY_BUF_CAP], _.len);
        } else {
            _.len = writeStr("false", _.buf[0..PRETTY_BUF_CAP], _.len);
        }
    }

    pub fn writeInt(*_, val: i64) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = writeI64(val, _.buf[0..PRETTY_BUF_CAP], _.len);
    }

    pub fn writeFloat(*_, val: f64) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = pfmtWriteF64(_.buf[0..PRETTY_BUF_CAP], _.len, val);
    }

    pub fn writeString(*_, s: string) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = pfmtWriteStringLiteral(_.buf[0..PRETTY_BUF_CAP], _.len, s, _.opts.escape_string);
    }

    pub fn writeSymbol(*_, desc: string) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = writeStr("Symbol(", _.buf[0..PRETTY_BUF_CAP], _.len);
        if (desc.len > 0) {
            _.len = writeStr(desc, _.buf[0..PRETTY_BUF_CAP], _.len);
        }
        _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ')' as u8);
    }

    pub fn writeFunction(*_, name: string) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = writeStr("[Function ", _.buf[0..PRETTY_BUF_CAP], _.len);
        if (name.len > 0) {
            _.len = writeStr(name, _.buf[0..PRETTY_BUF_CAP], _.len);
        } else {
            _.len = writeStr("anonymous", _.buf[0..PRETTY_BUF_CAP], _.len);
        }
        _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ']' as u8);
    }

    /// Opens an object container. Returns `false` if `max_depth` was exceeded (emitting `"[Object]"`).
    pub fn beginObject(*_, type_name: string): bool {
        if (_.truncated_depth > 0) {
            _.truncated_depth = _.truncated_depth + 1;
            return false;
        }
        if (_.depth >= _.opts.max_depth) {
            _.len = writeStr("[Object]", _.buf[0..PRETTY_BUF_CAP], _.len);
            _.truncated_depth = 1;
            return false;
        }
        if (!_.opts.min && type_name.len > 0) {
            let is_basic: bool = strEq(type_name, "Object");
            if (_.opts.print_basic_prototype || !is_basic) {
                _.len = writeStr(type_name, _.buf[0..PRETTY_BUF_CAP], _.len);
                _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ' ' as u8);
            }
        }
        _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, '{' as u8);
        if (_.depth < PRETTY_MAX_NEST) {
            _.item_counts[_.depth] = 0;
        }
        _.depth = _.depth + 1;
        return true;
    }

    /// Writes an object property key and separator before its value.
    pub fn writeKey(*_, k: string) {
        if (_.truncated_depth > 0 || _.depth == 0) {
            return;
        }
        let level: usize = _.depth - 1;
        let prev_count: usize = if (level < PRETTY_MAX_NEST) { _.item_counts[level] } else { 0 };
        if (prev_count > 0) {
            _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ',' as u8);
            if (_.opts.min) {
                _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ' ' as u8);
            }
        }
        if (!_.opts.min) {
            _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, '\n' as u8);
            _.len = pfmtWriteSpaces(_.buf[0..PRETTY_BUF_CAP], _.len, _.depth * _.opts.indent);
        }
        _.len = pfmtWriteStringLiteral(_.buf[0..PRETTY_BUF_CAP], _.len, k, _.opts.escape_string);
        _.len = writeStr(": ", _.buf[0..PRETTY_BUF_CAP], _.len);
        if (level < PRETTY_MAX_NEST) {
            _.item_counts[level] = prev_count + 1;
        }
    }

    /// Closes the current object container.
    pub fn endObject(*_) {
        if (_.truncated_depth > 0) {
            _.truncated_depth = _.truncated_depth - 1;
            return;
        }
        if (_.depth == 0) {
            return;
        }
        _.depth = _.depth - 1;
        let count: usize = if (_.depth < PRETTY_MAX_NEST) { _.item_counts[_.depth] } else { 0 };
        if (count > 0 && !_.opts.min) {
            _.len = writeStr(",\n", _.buf[0..PRETTY_BUF_CAP], _.len);
            _.len = pfmtWriteSpaces(_.buf[0..PRETTY_BUF_CAP], _.len, _.depth * _.opts.indent);
        }
        _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, '}' as u8);
    }

    /// Opens an array container. Returns `false` if `max_depth` was exceeded (emitting `"[Array]"`).
    pub fn beginArray(*_, type_name: string): bool {
        if (_.truncated_depth > 0) {
            _.truncated_depth = _.truncated_depth + 1;
            return false;
        }
        if (_.depth >= _.opts.max_depth) {
            _.len = writeStr("[Array]", _.buf[0..PRETTY_BUF_CAP], _.len);
            _.truncated_depth = 1;
            return false;
        }
        if (!_.opts.min && type_name.len > 0) {
            let is_basic: bool = strEq(type_name, "Array");
            if (_.opts.print_basic_prototype || !is_basic) {
                _.len = writeStr(type_name, _.buf[0..PRETTY_BUF_CAP], _.len);
                _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ' ' as u8);
            }
        }
        _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, '[' as u8);
        if (_.depth < PRETTY_MAX_NEST) {
            _.item_counts[_.depth] = 0;
        }
        _.depth = _.depth + 1;
        return true;
    }

    /// Prepares the separator and indentation for the next array element.
    pub fn nextItem(*_) {
        if (_.truncated_depth > 0 || _.depth == 0) {
            return;
        }
        let level: usize = _.depth - 1;
        let prev_count: usize = if (level < PRETTY_MAX_NEST) { _.item_counts[level] } else { 0 };
        if (prev_count > 0) {
            _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ',' as u8);
            if (_.opts.min) {
                _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ' ' as u8);
            }
        }
        if (!_.opts.min) {
            _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, '\n' as u8);
            _.len = pfmtWriteSpaces(_.buf[0..PRETTY_BUF_CAP], _.len, _.depth * _.opts.indent);
        }
        if (level < PRETTY_MAX_NEST) {
            _.item_counts[level] = prev_count + 1;
        }
    }

    /// Closes the current array container.
    pub fn endArray(*_) {
        if (_.truncated_depth > 0) {
            _.truncated_depth = _.truncated_depth - 1;
            return;
        }
        if (_.depth == 0) {
            return;
        }
        _.depth = _.depth - 1;
        let count: usize = if (_.depth < PRETTY_MAX_NEST) { _.item_counts[_.depth] } else { 0 };
        if (count > 0 && !_.opts.min) {
            _.len = writeStr(",\n", _.buf[0..PRETTY_BUF_CAP], _.len);
            _.len = pfmtWriteSpaces(_.buf[0..PRETTY_BUF_CAP], _.len, _.depth * _.opts.indent);
        }
        _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ']' as u8);
    }
}

/// Formats a string literal with surrounding quotes and optional escape sequences into `out`.
pub fn formatStringLiteral(s: string, escape: bool, out: []u8): usize {
    return pfmtWriteStringLiteral(out, 0, s, escape);
}

/// Formats a signed 64-bit integer into `out`.
pub fn formatPrimitiveInt(v: i64, out: []u8): usize {
    return writeI64(v, out, 0);
}

export {
    PRETTY_BUF_CAP,
    PRETTY_MAX_NEST,
    PrettyOptions,
    PrettyWriter,
    formatStringLiteral,
    formatPrimitiveInt
};
