// Source: Absorbed from /data/dev/bun/src/logger.zig & Rust `logfmt` / Go `go-logfmt`
// Pure-CP zero-allocation logfmt key=value structured log line parser, typed lookup, and atomic encoder.

import std from "std";

type LogfmtVec16 = @Vector(16, u8);

fn makeLogfmtClassLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    // bit 0: isWs (' ', '\t', '\r', '\n')
    // bit 1: isValidKeyChar (> 0x20 && != '=' && != '"' && != 0x7F)
    // bit 2: needsQuote (<= 0x20 || == '"' || == '=' || == '\\' || == 0x7F)
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let flags: u8 = 0;
        if (i == ' ' || i == '\t' || i == '\r' || i == '\n') {
            flags |= 1;
        }
        if (i > 0x20 && i != '=' && i != '"' && i != 0x7F) {
            flags |= 2;
        }
        if (i <= 0x20 || i == '"' || i == '=' || i == '\\' || i == 0x7F) {
            flags |= 4;
        }
        lut[i] = flags;
    }
    return lut;
}

const LOGFMT_CLASS_LUT: [u8; 256] = makeLogfmtClassLut();

fn isWs(b: u8): bool {
    return (LOGFMT_CLASS_LUT[usize(b)] & 1) != 0;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: LogfmtVec16 = a[i .. i + 16][0..16].*;
        const vb: LogfmtVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn isValidKeyChar(b: u8): bool {
    return (LOGFMT_CLASS_LUT[usize(b)] & 2) != 0;
}

fn isValidKeySlice(key: []const u8): bool {
    if (key.len == 0) return false;
    let i: usize = 0;
    const v_sp: LogfmtVec16 = @splat(0x20 as u8);
    const v_eq: LogfmtVec16 = @splat(61 as u8);
    const v_dq: LogfmtVec16 = @splat(34 as u8);
    const v_del: LogfmtVec16 = @splat(0x7F as u8);
    const v_zero: LogfmtVec16 = @splat(0 as u8);
    const v_one: LogfmtVec16 = @splat(1 as u8);
    while (i + 16 <= key.len) : (i += 16) {
        const v: LogfmtVec16 = key[i .. i + 16][0..16].*;
        const bad = (v <= v_sp) | (v == v_eq) | (v == v_dq) | (v == v_del);
        if (@reduce(.Or, @select(u8, bad, v_one, v_zero)) != 0) return false;
    }
    while (i < key.len) : (i += 1) {
        if (!isValidKeyChar(key[i])) return false;
    }
    return true;
}

pub struct LogfmtField {
    key: []const u8,
    raw_value: []const u8,
    is_quoted: bool,
    has_value: bool, // false for bare boolean flag `key` without `=`

    /// Unescape the value into `out` (handles `\"`, `\\`, `\n`, `\r`, `\t` for quoted values).
    pub fn decodeValue(out: []u8): ([]const u8)? {
        if (!_.is_quoted) {
            if (_.raw_value.len > out.len) return null;
            if (_.raw_value.len > 0) {
                @memcpy(out[0.._.raw_value.len], _.raw_value);
            }
            return out[0.._.raw_value.len];
        }
        let i: usize = 0;
        let w: usize = 0;
        const v_bs: LogfmtVec16 = @splat(92 as u8);
        const v_zero: LogfmtVec16 = @splat(0 as u8);
        const v_one: LogfmtVec16 = @splat(1 as u8);
        while (i < _.raw_value.len) {
            if (i + 16 <= _.raw_value.len && w + 16 <= out.len) {
                const v: LogfmtVec16 = _.raw_value[i .. i + 16][0..16].*;
                if (@reduce(.Or, @select(u8, v == v_bs, v_one, v_zero)) == 0) {
                    @memcpy(out[w .. w + 16], _.raw_value[i .. i + 16]);
                    w += 16;
                    i += 16;
                    continue;
                }
            }
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

    /// Parse field value as signed 64-bit integer.
    pub fn asInt(): i64? {
        if (_.raw_value.len == 0) return null;
        return std.fmt.parseInt(i64, _.raw_value, 10) catch return null;
    }

    /// Parse field value as unsigned 64-bit integer.
    pub fn asUint(): u64? {
        if (_.raw_value.len == 0) return null;
        return std.fmt.parseInt(u64, _.raw_value, 10) catch return null;
    }

    /// Parse field value as 64-bit float.
    pub fn asFloat(): f64? {
        if (_.raw_value.len == 0) return null;
        return std.fmt.parseFloat(f64, _.raw_value) catch return null;
    }

    /// Parse field value as boolean (`true`/`1` -> true, `false`/`0` -> false; bare flag -> true).
    pub fn asBool(): bool? {
        if (!_.has_value) return true;
        if (strEq(_.raw_value, "true") || strEq(_.raw_value, "1")) return true;
        if (strEq(_.raw_value, "false") || strEq(_.raw_value, "0")) return false;
        return null;
    }
}

pub struct LogfmtIter {
    src: []const u8,
    pos: usize,
    had_error: bool,

    pub static fn init(line: []const u8): LogfmtIter {
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
            const v_dq: LogfmtVec16 = @splat(34 as u8);
            const v_bs: LogfmtVec16 = @splat(92 as u8);
            const v_zero: LogfmtVec16 = @splat(0 as u8);
            const v_one: LogfmtVec16 = @splat(1 as u8);
            while (_.pos < _.src.len) {
                if (_.pos + 16 <= _.src.len) {
                    const v: LogfmtVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
                    if (@reduce(.Or, @select(u8, (v == v_dq) | (v == v_bs), v_one, v_zero)) == 0) {
                        _.pos += 16;
                        continue;
                    }
                }
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
        const v_sp: LogfmtVec16 = @splat(0x20 as u8);
        const v_dq2: LogfmtVec16 = @splat(34 as u8);
        const v_eq2: LogfmtVec16 = @splat(61 as u8);
        const v_zero2: LogfmtVec16 = @splat(0 as u8);
        const v_one2: LogfmtVec16 = @splat(1 as u8);
        while (_.pos + 16 <= _.src.len) {
            const v: LogfmtVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
            const stop = (v <= v_sp) | (v == v_dq2) | (v == v_eq2);
            if (@reduce(.Or, @select(u8, stop, v_one2, v_zero2)) != 0) break;
            _.pos += 16;
        }
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
pub fn getLogfmtValue(line: []const u8, key: []const u8, out: []u8): ([]const u8)? {
    let it = LogfmtIter.init(line);
    while (f := it.next()) {
        if (strEq(f.key, key)) {
            return f.decodeValue(out);
        }
    }
    return null;
}

/// Lookup a key in a logfmt line and parse it as `i64`.
pub fn getLogfmtInt(line: []const u8, key: []const u8): i64? {
    let it = LogfmtIter.init(line);
    while (f := it.next()) {
        if (strEq(f.key, key)) {
            return f.asInt();
        }
    }
    return null;
}

/// Lookup a key in a logfmt line and parse it as `bool`.
pub fn getLogfmtBool(line: []const u8, key: []const u8): bool? {
    let it = LogfmtIter.init(line);
    while (f := it.next()) {
        if (strEq(f.key, key)) {
            return f.asBool();
        }
    }
    return null;
}

/// Count valid fields in a logfmt line.
pub fn countLogfmtFields(line: []const u8): usize {
    let it = LogfmtIter.init(line);
    let count: usize = 0;
    while (f := it.next()) {
        * = f;
        count += 1;
    }
    return count;
}

fn valueNeedsQuotes(val: []const u8): bool {
    if (val.len == 0) return true;
    let i: usize = 0;
    const v_sp: LogfmtVec16 = @splat(0x20 as u8);
    const v_dq: LogfmtVec16 = @splat(34 as u8);
    const v_eq: LogfmtVec16 = @splat(61 as u8);
    const v_bs: LogfmtVec16 = @splat(92 as u8);
    const v_del: LogfmtVec16 = @splat(0x7F as u8);
    const v_zero: LogfmtVec16 = @splat(0 as u8);
    const v_one: LogfmtVec16 = @splat(1 as u8);
    while (i + 16 <= val.len) : (i += 16) {
        const v: LogfmtVec16 = val[i .. i + 16][0..16].*;
        const bad = (v <= v_sp) | (v == v_dq) | (v == v_eq) | (v == v_bs) | (v == v_del);
        if (@reduce(.Or, @select(u8, bad, v_one, v_zero)) != 0) return true;
    }
    while (i < val.len) {
        if ((LOGFMT_CLASS_LUT[usize(val[i])] & 4) != 0) return true;
        i += 1;
    }
    return false;
}

/// Append a `key=value` pair to `out` at `pos` (inserting a leading space if `pos > 0`).
/// Automatically quotes and escapes values containing whitespace, `=`, `"`, `\`, `\n`, `\r`, `\t`.
/// Atomic on buffer overflow: restores `pos` to its original value if `false` is returned.
pub fn appendLogfmtPair(out: []u8, pos: *usize, key: []const u8, val: []const u8): bool {
    const saved_pos = pos.*;
    if (!isValidKeySlice(key)) return false;
    if (pos.* > 0) {
        if (pos.* >= out.len) {
            pos.* = saved_pos;
            return false;
        }
        out[pos.*] = ' ';
        pos.* += 1;
    }
    if (pos.* + key.len + 1 > out.len) {
        pos.* = saved_pos;
        return false;
    }
    @memcpy(out[pos.* .. pos.* + key.len], key);
    pos.* += key.len;
    out[pos.*] = '=';
    pos.* += 1;

    if (!valueNeedsQuotes(val)) {
        if (pos.* + val.len > out.len) {
            pos.* = saved_pos;
            return false;
        }
        if (val.len > 0) {
            @memcpy(out[pos.* .. pos.* + val.len], val);
        }
        pos.* += val.len;
        return true;
    }

    if (pos.* >= out.len) {
        pos.* = saved_pos;
        return false;
    }
    out[pos.*] = '"';
    pos.* += 1;

    let vi: usize = 0;
    const v_dq: LogfmtVec16 = @splat(34 as u8);
    const v_bs: LogfmtVec16 = @splat(92 as u8);
    const v_nl: LogfmtVec16 = @splat(10 as u8);
    const v_cr: LogfmtVec16 = @splat(13 as u8);
    const v_tab: LogfmtVec16 = @splat(9 as u8);
    const v_zero: LogfmtVec16 = @splat(0 as u8);
    const v_one: LogfmtVec16 = @splat(1 as u8);
    while (vi < val.len) {
        if (vi + 16 <= val.len && pos.* + 16 <= out.len) {
            const v: LogfmtVec16 = val[vi .. vi + 16][0..16].*;
            const esc = (v == v_dq) | (v == v_bs) | (v == v_nl) | (v == v_cr) | (v == v_tab);
            if (@reduce(.Or, @select(u8, esc, v_one, v_zero)) == 0) {
                @memcpy(out[pos.* .. pos.* + 16], val[vi .. vi + 16]);
                pos.* += 16;
                vi += 16;
                continue;
            }
        }
        const c = val[vi];
        if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t') {
            if (pos.* + 2 > out.len) {
                pos.* = saved_pos;
                return false;
            }
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
            if (pos.* >= out.len) {
                pos.* = saved_pos;
                return false;
            }
            out[pos.*] = c;
            pos.* += 1;
        }
        vi += 1;
    }

    if (pos.* >= out.len) {
        pos.* = saved_pos;
        return false;
    }
    out[pos.*] = '"';
    pos.* += 1;
    return true;
}

/// Zero-allocation structured logfmt line builder (modeled after Rust `logfmt` / Go `go-logfmt`).
pub struct LogfmtEncoder {
    buf: []u8,
    pos: usize = 0,

    pub static fn init(buf: []u8): LogfmtEncoder {
        return LogfmtEncoder{ buf: buf, pos: 0 };
    }

    pub fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    pub fn reset(*_) {
        _.pos = 0;
    }

    pub fn kvStr(*_, key: []const u8, val: []const u8): bool {
        return appendLogfmtPair(_.buf, &_.pos, key, val);
    }

    pub fn kvInt(*_, key: []const u8, val: i64): bool {
        let tmp: [32]u8 = [0; 32];
        const s = std.fmt.bufPrint(tmp[0..], "{d}", .{val}) catch return false;
        return appendLogfmtPair(_.buf, &_.pos, key, s);
    }

    pub fn kvUint(*_, key: []const u8, val: u64): bool {
        let tmp: [32]u8 = [0; 32];
        const s = std.fmt.bufPrint(tmp[0..], "{d}", .{val}) catch return false;
        return appendLogfmtPair(_.buf, &_.pos, key, s);
    }

    pub fn kvFloat(*_, key: []const u8, val: f64): bool {
        let tmp: [48]u8 = [0; 48];
        const s = std.fmt.bufPrint(tmp[0..], "{d:.3}", .{val}) catch return false;
        return appendLogfmtPair(_.buf, &_.pos, key, s);
    }

    pub fn kvBool(*_, key: []const u8, val: bool): bool {
        return appendLogfmtPair(_.buf, &_.pos, key, if (val) "true" else "false");
    }

    pub fn kvFlag(*_, key: []const u8): bool {
        const saved = _.pos;
        if (!isValidKeySlice(key)) return false;
        if (_.pos > 0) {
            if (_.pos >= _.buf.len) {
                _.pos = saved;
                return false;
            }
            _.buf[_.pos] = ' ';
            _.pos += 1;
        }
        if (_.pos + key.len > _.buf.len) {
            _.pos = saved;
            return false;
        }
        @memcpy(_.buf[_.pos .. _.pos + key.len], key);
        _.pos += key.len;
        return true;
    }

    pub fn endLine(*_): bool {
        if (_.pos >= _.buf.len) return false;
        _.buf[_.pos] = '\n';
        _.pos += 1;
        return true;
    }
}
