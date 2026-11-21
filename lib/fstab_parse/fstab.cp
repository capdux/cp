// Source: Absorbed from /data/dev/bun/src/sys.zig (POSIX /etc/fstab & /proc/mounts Parser)

const U32_MAX: u32 = 0xFFFF_FFFF;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
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

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return strEql(s[0..prefix.len], prefix);
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn parseU32Decimal(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    let val: u32 = 0;
    for (c of s) {
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u32 = u32(c - '0');
        if (val > (U32_MAX - digit) / 10) {
            return null;
        }
        val = val * 10 + digit;
    }
    return val;
}

fn nextFieldSlice(s: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < s.len && isSpace(s[pos.*])) {
        pos.* += 1;
    }
    if (pos.* >= s.len || s[pos.*] == '#') {
        return null;
    }
    const start = pos.*;
    while (pos.* < s.len && !isSpace(s[pos.*])) {
        pos.* += 1;
    }
    return s[start..pos.*];
}

/// POSIX `/etc/fstab` 与 `/proc/mounts` 单条挂载记录（零拷贝切片）
export struct MountEntry {
    device: []const u8;
    mount_point: []const u8;
    fs_type: []const u8;
    options_csv: []const u8;
    dump_freq: u32;
    pass_no: u32;

    /// 解析单行 fstab / mounts 记录（跳过空行与 `#` 注释；4~6 空白分隔字段，省略 dump_freq/pass_no 默认为 0）
    fn parseLine(raw: []const u8): MountEntry? {
        const line = trimAscii(raw);
        if (line.len == 0 || line[0] == '#') {
            return null;
        }

        let pos: usize = 0;
        const device = nextFieldSlice(line, &pos) ?? return null;
        const mount_point = nextFieldSlice(line, &pos) ?? return null;
        const fs_type = nextFieldSlice(line, &pos) ?? return null;
        const options_csv = nextFieldSlice(line, &pos) ?? return null;

        let dump_freq: u32 = 0;
        let pass_no: u32 = 0;

        if (f4 := nextFieldSlice(line, &pos)) {
            dump_freq = parseU32Decimal(f4) ?? return null;
            if (f5 := nextFieldSlice(line, &pos)) {
                pass_no = parseU32Decimal(f5) ?? return null;
            }
        }

        return MountEntry{
            device: device,
            mount_point: mount_point,
            fs_type: fs_type,
            options_csv: options_csv,
            dump_freq: dump_freq,
            pass_no: pass_no,
        };
    }

    /// 检查挂载选项列表是否包含指定选项（匹配裸选项名或 `key=val` 中的 `key`）
    fn hasOption(opt: []const u8): bool {
        if (opt.len == 0) {
            return false;
        }
        let start: usize = 0;
        let i: usize = 0;
        while (i <= _.options_csv.len) {
            if (i == _.options_csv.len || _.options_csv[i] == ',') {
                const seg = _.options_csv[start..i];
                if (seg.len > 0) {
                    if (eq := findByte(seg, '=')) {
                        if (strEql(seg[0..eq], opt)) {
                            return true;
                        }
                    } else if (strEql(seg, opt)) {
                        return true;
                    }
                }
                start = i + 1;
            }
            i += 1;
        }
        return false;
    }

    /// 获取 `key=val` 形式挂载选项的值（如 `uid=1000`、`mode=0755`）
    fn getOptionValue(key: []const u8): ([]const u8)? {
        if (key.len == 0) {
            return null;
        }
        let start: usize = 0;
        let i: usize = 0;
        while (i <= _.options_csv.len) {
            if (i == _.options_csv.len || _.options_csv[i] == ',') {
                const seg = _.options_csv[start..i];
                if (eq := findByte(seg, '=')) {
                    if (strEql(seg[0..eq], key)) {
                        return seg[eq + 1..];
                    }
                }
                start = i + 1;
            }
            i += 1;
        }
        return null;
    }

    /// 判断是否为只读挂载（若同时出现 ro/rw，以后者为准）
    fn isReadOnly(): bool {
        let ro = false;
        let start: usize = 0;
        let i: usize = 0;
        while (i <= _.options_csv.len) {
            if (i == _.options_csv.len || _.options_csv[i] == ',') {
                const seg = _.options_csv[start..i];
                if (strEql(seg, "ro")) {
                    ro = true;
                } else if (strEql(seg, "rw")) {
                    ro = false;
                }
                start = i + 1;
            }
            i += 1;
        }
        return ro;
    }
}

/// `/etc/fstab` 与 `/proc/mounts` 零堆分配流式行迭代器
export struct MountIter {
    src: []const u8;
    pos: usize;

    fn init(content: []const u8): MountIter {
        return MountIter{
            src: content,
            pos: 0,
        };
    }

    /// 读取下一条合法 MountEntry（自动跳过空行与注释行）
    fn next(*_): MountEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            const line = _.src[start.._.pos];
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            if (entry := MountEntry.parseLine(line)) {
                return entry;
            }
        }
        return null;
    }
}

fn isOctalDigit(c: u8): bool {
    return c >= '0' && c <= '7';
}

/// 还原 `/proc/mounts` 路径中的 3 位八进制转义（如 `\040`->空格、`\011`->`\t`、`\012`->`\n`、`\134`->`\`）
export fn unescapeMountPath(raw: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let pos: usize = 0;

    while (i < raw.len) {
        if (raw[i] == '\\' && i + 3 < raw.len
            && isOctalDigit(raw[i + 1])
            && isOctalDigit(raw[i + 2])
            && isOctalDigit(raw[i + 3]))
        {
            const val: u32 = u32(raw[i + 1] - '0') * 64
                + u32(raw[i + 2] - '0') * 8
                + u32(raw[i + 3] - '0');
            if (val <= 255) {
                if (pos >= out.len) return null;
                out[pos] = u8(val);
                pos += 1;
                i += 4;
                continue;
            }
        }

        if (pos >= out.len) return null;
        out[pos] = raw[i];
        pos += 1;
        i += 1;
    }

    return out[0..pos];
}

fn mountPointMatchesPath(mount_point: []const u8, abs_path: []const u8): bool {
    if (mount_point.len == 0 || mount_point[0] != '/') {
        return false;
    }
    if (mount_point.len == 1 && mount_point[0] == '/') {
        return true;
    }
    if (strEql(abs_path, mount_point)) {
        return true;
    }
    if (abs_path.len > mount_point.len && startsWith(abs_path, mount_point)) {
        if (mount_point[mount_point.len - 1] == '/') {
            return true;
        }
        if (abs_path[mount_point.len] == '/') {
            return true;
        }
    }
    return false;
}

/// 按最长路径前缀匹配查找覆盖给定绝对路径的挂载点（同长度以后挂载者优先）
export fn findMountForPath(content: []const u8, abs_path: []const u8): MountEntry? {
    if (abs_path.len == 0 || abs_path[0] != '/') {
        return null;
    }
    let it = MountIter.init(content);
    let best: MountEntry? = null;
    let best_len: usize = 0;

    while (true) {
        if (entry := it.next()) {
            if (mountPointMatchesPath(entry.mount_point, abs_path)) {
                if (best == null || entry.mount_point.len >= best_len) {
                    best = entry;
                    best_len = entry.mount_point.len;
                }
            }
        } else {
            break;
        }
    }
    return best;
}
