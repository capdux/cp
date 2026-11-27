// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 8297 HTTP 103 Early Hints & RFC 8288 Link Codec)

type EhVec16 = @Vector(16, u8);

fn makeEhLowerLut(): [u8; 256] {
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

const EH_LOWER_LUT: [u8; 256] = makeEhLowerLut();

fn ehFindByte(s: []const u8, start: usize, ch: u8): usize {
    let i: usize = start;
    const v_ch: EhVec16 = @splat(ch);
    while (i + 16 <= s.len) : (i += 16) {
        const v: EhVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_ch)) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return s.len;
}

fn ehFindEither2(s: []const u8, start: usize, c1: u8, c2: u8): usize {
    let i: usize = start;
    const v1: EhVec16 = @splat(c1);
    const v2: EhVec16 = @splat(c2);
    while (i + 16 <= s.len) : (i += 16) {
        const v: EhVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v1) | (v == v2))) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == c1 || s[i] == c2) return i;
    }
    return s.len;
}

fn ehFindEither3(s: []const u8, start: usize, c1: u8, c2: u8, c3: u8): usize {
    let i: usize = start;
    const v1: EhVec16 = @splat(c1);
    const v2: EhVec16 = @splat(c2);
    const v3: EhVec16 = @splat(c3);
    while (i + 16 <= s.len) : (i += 16) {
        const v: EhVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v1) | (v == v2) | (v == v3))) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == c1 || s[i] == c2 || s[i] == c3) return i;
    }
    return s.len;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn hasLineBreak(s: []const u8): bool {
    return ehFindEither2(s, 0, '\r', '\n') < s.len;
}

