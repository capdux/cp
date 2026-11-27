// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (HTTP Cache Key Normalizer & Builder)

type CkVec16 = @Vector(16, u8);

fn makeCkLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

fn makeCkUpperLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'a' && i <= 'z') {
            lut[i] = @as(u8, @intCast(i - 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

const CK_LOWER_LUT: [u8; 256] = makeCkLowerLut();
const CK_UPPER_LUT: [u8; 256] = makeCkUpperLut();

fn ckFindByte(s: []const u8, start: usize, ch: u8): usize {
    let i: usize = start;
    const v_ch: CkVec16 = @splat(ch);
    while (i + 16 <= s.len) : (i += 16) {
        const v: CkVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_ch)) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return s.len;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isCtrlOrSpace(c: u8): bool {
    return c <= 0x20 || c == 0x7F;
}

fn hasLineBreak(s: []const u8): bool {
    let i: usize = 0;
    const v_cr: CkVec16 = @splat('\r' as u8);
    const v_lf: CkVec16 = @splat('\n' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: CkVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v_cr) | (v == v_lf))) return true;
    }
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
    const v_sp: CkVec16 = @splat(0x20 as u8);
    const v_del: CkVec16 = @splat(0x7F as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: CkVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v <= v_sp) | (v == v_del))) return true;
    }
    while (i < s.len) {
        if (isCtrlOrSpace(s[i])) {
            return true;
        }
        i += 1;
    }
    return false;
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
    const lo_A: CkVec16 = @splat(65 as u8);
    const hi_Z: CkVec16 = @splat(90 as u8);
    const case_bit: CkVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: CkVec16 = a[i .. i + 16][0..16].*;
        const vb: CkVec16 = b[i .. i + 16][0..16].*;
        const la = @select(u8, (va >= lo_A) & (va <= hi_Z), va | case_bit, va);
        const lb = @select(u8, (vb >= lo_A) & (vb <= hi_Z), vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (CK_LOWER_LUT[@as(usize, a[i])] != CK_LOWER_LUT[@as(usize, b[i])]) {
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
    return eqlIgnoreCase(s[0..prefix.len], prefix);
}

fn endsWithBytes(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) {
        return false;
    }
    const offset = s.len - suffix.len;
    let i: usize = 0;
    while (i + 16 <= suffix.len) : (i += 16) {
        const va: CkVec16 = s[offset + i .. offset + i + 16][0..16].*;
        const vb: CkVec16 = suffix[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
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
    if (s.len > 0) {
        @memcpy(out[pos.* .. pos.* + s.len], s);
    }
    pos.* += s.len;
    return true;
}

fn writeLower(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    const base = pos.*;
    let i: usize = 0;
    const lo_A: CkVec16 = @splat(65 as u8);
    const hi_Z: CkVec16 = @splat(90 as u8);
    const case_bit: CkVec16 = @splat(0x20 as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: CkVec16 = s[i .. i + 16][0..16].*;
        const lo = @select(u8, (v >= lo_A) & (v <= hi_Z), v | case_bit, v);
        out[base + i .. base + i + 16][0..16].* = lo;
    }
    while (i < s.len) {
        out[base + i] = CK_LOWER_LUT[@as(usize, s[i])];
        i += 1;
    }
    pos.* = base + s.len;
    return true;
}

fn writeUpper(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    const base = pos.*;
    let i: usize = 0;
    const lo_a: CkVec16 = @splat(97 as u8);
    const hi_z: CkVec16 = @splat(122 as u8);
    const case_mask: CkVec16 = @splat(0xDF as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: CkVec16 = s[i .. i + 16][0..16].*;
        const up = @select(u8, (v >= lo_a) & (v <= hi_z), v & case_mask, v);
        out[base + i .. base + i + 16][0..16].* = up;
    }
    while (i < s.len) {
        out[base + i] = CK_UPPER_LUT[@as(usize, s[i])];
        i += 1;
    }
    pos.* = base + s.len;
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
        i = ckFindByte(q, i, '&');
        const part = q[seg_start..i];
        if (i < q.len && q[i] == '&') {
            i += 1;
        }
        if (part.len == 0) {
            continue;
        }

        // 提取参数名（首个 '=' 之前，无 '=' 则整段为 key）
        const eq_pos = ckFindByte(part, 0, '=');
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
                i = ckFindByte(q, i, '&');
                const part = q[seg_start..i];
                if (i < q.len && q[i] == '&') {
                    i += 1;
                }
                if (part.len == 0) {
                    continue;
                }

                const eq_pos = ckFindByte(part, 0, '=');
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
    if (ckFindByte(name, 0, ':') < name.len) {
        return false;
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

    if (!writeLower(out, &cur, name)) return false;

    out[cur] = ':';
    cur += 1;

    if (val.len > 0) {
        @memcpy(out[cur .. cur + val.len], val);
        cur += val.len;
    }

    pos.* = cur;
    return true;
}
