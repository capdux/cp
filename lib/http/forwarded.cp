// Source: Absorbed from /data/dev/bun/src/http/HeaderValueIterator.cp | Synced: 2026-11-23T01:58:12Z

/// RFC 7239 `Forwarded` 头部单跳代理信息（零拷贝切片）
pub struct ForwardedHop {
    for_addr: []const u8 = "",
    by_addr: []const u8 = "",
    host: []const u8 = "",
    proto: []const u8 = "",
}

type FwdVec16 = @Vector(16, u8);

fn makeFwdLowerLut(): [u8; 256] {
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

const FWD_LOWER_LUT: [u8; 256] = makeFwdLowerLut();

fn fwdFindByte(s: []const u8, start: usize, ch: u8): usize {
    let i: usize = start;
    const v_ch: FwdVec16 = @splat(ch);
    while (i + 16 <= s.len) : (i += 16) {
        const v: FwdVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_ch)) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return s.len;
}

fn fwdFindEither2(s: []const u8, start: usize, c1: u8, c2: u8): usize {
    let i: usize = start;
    const v1: FwdVec16 = @splat(c1);
    const v2: FwdVec16 = @splat(c2);
    while (i + 16 <= s.len) : (i += 16) {
        const v: FwdVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v1) | (v == v2))) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == c1 || s[i] == c2) return i;
    }
    return s.len;
}

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) { start += 1; }
    while (end > start && isOws(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    const lo_A: FwdVec16 = @splat(65 as u8);
    const hi_Z: FwdVec16 = @splat(90 as u8);
    const case_bit: FwdVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: FwdVec16 = a[i .. i + 16][0..16].*;
        const vb: FwdVec16 = b[i .. i + 16][0..16].*;
        const la = @select(u8, (va >= lo_A) & (va <= hi_Z), va | case_bit, va);
        const lb = @select(u8, (vb >= lo_A) & (vb <= hi_Z), vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (FWD_LOWER_LUT[@as(usize, a[i])] != FWD_LOWER_LUT[@as(usize, b[i])]) { return false; }
        i += 1;
    }
    return true;
}

fn stripOuterQuotes(s: []const u8): []const u8 {
    if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
        return s[1..(s.len - 1)];
    }
    if (s.len >= 1 && s[0] == '"') {
        return s[1..s.len];
    }
    return s;
}

/// RFC 7239 `Forwarded` 多跳头部迭代器（零堆分配，支持引号与 `\"` 转义保护）
pub struct ForwardedIterator {
    header: []const u8,
    pos: usize = 0,

    static fn init(header: []const u8): ForwardedIterator {
        return ForwardedIterator{ header: header, pos: 0 };
    }

    fn next(*_): ForwardedHop? {
        while (_.pos < _.header.len) {
            const hop_start = _.pos;
            let in_quote = false;
            while (_.pos < _.header.len) {
                if (in_quote) {
                    _.pos = fwdFindEither2(_.header, _.pos, '\\', '"');
                    if (_.pos >= _.header.len) break;
                    const c = _.header[_.pos];
                    if (c == '\\' && _.pos + 1 < _.header.len) {
                        _.pos += 2;
                        continue;
                    }
                    if (c == '"') {
                        in_quote = false;
                    }
                    _.pos += 1;
                } else {
                    _.pos = fwdFindEither2(_.header, _.pos, '"', ',');
                    if (_.pos >= _.header.len) break;
                    const c = _.header[_.pos];
                    if (c == '"') {
                        in_quote = true;
                        _.pos += 1;
                    } else if (c == ',') {
                        break;
                    } else {
                        _.pos += 1;
                    }
                }
            }

            const hop_end = _.pos;
            if (_.pos < _.header.len && _.header[_.pos] == ',') {
                _.pos += 1;
            }

            const hop_str = trimOws(_.header[hop_start..hop_end]);
            if (hop_str.len == 0) {
                continue;
            }

            let hop = ForwardedHop{};
            let has_any = false;
            let p: usize = 0;

            while (p < hop_str.len) {
                const seg_start = p;
                let seg_quote = false;
                while (p < hop_str.len) {
                    if (seg_quote) {
                        p = fwdFindEither2(hop_str, p, '\\', '"');
                        if (p >= hop_str.len) break;
                        const c = hop_str[p];
                        if (c == '\\' && p + 1 < hop_str.len) {
                            p += 2;
                            continue;
                        }
                        if (c == '"') {
                            seg_quote = false;
                        }
                        p += 1;
                    } else {
                        p = fwdFindEither2(hop_str, p, '"', ';');
                        if (p >= hop_str.len) break;
                        const c = hop_str[p];
                        if (c == '"') {
                            seg_quote = true;
                            p += 1;
                        } else if (c == ';') {
                            break;
                        } else {
                            p += 1;
                        }
                    }
                }
                const seg_end = p;
                if (p < hop_str.len && hop_str[p] == ';') {
                    p += 1;
                }

                const seg = trimOws(hop_str[seg_start..seg_end]);
                if (seg.len == 0) { continue; }

                const eq_pos = fwdFindByte(seg, 0, '=');
                const eq_idx: usize? = if (eq_pos < seg.len) eq_pos else null;

                if (eq := eq_idx) {
                    const key = trimOws(seg[0..eq]);
                    const raw_val = trimOws(seg[(eq + 1)..seg.len]);
                    const val = stripOuterQuotes(raw_val);
                    if (key.len == 0) { continue; }

                    if (eqlIgnoreCaseAscii(key, "for")) {
                        hop.for_addr = val;
                        has_any = true;
                    } else if (eqlIgnoreCaseAscii(key, "by")) {
                        hop.by_addr = val;
                        has_any = true;
                    } else if (eqlIgnoreCaseAscii(key, "host")) {
                        hop.host = val;
                        has_any = true;
                    } else if (eqlIgnoreCaseAscii(key, "proto")) {
                        hop.proto = val;
                        has_any = true;
                    }
                }
            }

            if (has_any) {
                return hop;
            }
        }
        return null;
    }
}

