// Origin: Pure-CP Stdlib (Zero-Alloc SubRip SRT Subtitle Parser & Formatter) | Created: 2026-11-22
import std from "std";

/// SubRip (.srt) 单条字幕条目（零堆分配切片视图）
pub struct SrtCue {
    seq: u32 = 0,
    start_ms: u64 = 0,
    end_ms: u64 = 0,
    text: []const u8 = "",

    /// 返回字幕持续时长（毫秒），饱和减法防下溢
    pub fn durationMs(): u64 {
        if (_.end_ms > _.start_ms) {
            return _.end_ms - _.start_ms;
        }
        return 0;
    }
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isAsciiSpace(s[start])) {
        start += 1;
    }
    while (end > start && isAsciiSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn parseU32Strict(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isAsciiDigit(c)) {
            return null;
        }
        val = val * 10 + ((c - '0') as u64);
        if (val > 4294967295) {
            return null;
        }
        i += 1;
    }
    return val as u32;
}

fn parseTwoDigits(s: []const u8, pos: usize): u64? {
    if (pos + 2 > s.len) {
        return null;
    }
    const d0 = s[pos];
    const d1 = s[pos + 1];
    if (!isAsciiDigit(d0) || !isAsciiDigit(d1)) {
        return null;
    }
    return (((d0 - '0') as u64) * 10) + ((d1 - '0') as u64);
}

fn parseThreeDigits(s: []const u8, pos: usize): u64? {
    if (pos + 3 > s.len) {
        return null;
    }
    const d0 = s[pos];
    const d1 = s[pos + 1];
    const d2 = s[pos + 2];
    if (!isAsciiDigit(d0) || !isAsciiDigit(d1) || !isAsciiDigit(d2)) {
        return null;
    }
    return (((d0 - '0') as u64) * 100) + (((d1 - '0') as u64) * 10) + ((d2 - '0') as u64);
}

/// 解析标准 SRT 时间戳 `HH:MM:SS,mmm`（同时兼容 `.` 毫秒分隔符，严格校验分/秒 `< 60` 与 3 位毫秒）
pub fn parseSrtTimestampMs(s: []const u8): u64? {
    const t = trimAscii(s);
    // 最小长度 "00:00:00,000" = 12
    if (t.len < 12) {
        return null;
    }
    // 查找第一个 ':'（小时允许 >= 2 位数字）
    let colon1: usize = 0;
    while (colon1 < t.len && isAsciiDigit(t[colon1])) {
        colon1 += 1;
    }
    if (colon1 < 2 || colon1 >= t.len || t[colon1] != ':') {
        return null;
    }
    // 剩余必须严格为 ":MM:SS,mmm"（含冒号共 10 字节）
    if (t.len - colon1 != 10) {
        return null;
    }
    if (t[colon1 + 3] != ':') {
        return null;
    }
    const sep = t[colon1 + 6];
    if (sep != ',' && sep != '.') {
        return null;
    }

    let hours: u64 = 0;
    let i: usize = 0;
    while (i < colon1) {
        const digit = (t[i] - '0') as u64;
        if (hours > 1000000000) {
            return null;
        }
        hours = hours * 10 + digit;
        i += 1;
    }

    const mins = parseTwoDigits(t, colon1 + 1) ?? return null;
    const secs = parseTwoDigits(t, colon1 + 4) ?? return null;
    const millis = parseThreeDigits(t, colon1 + 7) ?? return null;

    if (mins >= 60 || secs >= 60) {
        return null;
    }

    return (hours * 3600000) + (mins * 60000) + (secs * 1000) + millis;
}

fn appendByte(out: []u8, pos: *usize, c: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = c;
    pos.* += 1;
    return true;
}

