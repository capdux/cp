// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (HTTP Cache Key Normalizer & Builder)

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isCtrlOrSpace(c: u8): bool {
    return c <= 0x20 || c == 0x7F;
}

fn hasLineBreak(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '\r' || s[i] == '\n') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn hasCtrlOrSpace(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (isCtrlOrSpace(s[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
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

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (toLowerAscii(s[i]) != toLowerAscii(prefix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn endsWithBytes(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) {
        return false;
    }
    const offset = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[offset + i] != suffix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeLower(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = toLowerAscii(s[i]);
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeUpper(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = toUpperAscii(s[i]);
        i += 1;
    }
    pos.* += s.len;
    return true;
}

/// 大小写不敏感识别常见追踪查询参数：`utm_*`、`fbclid`、`gclid`、`mc_cid`、`mc_eid`、`ref_src`
export fn isTrackingQueryParam(key: []const u8): bool {
    if (key.len == 0) {
        return false;
    }
    if (startsWithIgnoreCase(key, "utm_")) {
        return true;
    }
    return eqlIgnoreCase(key, "fbclid")
        || eqlIgnoreCase(key, "gclid")
        || eqlIgnoreCase(key, "mc_cid")
        || eqlIgnoreCase(key, "mc_eid")
        || eqlIgnoreCase(key, "ref_src");
}

/// 过滤掉追踪参数与空段，保留其余参数原序并去除首部 `?`（零堆分配）
export fn normalizeQueryWithoutTracking(query: []const u8, out: []u8): ([]const u8)? {
    let q = query;
    if (q.len > 0 && q[0] == '?') {
        q = q[1..];
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < q.len) {
        const seg_start = i;
        while (i < q.len && q[i] != '&') {
            i += 1;
        }
        const part = q[seg_start..i];
        if (i < q.len && q[i] == '&') {
            i += 1;
        }
        if (part.len == 0) {
            continue;
        }

        // 提取参数名（首个 '=' 之前，无 '=' 则整段为 key）
        let eq_pos: usize = 0;
        while (eq_pos < part.len && part[eq_pos] != '=') {
            eq_pos += 1;
        }
        const key = part[0..eq_pos];
        if (isTrackingQueryParam(key)) {
            continue;
        }

        if (pos > 0) {
            if (!writeByte(out, &pos, '&')) {
                return null;
            }
        }
        if (!writeSlice(out, &pos, part)) {
            return null;
        }
    }

    return out[0..pos];
}

/// 构造规范化 HTTP 缓存键：
/// - 方法转大写、scheme/host 转小写
/// - 去除默认端口（http `:80`、https `:443`）
/// - 空路径归约为 `/`（非 `/` 开头自动补 `/`）
/// - 按需剥离追踪查询参数与空段
/// 输出示例：`"GET https://example.com/api?id=1"`
export fn buildCacheKey(
    method: []const u8,
    scheme: []const u8,
    host: []const u8,
    path: []const u8,
    query: []const u8,
    strip_tracking: bool,
    out: []u8,
): ([]const u8)? {
    const m = trimAscii(method);
    const s = trimAscii(scheme);
    let h = trimAscii(host);

    if (m.len == 0 || s.len == 0 || h.len == 0) {
        return null;
    }
    if (hasCtrlOrSpace(m) || hasCtrlOrSpace(s) || hasCtrlOrSpace(h)) {
        return null;
    }
    if (hasLineBreak(path) || hasLineBreak(query)) {
        return null;
    }

    // 剥离协议默认端口（保留非默认端口与跨协议端口）
    if (eqlIgnoreCase(s, "http") && endsWithBytes(h, ":80") && h.len > 3) {
        h = h[0..(h.len - 3)];
    } else if (eqlIgnoreCase(s, "https") && endsWithBytes(h, ":443") && h.len > 4) {
        h = h[0..(h.len - 4)];
    }

    let pos: usize = 0;

    // 1. METHOD + " "
    if (!writeUpper(out, &pos, m)) {
        return null;
    }
    if (!writeByte(out, &pos, ' ')) {
        return null;
    }

    // 2. scheme + "://" + host
    if (!writeLower(out, &pos, s)) {
        return null;
    }
    if (!writeSlice(out, &pos, "://")) {
        return null;
    }
    if (!writeLower(out, &pos, h)) {
        return null;
    }

    // 3. path（空路径归约 "/"，缺少前导 "/" 时自动补齐）
    if (path.len == 0) {
        if (!writeByte(out, &pos, '/')) {
            return null;
        }
    } else {
        if (path[0] != '/') {
            if (!writeByte(out, &pos, '/')) {
                return null;
            }
        }
        if (!writeSlice(out, &pos, path)) {
            return null;
        }
    }

    // 4. query（去除首部 '?'，按需过滤追踪参数）
    let q = query;
    if (q.len > 0 && q[0] == '?') {
        q = q[1..];
    }
    if (q.len > 0) {
        if (strip_tracking) {
            let i: usize = 0;
            let wrote_any = false;
            while (i < q.len) {
                const seg_start = i;
                while (i < q.len && q[i] != '&') {
                    i += 1;
                }
                const part = q[seg_start..i];
                if (i < q.len && q[i] == '&') {
                    i += 1;
                }
                if (part.len == 0) {
                    continue;
                }

                let eq_pos: usize = 0;
                while (eq_pos < part.len && part[eq_pos] != '=') {
                    eq_pos += 1;
                }
                if (isTrackingQueryParam(part[0..eq_pos])) {
                    continue;
                }

                if (!wrote_any) {
                    if (!writeByte(out, &pos, '?')) {
                        return null;
                    }
                    wrote_any = true;
                } else {
                    if (!writeByte(out, &pos, '&')) {
                        return null;
                    }
                }
                if (!writeSlice(out, &pos, part)) {
                    return null;
                }
            }
        } else {
            if (!writeByte(out, &pos, '?')) {
                return null;
            }
            if (!writeSlice(out, &pos, q)) {
                return null;
            }
        }
    }

    return out[0..pos];
}

/// 以 `\n<lower_name>:<trimmed_val>` 格式向缓存键尾部追加 Vary 维度（原子写入，缓冲不足时保持 `pos` 不变）
export fn appendVaryDimension(
    out: []u8,
    pos: *usize,
    header_name: []const u8,
    header_val: []const u8,
): bool {
    if (pos.* > out.len) {
        return false;
    }

    const name = trimAscii(header_name);
    if (name.len == 0 || hasCtrlOrSpace(name)) {
        return false;
    }
    let ni: usize = 0;
    while (ni < name.len) {
        if (name[ni] == ':') {
            return false;
        }
        ni += 1;
    }

    const val = trimAscii(header_val);
    if (hasLineBreak(val)) {
        return false;
    }

    const needed = 1 + name.len + 1 + val.len;
    if (out.len - pos.* < needed) {
        return false;
    }

    let cur = pos.*;
    out[cur] = '\n';
    cur += 1;

    let i: usize = 0;
    while (i < name.len) {
        out[cur + i] = toLowerAscii(name[i]);
        i += 1;
    }
    cur += name.len;

    out[cur] = ':';
    cur += 1;

    let j: usize = 0;
    while (j < val.len) {
        out[cur + j] = val[j];
        j += 1;
    }
    cur += val.len;

    pos.* = cur;
    return true;
}
