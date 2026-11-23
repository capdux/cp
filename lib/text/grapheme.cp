// Source: Absorbed from /data/dev/bun/src/string/immutable/grapheme.cp | Synced: 2026-11-22T12:49:17Z

/// Unicode UAX #29 扩展字素簇断行属性分类
pub enum GraphemeBreakProperty: u8 {
    Other = 0,
    CR = 1,
    LF = 2,
    Control = 3,
    Extend = 4,
    ZWJ = 5,
    RegionalIndicator = 6,
    Prepend = 7,
    SpacingMark = 8,
    HangulL = 9,
    HangulV = 10,
    HangulT = 11,
    HangulLV = 12,
    HangulLVT = 13,
    ExtendedPictographic = 14,
}

/// 将 Unicode 码点分类为 UAX #29 Grapheme_Cluster_Break 属性（零分配 O(1)）
pub fn classifyCodepoint(cp: u21): GraphemeBreakProperty {
    if (cp == 0x000D) { return .CR; }
    if (cp == 0x000A) { return .LF; }
    if (cp == 0x200D) { return .ZWJ; }

    // Regional Indicator (A..Z 国旗码点)
    if (cp >= 0x1F1E6 && cp <= 0x1F1FF) {
        return .RegionalIndicator;
    }

    // Hangul 音节与字母 (GB6-GB8)
    if ((cp >= 0x1100 && cp <= 0x115F) || (cp >= 0xA960 && cp <= 0xA97C)) {
        return .HangulL;
    }
    if ((cp >= 0x1160 && cp <= 0x11A7) || (cp >= 0xD7B0 && cp <= 0xD7C6)) {
        return .HangulV;
    }
    if ((cp >= 0x11A8 && cp <= 0x11FF) || (cp >= 0xD7CB && cp <= 0xD7FB)) {
        return .HangulT;
    }
    if (cp >= 0xAC00 && cp <= 0xD7A3) {
        if (((cp - 0xAC00) % 28) == 0) {
            return .HangulLV;
        }
        return .HangulLVT;
    }

    // Control 字符
    if (cp <= 0x001F || (cp >= 0x007F && cp <= 0x009F) ||
        cp == 0x00AD || cp == 0x061C || cp == 0x200E || cp == 0x200F ||
        (cp >= 0x2028 && cp <= 0x202E) || (cp >= 0x2060 && cp <= 0x206F) || cp == 0xFEFF)
    {
        return .Control;
    }

    // Prepend 字符
    if ((cp >= 0x0600 && cp <= 0x0605) || cp == 0x06DD || cp == 0x070F ||
        cp == 0x0890 || cp == 0x0891 || cp == 0x08E2 || cp == 0x110BD || cp == 0x110CD)
    {
        return .Prepend;
    }

    // Extend 字符（组合音标、变体选择符 FE00..FE0F、肤色修饰符 1F3FB..1F3FF、ZWNJ 等）
    if ((cp >= 0x0300 && cp <= 0x036F) ||
        (cp >= 0x0483 && cp <= 0x0489) ||
        (cp >= 0x0591 && cp <= 0x05BD) || cp == 0x05BF || cp == 0x05C1 || cp == 0x05C2 || cp == 0x05C4 || cp == 0x05C5 || cp == 0x05C7 ||
        (cp >= 0x0610 && cp <= 0x061A) || (cp >= 0x064B && cp <= 0x065F) || cp == 0x0670 ||
        (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        cp == 0x200C ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE00 && cp <= 0xFE0F) ||
        (cp >= 0xFE20 && cp <= 0xFE2F) ||
        (cp >= 0x1F3FB && cp <= 0x1F3FF) ||
        (cp >= 0xE0020 && cp <= 0xE007F) ||
        (cp >= 0xE0100 && cp <= 0xE01EF))
    {
        return .Extend;
    }

    // SpacingMark 字符（常用 Devanagari / Bengali 等间隔附加符号）
    if (cp == 0x0903 || cp == 0x093B || (cp >= 0x093E && cp <= 0x0940) ||
        (cp >= 0x0949 && cp <= 0x094C) || cp == 0x094E || cp == 0x094F ||
        cp == 0x0982 || cp == 0x0983 || (cp >= 0x09BE && cp <= 0x09C0) ||
        cp == 0x09C7 || cp == 0x09C8 || cp == 0x09CB || cp == 0x09CC || cp == 0x09D7)
    {
        return .SpacingMark;
    }

    // Extended_Pictographic（Emoji 主区段）
    if (cp == 0x00A9 || cp == 0x00AE || cp == 0x203C || cp == 0x2049 ||
        cp == 0x2122 || cp == 0x2139 || (cp >= 0x2194 && cp <= 0x2199) || cp == 0x21A9 || cp == 0x21AA ||
        cp == 0x231A || cp == 0x231B || cp == 0x2328 || cp == 0x23CF ||
        (cp >= 0x23E9 && cp <= 0x23F3) || (cp >= 0x23F8 && cp <= 0x23FA) ||
        cp == 0x24C2 || cp == 0x25AA || cp == 0x25AB || cp == 0x25B6 || cp == 0x25C0 ||
        (cp >= 0x25FB && cp <= 0x25FE) ||
        (cp >= 0x2600 && cp <= 0x27BF) ||
        cp == 0x2934 || cp == 0x2935 || (cp >= 0x2B05 && cp <= 0x2B07) ||
        cp == 0x2B1B || cp == 0x2B1C || cp == 0x2B50 || cp == 0x2B55 ||
        cp == 0x3030 || cp == 0x303D || cp == 0x3297 || cp == 0x3299 ||
        (cp >= 0x1F000 && cp <= 0x1F0FF) ||
        (cp >= 0x1F300 && cp <= 0x1F3FA) ||
        (cp >= 0x1F400 && cp <= 0x1FAFF))
    {
        return .ExtendedPictographic;
    }

    return .Other;
}

