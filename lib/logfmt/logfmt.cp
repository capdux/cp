// Source: Absorbed from /data/dev/bun/src/logger.zig (Structured logfmt Parser & Encoder)
// Pure-CP zero-allocation logfmt key=value structured log line parser, lookup, and canonical encoder.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn isValidKeyChar(b: u8): bool {
    return b > 0x20 && b != '=' && b != '"' && b != 0x7F;
}

export struct LogfmtField {
    key: []const u8,
    raw_value: []const u8,
    is_quoted: bool,
    has_value: bool, // false for bare boolean flag `key` without `=`

    /// Unescape the value into `out` (handles `\"`, `\\`, `\n`, `\r`, `\t` for quoted values).
    pub fn decodeValue(out: []u8): ([]const u8)? {
        if (!_.is_quoted) {
            if (_.raw_value.len > out.len) return null;
            let i: usize = 0;
            while (i < _.raw_value.len) {
                out[i] = _.raw_value[i];
                i += 1;
            }
            return out[0.._.raw_value.len];
        }
        let i: usize = 0;
        let w: usize = 0;
        while (i < _.raw_value.len) {
            const c = _.raw_value[i];
            if (c == '\\' && i + 1 < _.raw_value.len) {
                const nxt = _.raw_value[i + 1];
                let ch: u8 = nxt;
                if (nxt == 'n') {
                    ch = '\n';
                } else if (nxt == 'r') {
                    ch = '\r';
                } else if (nxt == 't') {
                    ch = '\t';
                }
                if (w >= out.len) return null;
                out[w] = ch;
                w += 1;
                i += 2;
                continue;
            }
            if (w >= out.len) return null;
            out[w] = c;
            w += 1;
            i += 1;
        }
        return out[0..w];
    }
}

export struct LogfmtIter {
    src: []const u8,
    pos: usize,
    had_error: bool,

    pub fn init(line: []const u8): LogfmtIter {
        return LogfmtIter { src: line, pos: 0, had_error: false };
    }

    pub fn next(*_): LogfmtField? {
        if (_.had_error) return null;
        while (_.pos < _.src.len && isWs(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.src.len) return null;

        const kstart = _.pos;
        while (_.pos < _.src.len && isValidKeyChar(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos == kstart) {
            _.had_error = true;
            return null;
        }
        const key = _.src[kstart.._.pos];

        // Bare key (no '=') -> implicit boolean true flag
        if (_.pos >= _.src.len || isWs(_.src[_.pos])) {
            return LogfmtField {
                key: key,
                raw_value: "true",
                is_quoted: false,
                has_value: false,
            };
        }

        if (_.src[_.pos] != '=') {
            _.had_error = true;
            return null;
        }
        _.pos += 1; // skip '='

        if (_.pos >= _.src.len || isWs(_.src[_.pos])) {
            return LogfmtField {
                key: key,
                raw_value: "",
                is_quoted: false,
                has_value: true,
            };
        }

        if (_.src[_.pos] == '"') {
            _.pos += 1; // skip opening quote
            const vstart = _.pos;
            let closed = false;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (c == '\\') {
                    if (_.pos + 1 >= _.src.len) {
                        _.had_error = true;
                        return null;
                    }
                    _.pos += 2;
                    continue;
                }
                if (c == '"') {
                    closed = true;
                    break;
                }
                _.pos += 1;
            }
            if (!closed) {
                _.had_error = true;
                return null;
            }
            const raw_val = _.src[vstart.._.pos];
            _.pos += 1; // skip closing quote
            return LogfmtField {
                key: key,
                raw_value: raw_val,
                is_quoted: true,
                has_value: true,
            };
        }

        const vstart = _.pos;
        while (_.pos < _.src.len && !isWs(_.src[_.pos])) {
            if (_.src[_.pos] == '"' || _.src[_.pos] == '=') {
                _.had_error = true;
                return null;
            }
            _.pos += 1;
        }
        return LogfmtField {
            key: key,
            raw_value: _.src[vstart.._.pos],
            is_quoted: false,
            has_value: true,
        };
    }

    pub fn hasError(): bool {
        return _.had_error;
    }
}

/// Lookup a key in a logfmt line and decode its value into `out`.
export fn getLogfmtValue(line: []const u8, key: []const u8, out: []u8): ([]const u8)? {
    let it = LogfmtIter.init(line);
    while (f := it.next()) {
        if (strEq(f.key, key)) {
            return f.decodeValue(out);
        }
    }
    return null;
}

fn valueNeedsQuotes(val: []const u8): bool {
    if (val.len == 0) return true;
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        if (c <= 0x20 || c == '"' || c == '=' || c == '\\' || c == 0x7F) return true;
        i += 1;
    }
    return false;
}

/// Append a `key=value` pair to `out` at `pos` (inserting a leading space if `pos > 0`).
/// Automatically quotes and escapes values containing whitespace, `=`, `"`, `\`, `\n`, `\r`, `\t`.
export fn appendLogfmtPair(out: []u8, pos: *usize, key: []const u8, val: []const u8): bool {
    if (key.len == 0) return false;
    let ki: usize = 0;
    while (ki < key.len) {
        if (!isValidKeyChar(key[ki])) return false;
        ki += 1;
    }
    if (pos.* > 0) {
        if (pos.* >= out.len) return false;
        out[pos.*] = ' ';
        pos.* += 1;
    }
    if (pos.* + key.len + 1 > out.len) return false;
    let i: usize = 0;
    while (i < key.len) {
        out[pos.* + i] = key[i];
        i += 1;
    }
    pos.* += key.len;
    out[pos.*] = '=';
    pos.* += 1;

    if (!valueNeedsQuotes(val)) {
        if (pos.* + val.len > out.len) return false;
        let vi: usize = 0;
        while (vi < val.len) {
            out[pos.* + vi] = val[vi];
            vi += 1;
        }
        pos.* += val.len;
        return true;
    }

    if (pos.* >= out.len) return false;
    out[pos.*] = '"';
    pos.* += 1;

    let vi: usize = 0;
    while (vi < val.len) {
        const c = val[vi];
        if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t') {
            if (pos.* + 2 > out.len) return false;
            out[pos.*] = '\\';
            if (c == '\n') {
                out[pos.* + 1] = 'n';
            } else if (c == '\r') {
                out[pos.* + 1] = 'r';
            } else if (c == '\t') {
                out[pos.* + 1] = 't';
            } else {
                out[pos.* + 1] = c;
            }
            pos.* += 2;
        } else {
            if (pos.* >= out.len) return false;
            out[pos.*] = c;
            pos.* += 1;
        }
        vi += 1;
    }

    if (pos.* >= out.len) return false;
    out[pos.*] = '"';
    pos.* += 1;
    return true;
}
