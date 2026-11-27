// @cap/toml-writer — Zero-heap TOML 1.0 Serializer & Formatter
// Clean-room MIT implementation in pure CP (inspired by Rust `toml_writer` [321M downloads]).

pub fn isBareKey(k: string) -> bool {
    if (k.len == 0) { return false; }
    let i: usize = 0;
    while (i < k.len) {
        const c: u8 = k[i];
        const ok: bool = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (!ok) { return false; }
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
        let i: usize = 0;
        while (i < s.len) {
            if (_.pos >= _.out.len) {
                _.overflow = true;
                return;
            }
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
    }

    pub fn writeQuotedString(*_, s: string) {
        _.putByte('"');
        let i: usize = 0;
        while (i < s.len) {
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
            } else if (c < 0x20 || c == 0x7F) {
                const hex: string = "0123456789ABCDEF";
                _.putRaw("\\u00");
                _.putByte(hex[((c >> 4) & 0x0F) as usize]);
                _.putByte(hex[(c & 0x0F) as usize]);
            } else {
                _.putByte(c);
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
