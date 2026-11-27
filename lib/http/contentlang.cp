// Source: Absorbed from /data/dev/bun/src/http/ | Synced: 2026-11-22T12:49:17Z (RFC 9110 Content-Language Parser)

type ClVec16 = @Vector(16, u8);

const CL_ALPHA: u8 = 1;
const CL_DIGIT: u8 = 2;
const CL_ALNUM: u8 = 3;

fn makeClClassLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            lut[i] = CL_ALPHA;
        } else if (c >= '0' && c <= '9') {
            lut[i] = CL_DIGIT;
        }
    }
    return lut;
}

fn makeClLowerLut(): [u8; 256] {
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

fn makeClUpperLut(): [u8; 256] {
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

const CL_CLASS_LUT: [u8; 256] = makeClClassLut();
const CL_LOWER_LUT: [u8; 256] = makeClLowerLut();
const CL_UPPER_LUT: [u8; 256] = makeClUpperLut();

fn clSimdFindByte(s: []const u8, start: usize, needle: u8): usize {
    let i = start;
    const v_n: ClVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: ClVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_n);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return s.len;
}

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) {
        start += 1;
    }
    while (end > start && isOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn isAlpha(c: u8): bool {
    return (CL_CLASS_LUT[usize(c)] & CL_ALPHA) != 0;
}

fn isDigit(c: u8): bool {
    return (CL_CLASS_LUT[usize(c)] & CL_DIGIT) != 0;
}

fn isAlnum(c: u8): bool {
    return (CL_CLASS_LUT[usize(c)] & CL_ALNUM) != 0;
}

fn toLowerAscii(c: u8): u8 {
    return CL_LOWER_LUT[usize(c)];
}

fn toUpperAscii(c: u8): u8 {
    return CL_UPPER_LUT[usize(c)];
}

fn isAllAlpha(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isAlpha(s[i])) {
            return false;
        }
    }
    return true;
}

fn isAllDigit(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isDigit(s[i])) {
            return false;
        }
    }
    return true;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: ClVec16 = @splat(65 as u8);
    const hi_Z: ClVec16 = @splat(90 as u8);
    const case_bit: ClVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: ClVec16 = a[i .. i + 16][0..16].*;
        const vb: ClVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) : (i += 1) {
        if (CL_LOWER_LUT[usize(a[i])] != CL_LOWER_LUT[usize(b[i])]) {
            return false;
        }
    }
    return true;
}

fn matchesLangOrPrefix(item: []const u8, target: []const u8): bool {
    if (item.len < target.len) {
        return false;
    }
    if (!eqlIgnoreCaseAscii(item[0..target.len], target)) {
        return false;
    }
    if (item.len == target.len) {
        return true;
    }
    return item[target.len] == '-';
}

/// BCP 47 语言标签核心结构拆解（主语种 / 文字脚本 / 国家地区）
export struct LanguageTagParts {
    primary: []const u8;
    script: ([]const u8)?;
    region: ([]const u8)?;
}

/// 校验基本 BCP 47 语法：首段 `2..=3` 字母，后续 `"-"` 分隔 `1..=8` 字母数字，拒绝空段与非法字符
export fn isValidBcp47Tag(tag: []const u8): bool {
    if (tag.len == 0) {
        return false;
    }

    let pos: usize = 0;
    let seg_idx: usize = 0;
    while (pos < tag.len) {
        const seg_start = pos;
        while (pos < tag.len && tag[pos] != '-') {
            const c = tag[pos];
            if (seg_idx == 0) {
                if (!isAlpha(c)) {
                    return false;
                }
            } else {
                if (!isAlnum(c)) {
                    return false;
                }
            }
            pos += 1;
        }

        const seg_len = pos - seg_start;
        if (seg_idx == 0) {
            if (seg_len < 2 || seg_len > 3) {
                return false;
            }
        } else {
            if (seg_len == 0 || seg_len > 8) {
                return false;
            }
        }

        if (pos < tag.len && tag[pos] == '-') {
            pos += 1;
            if (pos == tag.len) {
                return false;
            }
        }
        seg_idx += 1;
    }

    return seg_idx > 0;
}

