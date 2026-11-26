// @cap/pretty-format — Pure CP zero-allocation structured value pretty-printer.
// Clean-room MIT implementation inspired by npm `pretty-format`.

pub const PRETTY_BUF_CAP: usize = 2048;
pub const PRETTY_MAX_NEST: usize = 32;

fn pfmtStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn pfmtWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn pfmtWriteStr(out: []u8, pos: usize, s: string): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        out[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn pfmtWriteSpaces(out: []u8, pos: usize, count: usize): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < count && w < out.len) {
        out[w] = ' ' as u8;
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn pfmtWriteU64(out: []u8, pos: usize, val: u64): usize {
    let tmp: [24]u8 = undefined;
    let count: usize = 0;
    let v: u64 = val;
    if (v == (0 as u64)) {
        return pfmtWriteByte(out, pos, '0' as u8);
    }
    while (v > (0 as u64)) {
        let digit: u8 = ((v % (10 as u64)) as u8) + ('0' as u8);
        tmp[count] = digit;
        count = count + 1;
        v = v / (10 as u64);
    }
    let w: usize = pos;
    let k: usize = count;
    while (k > 0 && w < out.len) {
        k = k - 1;
        out[w] = tmp[k];
        w = w + 1;
    }
    return w;
}

fn pfmtWriteI64(out: []u8, pos: usize, val: i64): usize {
    let w: usize = pos;
    if (val < (0 as i64)) {
        w = pfmtWriteByte(out, w, '-' as u8);
        if (val == (-9223372036854775807 as i64 - (1 as i64))) {
            return pfmtWriteStr(out, w, "9223372036854775808");
        }
        let mag: u64 = (0 as i64 - val) as u64;
        return pfmtWriteU64(out, w, mag);
    }
    return pfmtWriteU64(out, w, val as u64);
}

fn pfmtWriteF64(out: []u8, pos: usize, val: f64): usize {
    if (val != val) {
        return pfmtWriteStr(out, pos, "NaN");
    }
    if (val > 1.0e308) {
        return pfmtWriteStr(out, pos, "Infinity");
    }
    if (val < -1.0e308) {
        return pfmtWriteStr(out, pos, "-Infinity");
    }
    let w: usize = pos;
    let x: f64 = val;
    if (x < 0.0) {
        w = pfmtWriteByte(out, w, '-' as u8);
        x = -x;
    }
    let int_part: u64 = x as u64;
    w = pfmtWriteU64(out, w, int_part);

    let frac: f64 = x - (int_part as f64);
    let scaled: u64 = ((frac * 1000000.0) + 0.5) as u64;
    if (scaled >= (1000000 as u64)) {
        // Re-emit rounded integer part when fractional rounds up to 1.0
        w = pos;
        if (val < 0.0) {
            w = pfmtWriteByte(out, w, '-' as u8);
        }
        return pfmtWriteU64(out, w, int_part + (1 as u64));
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
        let j: usize = 0;
        while (j < trimmed && w < out.len) {
            out[w] = frac_buf[j];
            w = w + 1;
            j = j + 1;
        }
    }
    return w;
}

fn pfmtWriteStringLiteral(out: []u8, pos: usize, s: string, escape_string: bool): usize {
    let w: usize = pos;
    w = pfmtWriteByte(out, w, '"' as u8);
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        let ch: u8 = s[i];
        if (escape_string) {
            if (ch == ('\\' as u8)) {
                w = pfmtWriteStr(out, w, "\\\\");
            } else if (ch == ('"' as u8)) {
                w = pfmtWriteStr(out, w, "\\\"");
            } else if (ch == ('\n' as u8)) {
                w = pfmtWriteStr(out, w, "\\n");
            } else if (ch == ('\r' as u8)) {
                w = pfmtWriteStr(out, w, "\\r");
            } else if (ch == ('\t' as u8)) {
                w = pfmtWriteStr(out, w, "\\t");
            } else {
                w = pfmtWriteByte(out, w, ch);
            }
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
            buf: [0; 2048],
            len: 0,
            depth: 0,
            item_counts: [0; 32],
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
        let i: usize = 0;
        while (i < PRETTY_MAX_NEST) {
            _.item_counts[i] = 0;
            i = i + 1;
        }
    }

    pub fn isTruncated(): bool {
        return _.truncated_depth > 0;
    }

    pub fn writeNull(*_) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "null");
    }

    pub fn writeUndefined(*_) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "undefined");
    }

    pub fn writeBool(*_, val: bool) {
        if (_.truncated_depth > 0) {
            return;
        }
        if (val) {
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "true");
        } else {
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "false");
        }
    }

    pub fn writeInt(*_, val: i64) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = pfmtWriteI64(_.buf[0..PRETTY_BUF_CAP], _.len, val);
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
        _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "Symbol(");
        if (desc.len > 0) {
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, desc);
        }
        _.len = pfmtWriteByte(_.buf[0..PRETTY_BUF_CAP], _.len, ')' as u8);
    }

    pub fn writeFunction(*_, name: string) {
        if (_.truncated_depth > 0) {
            return;
        }
        _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "[Function ");
        if (name.len > 0) {
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, name);
        } else {
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "anonymous");
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
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "[Object]");
            _.truncated_depth = 1;
            return false;
        }
        if (!_.opts.min && type_name.len > 0) {
            let is_basic: bool = pfmtStrEq(type_name, "Object");
            if (_.opts.print_basic_prototype || !is_basic) {
                _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, type_name);
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
        _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, ": ");
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
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, ",\n");
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
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, "[Array]");
            _.truncated_depth = 1;
            return false;
        }
        if (!_.opts.min && type_name.len > 0) {
            let is_basic: bool = pfmtStrEq(type_name, "Array");
            if (_.opts.print_basic_prototype || !is_basic) {
                _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, type_name);
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
            _.len = pfmtWriteStr(_.buf[0..PRETTY_BUF_CAP], _.len, ",\n");
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
    return pfmtWriteI64(out, 0, v);
}

export {
    PRETTY_BUF_CAP,
    PRETTY_MAX_NEST,
    PrettyOptions,
    PrettyWriter,
    formatStringLiteral,
    formatPrimitiveInt
};