fn hasUriForbidden(s: []const u8): bool {
    return ehFindEither3(s, 0, '>', '\r', '\n') < s.len;
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
    const lo_A: EhVec16 = @splat(65 as u8);
    const hi_Z: EhVec16 = @splat(90 as u8);
    const case_bit: EhVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: EhVec16 = a[i .. i + 16][0..16].*;
        const vb: EhVec16 = b[i .. i + 16][0..16].*;
        const la = @select(u8, (va >= lo_A) & (va <= hi_Z), va | case_bit, va);
        const lb = @select(u8, (vb >= lo_A) & (vb <= hi_Z), vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (EH_LOWER_LUT[@as(usize, a[i])] != EH_LOWER_LUT[@as(usize, b[i])]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn unquoteParamValue(s: []const u8): ([]const u8)? {
    if (s.len == 0) {
        return s;
    }
    if (s[0] != '"') {
        return s;
    }
    if (s.len < 2) {
        return null;
    }
    let i: usize = 1;
    while (i < s.len) {
        i = ehFindEither2(s, i, '\\', '"');
        if (i >= s.len) return null;
        const c = s[i];
        if (c == '\\') {
            if (i + 1 >= s.len) {
                return null;
            }
            i += 2;
            continue;
        }
        if (i + 1 != s.len) {
            return null;
        }
        return s[1..i];
    }
    return null;
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

/// RFC 8297 / 8288 常见 Early Hints 关系类型
export enum EarlyHintRel {
    preload,
    preconnect,
    modulepreload,
    dns_prefetch,
    other,

    static fn classify(raw: []const u8): EarlyHintRel {
        const s = trimAscii(raw);
        if (eqlIgnoreCase(s, "preload")) return EarlyHintRel.preload;
        if (eqlIgnoreCase(s, "preconnect")) return EarlyHintRel.preconnect;
        if (eqlIgnoreCase(s, "modulepreload")) return EarlyHintRel.modulepreload;
        if (eqlIgnoreCase(s, "dns-prefetch")) return EarlyHintRel.dns_prefetch;
        return EarlyHintRel.other;
    }

    fn toStr(): []const u8 {
        return switch (_) {
            .preload => "preload",
            .preconnect => "preconnect",
            .modulepreload => "modulepreload",
            .dns_prefetch => "dns-prefetch",
            .other => "",
        };
    }
}

/// RFC 8288 / 8297 单条 Link 预加载提示（零堆分配切片视图）
export struct EarlyHintLink {
    uri: []const u8;
    rel: EarlyHintRel;
    rel_raw: []const u8;
    as_type: ([]const u8)?;
    crossorigin: ([]const u8)?;
    fetchpriority: ([]const u8)?;
    nopush: bool;

    /// 判断该 Link 是否属于可预加载/预连接提示（`preload`/`preconnect`/`modulepreload`/`dns-prefetch`）
    fn isPreloadable(): bool {
        return _.rel != EarlyHintRel.other;
    }
}

fn parseSingleLink(seg: []const u8): EarlyHintLink? {
    if (seg.len < 3 || seg[0] != '<') {
        return null;
    }
    const close_angle = ehFindByte(seg, 1, '>');
    if (close_angle >= seg.len) {
        return null;
    }

    const uri = trimAscii(seg[1..close_angle]);
    if (uri.len == 0 || hasLineBreak(uri)) {
        return null;
    }

    let i: usize = close_angle + 1;
    while (i < seg.len && isSpace(seg[i])) {
        i += 1;
    }
    if (i >= seg.len || seg[i] != ';') {
        // 缺少必选 rel 参数或紧跟非法字符
        return null;
    }

    let rel_raw: []const u8 = "";
    let rel = EarlyHintRel.other;
    let as_type: ([]const u8)? = null;
    let crossorigin: ([]const u8)? = null;
    let fetchpriority: ([]const u8)? = null;
    let nopush = false;

    while (i < seg.len) {
        if (seg[i] == ';') {
            i += 1;
        }
        const p_start = i;
        let in_quote = false;
        while (i < seg.len) {
            if (in_quote) {
                i = ehFindEither2(seg, i, '\\', '"');
                if (i >= seg.len) break;
                if (seg[i] == '\\') {
                    if (i + 1 < seg.len) {
                        i += 2;
                        continue;
                    }
                    i += 1;
                    break;
                }
                in_quote = false;
                i += 1;
            } else {
                i = ehFindEither2(seg, i, '"', ';');
                if (i >= seg.len) break;
                if (seg[i] == ';') break;
                in_quote = true;
                i += 1;
            }
        }
        if (in_quote) {
            return null;
        }

        const param = trimAscii(seg[p_start..i]);
        if (param.len == 0) {
            continue;
        }

        const eq_pos = ehFindByte(param, 0, '=');
        if (eq_pos >= param.len) {
            if (eqlIgnoreCase(param, "nopush")) {
                nopush = true;
            } else if (eqlIgnoreCase(param, "crossorigin")) {
                crossorigin = "";
            }
            continue;
        }

        const pkey = trimAscii(param[0..eq_pos]);
        const pval_raw = trimAscii(param[(eq_pos + 1)..]);
        const pval = unquoteParamValue(pval_raw) ?? return null;

        if (eqlIgnoreCase(pkey, "rel")) {
            if (rel_raw.len == 0) {
                rel_raw = pval;
                rel = EarlyHintRel.classify(pval);
            }
        } else if (eqlIgnoreCase(pkey, "as")) {
            as_type = pval;
        } else if (eqlIgnoreCase(pkey, "crossorigin")) {
            crossorigin = pval;
        } else if (eqlIgnoreCase(pkey, "fetchpriority")) {
            fetchpriority = pval;
        } else if (eqlIgnoreCase(pkey, "nopush")) {
            nopush = true;
        }
    }

    if (rel_raw.len == 0) {
        return null;
    }

    return EarlyHintLink{
        uri: uri,
        rel: rel,
        rel_raw: rel_raw,
        as_type: as_type,
        crossorigin: crossorigin,
        fetchpriority: fetchpriority,
        nopush: nopush,
    };
}

/// RFC 8288 `Link` 头部多条目流式迭代器（尖括号与引号感知，自动跳过畸形项）
export struct EarlyHintIter {
    src: []const u8;
    pos: usize;

    static fn init(link_header: []const u8): EarlyHintIter {
        return EarlyHintIter{
            src: link_header,
            pos: 0,
        };
    }

    fn next(*_): EarlyHintLink? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            let in_angle = false;
            let in_quote = false;

            while (_.pos < _.src.len) {
                if (in_angle) {
                    _.pos = ehFindByte(_.src, _.pos, '>');
                    if (_.pos >= _.src.len) break;
                    in_angle = false;
                    _.pos += 1;
                } else if (in_quote) {
                    _.pos = ehFindEither2(_.src, _.pos, '\\', '"');
                    if (_.pos >= _.src.len) break;
                    if (_.src[_.pos] == '\\') {
                        if (_.pos + 1 < _.src.len) {
                            _.pos += 2;
                            continue;
                        }
                        _.pos += 1;
                        break;
                    }
                    in_quote = false;
                    _.pos += 1;
                } else {
                    _.pos = ehFindEither3(_.src, _.pos, '<', '"', ',');
                    if (_.pos >= _.src.len) break;
                    const c = _.src[_.pos];
                    if (c == ',') break;
                    if (c == '<') {
                        in_angle = true;
                    } else {
                        in_quote = true;
                    }
                    _.pos += 1;
                }
            }

            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0 || in_angle || in_quote) {
                continue;
            }
            if (link := parseSingleLink(seg)) {
                return link;
            }
        }
        return null;
    }
}

/// 规范化输出单条 RFC 8288 `Link` 头值（防 CRLF 注入，缓冲不足返回 `null`）
export fn formatEarlyHintLink(link: EarlyHintLink, out: []u8): ([]const u8)? {
    if (link.uri.len == 0 || hasUriForbidden(link.uri)) {
        return null;
    }

    const rel_str: []const u8 = if (link.rel_raw.len > 0) link.rel_raw else link.rel.toStr();
    if (rel_str.len == 0 || hasLineBreak(rel_str)) {
        return null;
    }

    let pos: usize = 0;
    if (!writeSlice(out, &pos, "<")) return null;
    if (!writeSlice(out, &pos, link.uri)) return null;
    if (!writeSlice(out, &pos, ">; rel=")) return null;
    if (!writeSlice(out, &pos, rel_str)) return null;

    if (as_v := link.as_type) {
        if (as_v.len > 0) {
            if (hasLineBreak(as_v)) return null;
            if (!writeSlice(out, &pos, "; as=")) return null;
            if (!writeSlice(out, &pos, as_v)) return null;
        }
    }

    if (co_v := link.crossorigin) {
        if (hasLineBreak(co_v)) return null;
        if (co_v.len == 0) {
            if (!writeSlice(out, &pos, "; crossorigin")) return null;
        } else {
            if (!writeSlice(out, &pos, "; crossorigin=")) return null;
            if (!writeSlice(out, &pos, co_v)) return null;
        }
    }

    if (fp_v := link.fetchpriority) {
        if (fp_v.len > 0) {
            if (hasLineBreak(fp_v)) return null;
            if (!writeSlice(out, &pos, "; fetchpriority=")) return null;
            if (!writeSlice(out, &pos, fp_v)) return null;
        }
    }

    if (link.nopush) {
        if (!writeSlice(out, &pos, "; nopush")) return null;
    }

    return out[0..pos];
}

/// 格式化完整 HTTP/1.1 `103 Early Hints` 响应报文块（空列表、非法条目或缓冲不足返回 `null`）
export fn formatEarlyHints103Block(links: []const EarlyHintLink, out: []u8): ([]const u8)? {
    if (links.len == 0) {
        return null;
    }
    let pos: usize = 0;
    if (!writeSlice(out, &pos, "HTTP/1.1 103 Early Hints\r\n")) {
        return null;
    }
    let i: usize = 0;
    while (i < links.len) {
        if (!writeSlice(out, &pos, "Link: ")) {
            return null;
        }
        const written = formatEarlyHintLink(links[i], out[pos..]) ?? return null;
        pos += written.len;
        if (!writeSlice(out, &pos, "\r\n")) {
            return null;
        }
        i += 1;
    }
    if (!writeSlice(out, &pos, "\r\n")) {
        return null;
    }
    return out[0..pos];
}
