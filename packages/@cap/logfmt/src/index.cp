// @cap/logfmt — Zero-allocation logfmt Structured Key=Value Log Parser & Writer
// Clean-room MIT implementation in pure CP.

pub const MAX_FIELDS: usize = 64;
const KEY_BUF_CAP: usize = 512;
const VAL_BUF_CAP: usize = 1024;

pub struct LogfmtField {
    pub key_start: usize,
    pub key_len: usize,
    pub val_start: usize,
    pub val_len: usize,
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9 || b == 10 || b == 13;
}

fn bytesEq(a: []const u8, b: []const u8) -> bool {
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
            while (i < n && isSpace(line[i])) {
                i += 1;
            }
            if (i >= n) {
                break;
            }

            // Read key until '=', whitespace, or '"'
            const k_start_in_line = i;
            while (i < n && !isSpace(line[i]) && line[i] != 61 && line[i] != 34) {
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
            let kc: usize = 0;
            while (kc < k_len) {
                key_buf[key_used] = line[k_start_in_line + kc];
                key_used += 1;
                kc += 1;
            }

            const v_slot = val_used;
            let v_len: usize = 0;

            if (i < n && line[i] == 61) {
                // '=' present
                i += 1;
                if (i < n && !isSpace(line[i])) {
                    if (line[i] == 34) {
                        // Quoted string value
                        i += 1;
                        while (i < n && line[i] != 34) {
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
                        while (i < n && !isSpace(line[i])) {
                            if (val_used < VAL_BUF_CAP) {
                                val_buf[val_used] = line[i];
                                val_used += 1;
                                v_len += 1;
                            }
                            i += 1;
                        }
                    }
                }
            } else {
                // Bare flag key -> default value "true"
                const tstr = "true";
                if (val_used + 4 <= VAL_BUF_CAP) {
                    let tc: usize = 0;
                    while (tc < 4) {
                        val_buf[val_used] = tstr[tc];
                        val_used += 1;
                        tc += 1;
                    }
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
    while (i < val.len) {
        const b = val[i];
        if (b <= 32 || b == 61 || b == 34 || b == 92) {
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
        let k: usize = 0;
        while (k < key.len) {
            _.buf[p] = key[k];
            p += 1;
            k += 1;
        }
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
            let i: usize = 0;
            while (i < val.len) {
                _.buf[p] = val[i];
                p += 1;
                i += 1;
            }
        }

        _.pos = p;
        _.count += 1;
        return true;
    }

    /// Writes a signed 64-bit integer field `key=val`.
    pub fn writeInt(*_, key: []const u8, val: i64) -> bool {
        let num_buf: [24]u8 = undefined;
        let nlen: usize = 0;
        let mag: u64 = 0;
        let neg = false;

        if (val < 0) {
            neg = true;
            // Safe magnitude conversion without signed overflow
            const bits: u64 = @bitCast(val);
            mag = (0 as u64) -% bits;
        } else {
            mag = val as u64;
        }

        let rev: [20]u8 = undefined;
        let rlen: usize = 0;
        if (mag == 0) {
            rev[0] = 48;
            rlen = 1;
        } else {
            while (mag > 0) {
                rev[rlen] = (48 + (mag % 10)) as u8;
                mag = mag / 10;
                rlen += 1;
            }
        }

        if (neg) {
            num_buf[nlen] = 45; // '-'
            nlen += 1;
        }
        let k: usize = 0;
        while (k < rlen) {
            num_buf[nlen] = rev[rlen - 1 - k];
            nlen += 1;
            k += 1;
        }

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
