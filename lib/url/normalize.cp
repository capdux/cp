// Source: Absorbed from /data/dev/bun/src/url.zig | Synced: 2026-11-21T14:47:37Z (RFC 3986 URI Normalization)

type UriNormVec16 = @Vector(16, u8);

const HEX_UPPER: []const u8 = "0123456789ABCDEF";

fn makeUriHexLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= '0' && i <= '9') {
            lut[i] = @as(u8, @intCast(i - '0'));
        } else if (i >= 'a' && i <= 'f') {
            lut[i] = @as(u8, @intCast(i - 'a' + 10));
        } else if (i >= 'A' && i <= 'F') {
            lut[i] = @as(u8, @intCast(i - 'A' + 10));
        }
    }
    return lut;
}

fn makeUriUnreservedLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9') ||
            i == '-' || i == '.' || i == '_' || i == '~')
        {
            lut[i] = 1;
        }
    }
    return lut;
}

fn makeUriLowerLut(): [u8; 256] {
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

const URI_HEX_LUT: [u8; 256] = makeUriHexLut();
const URI_UNRESERVED_LUT: [u8; 256] = makeUriUnreservedLut();
const URI_LOWER_LUT: [u8; 256] = makeUriLowerLut();

fn toLowerAscii(c: u8): u8 {
    return URI_LOWER_LUT[usize(c)];
}

fn findByteSimd(s: []const u8, start: usize, needle: u8): usize {
    let i = start;
    const v_needle: UriNormVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: UriNormVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return s.len;
}

fn copyToLowerSimd(dst: []u8, src: []const u8) {
    let i: usize = 0;
    const lo_A: UriNormVec16 = @splat(65 as u8);
    const hi_Z: UriNormVec16 = @splat(90 as u8);
    const case_bit: UriNormVec16 = @splat(0x20 as u8);
    while (i + 16 <= src.len) : (i += 16) {
        const v: UriNormVec16 = src[i .. i + 16][0..16].*;
        const m = (v >= lo_A) & (v <= hi_Z);
        const lowered = @select(u8, m, v | case_bit, v);
        const arr: [16]u8 = lowered;
        @memcpy(dst[i .. i + 16], arr[0..16]);
    }
    while (i < src.len) : (i += 1) {
        dst[i] = URI_LOWER_LUT[usize(src[i])];
    }
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: UriNormVec16 = @splat(65 as u8);
    const hi_Z: UriNormVec16 = @splat(90 as u8);
    const case_bit: UriNormVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: UriNormVec16 = a[i .. i + 16][0..16].*;
        const vb: UriNormVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (URI_LOWER_LUT[usize(a[i])] != URI_LOWER_LUT[usize(b[i])]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn hexNibble(c: u8): u8? {
    const v = URI_HEX_LUT[usize(c)];
    if (v == 0xFF) return null;
    return v;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: UriNormVec16 = a[i .. i + 16][0..16].*;
        const vb: UriNormVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// RFC 3986 §2.3 非保留字符判定：`ALPHA / DIGIT / "-" / "." / "_" / "~"`
pub fn isUnreserved(b: u8): bool {
    return URI_UNRESERVED_LUT[usize(b)] != 0;
}

/// 查询常见协议的默认端口号（大小写无关）
pub fn defaultPortForScheme(scheme: []const u8): u16? {
    if (asciiEqlIgnoreCase(scheme, "http") || asciiEqlIgnoreCase(scheme, "ws")) return 80;
    if (asciiEqlIgnoreCase(scheme, "https") || asciiEqlIgnoreCase(scheme, "wss")) return 443;
    if (asciiEqlIgnoreCase(scheme, "ftp")) return 21;
    if (asciiEqlIgnoreCase(scheme, "ssh")) return 22;
    if (asciiEqlIgnoreCase(scheme, "postgres") || asciiEqlIgnoreCase(scheme, "postgresql")) return 5432;
    if (asciiEqlIgnoreCase(scheme, "mysql")) return 3306;
    if (asciiEqlIgnoreCase(scheme, "redis")) return 6379;
    return null;
}

/// RFC 3986 §6.2.2.2 百分号编码规范化：
/// 解码非保留字符的 `%XX`（如 `%7E`->`~`、`%41`->`A`）；其余合法 `%xx` 统一转大写；遇非法 `%` 或缓冲不足返回 null
pub fn normalizePercentEncoding(input: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let pos: usize = 0;

    while (i < input.len) {
        const pct = findByteSimd(input, i, '%');
        if (pct > i) {
            const seg_len = pct - i;
            if (pos + seg_len > out.len) return null;
            @memcpy(out[pos .. pos + seg_len], input[i..pct]);
            pos += seg_len;
        }
        if (pct >= input.len) break;
        i = pct;

        if (i + 2 >= input.len) {
            return null;
        }
        const hi = hexNibble(input[i + 1]) ?? return null;
        const lo = hexNibble(input[i + 2]) ?? return null;
        const decoded: u8 = (hi << 4) | lo;

        if (URI_UNRESERVED_LUT[usize(decoded)] != 0) {
            if (pos >= out.len) return null;
            out[pos] = decoded;
            pos += 1;
        } else {
            if (pos + 3 > out.len) return null;
            out[pos] = '%';
            out[pos + 1] = HEX_UPPER[usize(hi)];
            out[pos + 2] = HEX_UPPER[usize(lo)];
            pos += 3;
        }
        i += 3;
    }

    return out[0..pos];
}

fn popLastSegment(out: []u8, pos_in: usize): usize {
    let pos = pos_in;
    while (pos > 0 && out[pos - 1] != '/') {
        pos -= 1;
    }
    if (pos > 0 && out[pos - 1] == '/') {
        pos -= 1;
    }
    return pos;
}

/// RFC 3986 §5.2.4 路径点段归约（零堆分配，严格遵循规则 A–E）
pub fn removeDotSegments(path: []const u8, out: []u8): ([]const u8)? {
    let in_pos: usize = 0;
    let virtual_slash = false;
    let out_pos: usize = 0;

    while (virtual_slash || in_pos < path.len) {
        const rest = path[in_pos..];

        // Rule A: 前缀 "../" 或 "./" -> 直接移除
        if (!virtual_slash) {
            if (startsWith(rest, "../")) {
                in_pos += 3;
                continue;
            }
            if (startsWith(rest, "./")) {
                in_pos += 2;
                continue;
            }
        }

        // Rule B: 前缀 "/./" 或恰好 "/." -> 替换为 "/"
        if (virtual_slash) {
            if (startsWith(rest, "./")) {
                in_pos += 2;
                virtual_slash = true;
                continue;
            }
            if (strEql(rest, ".")) {
                in_pos = path.len;
                virtual_slash = true;
                continue;
            }
        } else {
            if (startsWith(rest, "/./")) {
                in_pos += 3;
                virtual_slash = true;
                continue;
            }
            if (strEql(rest, "/.")) {
                in_pos = path.len;
                virtual_slash = true;
                continue;
            }
        }

        // Rule C: 前缀 "/../" 或恰好 "/.." -> 替换为 "/" 并弹出输出缓冲最后一段
        if (virtual_slash) {
            if (startsWith(rest, "../")) {
                in_pos += 3;
                virtual_slash = true;
                out_pos = popLastSegment(out, out_pos);
                continue;
            }
            if (strEql(rest, "..")) {
                in_pos = path.len;
                virtual_slash = true;
                out_pos = popLastSegment(out, out_pos);
                continue;
            }
        } else {
            if (startsWith(rest, "/../")) {
                in_pos += 4;
                virtual_slash = true;
                out_pos = popLastSegment(out, out_pos);
                continue;
            }
            if (strEql(rest, "/..")) {
                in_pos = path.len;
                virtual_slash = true;
                out_pos = popLastSegment(out, out_pos);
                continue;
            }
        }

        // Rule D: 仅剩 "." 或 ".." -> 消费完毕
        if (!virtual_slash && (strEql(rest, ".") || strEql(rest, ".."))) {
            in_pos = path.len;
            continue;
        }

        // Rule E: 将下一路径段（含其前导 '/'）移入输出缓冲
        if (virtual_slash) {
            if (out_pos >= out.len) return null;
            out[out_pos] = '/';
            out_pos += 1;
            virtual_slash = false;
            const next_slash = findByteSimd(path, in_pos, '/');
            const seg_len = next_slash - in_pos;
            if (seg_len > 0) {
                if (out_pos + seg_len > out.len) return null;
                @memcpy(out[out_pos .. out_pos + seg_len], path[in_pos..next_slash]);
                out_pos += seg_len;
                in_pos = next_slash;
            }
        } else {
            if (path[in_pos] == '/') {
                if (out_pos >= out.len) return null;
                out[out_pos] = '/';
                out_pos += 1;
                in_pos += 1;
            }
            const next_slash = findByteSimd(path, in_pos, '/');
            const seg_len = next_slash - in_pos;
            if (seg_len > 0) {
                if (out_pos + seg_len > out.len) return null;
                @memcpy(out[out_pos .. out_pos + seg_len], path[in_pos..next_slash]);
                out_pos += seg_len;
                in_pos = next_slash;
            }
        }
    }

    return out[0..out_pos];
}

/// 规范化 URI Origin：小写化 scheme 与 host，若 port 等于默认端口则自动剥离
pub fn normalizeOrigin(scheme: []const u8, host: []const u8, port: u16?, out: []u8): ([]const u8)? {
    if (scheme.len == 0 || host.len == 0) {
        return null;
    }

    let pos: usize = 0;
    if (scheme.len + 3 + host.len > out.len) {
        return null;
    }
    copyToLowerSimd(out[pos .. pos + scheme.len], scheme);
    pos += scheme.len;
    out[pos] = ':';
    out[pos + 1] = '/';
    out[pos + 2] = '/';
    pos += 3;
    copyToLowerSimd(out[pos .. pos + host.len], host);
    pos += host.len;

    if (p := port) {
        let is_default = false;
        if (def_p := defaultPortForScheme(scheme)) {
            if (p == def_p) {
                is_default = true;
            }
        }
        if (!is_default) {
            if (pos >= out.len) return null;
            out[pos] = ':';
            pos += 1;

            let tmp: [5]u8 = [0, 0, 0, 0, 0];
            let v: u16 = p;
            let len: usize = 0;
            if (v == 0) {
                tmp[0] = '0';
                len = 1;
            } else {
                while (v > 0) {
                    tmp[len] = u8('0') + u8(v % 10);
                    v /= 10;
                    len += 1;
                }
            }
            if (pos + len > out.len) return null;
            let k: usize = len;
            while (k > 0) {
                k -= 1;
                out[pos] = tmp[k];
                pos += 1;
            }
        }
    }

    return out[0..pos];
}
