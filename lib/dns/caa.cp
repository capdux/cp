// Source: Absorbed from /data/dev/bun/src/dns.zig | Synced: 2026-11-21T16:36:24Z (RFC 8659 DNS CAA Record Parser & Issuer Policy)
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning/comparison and comptime 256-entry LUTs.

type CaaVec16 = @Vector(16, u8);

fn makeCaaAlnumLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            lut[i] = 1;
        }
    }
    return lut;
}

fn makeCaaWsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' ' as usize] = 1;
    lut['\t' as usize] = 1;
    lut['\r' as usize] = 1;
    lut['\n' as usize] = 1;
    return lut;
}

const CAA_ALNUM_LUT: [u8; 256] = makeCaaAlnumLut();
const CAA_WS_LUT: [u8; 256] = makeCaaWsLut();

fn isSpace(c: u8): bool {
    return CAA_WS_LUT[c as usize] != 0;
}

fn isAlphaNum(c: u8): bool {
    return CAA_ALNUM_LUT[c as usize] != 0;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
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

fn stripTrailingDot(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && s[end - 1] == '.') {
        end -= 1;
    }
    return s[0..end];
}

fn simdFindByteCaa(s: []const u8, needle: u8): usize {
    let i: usize = 0;
    const n = s.len;
    const v_needle: CaaVec16 = @splat(needle);
    while (i + 16 <= n) : (i += 16) {
        const chunk: CaaVec16 = s[i..][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
    }
    while (i < n && s[i] != needle) : (i += 1) {}
    return i;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const n = a.len;
    const v_A: CaaVec16 = @splat('A');
    const v_Z: CaaVec16 = @splat('Z');
    const v_case: CaaVec16 = @splat(32);
    while (i + 16 <= n) : (i += 16) {
        const va: CaaVec16 = a[i..][0..16].*;
        const vb: CaaVec16 = b[i..][0..16].*;
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

fn isValidCaaTag(s: []const u8): bool {
    if (s.len == 0 || s.len > 255) {
        return false;
    }
    let i: usize = 0;
    const n = s.len;
    const v_0: CaaVec16 = @splat('0');
    const v_10: CaaVec16 = @splat(10);
    const v_a: CaaVec16 = @splat('a');
    const v_26: CaaVec16 = @splat(26);
    const v_case: CaaVec16 = @splat(0x20);
    while (i + 16 <= n) : (i += 16) {
        const chunk: CaaVec16 = s[i..][0..16].*;
        const m_dig: u16 = @bitCast((chunk -% v_0) < v_10);
        const m_alp: u16 = @bitCast(((chunk | v_case) -% v_a) < v_26);
        if ((~(m_dig | m_alp)) != 0) return false;
    }
    while (i < n) : (i += 1) {
        if (CAA_ALNUM_LUT[s[i] as usize] == 0) {
            return false;
        }
    }
    return true;
}

fn parseU8Dec(s: []const u8): u8? {
    if (s.len == 0 || s.len > 3) {
        return null;
    }
    let acc: u16 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + u16(c - '0');
        if (acc > 255) {
            return null;
        }
        i += 1;
    }
    return acc as u8;
}

/// RFC 8659 DNS CAA 属性标签类型
export enum CaaTag {
    issue,
    issuewild,
    iodef,
    unknown,

    static fn classify(raw: []const u8): CaaTag {
        if (eqlIgnoreCase(raw, "issue")) return CaaTag.issue;
        if (eqlIgnoreCase(raw, "issuewild")) return CaaTag.issuewild;
        if (eqlIgnoreCase(raw, "iodef")) return CaaTag.iodef;
        return CaaTag.unknown;
    }
}

/// RFC 8659 DNS CAA 证书颁发机构授权记录（零堆分配切片视图）
export struct CaaRecord {
    flags: u8;
    tag_raw: []const u8;
    tag: CaaTag;
    value: []const u8;

    /// 判断是否设置了 Issuer Critical 标志位（bit 7, `0x80`）
    fn isCritical(): bool {
        return (_.flags & 0x80) != 0;
    }

    /// 剥离 `;param=val` 参数部分并裁剪空白，返回授权 CA 域名
    fn issuerDomain(): []const u8 {
        const i = simdFindByteCaa(_.value, ';');
        return trimAscii(_.value[0..i]);
    }

    /// 判断该记录是否显式禁止所有 CA 签发（`value` 裁剪后为 `";"` 或空串，或分号前域名为空）
    fn forbidsAllIssuance(): bool {
        const v = trimAscii(_.value);
        if (v.len == 0 || (v.len == 1 && v[0] == ';')) {
            return true;
        }
        return _.issuerDomain().len == 0;
    }

    /// 解析 RFC 8659 文本格式 CAA 记录（如 `0 issue "letsencrypt.org"`、`128 issuewild ";"`）
    static fn parseText(rdata: []const u8): CaaRecord? {
        let pos: usize = 0;
        while (pos < rdata.len && isSpace(rdata[pos])) {
            pos += 1;
        }
        if (pos >= rdata.len) {
            return null;
        }

        // 1. flags (0..=255)
        const flag_start = pos;
        while (pos < rdata.len && !isSpace(rdata[pos])) {
            pos += 1;
        }
        const flags = parseU8Dec(rdata[flag_start..pos]) ?? return null;

        while (pos < rdata.len && isSpace(rdata[pos])) {
            pos += 1;
        }
        if (pos >= rdata.len) {
            return null;
        }

        // 2. tag (非空 ASCII 字母数字)
        const tag_start = pos;
        while (pos < rdata.len && !isSpace(rdata[pos])) {
            pos += 1;
        }
        const tag_raw = rdata[tag_start..pos];
        if (!isValidCaaTag(tag_raw)) {
            return null;
        }

        while (pos < rdata.len && isSpace(rdata[pos])) {
            pos += 1;
        }
        if (pos >= rdata.len) {
            return null;
        }

        // 3. value（双引号包裹或单个无空白 token）
        let value: []const u8 = "";
        if (rdata[pos] == '"') {
            pos += 1;
            const val_start = pos;
            let closed = false;
            while (pos < rdata.len) {
                const c = rdata[pos];
                if (c == '\\') {
                    if (pos + 1 >= rdata.len) {
                        return null;
                    }
                    pos += 2;
                    continue;
                }
                if (c == '"') {
                    closed = true;
                    break;
                }
                pos += 1;
            }
            if (!closed) {
                return null;
            }
            value = rdata[val_start..pos];
            pos += 1;
            while (pos < rdata.len && isSpace(rdata[pos])) {
                pos += 1;
            }
            if (pos != rdata.len) {
                return null;
            }
        } else {
            const val_start = pos;
            while (pos < rdata.len && !isSpace(rdata[pos])) {
                pos += 1;
            }
            value = rdata[val_start..pos];
            while (pos < rdata.len && isSpace(rdata[pos])) {
                pos += 1;
            }
            if (pos != rdata.len) {
                return null;
            }
        }

        return CaaRecord{
            flags: flags,
            tag_raw: tag_raw,
            tag: CaaTag.classify(tag_raw),
            value: value,
        };
    }

    /// 解析 RFC 8659 §4.1 线缆二进制格式 `[flags:1][tag_len:1][tag:tag_len][value:rest]`
    static fn parseWire(rdata: []const u8): CaaRecord? {
        if (rdata.len < 2) {
            return null;
        }
        const flags = rdata[0];
        const tag_len: usize = usize(rdata[1]);
        if (tag_len == 0 || rdata.len - 2 < tag_len) {
            return null;
        }
        const tag_raw = rdata[2..(tag_len + 2)];
        if (!isValidCaaTag(tag_raw)) {
            return null;
        }
        const value = rdata[(tag_len + 2)..];
        return CaaRecord{
            flags: flags,
            tag_raw: tag_raw,
            tag: CaaTag.classify(tag_raw),
            value: value,
        };
    }
}

/// 遵循 RFC 8659 判定指定 CA 域名是否被授权签发普通或通配符证书：
/// 1. 存在不可识别的 `critical` 标签时拒绝签发；
/// 2. 通配符证书优先以 `issuewild` 判定，无 `issuewild` 时回退 `issue`；
/// 3. 无相关约束记录时默认允许签发；有约束记录时需至少一条匹配 `ca_domain`。
export fn isCaAuthorized(
    records: []const CaaRecord,
    ca_domain: []const u8,
    is_wildcard: bool,
): bool {
    const target = stripTrailingDot(trimAscii(ca_domain));
    if (target.len == 0) {
        return false;
    }

    let has_issuewild = false;
    let i: usize = 0;
    while (i < records.len) {
        const rec = records[i];
        if (rec.tag == CaaTag.unknown && rec.isCritical()) {
            return false;
        }
        if (rec.tag == CaaTag.issuewild) {
            has_issuewild = true;
        }
        i += 1;
    }

    const effective_tag = if (is_wildcard && has_issuewild) CaaTag.issuewild else CaaTag.issue;

    let has_relevant = false;
    let j: usize = 0;
    while (j < records.len) {
        const rec = records[j];
        if (rec.tag == effective_tag) {
            has_relevant = true;
            const dom = stripTrailingDot(rec.issuerDomain());
            if (dom.len > 0 && eqlIgnoreCase(dom, target)) {
                return true;
            }
        }
        j += 1;
    }

    return !has_relevant;
}