/// 跨多码点规则状态机（支持 GB11 Emoji ZWJ 链与 GB12/13 Regional Indicator 配对）
pub struct GraphemeBreakState {
    ri_odd: bool = false,
    in_ext_pict: bool = false,
    seen_zwj_after_ext_pict: bool = false,

    static fn init(): GraphemeBreakState {
        return GraphemeBreakState{};
    }

    fn reset(*_) {
        _.ri_odd = false;
        _.in_ext_pict = false;
        _.seen_zwj_after_ext_pict = false;
    }
}

/// 判定连续两码点 (prev, next) 之间是否存在扩展字素簇边界（true = 断开，false = 合并）
pub fn shouldBreak(state: *GraphemeBreakState, prev: u21, next: u21): bool {
    const p1 = classifyCodepoint(prev);
    const p2 = classifyCodepoint(next);

    // 追踪前驱状态（用于 GB11 与 GB12/GB13）
    if (p1 == .ExtendedPictographic) {
        state.in_ext_pict = true;
        state.seen_zwj_after_ext_pict = false;
    } else if (p1 == .Extend) {
        // 保持 in_ext_pict 状态，但清除刚见的 ZWJ
        state.seen_zwj_after_ext_pict = false;
    } else if (p1 == .ZWJ) {
        if (state.in_ext_pict) {
            state.seen_zwj_after_ext_pict = true;
        }
    } else {
        state.in_ext_pict = false;
        state.seen_zwj_after_ext_pict = false;
    }

    if (p1 == .RegionalIndicator) {
        state.ri_odd = !state.ri_odd;
    } else {
        state.ri_odd = false;
    }

    // GB3: CR × LF
    if (p1 == .CR && p2 == .LF) {
        return false;
    }

    // GB4: (Control | CR | LF) ÷
    if (p1 == .Control || p1 == .CR || p1 == .LF) {
        return true;
    }

    // GB5: ÷ (Control | CR | LF)
    if (p2 == .Control || p2 == .CR || p2 == .LF) {
        return true;
    }

    // GB6: L × (L | V | LV | LVT)
    if (p1 == .HangulL && (p2 == .HangulL || p2 == .HangulV || p2 == .HangulLV || p2 == .HangulLVT)) {
        return false;
    }

    // GB7: (LV | V) × (V | T)
    if ((p1 == .HangulLV || p1 == .HangulV) && (p2 == .HangulV || p2 == .HangulT)) {
        return false;
    }

    // GB8: (LVT | T) × T
    if ((p1 == .HangulLVT || p1 == .HangulT) && p2 == .HangulT) {
        return false;
    }

    // GB9: × (Extend | ZWJ)
    if (p2 == .Extend || p2 == .ZWJ) {
        return false;
    }

    // GB9a: × SpacingMark
    if (p2 == .SpacingMark) {
        return false;
    }

    // GB9b: Prepend ×
    if (p1 == .Prepend) {
        return false;
    }

    // GB11: \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}
    if (state.seen_zwj_after_ext_pict && p2 == .ExtendedPictographic) {
        state.seen_zwj_after_ext_pict = false;
        return false;
    }

    // GB12 / GB13: sot (RI RI)* RI × RI
    if (p1 == .RegionalIndicator && p2 == .RegionalIndicator && state.ri_odd) {
        return false;
    }

    // GB999: Any ÷ Any
    return true;
}