fn appendRaw(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) {
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

fn appendU64Decimal(out: []u8, pos: *usize, v: u64, min_width: usize): bool {
    let tmp: [20]u8 = undefined;
    let len: usize = 0;
    let n = v;
    if (n == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (n > 0) {
            tmp[len] = ('0' as u8) + ((n % 10) as u8);
            len += 1;
            n = n / 10;
        }
    }
    let pad: usize = if (min_width > len) min_width - len else 0;
    while (pad > 0) {
        if (!appendByte(out, pos, '0')) { return false; }
        pad -= 1;
    }
    while (len > 0) {
        len -= 1;
        if (!appendByte(out, pos, tmp[len])) { return false; }
    }
    return true;
}

/// 将毫秒时间戳格式化为标准 `HH:MM:SS,mmm`，缓冲不足返回 `null`
pub fn formatSrtTimestampMs(ms: u64, out: []u8): ([]const u8)? {
    const hours = ms / 3600000;
    const rem_h = ms % 3600000;
    const mins = rem_h / 60000;
    const rem_m = rem_h % 60000;
    const secs = rem_m / 1000;
    const millis = rem_m % 1000;

    let pos: usize = 0;
    if (!appendU64Decimal(out, &pos, hours, 2)) { return null; }
    if (!appendByte(out, &pos, ':')) { return null; }
    if (!appendU64Decimal(out, &pos, mins, 2)) { return null; }
    if (!appendByte(out, &pos, ':')) { return null; }
    if (!appendU64Decimal(out, &pos, secs, 2)) { return null; }
    if (!appendByte(out, &pos, ',')) { return null; }
    if (!appendU64Decimal(out, &pos, millis, 3)) { return null; }
    return out[0..pos];
}

struct LineSlice {
    line: []const u8,
    next_pos: usize,
}

fn nextLine(src: []const u8, pos: usize): LineSlice? {
    if (pos >= src.len) {
        return null;
    }
    let end = pos;
    while (end < src.len && src[end] != '\n') {
        end += 1;
    }
    let line_end = end;
    if (line_end > pos && src[line_end - 1] == '\r') {
        line_end -= 1;
    }
    const next_pos = if (end < src.len) end + 1 else src.len;
    return LineSlice{
        line: src[pos..line_end],
        next_pos: next_pos,
    };
}

fn isBlankLine(line: []const u8): bool {
    let i: usize = 0;
    while (i < line.len) {
        if (!isAsciiSpace(line[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findArrow(line: []const u8): usize? {
    if (line.len < 3) {
        return null;
    }
    let i: usize = 0;
    const limit = line.len - 3;
    while (i <= limit) {
        if (line[i] == '-' && line[i + 1] == '-' && line[i + 2] == '>') {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 提取 `-->` 右侧的结束时间戳 token（允许后接可选坐标参数如 `X1:0`）
fn firstToken(s: []const u8): []const u8 {
    const t = trimAscii(s);
    let end: usize = 0;
    while (end < t.len && !isAsciiSpace(t[end])) {
        end += 1;
    }
    return t[0..end];
}

/// 零堆分配 SubRip (.srt) 字幕条目流式迭代器（支持 UTF-8 BOM、CRLF/LF、多行正文与畸形块跳过）
pub struct SrtCueIter {
    src: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): SrtCueIter {
        let start: usize = 0;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return SrtCueIter{
            src: input,
            pos: start,
        };
    }

    pub fn next(*_): SrtCue? {
        while (_.pos < _.src.len) {
            // 1. 跳过空行
            let block_start = _.pos;
            while (ls := nextLine(_.src, _.pos)) {
                if (!isBlankLine(ls.line)) {
                    block_start = _.pos;
                    break;
                }
                _.pos = ls.next_pos;
                block_start = _.pos;
            }
            if (_.pos >= _.src.len) {
                return null;
            }

            // 2. 读取连续非空行块边界
            const line1 = nextLine(_.src, block_start) ?? return null;
            let cursor = line1.next_pos;
            let block_end = cursor;
            while (ls := nextLine(_.src, cursor)) {
                if (isBlankLine(ls.line)) {
                    _.pos = ls.next_pos;
                    break;
                }
                cursor = ls.next_pos;
                block_end = cursor;
                _.pos = cursor;
            }

            // 3. 解析第 1 行序号
            const seq_opt = parseU32Strict(trimAscii(line1.line));
            if (seq_opt == null) {
                continue;
            }
            const seq = seq_opt.?;

            // 4. 解析第 2 行时间轴 `start --> end`
            if (line1.next_pos >= block_end) {
                continue;
            }
            const line2 = nextLine(_.src, line1.next_pos) ?? continue;
            const arrow_opt = findArrow(line2.line);
            if (arrow_opt == null) {
                continue;
            }
            const arrow = arrow_opt.?;
            const start_str = trimAscii(line2.line[0..arrow]);
            const end_str = firstToken(line2.line[arrow + 3 ..]);

            const start_opt = parseSrtTimestampMs(start_str);
            const end_opt = parseSrtTimestampMs(end_str);
            if (start_opt == null || end_opt == null) {
                continue;
            }
            const start_ms = start_opt.?;
            const end_ms = end_opt.?;
            if (end_ms < start_ms) {
                continue;
            }

            // 5. 提取多行正文（剥离尾部 \r\n）
            let text_start = line2.next_pos;
            let text_end = block_end;
            if (text_start > text_end) {
                text_start = text_end;
            }
            while (text_end > text_start && (_.src[text_end - 1] == '\n' || _.src[text_end - 1] == '\r')) {
                text_end -= 1;
            }

            return SrtCue{
                seq: seq,
                start_ms: start_ms,
                end_ms: end_ms,
                text: _.src[text_start..text_end],
            };
        }
        return null;
    }
}

/// 规范化输出完整 SRT 字幕条目块，缓冲不足返回 `null`
pub fn formatSrtCue(cue: SrtCue, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (!appendU64Decimal(out, &pos, cue.seq as u64, 1)) { return null; }
    if (!appendByte(out, &pos, '\n')) { return null; }

    let ts_start_buf: [24]u8 = undefined;
    const ts_start = formatSrtTimestampMs(cue.start_ms, &ts_start_buf) ?? return null;
    if (!appendRaw(out, &pos, ts_start)) { return null; }
    if (!appendRaw(out, &pos, " --> ")) { return null; }

    let ts_end_buf: [24]u8 = undefined;
    const ts_end = formatSrtTimestampMs(cue.end_ms, &ts_end_buf) ?? return null;
    if (!appendRaw(out, &pos, ts_end)) { return null; }
    if (!appendByte(out, &pos, '\n')) { return null; }

    if (!appendRaw(out, &pos, cue.text)) { return null; }
    if (!appendByte(out, &pos, '\n')) { return null; }
    return out[0..pos];
}

fn shiftMs(ts: u64, offset_ms: i64): u64 {
    if (offset_ms >= 0) {
        const add = offset_ms as u64;
        const max_u64 = std.math.maxInt(u64);
        if (max_u64 - ts < add) {
            return max_u64;
        }
        return ts + add;
    }
    // 负偏移安全取绝对值，饱和钳位至 0
    const neg_u64: u64 = if (offset_ms == std.math.minInt(i64))
        (std.math.maxInt(i64) as u64) + 1
    else
        ((-offset_ms) as u64);
    if (ts <= neg_u64) {
        return 0;
    }
    return ts - neg_u64;
}

/// 整体平移字幕条目的时间轴，负偏移饱和钳位至 0
pub fn shiftSrtCue(cue: SrtCue, offset_ms: i64): SrtCue {
    return SrtCue{
        seq: cue.seq,
        start_ms: shiftMs(cue.start_ms, offset_ms),
        end_ms: shiftMs(cue.end_ms, offset_ms),
        text: cue.text,
    };
}
