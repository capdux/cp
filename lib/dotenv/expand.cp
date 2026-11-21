// Source: Absorbed from /data/dev/bun/src/env_loader.zig (POSIX Env Variable Template Expander)

/// 环境变量键值对（零拷贝切片）
export struct EnvPair {
    key: []const u8;
    value: []const u8;
}

fn strEql(a: []const u8, b: []const u8): bool {
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

fn isVarStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isVarContinue(c: u8): bool {
    return isVarStart(c) || (c >= '0' && c <= '9');
}

/// 在键值对数组中查找环境变量（同名后者覆盖前者）
export fn lookupEnv(pairs: []const EnvPair, key: []const u8): ([]const u8)? {
    let i: usize = pairs.len;
    while (i > 0) {
        i -= 1;
        if (strEql(pairs[i].key, key)) {
            return pairs[i].value;
        }
    }
    return null;
}

struct OutWriter {
    out: []u8;
    pos: usize;

    fn writeByte(*_, b: u8): bool {
        if (_.pos >= _.out.len) {
            return false;
        }
        _.out[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeSlice(*_, s: []const u8): bool {
        if (_.pos + s.len > _.out.len) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }
}

fn findMatchingBrace(s: []const u8, open_brace_idx: usize): usize? {
    let depth: usize = 1;
    let i: usize = open_brace_idx + 1;
    while (i < s.len) {
        if (s[i] == '\\' && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == '$' && i + 1 < s.len && s[i + 1] == '{') {
            depth += 1;
            i += 2;
            continue;
        }
        if (s[i] == '}') {
            depth -= 1;
            if (depth == 0) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn expandImpl(template: []const u8, pairs: []const EnvPair, w: *OutWriter, depth: u8): bool {
    if (depth > 8) {
        return false;
    }
    let i: usize = 0;
    while (i < template.len) {
        const c = template[i];

        // 1. \$ 转义为字面 $
        if (c == '\\' && i + 1 < template.len && template[i + 1] == '$') {
            if (!w.writeByte('$')) return false;
            i += 2;
            continue;
        }

        if (c != '$') {
            if (!w.writeByte(c)) return false;
            i += 1;
            continue;
        }

        // 末尾孤立 '$'
        if (i + 1 >= template.len) {
            if (!w.writeByte('$')) return false;
            i += 1;
            continue;
        }

        const nxt = template[i + 1];

        // 2. $$ 转义为字面 $
        if (nxt == '$') {
            if (!w.writeByte('$')) return false;
            i += 2;
            continue;
        }

        // 3. ${VAR}, ${VAR:-def}, ${VAR-def}, ${VAR:+alt}, ${VAR+alt}
        if (nxt == '{') {
            const close_idx = findMatchingBrace(template, i + 1) ?? return false;
            const inner = template[i + 2..close_idx];
            if (inner.len == 0 || !isVarStart(inner[0])) {
                return false;
            }
            let name_end: usize = 1;
            while (name_end < inner.len && isVarContinue(inner[name_end])) {
                name_end += 1;
            }
            const name = inner[0..name_end];
            const rest = inner[name_end..];
            const val_opt = lookupEnv(pairs, name);

            if (rest.len == 0) {
                if (v := val_opt) {
                    if (!w.writeSlice(v)) return false;
                }
            } else if (rest.len >= 2 && rest[0] == ':' && rest[1] == '-') {
                // ${VAR:-default}：未设置或为空时使用 default
                const def_expr = rest[2..];
                if (v := val_opt) {
                    if (v.len > 0) {
                        if (!w.writeSlice(v)) return false;
                    } else {
                        if (!expandImpl(def_expr, pairs, w, depth + 1)) return false;
                    }
                } else {
                    if (!expandImpl(def_expr, pairs, w, depth + 1)) return false;
                }
            } else if (rest[0] == '-') {
                // ${VAR-default}：仅未设置时使用 default
                const def_expr = rest[1..];
                if (v := val_opt) {
                    if (!w.writeSlice(v)) return false;
                } else {
                    if (!expandImpl(def_expr, pairs, w, depth + 1)) return false;
                }
            } else if (rest.len >= 2 && rest[0] == ':' && rest[1] == '+') {
                // ${VAR:+alt}：已设置且非空时使用 alt
                const alt_expr = rest[2..];
                if (v := val_opt) {
                    if (v.len > 0) {
                        if (!expandImpl(alt_expr, pairs, w, depth + 1)) return false;
                    }
                }
            } else if (rest[0] == '+') {
                // ${VAR+alt}：已设置（含空串）时使用 alt
                const alt_expr = rest[1..];
                if (val_opt != null) {
                    if (!expandImpl(alt_expr, pairs, w, depth + 1)) return false;
                }
            } else {
                return false;
            }

            i = close_idx + 1;
            continue;
        }

        // 4. 裸 $VAR
        if (isVarStart(nxt)) {
            let name_end: usize = i + 2;
            while (name_end < template.len && isVarContinue(template[name_end])) {
                name_end += 1;
            }
            const name = template[i + 1..name_end];
            if (v := lookupEnv(pairs, name)) {
                if (!w.writeSlice(v)) return false;
            }
            i = name_end;
            continue;
        }

        // 其他非变量起始字符后的 '$' 原样保留
        if (!w.writeByte('$')) return false;
        i += 1;
    }
    return true;
}

/// 按 POSIX Shell 风格展开环境变量模板并写入 out（零堆分配）
/// 未闭合 `${...`、非法表达式或缓冲区不足时返回 null
export fn expandEnv(template: []const u8, pairs: []const EnvPair, out: []u8): ([]const u8)? {
    let w = OutWriter{ out: out, pos: 0 };
    if (!expandImpl(template, pairs, &w, 0)) {
        return null;
    }
    return out[0..w.pos];
}

/// 判断模板中是否包含未转义的环境变量引用（`$VAR` 或 `${...}`）
export fn hasEnvRefs(template: []const u8): bool {
    let i: usize = 0;
    while (i < template.len) {
        const c = template[i];
        if (c == '\\' && i + 1 < template.len && template[i + 1] == '$') {
            i += 2;
            continue;
        }
        if (c == '$' && i + 1 < template.len) {
            const nxt = template[i + 1];
            if (nxt == '$') {
                i += 2;
                continue;
            }
            if (nxt == '{' || isVarStart(nxt)) {
                return true;
            }
        }
        i += 1;
    }
    return false;
}
