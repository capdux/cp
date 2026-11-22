// Origin: Pure-CP Stdlib (W3C WebVTT Subtitle Parser & Formatter) | Created: 2026-11-22

/// W3C WebVTT 字幕条目（零拷贝切片指向原始输入）
pub struct VttCue {
    id: ([]const u8)? = null,
    start_ms: u64,
    end_ms: u64,
    settings: []const u8 = "",
    payload: []const u8 = "",

    /// 返回字幕持续时长（毫秒，饱和减法防下溢）
    pub fn durationMs(): u64 {
        if (_.end_ms > _.start_ms) {
            return _.end_ms - _.start_ms;
        }
        return 0;
    }
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn stripBom(s: []const u8): []const u8 {
    if (s.len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        return s[3..];
    }
    return s;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpaceOrTab(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpaceOrTab(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn trimLineEnd(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[0..end];
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

fn findArrow(s: []const u8): usize? {
    if (s.len < 3) {
        return null;
    }
    let i: usize = 0;
    while (i + 3 <= s.len) {
        if (s[i] == '-' && s[i + 1] == '-' && s[i + 2] == '>') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn parseUint(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) {
            return null;
        }
        if (acc > 1844674407370955161) {
            return null;
        }
        acc = acc * 10 + ((c - '0') as u64);
        i += 1;
    }
    return acc;
}

/// 校验 WebVTT 文件头（支持可选 UTF-8 BOM；首行以 `"WEBVTT"` 开头且后续为空/空白/Tab/换行）
pub fn isWebVttHeader(input: []const u8): bool {
    const s = stripBom(input);
    if (!startsWith(s, "WEBVTT")) {
        return false;
    }
    if (s.len == 6) {
        return true;
    }
    const next_c = s[6];
    return next_c == ' ' || next_c == '\t' || next_c == '\r' || next_c == '\n';
}

/// 解析 WebVTT 时间戳为毫秒（支持 `MM:SS.mmm` 与 `HH:MM:SS.mmm`，严格校验分/秒 `< 60` 与 3 位毫秒）
pub fn parseVttTimestampMs(s: []const u8): u64? {
    if (s.len < 9) {
        return null;
    }
    // Locate '.'
    let dot_pos: usize = 0;
    let found_dot = false;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '.') {
            if (found_dot) return null;
            dot_pos = i;
            found_dot = true;
        }
        i += 1;
    }
    if (!found_dot) {
        return null;
    }
    const frac = s[dot_pos + 1 ..];
    if (frac.len != 3) {
        return null;
    }
    const millis = parseUint(frac) ?? return null;

    // Split left of '.' by ':'
    const left = s[0..dot_pos];
    let c1: usize = 0;
    let c2: usize = 0;
    let colon_count: usize = 0;
    let j: usize = 0;
    while (j < left.len) {
        if (left[j] == ':') {
            if (colon_count == 0) {
                c1 = j;
            } else if (colon_count == 1) {
                c2 = j;
            } else {
                return null;
            }
            colon_count += 1;
        }
        j += 1;
    }

    let hours: u64 = 0;
    let mins: u64 = 0;
    let secs: u64 = 0;

    if (colon_count == 1) {
        const m_str = left[0..c1];
        const s_str = left[c1 + 1 ..];
        if (m_str.len != 2 || s_str.len != 2) {
            return null;
        }
        mins = parseUint(m_str) ?? return null;
        secs = parseUint(s_str) ?? return null;
    } else if (colon_count == 2) {
        const h_str = left[0..c1];
        const m_str = left[c1 + 1 .. c2];
        const s_str = left[c2 + 1 ..];
        if (h_str.len < 2 || m_str.len != 2 || s_str.len != 2) {
            return null;
        }
        hours = parseUint(h_str) ?? return null;
        mins = parseUint(m_str) ?? return null;
        secs = parseUint(s_str) ?? return null;
    } else {
        return null;
    }

    if (mins >= 60 || secs >= 60) {
        return null;
    }

    const total_secs = hours * 3600 + mins * 60 + secs;
    return total_secs * 1000 + millis;
}

/// 将毫秒时间戳格式化为标准 `HH:MM:SS.mmm`；输出缓冲不足返回 `null`
pub fn formatVttTimestampMs(ms: u64, out: []u8): ([]const u8)? {
    const millis = ms % 1000;
    const total_secs = ms / 1000;
    const secs = total_secs % 60;
    const total_mins = total_secs / 60;
    const mins = total_mins % 60;
    const hours = total_mins / 60;

    let h_buf: [20]u8 = undefined;
    let h_len: usize = 0;
    if (hours < 100) {
        h_buf[0] = ('0' as u8) + ((hours / 10) as u8);
        h_buf[1] = ('0' as u8) + ((hours % 10) as u8);
        h_len = 2;
    } else {
        let hv = hours;
        let tmp: [20]u8 = undefined;
        let tn: usize = 0;
        while (hv > 0) {
            tmp[tn] = ('0' as u8) + ((hv % 10) as u8);
            tn += 1;
            hv = hv / 10;
        }
        let k: usize = 0;
        while (k < tn) {
            h_buf[k] = tmp[tn - 1 - k];
            k += 1;
        }
        h_len = tn;
    }

    const total_len = h_len + 10; // ":MM:SS.mmm" = 10 bytes
    if (out.len < total_len) {
        return null;
    }

    let p: usize = 0;
    let i: usize = 0;
    while (i < h_len) {
        out[p] = h_buf[i];
        p += 1;
        i += 1;
    }
    out[p] = ':';
    out[p + 1] = ('0' as u8) + ((mins / 10) as u8);
    out[p + 2] = ('0' as u8) + ((mins % 10) as u8);
    out[p + 3] = ':';
    out[p + 4] = ('0' as u8) + ((secs / 10) as u8);
    out[p + 5] = ('0' as u8) + ((secs % 10) as u8);
    out[p + 6] = '.';
    out[p + 7] = ('0' as u8) + ((millis / 100) as u8);
    out[p + 8] = ('0' as u8) + (((millis / 10) % 10) as u8);
    out[p + 9] = ('0' as u8) + ((millis % 10) as u8);
    return out[0..total_len];
}

struct LineSpan {
    text: []const u8,
    next_pos: usize,
}

fn readLine(s: []const u8, start: usize): LineSpan? {
    if (start >= s.len) {
        return null;
    }
    let i = start;
    while (i < s.len && s[i] != '\n') {
        i += 1;
    }
    let line_end = i;
    if (line_end > start && s[line_end - 1] == '\r') {
        line_end -= 1;
    }
    const next_pos = if (i < s.len) i + 1 else s.len;
    return LineSpan {
        text: s[start..line_end],
        next_pos: next_pos,
    };
}

fn isNonCueBlockHeader(line: []const u8): bool {
    if (startsWith(line, "NOTE")) {
        return line.len == 4 || isSpaceOrTab(line[4]);
    }
    if (startsWith(line, "STYLE")) {
        return line.len == 5 || isSpaceOrTab(line[5]);
    }
    if (startsWith(line, "REGION")) {
        return line.len == 6 || isSpaceOrTab(line[6]);
    }
    return false;
}

/// 零堆分配 WebVTT Cue 流式迭代器（自动跳过 `WEBVTT` 头、`NOTE`/`STYLE`/`REGION` 块）
pub struct VttCueIter {
    input: []const u8,
    pos: usize = 0,
    header_done: bool = false,

    pub static fn init(input: []const u8): VttCueIter {
        return VttCueIter {
            input: stripBom(input),
            pos: 0,
            header_done: false,
        };
    }

    fn skipBlankLines(*_) {
        while (line := readLine(_.input, _.pos)) {
            if (trimSpaces(line.text).len == 0) {
                _.pos = line.next_pos;
            } else {
                break;
            }
        }
    }

    fn nextBlock(*_): ([]const u8)? {
        _.skipBlankLines();
        if (_.pos >= _.input.len) {
            return null;
        }
        const block_start = _.pos;
        let block_end = _.pos;
        while (line := readLine(_.input, _.pos)) {
            if (trimSpaces(line.text).len == 0) {
                _.pos = line.next_pos;
                break;
            }
            block_end = line.next_pos;
            _.pos = line.next_pos;
        }
        return trimLineEnd(_.input[block_start..block_end]);
    }

    /// 读取下一条合法 `VttCue`；遍历结束返回 `null`
    pub fn next(*_): VttCue? {
        if (!_.header_done) {
            _.header_done = true;
            if (isWebVttHeader(_.input)) {
                * = _.nextBlock();
            }
        }

        while (block := _.nextBlock()) {
            const l1 = readLine(block, 0) ?? continue;
            if (isNonCueBlockHeader(l1.text)) {
                continue;
            }

            let cue_id: ([]const u8)? = null;
            let timing_line = l1.text;
            let payload_start = l1.next_pos;

            if (findArrow(l1.text) == null) {
                const l2 = readLine(block, l1.next_pos) ?? continue;
                if (findArrow(l2.text) == null) {
                    continue;
                }
                cue_id = l1.text;
                timing_line = l2.text;
                payload_start = l2.next_pos;
            }

            const arrow_pos = findArrow(timing_line) ?? continue;
            const start_str = trimSpaces(timing_line[0..arrow_pos]);
            const right = trimSpaces(timing_line[arrow_pos + 3 ..]);

            let end_tok_end: usize = 0;
            while (end_tok_end < right.len && !isSpaceOrTab(right[end_tok_end])) {
                end_tok_end += 1;
            }
            const end_str = right[0..end_tok_end];
            const settings = trimSpaces(right[end_tok_end..]);

            const start_ms = parseVttTimestampMs(start_str) ?? continue;
            const end_ms = parseVttTimestampMs(end_str) ?? continue;
            if (end_ms < start_ms) {
                continue;
            }

            const payload = if (payload_start < block.len) block[payload_start..] else "";
            return VttCue {
                id: cue_id,
                start_ms: start_ms,
                end_ms: end_ms,
                settings: settings,
                payload: payload,
            };
        }
        return null;
    }
}

fn appendBytes(out: []u8, pos: *usize, s: []const u8): bool {
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

/// 规范化输出完整 WebVTT Cue 块；输出缓冲不足返回 `null`
pub fn formatVttCue(cue: VttCue, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (id := cue.id) {
        if (!appendBytes(out, &pos, id)) return null;
        if (!appendBytes(out, &pos, "\n")) return null;
    }

    let ts_buf: [32]u8 = undefined;
    const start_s = formatVttTimestampMs(cue.start_ms, ts_buf[0..]) ?? return null;
    if (!appendBytes(out, &pos, start_s)) return null;
    if (!appendBytes(out, &pos, " --> ")) return null;

    const end_s = formatVttTimestampMs(cue.end_ms, ts_buf[0..]) ?? return null;
    if (!appendBytes(out, &pos, end_s)) return null;

    if (cue.settings.len > 0) {
        if (!appendBytes(out, &pos, " ")) return null;
        if (!appendBytes(out, &pos, cue.settings)) return null;
    }

    if (cue.payload.len > 0) {
        if (!appendBytes(out, &pos, "\n")) return null;
        if (!appendBytes(out, &pos, cue.payload)) return null;
    }

    return out[0..pos];
}
