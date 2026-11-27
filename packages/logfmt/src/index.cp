// @cap/logfmt — Zero-allocation logfmt Structured Key=Value Log Parser & Writer
// Clean-room MIT implementation in pure CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD quote/escape scanning, comptime 256-entry LUT, and @memcpy.

import { isSpace } from "cap:ascii";
import { bytesEq } from "cap:bin";
import { writeI64 } from "cap:fmt";

type LfVec16 = @Vector(16, u8);

fn lfBuildCharLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    // bit0: whitespace, bit1: key terminator (space/'='/'"'), bit2: needsQuote (<=32/'='/'"'/'\')
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let flags: u8 = 0;
        if (c == 32 || c == 9 || c == 10 || c == 13 || c == 11 || c == 12) {
            flags |= 3;
        }
        if (c == 61 || c == 34) {
            flags |= 2;
        }
        if (c <= 32 || c == 61 || c == 34 || c == 92) {
            flags |= 4;
        }
        lut[c] = flags;
        c += 1;
    }
    return lut;
}

const LF_CHAR_LUT: [256]u8 = lfBuildCharLut();

fn lfScanQuotedPlain(line: []const u8, start: usize) -> usize {
    let i: usize = start;
    const v_qt: LfVec16 = @splat(34 as u8);
    const v_bs: LfVec16 = @splat(92 as u8);
    while (i + 16 <= line.len) {
        const v: LfVec16 = line[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v_qt) | (v == v_bs))) {
            break;
        }
        i += 16;
    }
    while (i < line.len && line[i] != 34 && line[i] != 92) {
        i += 1;
    }
    return i;
}

pub const MAX_FIELDS: usize = 64;
const KEY_BUF_CAP: usize = 512;
const VAL_BUF_CAP: usize = 1024;

pub struct LogfmtField {
    pub key_start: usize,
    pub key_len: usize,
    pub val_start: usize,
    pub val_len: usize,
}

pub struct LogfmtRecord {
    key_buf: [512]u8,
    key_used: usize,
    val_buf: [1024]u8,
    val_used: usize,
    fields: [64]LogfmtField,
    count: usize,

