// Source: Absorbed from /data/dev/bun/src/ini.zig | Synced: 2026-11-21T15:58:36Z (Java .properties Zero-Alloc Parser)

pub struct PropertyEntry {
    key: []const u8;
    value: []const u8;
    line_no: usize;
}

fn isPropWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\f';
}

fn isLineBreak(c: u8): bool {
    return c == '\n' || c == '\r';
}

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn parseHex4(src: []const u8, pos: usize): u16? {
    if (pos + 4 > src.len) {
        return null;
    }
    const h0 = hexNibble(src[pos]) ?? return null;
    const h1 = hexNibble(src[pos + 1]) ?? return null;
    const h2 = hexNibble(src[pos + 2]) ?? return null;
    const h3 = hexNibble(src[pos + 3]) ?? return null;
    return (u16(h0) << 12) | (u16(h1) << 8) | (u16(h2) << 4) | u16(h3);
}

fn encodeUtf8(cp: u32, out: []u8): usize {
    if (cp <= 0x7F) {
        out[0] = cp as u8;
        return 1;
    }
    if (cp <= 0x7FF) {
        out[0] = (0xC0 | (cp >> 6)) as u8;
        out[1] = (0x80 | (cp & 0x3F)) as u8;
        return 2;
    }
    if (cp <= 0xFFFF) {
        out[0] = (0xE0 | (cp >> 12)) as u8;
        out[1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[2] = (0x80 | (cp & 0x3F)) as u8;
        return 3;
    }
    out[0] = (0xF0 | (cp >> 18)) as u8;
    out[1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
    out[2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
    out[3] = (0x80 | (cp & 0x3F)) as u8;
    return 4;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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

pub struct PropertiesIter {
    src: []const u8;
    pos: usize;
    line_no: usize;

    static fn init(content: []const u8): PropertiesIter {
        let start: usize = 0;
        if (content.len >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF) {
            start = 3;
        }
        return PropertiesIter {
            src: content,
            pos: start,
            line_no: 1,
        };
    }

    fn consumeLineBreak(*_): bool {
        if (_.pos >= _.src.len) {
            return false;
        }
        if (_.src[_.pos] == '\r') {
            _.pos += 1;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            _.line_no += 1;
            return true;
        }
        if (_.src[_.pos] == '\n') {
            _.pos += 1;
            _.line_no += 1;
            return true;
        }
        return false;
    }

    fn skipPropWhitespace(*_) {
        while (_.pos < _.src.len && isPropWhitespace(_.src[_.pos])) {
            _.pos += 1;
        }
    }

    fn skipPhysicalLine(*_) {
        while (_.pos < _.src.len && !isLineBreak(_.src[_.pos])) {
            _.pos += 1;
        }
        * = _.consumeLineBreak();
    }

    // 解码已消费 '\' 后的转义序列到 tmp[0..4]，返回写入字节数；非法转义返回 null
    fn decodeEscape(*_, tmp: []u8): usize? {
        if (_.pos >= _.src.len) {
            return 0;
        }
        const c = _.src[_.pos];
        _.pos += 1;

        if (c == 'n') {
            tmp[0] = '\n';
            return 1;
        }
        if (c == 'r') {
            tmp[0] = '\r';
            return 1;
        }
        if (c == 't') {
            tmp[0] = '\t';
            return 1;
        }
        if (c == 'f') {
            tmp[0] = '\f';
            return 1;
        }
        if (c == '\\') {
            tmp[0] = '\\';
            return 1;
        }
        if (c == 'u') {
            const unit1 = parseHex4(_.src, _.pos) ?? return null;
            _.pos += 4;
            if (unit1 >= 0xD800 && unit1 <= 0xDBFF) {
                // 高代理项必须紧跟 \uDC00..\uDFFF 低代理项
                if (_.pos + 6 > _.src.len || _.src[_.pos] != '\\' || _.src[_.pos + 1] != 'u') {
                    return null;
                }
                const unit2 = parseHex4(_.src, _.pos + 2) ?? return null;
                if (unit2 < 0xDC00 || unit2 > 0xDFFF) {
                    return null;
                }
                _.pos += 6;
                const hi: u32 = u32(unit1 - 0xD800);
                const lo: u32 = u32(unit2 - 0xDC00);
                const base: u32 = 0x10000;
                const cp: u32 = base + (hi << 10) + lo;
                return encodeUtf8(cp, tmp);
            }
            if (unit1 >= 0xDC00 && unit1 <= 0xDFFF) {
                // 孤立低代理项非法
                return null;
            }
            return encodeUtf8(u32(unit1), tmp);
        }
        // Java .properties 规范：其他 \c（如 \= \: \ \# \!）产出字面 c
        tmp[0] = c;
        return 1;
    }

    fn finishLogicalLine(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '\\') {
                _.pos += 1;
                if (_.consumeLineBreak()) {
                    _.skipPropWhitespace();
                    continue;
                }
                if (_.pos < _.src.len) {
                    _.pos += 1;
                }
                continue;
            }
            if (_.consumeLineBreak()) {
                return;
            }
            _.pos += 1;
        }
    }

    fn parseKey(*_, key_buf: []u8): usize? {
        let out_len: usize = 0;
        let overflow = false;
        let bad_escape = false;
        let tmp: [4]u8 = [0; 4];

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '=' || c == ':' || isPropWhitespace(c) || isLineBreak(c)) {
                break;
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.consumeLineBreak()) {
                    _.skipPropWhitespace();
                    continue;
                }
                if (n := _.decodeEscape(&tmp)) {
                    if (out_len + n <= key_buf.len) {
                        let k: usize = 0;
                        while (k < n) {
                            key_buf[out_len + k] = tmp[k];
                            k += 1;
                        }
                        out_len += n;
                    } else {
                        overflow = true;
                    }
                } else {
                    bad_escape = true;
                }
                continue;
            }
            _.pos += 1;
            if (out_len < key_buf.len) {
                key_buf[out_len] = c;
                out_len += 1;
            } else {
                overflow = true;
            }
        }

        if (overflow || bad_escape) {
            _.finishLogicalLine();
            return null;
        }
        return out_len;
    }

    fn parseValue(*_, val_buf: []u8, write_out: bool): usize? {
        let out_len: usize = 0;
        let overflow = false;
        let bad_escape = false;
        let tmp: [4]u8 = [0; 4];

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (_.consumeLineBreak()) {
                break;
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.consumeLineBreak()) {
                    _.skipPropWhitespace();
                    continue;
                }
                if (_.pos >= _.src.len) {
                    break;
                }
                if (n := _.decodeEscape(&tmp)) {
                    if (write_out) {
                        if (out_len + n <= val_buf.len) {
                            let k: usize = 0;
                            while (k < n) {
                                val_buf[out_len + k] = tmp[k];
                                k += 1;
                            }
                            out_len += n;
                        } else {
                            overflow = true;
                        }
                    }
                } else {
                    bad_escape = true;
                }
                continue;
            }
            _.pos += 1;
            if (write_out) {
                if (out_len < val_buf.len) {
                    val_buf[out_len] = c;
                    out_len += 1;
                } else {
                    overflow = true;
                }
            }
        }

        if (overflow || bad_escape) {
            return null;
        }
        return out_len;
    }

    fn skipSeparator(*_) {
        _.skipPropWhitespace();
        if (_.pos < _.src.len && (_.src[_.pos] == '=' || _.src[_.pos] == ':')) {
            _.pos += 1;
            _.skipPropWhitespace();
        }
    }

    fn next(*_, key_buf: []u8, val_buf: []u8): PropertyEntry? {
        while (_.pos < _.src.len) {
            _.skipPropWhitespace();
            if (_.pos >= _.src.len) {
                return null;
            }
            if (_.consumeLineBreak()) {
                continue;
            }
            const first = _.src[_.pos];
            if (first == '#' || first == '!') {
                _.skipPhysicalLine();
                continue;
            }

            const stmt_line = _.line_no;
            const key_len = _.parseKey(key_buf) ?? continue;
            _.skipSeparator();
            const val_len = _.parseValue(val_buf, true) ?? continue;

            return PropertyEntry {
                key: key_buf[0..key_len],
                value: val_buf[0..val_len],
                line_no: stmt_line,
            };
        }
        return null;
    }
}

pub fn getProperty(content: []const u8, target_key: []const u8, val_buf: []u8): ([]const u8)? {
    let it = PropertiesIter.init(content);
    let key_scratch: [512]u8 = [0; 512];
    let found = false;
    let found_len: usize = 0;

    while (it.pos < it.src.len) {
        it.skipPropWhitespace();
        if (it.pos >= it.src.len) {
            break;
        }
        if (it.consumeLineBreak()) {
            continue;
        }
        const first = it.src[it.pos];
        if (first == '#' || first == '!') {
            it.skipPhysicalLine();
            continue;
        }

        const key_len = it.parseKey(&key_scratch) ?? continue;
        it.skipSeparator();

        if (bytesEql(key_scratch[0..key_len], target_key)) {
            if (val_len := it.parseValue(val_buf, true)) {
                found = true;
                found_len = val_len;
            } else {
                found = false;
                found_len = 0;
            }
        } else {
            * = it.parseValue(val_buf, false);
        }
    }

    if (!found) {
        return null;
    }
    return val_buf[0..found_len];
}
