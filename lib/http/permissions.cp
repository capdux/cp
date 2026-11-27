// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (W3C Permissions-Policy Header Parser)
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning/comparison and comptime 256-entry LUTs.

type PermVec16 = @Vector(16, u8);

fn makePermFeatureLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_') {
            lut[i] = 1;
        }
    }
    return lut;
}

fn makePermWsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' ' as usize] = 1;
    lut['\t' as usize] = 1;
    lut['\r' as usize] = 1;
    lut['\n' as usize] = 1;
    return lut;
}

const PERM_FEATURE_LUT: [u8; 256] = makePermFeatureLut();
const PERM_WS_LUT: [u8; 256] = makePermWsLut();

fn isSpace(c: u8): bool {
    return PERM_WS_LUT[c as usize] != 0;
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
    const n = a.len;
    while (i + 16 <= n) : (i += 16) {
        const va: PermVec16 = a[i..][0..16].*;
        const vb: PermVec16 = b[i..][0..16].*;
        const neq: u16 = @bitCast(va != vb);
        if (neq != 0) return false;
    }
    while (i < n) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const n = a.len;
    const v_A: PermVec16 = @splat('A');
    const v_Z: PermVec16 = @splat('Z');
    const v_case: PermVec16 = @splat(32);
    while (i + 16 <= n) : (i += 16) {
        const va: PermVec16 = a[i..][0..16].*;
        const vb: PermVec16 = b[i..][0..16].*;
        const ma = (va >= v_A) & (va <= v_Z);
        const mb = (vb >= v_A) & (vb <= v_Z);
        const la = @select(u8, ma, va +% v_case, va);
        const lb = @select(u8, mb, vb +% v_case, vb);
        const neq: u16 = @bitCast(la != lb);
        if (neq != 0) return false;
    }
    while (i < n) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

fn isFeatureChar(c: u8): bool {
    return PERM_FEATURE_LUT[c as usize] != 0;
}

fn isValidFeature(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    const n = s.len;
    const v_0: PermVec16 = @splat('0');
    const v_10: PermVec16 = @splat(10);
    const v_a: PermVec16 = @splat('a');
    const v_26: PermVec16 = @splat(26);
    const v_case: PermVec16 = @splat(0x20);
    const v_dash: PermVec16 = @splat('-');
    const v_under: PermVec16 = @splat('_');
    while (i + 16 <= n) : (i += 16) {
        const chunk: PermVec16 = s[i..][0..16].*;
        const m_dig: u16 = @bitCast((chunk -% v_0) < v_10);
        const m_alp: u16 = @bitCast(((chunk | v_case) -% v_a) < v_26);
        const m_dash: u16 = @bitCast(chunk == v_dash);
        const m_under: u16 = @bitCast(chunk == v_under);
        if ((~(m_dig | m_alp | m_dash | m_under)) != 0) return false;
    }
    while (i < n) : (i += 1) {
        if (PERM_FEATURE_LUT[s[i] as usize] == 0) {
            return false;
        }
    }
    return true;
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    const n = s.len;
    const v_needle: PermVec16 = @splat(needle);
    while (i + 16 <= n) : (i += 16) {
        const chunk: PermVec16 = s[i..][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
    }
    while (i < n) : (i += 1) {
        if (s[i] == needle) {
            return i;
        }
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

    static fn init(header: []const u8): PermissionsPolicyIter {
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
