// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc BibTeX Citation Parser & Writer)

/// BibTeX 文献条目（零拷贝切片指向原始 `.bib` 文本）
pub struct BibEntry {
    entry_type: []const u8,
    cite_key: []const u8,
    body: []const u8,
}

/// BibTeX 字段键值对（`value` 已剥离外层 `{...}` 或 `"..."`）
pub struct BibField {
    name: []const u8,
    value: []const u8,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '_' || c == '-' || c == ':' || c == '.';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn skipWsAndComments(s: []const u8, start: usize): usize {
    let i = start;
    while (i < s.len) {
        if (isSpace(s[i])) {
            i += 1;
        } else if (s[i] == '%') {
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
        } else {
            break;
        }
    }
    return i;
}

/// 从开分隔符 `open_pos`（`'{'` 或 `'('`）起，平衡扫描到匹配的闭分隔符位置
fn findMatchingDelim(s: []const u8, open_pos: usize, open_ch: u8, close_ch: u8): usize? {
    let i = open_pos + 1;
    let brace_depth: usize = if (open_ch == '{') 1 else 0;
    let paren_depth: usize = if (open_ch == '(') 1 else 0;
    let in_quotes = false;

    while (i < s.len) {
        const c = s[i];
        if (in_quotes) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == '{') {
                brace_depth += 1;
            } else if (c == '}' && brace_depth > 0) {
                brace_depth -= 1;
            } else if (c == '"') {
                if ((open_ch == '{' && brace_depth == 1) || (open_ch == '(' && brace_depth == 0)) {
                    in_quotes = false;
                }
            }
            i += 1;
            continue;
        }

        if (c == '"') {
            if ((open_ch == '{' && brace_depth == 1) || (open_ch == '(' && brace_depth == 0)) {
                in_quotes = true;
            }
            i += 1;
            continue;
        }

        if (c == '{') {
            brace_depth += 1;
        } else if (c == '}') {
            if (brace_depth == 0) return null;
            brace_depth -= 1;
            if (open_ch == '{' && close_ch == '}' && brace_depth == 0) {
                return i;
            }
        } else if (open_ch == '(' && brace_depth == 0) {
            if (c == '(') {
                paren_depth += 1;
            } else if (c == ')') {
                paren_depth -= 1;
                if (paren_depth == 0) {
                    return i;
                }
            }
        }
        i += 1;
    }
    return null;
}

fn isMetaEntryType(entry_type: []const u8): bool {
    return eqlIgnoreCaseAscii(entry_type, "comment")
        || eqlIgnoreCaseAscii(entry_type, "preamble")
        || eqlIgnoreCaseAscii(entry_type, "string");
}

/// 零堆分配 BibTeX 文献条目流式迭代器（自动跳过 `%` 注释与 `@comment`/`@preamble`/`@string`）
pub struct BibEntryIter {
    input: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): BibEntryIter {
        return BibEntryIter {
            input: input,
            pos: 0,
        };
    }

    /// 读取下一条合法 `BibEntry`；遍历结束返回 `null`
    pub fn next(*_): BibEntry? {
        while (_.pos < _.input.len) {
            _.pos = skipWsAndComments(_.input, _.pos);
            if (_.pos >= _.input.len) {
                return null;
            }
            if (_.input[_.pos] != '@') {
                _.pos += 1;
                continue;
            }
            _.pos += 1; // skip '@'
            const type_start = _.pos;
            while (_.pos < _.input.len && isIdentChar(_.input[_.pos])) {
                _.pos += 1;
            }
            const entry_type = _.input[type_start.._.pos];
            if (entry_type.len == 0) {
                continue;
            }

            _.pos = skipWsAndComments(_.input, _.pos);
            if (_.pos >= _.input.len) {
                return null;
            }
            const open_ch = _.input[_.pos];
            if (open_ch != '{' && open_ch != '(') {
                continue;
            }
            const close_ch: u8 = if (open_ch == '{') '}' else ')';
            const close_opt = findMatchingDelim(_.input, _.pos, open_ch, close_ch);
            if (close_opt == null) {
                _.pos = _.input.len;
                return null;
            }
            const close_pos = close_opt.?;
            const inner = _.input[_.pos + 1 .. close_pos];
            _.pos = close_pos + 1;

            if (isMetaEntryType(entry_type)) {
                continue;
            }

            // Split cite_key and field body at first ','
            let comma_pos: usize = 0;
            let found_comma = false;
            let k: usize = 0;
            while (k < inner.len) {
                if (inner[k] == ',') {
                    comma_pos = k;
                    found_comma = true;
                    break;
                }
                k += 1;
            }
            if (!found_comma) {
                continue;
            }
            const cite_key = trimAscii(inner[0..comma_pos]);
            if (cite_key.len == 0) {
                continue;
            }
            const body = trimAscii(inner[comma_pos + 1 ..]);
            return BibEntry {
                entry_type: entry_type,
                cite_key: cite_key,
                body: body,
            };
        }
        return null;
    }
}