/// 提取合法 BCP 47 语言标签中的 `primary`、`script`（4 字母）与 `region`（2 字母或 3 数字）
export fn parseLanguageTagParts(tag: []const u8): LanguageTagParts? {
    if (!isValidBcp47Tag(tag)) {
        return null;
    }

    let pos: usize = 0;
    while (pos < tag.len && tag[pos] != '-') {
        pos += 1;
    }
    const primary = tag[0..pos];
    let script: ([]const u8)? = null;
    let region: ([]const u8)? = null;

    if (pos < tag.len && tag[pos] == '-') {
        pos += 1;
    }

    while (pos < tag.len) {
        const seg_start = pos;
        while (pos < tag.len && tag[pos] != '-') {
            pos += 1;
        }
        const seg = tag[seg_start..pos];
        if (pos < tag.len && tag[pos] == '-') {
            pos += 1;
        }

        // Singleton extension / private-use prefix terminates script/region parsing
        if (seg.len == 1) {
            break;
        }

        if (script == null && region == null && seg.len == 4 && isAllAlpha(seg)) {
            script = seg;
            continue;
        }

        if (region == null && ((seg.len == 2 && isAllAlpha(seg)) || (seg.len == 3 && isAllDigit(seg)))) {
            region = seg;
            break;
        }

        // Allow 3-letter extlang subtags before script/region; stop on variants
        if (!(script == null && region == null && seg.len == 3 && isAllAlpha(seg))) {
            break;
        }
    }

    return LanguageTagParts{
        primary: primary,
        script: script,
        region: region,
    };
}

/// RFC 9110 §8.5 `Content-Language` 零分配 BCP 47 标签迭代器
export struct ContentLanguageIter {
    src: []const u8;
    pos: usize;

    static fn init(header: []const u8): ContentLanguageIter {
        return ContentLanguageIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            _.pos = clSimdFindByte(_.src, _.pos, ',');
            const seg = trimOws(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (isValidBcp47Tag(seg)) {
                return seg;
            }
        }
        return null;
    }
}

/// 按 RFC 5646 §2.1.1 规范化 BCP 47 语言标签大小写：
/// - `primary` 全小写（如 `en`, `zh`）
/// - `script`（4 字母）首字母大写其余小写（如 `Hans`）
/// - `region`（2 字母）全大写（如 `CN`, `US`）
/// - singleton（单字符扩展前缀）及其后续子标签全小写
/// 非法标签或输出缓冲不足返回 `null`
export fn canonicalizeLanguageTag(tag: []const u8, out: []u8): ([]const u8)? {
    if (!isValidBcp47Tag(tag) || out.len < tag.len) {
        return null;
    }

    let pos: usize = 0;
    let seg_idx: usize = 0;
    let in_singleton = false;

    while (pos < tag.len) {
        const seg_start = pos;
        while (pos < tag.len && tag[pos] != '-') {
            pos += 1;
        }
        const seg = tag[seg_start..pos];

        if (seg_idx == 0) {
            let i: usize = 0;
            while (i < seg.len) {
                out[seg_start + i] = toLowerAscii(seg[i]);
                i += 1;
            }
        } else if (in_singleton) {
            let i: usize = 0;
            while (i < seg.len) {
                out[seg_start + i] = toLowerAscii(seg[i]);
                i += 1;
            }
        } else if (seg.len == 1) {
            in_singleton = true;
            out[seg_start] = toLowerAscii(seg[0]);
        } else if (seg.len == 4 && isAllAlpha(seg)) {
            out[seg_start] = toUpperAscii(seg[0]);
            let i: usize = 1;
            while (i < seg.len) {
                out[seg_start + i] = toLowerAscii(seg[i]);
                i += 1;
            }
        } else if (seg.len == 2 && isAllAlpha(seg)) {
            out[seg_start] = toUpperAscii(seg[0]);
            out[seg_start + 1] = toUpperAscii(seg[1]);
        } else {
            let i: usize = 0;
            while (i < seg.len) {
                out[seg_start + i] = toLowerAscii(seg[i]);
                i += 1;
            }
        }

        if (pos < tag.len && tag[pos] == '-') {
            out[pos] = '-';
            pos += 1;
        }
        seg_idx += 1;
    }

    return out[0..tag.len];
}

/// 大小写不敏感查询 `Content-Language` 头部是否包含指定语言或主语种前缀
export fn hasContentLanguage(header: []const u8, target: []const u8): bool {
    if (!isValidBcp47Tag(target)) {
        return false;
    }
    let it = ContentLanguageIter.init(header);
    while (item := it.next()) {
        if (matchesLangOrPrefix(item, target)) {
            return true;
        }
    }
    return false;
}