/// 单步安全 UTF-8 解码结果
pub struct DecodedCodepoint {
    cp: u21,
    len: usize,
}

/// 安全解码 UTF-8 首码点；非法或截断序列回退为 U+FFFD 并推进 1 字节
pub fn decodeUtf8Step(bytes: []const u8): DecodedCodepoint {
    if (bytes.len == 0) {
        return DecodedCodepoint{ cp: 0, len: 0 };
    }
    const b0 = bytes[0];
    if (b0 < 0x80) {
        return DecodedCodepoint{ cp: b0 as u21, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0) {
        if (bytes.len >= 2 && (bytes[1] & 0xC0) == 0x80) {
            const cp: u21 = (((b0 & 0x1F) as u21) << 6) | ((bytes[1] & 0x3F) as u21);
            if (cp >= 0x80) {
                return DecodedCodepoint{ cp: cp, len: 2 };
            }
        }
        return DecodedCodepoint{ cp: 0xFFFD, len: 1 };
    }
    if ((b0 & 0xF0) == 0xE0) {
        if (bytes.len >= 3 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80) {
            const cp: u21 = (((b0 & 0x0F) as u21) << 12) | (((bytes[1] & 0x3F) as u21) << 6) | ((bytes[2] & 0x3F) as u21);
            if (cp >= 0x800 && !(cp >= 0xD800 && cp <= 0xDFFF)) {
                return DecodedCodepoint{ cp: cp, len: 3 };
            }
        }
        return DecodedCodepoint{ cp: 0xFFFD, len: 1 };
    }
    if ((b0 & 0xF8) == 0xF0) {
        if (bytes.len >= 4 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80 && (bytes[3] & 0xC0) == 0x80) {
            const cp: u21 = (((b0 & 0x07) as u21) << 18) |
                (((bytes[1] & 0x3F) as u21) << 12) |
                (((bytes[2] & 0x3F) as u21) << 6) |
                ((bytes[3] & 0x3F) as u21);
            if (cp >= 0x10000 && cp <= 0x10FFFF) {
                return DecodedCodepoint{ cp: cp, len: 4 };
            }
        }
        return DecodedCodepoint{ cp: 0xFFFD, len: 1 };
    }
    return DecodedCodepoint{ cp: 0xFFFD, len: 1 };
}

/// 零堆分配 UTF-8 扩展字素簇迭代器
pub struct GraphemeIterator {
    bytes: []const u8,
    pos: usize = 0,

    static fn init(s: []const u8): GraphemeIterator {
        return GraphemeIterator{ bytes: s, pos: 0 };
    }

    fn reset(*_) {
        _.pos = 0;
    }

    /// 返回下一个完整扩展字素簇的原始 UTF-8 字节切片；迭代结束返回 null
    fn next(*_): ([]const u8)? {
        if (_.pos >= _.bytes.len) {
            return null;
        }
        const cluster_start = _.pos;
        const first = decodeUtf8Step(_.bytes[_.pos..]);
        _.pos += first.len;

        let prev_cp = first.cp;
        let state = GraphemeBreakState.init();

        while (_.pos < _.bytes.len) {
            const cur = decodeUtf8Step(_.bytes[_.pos..]);
            if (shouldBreak(*state, prev_cp, cur.cp)) {
                break;
            }
            _.pos += cur.len;
            prev_cp = cur.cp;
        }

        return _.bytes[cluster_start.._.pos];
    }
}

/// 统计 UTF-8 字符串中的扩展字素簇总数（用户感知字符数）
pub fn count(s: []const u8): usize {
    let it = GraphemeIterator.init(s);
    let total: usize = 0;
    while (it.next() != null) {
        total += 1;
    }
    return total;
}

/// 按扩展字素簇索引区间 `[start, end)` 零分配截取子串；越界或反转返回空切片
pub fn sliceByGraphemes(s: []const u8, start: usize, end: usize): []const u8 {
    if (start >= end || s.len == 0) {
        return s[0..0];
    }
    let it = GraphemeIterator.init(s);
    let byte_start: usize = s.len;
    let byte_end: usize = s.len;
    let idx: usize = 0;

    while (idx < end) {
        const before_pos = it.pos;
        if (it.next() == null) {
            break;
        }
        if (idx == start) {
            byte_start = before_pos;
        }
        if (idx + 1 == end) {
            byte_end = it.pos;
        }
        idx += 1;
    }

    if (byte_start >= s.len) {
        return s[0..0];
    }
    if (byte_end < byte_start) {
        return s[0..0];
    }
    return s[byte_start..byte_end];
}