    /// Parses a single logfmt line into a zero-heap `LogfmtRecord`.
    /// Supports `k=v`, quoted escaped strings `msg="hello \"world\"\n"`,
    /// bare boolean flags (`debug` -> `"true"`), and empty values (`err=`).
    pub static fn parse(line: []const u8) -> LogfmtRecord {
        let key_buf: [512]u8 = undefined;
        let val_buf: [1024]u8 = undefined;
        let fields: [64]LogfmtField = undefined;

        let key_used: usize = 0;
        let val_used: usize = 0;
        let count: usize = 0;

        let i: usize = 0;
        const n = line.len;

        while (i < n && count < MAX_FIELDS) {
            while (i < n && (LF_CHAR_LUT[line[i] as usize] & 1) != 0) {
                i += 1;
            }
            if (i >= n) {
                break;
            }

            // Read key until '=', whitespace, or '"'
            const k_start_in_line = i;
            while (i < n && (LF_CHAR_LUT[line[i] as usize] & 2) == 0) {
                i += 1;
            }
            const k_end_in_line = i;
            const k_len = k_end_in_line - k_start_in_line;

            if (k_len == 0) {
                // Skip stray '=' or '"'
                i += 1;
                continue;
            }

            if (key_used + k_len > KEY_BUF_CAP) {
                break;
            }
            const k_slot = key_used;
            @memcpy(key_buf[key_used .. key_used + k_len], line[k_start_in_line .. k_end_in_line]);
            key_used += k_len;

            const v_slot = val_used;
            let v_len: usize = 0;

            if (i < n && line[i] == 61) {
                // '=' present
                i += 1;
                if (i < n && (LF_CHAR_LUT[line[i] as usize] & 1) == 0) {
                    if (line[i] == 34) {
                        // Quoted string value (16-lane SIMD plain-run + @memcpy fast-path)
                        i += 1;
                        while (i < n && line[i] != 34) {
                            const plain_end = lfScanQuotedPlain(line, i);
                            if (plain_end > i) {
                                const run = plain_end - i;
                                const avail = VAL_BUF_CAP - val_used;
                                const copy_n = if (run <= avail) run else avail;
                                if (copy_n > 0) {
                                    @memcpy(val_buf[val_used .. val_used + copy_n], line[i .. i + copy_n]);
                                    val_used += copy_n;
                                    v_len += copy_n;
                                }
                                i = plain_end;
                                if (i >= n || line[i] == 34) {
                                    break;
                                }
                            }
                            let ch = line[i];
                            if (ch == 92 && i + 1 < n) {
                                const esc = line[i + 1];
                                if (esc == 110) {
                                    ch = 10; // \n
                                } else if (esc == 114) {
                                    ch = 13; // \r
                                } else if (esc == 116) {
                                    ch = 9;  // \t
                                } else {
                                    ch = esc;
                                }
                                i += 2;
                            } else {
                                i += 1;
                            }
                            if (val_used < VAL_BUF_CAP) {
                                val_buf[val_used] = ch;
                                val_used += 1;
                                v_len += 1;
                            }
                        }
                        if (i < n && line[i] == 34) {
                            i += 1;
                        }
                    } else {
                        // Unquoted value
                        const v_start_in_line = i;
                        while (i < n && (LF_CHAR_LUT[line[i] as usize] & 1) == 0) {
                            i += 1;
                        }
                        const run = i - v_start_in_line;
                        const avail = VAL_BUF_CAP - val_used;
                        const copy_n = if (run <= avail) run else avail;
                        if (copy_n > 0) {
                            @memcpy(val_buf[val_used .. val_used + copy_n], line[v_start_in_line .. v_start_in_line + copy_n]);
                            val_used += copy_n;
                            v_len += copy_n;
                        }
                    }
                }
            } else {
                // Bare flag key -> default value "true"
                if (val_used + 4 <= VAL_BUF_CAP) {
                    @memcpy(val_buf[val_used .. val_used + 4], "true"[0..4]);
                    val_used += 4;
                    v_len = 4;
                }
            }

            fields[count] = LogfmtField {
                key_start: k_slot,
                key_len: k_len,
                val_start: v_slot,
                val_len: v_len,
            };
            count += 1;
        }

        return LogfmtRecord {
            key_buf: key_buf,
            key_used: key_used,
            val_buf: val_buf,
            val_used: val_used,
            fields: fields,
            count: count,
        };
    }

    /// Returns the number of parsed fields.
    pub fn len() -> usize {
        return _.count;
    }

    /// Returns true if `key` exists in the record.
    pub fn has(key: []const u8) -> bool {
        let idx = _.count;
        while (idx > 0) {
            idx -= 1;
            const f = _.fields[idx];
            if (bytesEq(_.key_buf[f.key_start..f.key_start + f.key_len], key)) {
                return true;
            }
        }
        return false;
    }

    /// Returns the value for `key` (last occurrence wins), or `""` if not found.
    pub fn get(*_, key: []const u8) -> []const u8 {
        let idx = _.count;
        while (idx > 0) {
            idx -= 1;
            const f = _.fields[idx];
            if (bytesEq(_.key_buf[f.key_start..f.key_start + f.key_len], key)) {
                return _.val_buf[f.val_start..f.val_start + f.val_len];
            }
        }
        return "";
    }
}

