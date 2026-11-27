// @cap/toml-writer — Zero-heap TOML 1.0 Serializer & Formatter
// Clean-room MIT implementation in pure CP (inspired by Rust `toml_writer` [321M downloads]).

import { findByteInBytes } from "cap:bin";

type TwVec16 = @Vector(16, u8);

fn twBuildBareKeyLut() -> [256]u8 {
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        const b: u8 = c as u8;
        if ((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') ||
            (b >= '0' && b <= '9') || b == '_' || b == '-') {
            lut[c] = 1;
        }
        c += 1;
    }
    return lut;
}

const TW_BARE_KEY_LUT: [256]u8 = twBuildBareKeyLut();

fn twFindCtrlByte(s: string, start: usize, limit: usize) -> usize {
    let i: usize = start;
    const v_20: TwVec16 = @splat(0x20 as u8);
    const v_7f: TwVec16 = @splat(0x7F as u8);
    while (i + 16 <= limit) {
        const v: TwVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v < v_20) || @reduce(.Or, v == v_7f)) {
            let j: usize = 0;
            while (j < 16) {
                const c = s[i + j];
                if (c < 0x20 || c == 0x7F) {
                    return i + j;
                }
                j += 1;
            }
        }
        i += 16;
    }
    while (i < limit) {
        const c = s[i];
        if (c < 0x20 || c == 0x7F) {
            return i;
        }
        i += 1;
    }
    return limit;
}

pub fn isBareKey(k: string) -> bool {
    if (k.len == 0) { return false; }
    let i: usize = 0;
    while (i < k.len) {
        if (TW_BARE_KEY_LUT[k[i] as usize] == 0) { return false; }
        i += 1;
    }
    return true;
}

pub struct TomlWriter {
    pub out: []u8,
    pub pos: usize,
    pub overflow: bool,

    pub static fn init(out: []u8) -> TomlWriter {
        return TomlWriter { out: out, pos: 0, overflow: false };
    }

    pub fn asSlice() -> string {
        return _.out[0.._.pos];
    }

    pub fn putByte(*_, b: u8) {
        if (_.pos >= _.out.len) {
            _.overflow = true;
            return;
        }
        _.out[_.pos] = b;
        _.pos += 1;
    }

    pub fn putRaw(*_, s: string) {
        if (s.len == 0) { return; }
        if (_.pos >= _.out.len) {
            _.overflow = true;
            return;
        }
        const avail = _.out.len - _.pos;
        const n = if (s.len <= avail) { s.len } else { avail };
        if (n < s.len) {
            _.overflow = true;
        }
        @memcpy(_.out[_.pos .. _.pos + n], s[0..n]);
        _.pos += n;
    }

    pub fn writeQuotedString(*_, s: string) {
        _.putByte('"');
        let i: usize = 0;
        let next_q = findByteInBytes(s, '"', 0) ?? s.len;
        let next_bs = findByteInBytes(s, '\\', 0) ?? s.len;
        while (i < s.len) {
            if (next_q < i) {
                next_q = findByteInBytes(s, '"', i) ?? s.len;
            }
            if (next_bs < i) {
                next_bs = findByteInBytes(s, '\\', i) ?? s.len;
            }
            const limit = if (next_q < next_bs) { next_q } else { next_bs };
            const seg_end = twFindCtrlByte(s, i, limit);
            if (seg_end > i) {
                _.putRaw(s[i..seg_end]);
                i = seg_end;
                if (i >= s.len) { break; }
            }
            const c: u8 = s[i];
            if (c == '"') {
                _.putRaw("\\\"");
            } else if (c == '\\') {
                _.putRaw("\\\\");
            } else if (c == '\n') {
                _.putRaw("\\n");
            } else if (c == '\r') {
                _.putRaw("\\r");
            } else if (c == '\t') {
                _.putRaw("\\t");
            } else if (c == 0x08) {
                _.putRaw("\\b");
            } else if (c == 0x0C) {
                _.putRaw("\\f");
            } else {
                const hex: string = "0123456789ABCDEF";
                _.putRaw("\\u00");
                _.putByte(hex[((c >> 4) & 0x0F) as usize]);
                _.putByte(hex[(c & 0x0F) as usize]);
            }
            i += 1;
        }
        _.putByte('"');
    }

    pub fn writeKey(*_, k: string) {
        if (isBareKey(k)) {
            _.putRaw(k);
        } else {
            _.writeQuotedString(k);
        }
    }

    pub fn writeI64(*_, val: i64) {
        if (val == 0) {
            _.putByte('0');
            return;
        }
        let v: i64 = val;
        if (v < 0) {
            _.putByte('-');
            if (v == -9223372036854775807 - 1) {
                _.putRaw("9223372036854775808");
                return;
            }
            v = -v;
        }
        let tmp: [24]u8 = undefined;
        let n: usize = 0;
        while (v > 0 && n < 24) {
            tmp[n] = ('0' as u8) + (@rem(v, 10) as u8);
            v = @divTrunc(v, 10);
            n += 1;
        }
        while (n > 0) {
            n -= 1;
            _.putByte(tmp[n]);
        }
    }

    pub fn writeBool(*_, b: bool) {
        _.putRaw(if (b) "true" else "false");
    }

    /// Write a `# comment` line.
    pub fn writeComment(*_, text: string) {
        _.putRaw("# ");
        _.putRaw(text);
        _.putByte('\n');
    }

    /// Write a `[table]` header.
    pub fn writeTableHeader(*_, name: string) {
        if (_.pos > 0) {
            _.putByte('\n');
        }
        _.putByte('[');
        _.writeKey(name);
        _.putRaw("]\n");
    }

    /// Write a `[[array_of_tables]]` header.
    pub fn writeArrayTableHeader(*_, name: string) {
        if (_.pos > 0) {
            _.putByte('\n');
        }
        _.putRaw("[[");
        _.writeKey(name);
        _.putRaw("]]\n");
    }

    /// Write `key = "escaped string"\n`
    pub fn writeKvStr(*_, key: string, val: string) {
        _.writeKey(key);
        _.putRaw(" = ");
        _.writeQuotedString(val);
        _.putByte('\n');
    }

    /// Write `key = 123\n`
    pub fn writeKvInt(*_, key: string, val: i64) {
        _.writeKey(key);
        _.putRaw(" = ");
        _.writeI64(val);
        _.putByte('\n');
    }

    /// Write `key = true\n`
    pub fn writeKvBool(*_, key: string, val: bool) {
        _.writeKey(key);
        _.putRaw(" = ");
        _.writeBool(val);
        _.putByte('\n');
    }

    /// Write `key = ["a", "b", "c"]\n`
    pub fn writeKvStrArray(*_, key: string, items: []const string) {
        _.writeKey(key);
        _.putRaw(" = [");
        let i: usize = 0;
        while (i < items.len) {
            if (i > 0) { _.putRaw(", "); }
            _.writeQuotedString(items[i]);
            i += 1;
        }
        _.putRaw("]\n");
    }
}
