// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (W3C Permissions-Policy Header Parser)

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isFeatureChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '-'
        || c == '_';
}

fn isValidFeature(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    for (c of s) {
        if (!isFeatureChar(c)) {
            return false;
        }
    }
    return true;
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 在 allowlist 内部扫描是否包含指定的未加引号 token（如 `*` 或 `self`）
fn allowlistHasBareToken(raw: []const u8, target: []const u8): bool {
    let i: usize = 0;
    while (i < raw.len) {
        while (i < raw.len && isSpace(raw[i])) {
            i += 1;
        }
        if (i >= raw.len) {
            break;
        }
        if (raw[i] == '"') {
            // 跳过双引号字符串
            i += 1;
            while (i < raw.len) {
                if (raw[i] == '\\' && i + 1 < raw.len) {
                    i += 2;
                    continue;
                }
                if (raw[i] == '"') {
                    i += 1;
                    break;
                }
                i += 1;
            }
            continue;
        }
        const start = i;
        while (i < raw.len && !isSpace(raw[i]) && raw[i] != '"') {
            i += 1;
        }
        if (asciiEqlIgnoreCase(raw[start..i], target)) {
            return true;
        }
    }
    return false;
}

/// 在 allowlist 内部扫描是否包含双引号包裹的精确匹配 origin
fn allowlistHasQuotedOrigin(raw: []const u8, origin: []const u8): bool {
    let i: usize = 0;
    while (i < raw.len) {
        while (i < raw.len && isSpace(raw[i])) {
            i += 1;
        }
        if (i >= raw.len) {
            break;
        }
        if (raw[i] == '"') {
            i += 1;
            const start = i;
            let closed = false;
            while (i < raw.len) {
                if (raw[i] == '\\' && i + 1 < raw.len) {
                    i += 2;
                    continue;
                }
                if (raw[i] == '"') {
                    const val = raw[start..i];
                    i += 1;
                    closed = true;
                    if (strEql(val, origin)) {
                        return true;
                    }
                    break;
                }
                i += 1;
            }
            if (!closed) {
                break;
            }
        } else {
            while (i < raw.len && !isSpace(raw[i]) && raw[i] != '"') {
                i += 1;
            }
        }
    }
    return false;
}

/// 单条 W3C Permissions-Policy 特性指令（零拷贝切片）
export struct PermissionDirective {
    feature: []const u8;
    allowlist_raw: []const u8;
    is_wildcard: bool;

    /// 是否完全禁用该特性（空允许列表 `()` 且非 `*`）
    fn isDisabled(): bool {
        if (_.is_wildcard) {
            return false;
        }
        return trimAscii(_.allowlist_raw).len == 0;
    }

    /// 是否允许全部来源（`feature=*` 或允许列表内含 `*`）
    fn allowsAll(): bool {
        if (_.is_wildcard) {
            return true;
        }
        return allowlistHasBareToken(_.allowlist_raw, "*");
    }

    /// 是否允许当前同源（`self` 或 `*`）
    fn allowsSelf(): bool {
        if (_.allowsAll()) {
            return true;
        }
        return allowlistHasBareToken(_.allowlist_raw, "self");
    }

    /// 是否允许指定外部 origin（`*` 或引号包裹的精确匹配来源）
    fn allowsOrigin(origin: []const u8): bool {
        if (_.allowsAll()) {
            return true;
        }
        if (origin.len == 0) {
            return false;
        }
        return allowlistHasQuotedOrigin(_.allowlist_raw, origin);
    }
}

/// W3C Permissions-Policy 响应头逗号分隔流式解析器（零堆分配）
export struct PermissionsPolicyIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): PermissionsPolicyIter {
        return PermissionsPolicyIter{
            src: header,
            pos: 0,
        };
    }

    /// 读取下一条合法的 Permissions-Policy 特性指令
    fn next(*_): PermissionDirective? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            let in_quote = false;
            let escaped = false;
            let paren_depth: usize = 0;

            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_quote = false;
                    }
                } else {
                    if (c == '"') {
                        in_quote = true;
                    } else if (c == '(') {
                        paren_depth += 1;
                    } else if (c == ')' && paren_depth > 0) {
                        paren_depth -= 1;
                    } else if (c == ',' && paren_depth == 0) {
                        break;
                    }
                }
                _.pos += 1;
            }

            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0) {
                continue;
            }

            const eq_idx = findByte(seg, '=') ?? continue;
            const feature = trimAscii(seg[0..eq_idx]);
            if (!isValidFeature(feature)) {
                continue;
            }

            const rhs = trimAscii(seg[eq_idx + 1..]);
            if (rhs.len == 0) {
                continue;
            }

            // 1. 括号列表：feature=(...)
            if (rhs[0] == '(') {
                let close_idx: usize? = null;
                let k: usize = 1;
                let q = false;
                let esc = false;
                while (k < rhs.len) {
                    const ch = rhs[k];
                    if (q) {
                        if (esc) {
                            esc = false;
                        } else if (ch == '\\') {
                            esc = true;
                        } else if (ch == '"') {
                            q = false;
                        }
                    } else {
                        if (ch == '"') {
                            q = true;
                        } else if (ch == ')') {
                            close_idx = k;
                            break;
                        }
                    }
                    k += 1;
                }
                const ci = close_idx ?? continue;
                return PermissionDirective{
                    feature: feature,
                    allowlist_raw: trimAscii(rhs[1..ci]),
                    is_wildcard: false,
                };
            }

            // 2. 通配：feature=*
            if (rhs.len == 1 && rhs[0] == '*') {
                return PermissionDirective{
                    feature: feature,
                    allowlist_raw: "*",
                    is_wildcard: true,
                };
            }

            // 3. 单项形式：feature=self 或 feature="https://..."
            return PermissionDirective{
                feature: feature,
                allowlist_raw: rhs,
                is_wildcard: false,
            };
        }
        return null;
    }
}

/// 按特性名大小写不敏感查找 Permissions-Policy 指令
export fn findPermission(header: []const u8, feature: []const u8): PermissionDirective? {
    let it = PermissionsPolicyIter.init(header);
    while (true) {
        if (dir := it.next()) {
            if (asciiEqlIgnoreCase(dir.feature, feature)) {
                return dir;
            }
        } else {
            break;
        }
    }
    return null;
}