fn needsQuotes(val: []const u8) -> bool {
    if (val.len == 0) {
        return true;
    }
    let i: usize = 0;
    const v_32: LfVec16 = @splat(32 as u8);
    const v_eq: LfVec16 = @splat(61 as u8);
    const v_qt: LfVec16 = @splat(34 as u8);
    const v_bs: LfVec16 = @splat(92 as u8);
    while (i + 16 <= val.len) {
        const v: LfVec16 = val[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v <= v_32) | (v == v_eq) | (v == v_qt) | (v == v_bs))) {
            return true;
        }
        i += 16;
    }
    while (i < val.len) {
        if ((LF_CHAR_LUT[val[i] as usize] & 4) != 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub struct LogfmtWriter {
    buf: []u8,
    pos: usize,
    count: usize,

    /// Initializes a logfmt writer backed by caller buffer `buf`.
    pub static fn init(buf: []u8) -> LogfmtWriter {
        return LogfmtWriter {
            buf: buf,
            pos: 0,
            count: 0,
        };
    }

    /// Writes a string field `key=val`, automatically quoting and escaping when needed.
    /// Returns false without partial writes if the buffer does not have enough space.
    pub fn writeStr(*_, key: []const u8, val: []const u8) -> bool {
        if (key.len == 0) {
            return false;
        }
        const saved_pos = _.pos;
        let p = _.pos;

        if (_.count > 0) {
            if (p >= _.buf.len) {
                return false;
            }
            _.buf[p] = 32; // ' '
            p += 1;
        }

        if (p + key.len + 1 > _.buf.len) {
            _.pos = saved_pos;
            return false;
        }
        @memcpy(_.buf[p .. p + key.len], key[0..key.len]);
        p += key.len;
        _.buf[p] = 61; // '='
        p += 1;

        if (needsQuotes(val)) {
            if (p >= _.buf.len) {
                _.pos = saved_pos;
                return false;
            }
            _.buf[p] = 34; // '"'
            p += 1;

            let i: usize = 0;
            while (i < val.len) {
                const b = val[i];
                if (b == 34 || b == 92) {
                    if (p + 2 > _.buf.len) {
                        _.pos = saved_pos;
                        return false;
                    }
                    _.buf[p] = 92;
                    _.buf[p + 1] = b;
                    p += 2;
                } else if (b == 10) {
                    if (p + 2 > _.buf.len) {
                        _.pos = saved_pos;
                        return false;
                    }
                    _.buf[p] = 92;
                    _.buf[p + 1] = 110;
                    p += 2;
                } else if (b == 13) {
                    if (p + 2 > _.buf.len) {
                        _.pos = saved_pos;
                        return false;
                    }
                    _.buf[p] = 92;
                    _.buf[p + 1] = 114;
                    p += 2;
                } else if (b == 9) {
                    if (p + 2 > _.buf.len) {
                        _.pos = saved_pos;
                        return false;
                    }
                    _.buf[p] = 92;
                    _.buf[p + 1] = 116;
                    p += 2;
                } else {
                    if (p >= _.buf.len) {
                        _.pos = saved_pos;
                        return false;
                    }
                    _.buf[p] = b;
                    p += 1;
                }
                i += 1;
            }

            if (p >= _.buf.len) {
                _.pos = saved_pos;
                return false;
            }
            _.buf[p] = 34; // '"'
            p += 1;
        } else {
            if (p + val.len > _.buf.len) {
                _.pos = saved_pos;
                return false;
            }
            if (val.len > 0) {
                @memcpy(_.buf[p .. p + val.len], val[0..val.len]);
                p += val.len;
            }
        }

        _.pos = p;
        _.count += 1;
        return true;
    }

    /// Writes a signed 64-bit integer field `key=val`.
    pub fn writeInt(*_, key: []const u8, val: i64) -> bool {
        let num_buf: [24]u8 = undefined;
        const nlen = writeI64(val, num_buf[0..24], 0);
        return _.writeStr(key, num_buf[0..nlen]);
    }

    /// Writes a boolean field `key=true` or `key=false`.
    pub fn writeBool(*_, key: []const u8, val: bool) -> bool {
        if (val) {
            return _.writeStr(key, "true");
        }
        return _.writeStr(key, "false");
    }

    /// Returns the formatted logfmt byte slice written so far.
    pub fn written() -> []const u8 {
        return _.buf[0.._.pos];
    }

    /// Returns the number of bytes written so far.
    pub fn len() -> usize {
        return _.pos;
    }
}
