// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (W3C Server-Timing Header Parser & Builder)

const U64_MAX: u64 = 0xFFFF_FFFF_FFFF_FFFF;

type StVec16 = @Vector(16, u8);

fn makeStSpaceLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[usize(' ')] = 1;
    lut[usize('\t')] = 1;
    lut[usize('\r')] = 1;
    lut[usize('\n')] = 1;
    return lut;
}

fn makeStTokenLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if ((i >= '0' && i <= '9') || (i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z')) {
            lut[i] = 1;
        }
    }
    const extra = "!#$%&'*+-.^_`|~";
    let k: usize = 0;
    while (k < extra.len) : (k += 1) {
        lut[usize(extra[k])] = 1;
    }
    return lut;
}

const ST_SPACE_LUT: [u8; 256] = makeStSpaceLut();
const ST_TOKEN_LUT: [u8; 256] = makeStTokenLut();

fn isSpace(c: u8): bool {
    return ST_SPACE_LUT[usize(c)] != 0;
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
    while (i + 16 <= a.len) : (i += 16) {
        const va: StVec16 = a[i .. i + 16][0..16].*;
        const vb: StVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) {
            return false;
        }
    }
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
    const lo_A: StVec16 = @splat(65 as u8);
    const hi_Z: StVec16 = @splat(90 as u8);
    const case_bit: StVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: StVec16 = a[i .. i + 16][0..16].*;
        const vb: StVec16 = b[i .. i + 16][0..16].*;
        const la = @select(u8, (va >= lo_A) & (va <= hi_Z), va | case_bit, va);
        const lb = @select(u8, (vb >= lo_A) & (vb <= hi_Z), vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) {
            return false;
        }
    }
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isTokenChar(c: u8): bool {
    return ST_TOKEN_LUT[usize(c)] != 0;
}

fn isValidToken(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    const v_a: StVec16 = @splat(97 as u8);
    const v_z: StVec16 = @splat(122 as u8);
    const v_A: StVec16 = @splat(65 as u8);
    const v_Z: StVec16 = @splat(90 as u8);
    const v_0: StVec16 = @splat(48 as u8);
    const v_9: StVec16 = @splat(57 as u8);
    const v_dash: StVec16 = @splat(45 as u8);
    const v_us: StVec16 = @splat(95 as u8);
    const v_dot: StVec16 = @splat(46 as u8);
    const v_zero: StVec16 = @splat(0 as u8);
    const v_one: StVec16 = @splat(1 as u8);
    while (i < s.len) {
        if (i + 16 <= s.len) {
            const v: StVec16 = s[i .. i + 16][0..16].*;
            const ok = ((v >= v_a) & (v <= v_z)) | ((v >= v_A) & (v <= v_Z)) | ((v >= v_0) & (v <= v_9)) | (v == v_dash) | (v == v_us) | (v == v_dot);
            if (@reduce(.Or, @select(u8, ok, v_zero, v_one)) == 0) {
                i += 16;
                continue;
            }
        }
        if (!isTokenChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    const v_n: StVec16 = @splat(needle);
    const v_zero: StVec16 = @splat(0 as u8);
    const v_one: StVec16 = @splat(1 as u8);
    while (i + 16 <= s.len) {
        const v: StVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, @select(u8, v == v_n, v_one, v_zero)) != 0) {
            break;
        }
        i += 16;
    }
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 将非负十进制毫秒字符串（如 "53.2"、"0.125"、"10"）解析为微秒整数（防溢出）
fn parseMsToMicros(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let ms_part: u64 = 0;
    let has_int_digit = false;

    while (i < s.len && s[i] != '.') {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        has_int_digit = true;
        const d: u64 = u64(c - '0');
        if (ms_part > (U64_MAX - d) / 10) {
            return null;
        }
        ms_part = ms_part * 10 + d;
        i += 1;
    }

    let frac_us: u64 = 0;
    let has_frac_digit = false;
    if (i < s.len && s[i] == '.') {
        i += 1;
        let scale: u64 = 100;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') {
                return null;
            }
            has_frac_digit = true;
            if (scale > 0) {
                frac_us += u64(c - '0') * scale;
                scale /= 10;
            }
            i += 1;
        }
    }

    if (!has_int_digit && !has_frac_digit) {
        return null;
    }
    if (ms_part > (U64_MAX - frac_us) / 1000) {
        return null;
    }
    return ms_part * 1000 + frac_us;
}

/// W3C Server-Timing 单条指标（零拷贝切片 + 微秒纯整数时长）
export struct ServerTimingMetric {
    name: []const u8;
    dur_us: u64?;
    desc: ([]const u8)?;
}

/// W3C Server-Timing 响应头流式解析迭代器（零堆分配）
/// 支持引号内逗号/分号与反斜杠转义，参数大小写不敏感，重复参数首现优先
export struct ServerTimingIter {
    src: []const u8;
    pos: usize;

    static fn init(header: []const u8): ServerTimingIter {
        return ServerTimingIter{
            src: header,
            pos: 0,
        };
    }