/// 解析 `Forwarded` 头部的第一跳代理信息
pub fn parseFirstHop(header: []const u8): ForwardedHop? {
    let it = ForwardedIterator.init(header);
    return it.next();
}

/// 从 `X-Forwarded-For: client, proxy1, proxy2` 按可信代理跳数 `trusted_proxies` 安全提取客户端 IP
/// 自动裁剪空白并跳过空项；跳过最右侧 `trusted_proxies` 个代理地址（越界时回退至最左侧项）
pub fn extractClientIpFromXff(xff_header: []const u8, trusted_proxies: usize): ([]const u8)? {
    let items: [64][]const u8 = [""; 64];
    let count: usize = 0;
    let pos: usize = 0;

    while (pos < xff_header.len && count < 64) {
        const start = pos;
        pos = fwdFindByte(xff_header, pos, ',');
        const entry = trimOws(xff_header[start..pos]);
        if (pos < xff_header.len && xff_header[pos] == ',') {
            pos += 1;
        }
        if (entry.len > 0) {
            items[count] = entry;
            count += 1;
        }
    }

    if (count == 0) {
        return null;
    }
    if (trusted_proxies >= count) {
        return items[0];
    }
    return items[(count - 1) - trusted_proxies];
}

/// 从 `Forwarded` 的 `for` 值中剥离端口与 IPv6 方括号：
/// `[2001:db8::1]:4711` -> `2001:db8::1`，`192.0.2.60:8080` -> `192.0.2.60`，`_hidden` 保持原样
pub fn stripPortFromForwardedFor(val: []const u8): []const u8 {
    const s = stripOuterQuotes(trimOws(val));
    if (s.len == 0) { return ""; }

    // 带方括号 IPv6: [2001:db8::1]:4711 或 [2001:db8::1]
    if (s[0] == '[') {
        const rb = fwdFindByte(s, 1, ']');
        if (rb < s.len) {
            return s[1..rb];
        }
        return s;
    }

    // 统计冒号数：>= 2 个冒号为裸 IPv6 地址，保持原样
    let colon_count: usize = 0;
    let last_colon: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const next_c = fwdFindByte(s, i, ':');
        if (next_c >= s.len) break;
        colon_count += 1;
        last_colon = next_c;
        i = next_c + 1;
    }

    if (colon_count == 1 && last_colon + 1 < s.len) {
        // 校验冒号后全为数字端口（避免误伤非法后缀）
        let all_digits = true;
        let k = last_colon + 1;
        while (k < s.len) {
            if (s[k] < '0' || s[k] > '9') {
                all_digits = false;
                break;
            }
            k += 1;
        }
        if (all_digits) {
            return s[0..last_colon];
        }
    }

    return s;
}