/// 零堆分配 BibTeX 字段体流式迭代器（支持 `{val}`、`"val"` 与裸数字/宏原子值）
pub struct BibFieldIter {
    body: []const u8,
    pos: usize = 0,

    pub static fn init(body: []const u8): BibFieldIter {
        return BibFieldIter {
            body: body,
            pos: 0,
        };
    }

    /// 读取下一组 `BibField`；遍历结束返回 `null`
    pub fn next(*_): BibField? {
        while (_.pos < _.body.len) {
            _.pos = skipWsAndComments(_.body, _.pos);
            if (_.pos < _.body.len && _.body[_.pos] == ',') {
                _.pos += 1;
                continue;
            }
            break;
        }
        if (_.pos >= _.body.len) {
            return null;
        }

        const name_start = _.pos;
        while (_.pos < _.body.len && isIdentChar(_.body[_.pos])) {
            _.pos += 1;
        }
        const name = _.body[name_start.._.pos];
        if (name.len == 0) {
            _.pos = _.body.len;
            return null;
        }

        _.pos = skipWsAndComments(_.body, _.pos);
        if (_.pos >= _.body.len || _.body[_.pos] != '=') {
            _.pos = _.body.len;
            return null;
        }
        _.pos += 1; // skip '='
        _.pos = skipWsAndComments(_.body, _.pos);
        if (_.pos >= _.body.len) {
            return null;
        }

        let value: []const u8 = "";
        const ch = _.body[_.pos];
        if (ch == '{') {
            const close_opt = findMatchingDelim(_.body, _.pos, '{', '}');
            if (close_opt == null) {
                _.pos = _.body.len;
                return null;
            }
            const close_pos = close_opt.?;
            value = _.body[_.pos + 1 .. close_pos];
            _.pos = close_pos + 1;
        } else if (ch == '"') {
            const val_start = _.pos + 1;
            let i = val_start;
            let depth: usize = 0;
            let closed = false;
            while (i < _.body.len) {
                const c = _.body[i];
                if (c == '\\' && i + 1 < _.body.len) {
                    i += 2;
                    continue;
                }
                if (c == '{') {
                    depth += 1;
                } else if (c == '}' && depth > 0) {
                    depth -= 1;
                } else if (c == '"' && depth == 0) {
                    value = _.body[val_start..i];
                    _.pos = i + 1;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) {
                _.pos = _.body.len;
                return null;
            }
        } else {
            const val_start = _.pos;
            while (_.pos < _.body.len && _.body[_.pos] != ',' && _.body[_.pos] != '%' && _.body[_.pos] != '\n' && _.body[_.pos] != '\r') {
                _.pos += 1;
            }
            value = trimAscii(_.body[val_start.._.pos]);
        }

        // Skip optional trailing comma
        _.pos = skipWsAndComments(_.body, _.pos);
        if (_.pos < _.body.len && _.body[_.pos] == ',') {
            _.pos += 1;
        }

        return BibField {
            name: name,
            value: value,
        };
    }
}

/// 按字段名（ASCII 大小写不敏感）查找条目字段值
pub fn findBibField(entry: BibEntry, field_name: []const u8): ([]const u8)? {
    let it = BibFieldIter.init(entry.body);
    while (f := it.next()) {
        if (eqlIgnoreCaseAscii(f.name, field_name)) {
            return f.value;
        }
    }
    return null;
}

fn appendBytes(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

/// 规范化输出 BibTeX 字段行 `"  name = {value},\n"`；缓冲不足返回 `null`
pub fn formatBibField(name: []const u8, value: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (!appendBytes(out, &pos, "  ")) return null;
    if (!appendBytes(out, &pos, name)) return null;
    if (!appendBytes(out, &pos, " = {")) return null;
    if (!appendBytes(out, &pos, value)) return null;
    if (!appendBytes(out, &pos, "},\n")) return null;
    return out[0..pos];
}
