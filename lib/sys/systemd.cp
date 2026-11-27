// Source: Absorbed from /data/dev/bun/src/ini.zig | Synced: 2026-11-21T15:22:08Z (Linux systemd Unit File Parser)

/// 单条解析后的 systemd 单元配置项
pub struct UnitEntry {
    section: []const u8 = "",
    key: []const u8 = "",
    value: []const u8 = "",
    line_no: usize = 0,
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimHoriz(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isHorizSpace(s[start])) { start += 1; }
    while (end > start && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn trimRightHoriz(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[0..end];
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn appendBytes(buf: []u8, pos: *usize, s: []const u8) {
    let i: usize = 0;
    while (i < s.len && pos.* < buf.len) {
        buf[pos.*] = s[i];
        pos.* += 1;
        i += 1;
    }
}

/// 零堆分配 Linux systemd `.service`/`.timer`/`.socket` 单元文件流式迭代器
/// 自动剥离 UTF-8 BOM、跳过空行与 `#`/`;` 注释行、识别 `[Section]` 段头、支持行尾 `\` 续行并保留重复键顺序
pub struct UnitIter {
    src: []const u8,
    pos: usize = 0,
    line_no: usize = 1,
    current_section: []const u8 = "",

    static fn init(content: []const u8): UnitIter {
        let start_pos: usize = 0;
        if (content.len >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF) {
            start_pos = 3;
        }
        return UnitIter{
            src: content,
            pos: start_pos,
            line_no: 1,
            current_section: "",
        };
    }

    fn readRawLine(*_): ([]const u8)? {
        if (_.pos >= _.src.len) { return null; }
        const start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != '\n') {
            _.pos += 1;
        }
        let end = _.pos;
        if (end > start && _.src[end - 1] == '\r') {
            end -= 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
        }
        _.line_no += 1;
        return _.src[start..end];
    }

    /// 读取下一条配置项，将续行折叠后的值写入 `val_buf`
    fn next(*_, val_buf: []u8): UnitEntry? {
        while (true) {
            const cur_line = _.line_no;
            const raw = _.readRawLine() ?? return null;
            const trimmed = trimHoriz(raw);

            if (trimmed.len == 0 || trimmed[0] == '#' || trimmed[0] == ';') {
                continue;
            }

            // 段头 [Unit] / [Service] / [Install] ...
            if (trimmed[0] == '[' && trimmed.len >= 2 && trimmed[trimmed.len - 1] == ']') {
                _.current_section = trimHoriz(trimmed[1..(trimmed.len - 1)]);
                continue;
            }

            // 查找首个 '='
            let eq_idx: usize? = null;
            let i: usize = 0;
            while (i < trimmed.len) {
                if (trimmed[i] == '=') {
                    eq_idx = i;
                    break;
                }
                i += 1;
            }
            const eq = eq_idx ?? continue;
            const key = trimHoriz(trimmed[0..eq]);
            if (key.len == 0) {
                continue;
            }

            const first_val = trimHoriz(trimmed[(eq + 1)..trimmed.len]);
            let wpos: usize = 0;
            let has_cont = (first_val.len > 0 && first_val[first_val.len - 1] == '\\');
            const first_piece = if (has_cont) trimRightHoriz(first_val[0..(first_val.len - 1)]) else first_val;
            if (first_piece.len > 0) {
                appendBytes(val_buf, *wpos, first_piece);
            }

            // systemd.syntax(7) 行尾 '\' 续行：跳过续行中的 '#' / ';' 注释行，以单空格拼接
            while (has_cont) {
                const next_raw = _.readRawLine() ?? break;
                const nt = trimHoriz(next_raw);
                if (nt.len > 0 && (nt[0] == '#' || nt[0] == ';')) {
                    continue;
                }
                has_cont = (nt.len > 0 && nt[nt.len - 1] == '\\');
                const piece = if (has_cont) trimRightHoriz(nt[0..(nt.len - 1)]) else nt;
                if (piece.len > 0) {
                    if (wpos > 0 && wpos < val_buf.len && val_buf[wpos - 1] != ' ') {
                        val_buf[wpos] = ' ';
                        wpos += 1;
                    }
                    appendBytes(val_buf, *wpos, piece);
                }
            }

            return UnitEntry{
                section: _.current_section,
                key: key,
                value: trimHoriz(val_buf[0..wpos]),
                line_no: cur_line,
            };
        }
        return null;
    }
}

/// 按 systemd 语义查询 `[section]` 下 `key` 的最终生效值：
/// 同名键后者覆盖前者，空赋值 `Key=` 清空先前值；结果写入 `val_buf`
pub fn getUnitValue(content: []const u8, section: []const u8, key: []const u8, val_buf: []u8): ([]const u8)? {
    let it = UnitIter.init(content);
    let scratch: [1024]u8 = undefined;
    let found = false;
    let saved_len: usize = 0;

    while (entry := it.next(scratch[0..])) {
        if (eqlBytes(entry.section, section) && eqlBytes(entry.key, key)) {
            if (entry.value.len == 0) {
                found = false;
                saved_len = 0;
            } else {
                if (entry.value.len > val_buf.len) {
                    return null;
                }
                @memcpy(val_buf[0..entry.value.len], entry.value);
                saved_len = entry.value.len;
                found = true;
            }
        }
    }

    if (!found) {
        return null;
    }
    return val_buf[0..saved_len];
}

/// 统计 `[section]` 下指定 `key` 出现的条目总数（如 `[Service]` 下的 `Environment` / `ExecStartPre`）
pub fn countSectionKey(content: []const u8, section: []const u8, key: []const u8): usize {
    let it = UnitIter.init(content);
    let scratch: [16]u8 = [0; 16];
    let count: usize = 0;
    while (entry := it.next(scratch[0..])) {
        if (eqlBytes(entry.section, section) && eqlBytes(entry.key, key)) {
            count += 1;
        }
    }
    return count;
}
