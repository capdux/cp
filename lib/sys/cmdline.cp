// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T15:11:29Z (POSIX NUL-Delimited cmdline & environ Parser)

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

/// 零堆分配 NUL (`\0`) 分隔参数迭代器（适用于 POSIX `/proc/<pid>/cmdline`）
/// 兼容末尾有/无 `\0`，保留中间空参数
pub struct NulArgIter {
    buf: []const u8,
    pos: usize = 0,

    static fn init(buf: []const u8): NulArgIter {
        return NulArgIter{ buf: buf, pos: 0 };
    }

    fn next(*_): ([]const u8)? {
        if (_.pos >= _.buf.len) {
            return null;
        }
        const start = _.pos;
        while (_.pos < _.buf.len && _.buf[_.pos] != 0) {
            _.pos += 1;
        }
        const slice = _.buf[start.._.pos];
        if (_.pos < _.buf.len && _.buf[_.pos] == 0) {
            _.pos += 1;
        }
        return slice;
    }
}

/// 统计 NUL 分隔缓冲区中的参数总数
pub fn countArgs(buf: []const u8): usize {
    let it = NulArgIter.init(buf);
    let n: usize = 0;
    while (it.next() != null) {
        n += 1;
    }
    return n;
}

/// 按 0-based 下标提取第 `index` 个参数切片，越界返回 `null`
pub fn argAt(buf: []const u8, index: usize): ([]const u8)? {
    let it = NulArgIter.init(buf);
    let i: usize = 0;
    while (arg := it.next()) {
        if (i == index) {
            return arg;
        }
        i += 1;
    }
    return null;
}

/// `/proc/<pid>/environ` 单条环境变量键值对（零拷贝切片）
pub struct ProcEnvEntry {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// 零堆分配 `/proc/<pid>/environ` 环境变量迭代器
/// 按 `\0` 切分，每项按首个 `=` 拆为 `key` 与 `value`，自动跳过无 `=` 或空 `key` 的畸形项
pub struct ProcEnvIter {
    inner: NulArgIter,

    static fn init(buf: []const u8): ProcEnvIter {
        return ProcEnvIter{ inner: NulArgIter.init(buf) };
    }

    fn next(*_): ProcEnvEntry? {
        while (item := _.inner.next()) {
            let eq_idx: usize? = null;
            let i: usize = 0;
            while (i < item.len) {
                if (item[i] == '=') {
                    eq_idx = i;
                    break;
                }
                i += 1;
            }
            if (eq := eq_idx) {
                if (eq == 0) {
                    continue;
                }
                return ProcEnvEntry{
                    key: item[0..eq],
                    value: item[(eq + 1)..item.len],
                };
            }
        }
        return null;
    }
}

/// 在 `/proc/<pid>/environ` 缓冲区中精确查找环境变量名（遵循 POSIX `getenv` 语义，返回首次匹配值）
pub fn findProcEnv(buf: []const u8, key: []const u8): ([]const u8)? {
    if (key.len == 0) { return null; }
    let it = ProcEnvIter.init(buf);
    while (entry := it.next()) {
        if (eqlBytes(entry.key, key)) {
            return entry.value;
        }
    }
    return null;
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

fn argNeedsShellQuoting(arg: []const u8): bool {
    if (arg.len == 0) { return true; }
    let i: usize = 0;
    while (i < arg.len) {
        const c = arg[i];
        if (c <= 0x20 || c == 0x7F || c == '\'' || c == '"' || c == '\\' ||
            c == '$' || c == '`' || c == ';' || c == '|' || c == '&' ||
            c == '<' || c == '>' || c == '(' || c == ')' || c == '*' ||
            c == '?' || c == '[' || c == ']' || c == '#' || c == '~')
        {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 将 NUL 分隔的 argv 缓冲区格式化为人类可读的空格分隔命令行
/// 空参数或含空格/引号/Shell 元字符的参数自动以 POSIX 单引号包裹（内部 `'` 转义为 `'\''`）
pub fn formatCmdlineDisplay(buf: []const u8, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    let it = NulArgIter.init(buf);
    let first = true;

    while (arg := it.next()) {
        if (!first) {
            w.writeByte(' ');
        }
        first = false;

        if (!argNeedsShellQuoting(arg)) {
            w.writeStr(arg);
        } else {
            w.writeByte('\'');
            let i: usize = 0;
            while (i < arg.len) {
                if (arg[i] == '\'') {
                    w.writeStr("'\\''");
                } else {
                    w.writeByte(arg[i]);
                }
                i += 1;
            }
            w.writeByte('\'');
        }
    }

    return w.finish();
}