    /// 迭代下一个合法的 Server-Timing 指标
    fn next(*_): ServerTimingMetric? {
        while (_.pos < _.src.len) {
            // 1. 按顶层逗号切分单条指标（忽略引号内逗号）
            const item_start = _.pos;
            let in_quote = false;
            let escaped = false;
            const v_dq: StVec16 = @splat(34 as u8);
            const v_comma: StVec16 = @splat(44 as u8);
            const v_zero: StVec16 = @splat(0 as u8);
            const v_one: StVec16 = @splat(1 as u8);
            while (_.pos < _.src.len) {
                if (!in_quote && _.pos + 16 <= _.src.len) {
                    const v: StVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
                    if (@reduce(.Or, @select(u8, (v == v_dq) | (v == v_comma), v_one, v_zero)) == 0) {
                        _.pos += 16;
                        continue;
                    }
                }
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
                    } else if (c == ',') {
                        break;
                    }
                }
                _.pos += 1;
            }
            const item_raw = trimAscii(_.src[item_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (item_raw.len == 0) {
                continue;
            }

            // 2. 按顶层分号拆分 metric-name 与参数列表
            let p: usize = findByte(item_raw, ';') ?? item_raw.len;
            const name = trimAscii(item_raw[0..p]);
            if (!isValidToken(name)) {
                continue;
            }

            let dur_us: u64? = null;
            let desc: ([]const u8)? = null;

            if (p < item_raw.len && item_raw[p] == ';') {
                p += 1;
            }
            const v_semi: StVec16 = @splat(59 as u8);
            while (p < item_raw.len) {
                const seg_start = p;
                let q_in = false;
                let q_esc = false;
                while (p < item_raw.len) {
                    if (!q_in && p + 16 <= item_raw.len) {
                        const v: StVec16 = item_raw[p .. p + 16][0..16].*;
                        if (@reduce(.Or, @select(u8, (v == v_dq) | (v == v_semi), v_one, v_zero)) == 0) {
                            p += 16;
                            continue;
                        }
                    }
                    const ch = item_raw[p];
                    if (q_in) {
                        if (q_esc) {
                            q_esc = false;
                        } else if (ch == '\\') {
                            q_esc = true;
                        } else if (ch == '"') {
                            q_in = false;
                        }
                    } else {
                        if (ch == '"') {
                            q_in = true;
                        } else if (ch == ';') {
                            break;
                        }
                    }
                    p += 1;
                }
                const param = trimAscii(item_raw[seg_start..p]);
                if (p < item_raw.len && item_raw[p] == ';') {
                    p += 1;
                }
                if (param.len == 0) {
                    continue;
                }

                if (eq_idx := findByte(param, '=')) {
                    const pkey = trimAscii(param[0..eq_idx]);
                    let pval = trimAscii(param[eq_idx + 1..]);
                    if (pval.len >= 2 && pval[0] == '"' && pval[pval.len - 1] == '"') {
                        pval = pval[1..pval.len - 1];
                    }
                    if (asciiEqlIgnoreCase(pkey, "dur")) {
                        if (dur_us == null) {
                            if (us := parseMsToMicros(pval)) {
                                dur_us = us;
                            }
                        }
                    } else if (asciiEqlIgnoreCase(pkey, "desc")) {
                        if (desc == null) {
                            desc = pval;
                        }
                    }
                }
            }

            return ServerTimingMetric{
                name: name,
                dur_us: dur_us,
                desc: desc,
            };
        }
        return null;
    }
}

/// 按指标名称精确查找首个匹配的 Server-Timing 指标
export fn findServerTiming(header: []const u8, name: []const u8): ServerTimingMetric? {
    let it = ServerTimingIter.init(header);
    while (true) {
        if (m := it.next()) {
            if (strEql(m.name, name)) {
                return m;
            }
        } else {
            break;
        }
    }
    return null;
}

struct AppendWriter {
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
        if (s.len > 0) {
            @memcpy(_.out[_.pos .. _.pos + s.len], s);
            _.pos += s.len;
        }
        return true;
    }

    fn writeU64(*_, val_in: u64): bool {
        let tmp: [20]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let v = val_in;
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
        if (_.pos + len > _.out.len) {
            return false;
        }
        let i: usize = len;
        while (i > 0) {
            i -= 1;
            _.out[_.pos] = tmp[i];
            _.pos += 1;
        }
        return true;
    }
}

/// 向响应头缓冲区追加一条规范化 Server-Timing 指标（零堆分配，失败时原子回滚 pos 并返回 false）
export fn appendServerTiming(out: []u8, pos: *usize, name: []const u8, dur_us: u64?, desc: ([]const u8)?): bool {
    if (!isValidToken(name)) {
        return false;
    }
    const saved_pos = pos.*;
    let w = AppendWriter{ out: out, pos: saved_pos };

    if (w.pos > 0) {
        if (!w.writeSlice(", ")) return false;
    }
    if (!w.writeSlice(name)) return false;

    if (d_us := dur_us) {
        if (!w.writeSlice(";dur=")) return false;
        const ms = d_us / 1000;
        const rem = d_us % 1000;
        if (!w.writeU64(ms)) return false;
        if (rem > 0) {
            if (!w.writeByte('.')) return false;
            const d1: u8 = u8('0') + u8(rem / 100);
            const d2: u8 = u8('0') + u8((rem / 10) % 10);
            const d3: u8 = u8('0') + u8(rem % 10);
            if (!w.writeByte(d1)) return false;
            if (d2 != '0' || d3 != '0') {
                if (!w.writeByte(d2)) return false;
            }
            if (d3 != '0') {
                if (!w.writeByte(d3)) return false;
            }
        }
    }

    if (ds := desc) {
        if (!w.writeSlice(";desc=")) return false;
        if (isValidToken(ds)) {
            if (!w.writeSlice(ds)) return false;
        } else {
            if (!w.writeByte('"')) return false;
            for (c of ds) {
                if (c == '"' || c == '\\') {
                    if (!w.writeByte('\\')) return false;
                }
                if (!w.writeByte(c)) return false;
            }
            if (!w.writeByte('"')) return false;
        }
    }

    pos.* = w.pos;
    return true;
}
