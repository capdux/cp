// Source: Absorbed from /data/dev/bun/src/shell/builtin/exit.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// Pure-CP Zero-Allocation POSIX Shell `exit` Code Parser & `export` Declaration Codec
// Absorbed from Bun: src/shell/builtin/{exit,export}.cp & docs/runtime/shell.mdx
// ============================================================================

/// POSIX 标准退出码常量
pub const SHELL_EXIT_SUCCESS: u8 = 0;
pub const SHELL_EXIT_GENERAL_ERROR: u8 = 1;
pub const SHELL_EXIT_MISUSE: u8 = 2;
pub const SHELL_EXIT_NOT_EXECUTABLE: u8 = 126;
pub const SHELL_EXIT_NOT_FOUND: u8 = 127;
pub const SHELL_EXIT_SIGNAL_BASE: u8 = 128;

/// `exit` / `export` 参数解析与格式化错误集
pub err ShellExitExportError {
    NumericArgRequired,
    TooManyArguments,
    InvalidIdentifier,
    BufferTooSmall,
}

/// 零拷贝 `export` 参数解析结果（对齐 Bun `src/shell/builtin/export.cp`）
pub struct ExportEntry {
    name: []const u8 = "",
    value: []const u8 = "",
    has_value: bool = false,
}

/// 判断是否为合法 POSIX Shell 环境变量标识符（`[A-Za-z_][A-Za-z0-9_]*`）
pub fn isValidShellIdentifier(name: []const u8): bool {
    if (name.len == 0) {
        return false;
    }
    const first = name[0];
    const first_ok = (first >= 'a' && first <= 'z') ||
        (first >= 'A' && first <= 'Z') ||
        first == '_';
    if (!first_ok) {
        return false;
    }
    let i: usize = 1;
    while (i < name.len) {
        const ch = name[i];
        const ok = (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_';
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 逐位模 256 解析 POSIX `exit` 退出码字符串（支持 `+`/`-` 符号与任意长度十进制整数，永不溢出）：
/// - `"0"` -> `0`, `"42"` -> `42`, `"256"` -> `0`, `"257"` -> `1`, `"-1"` -> `255`
/// - 非数字或空串返回 `err.NumericArgRequired`
pub fn parseExitCodeStr(raw: []const u8): u8! {
    if (raw.len == 0) {
        return err.NumericArgRequired;
    }
    let idx: usize = 0;
    let negative = false;
    if (raw[0] == '+') {
        idx = 1;
    } else if (raw[0] == '-') {
        negative = true;
        idx = 1;
    }
    if (idx >= raw.len) {
        return err.NumericArgRequired;
    }
    let rem: u16 = 0;
    while (idx < raw.len) {
        const ch = raw[idx];
        if (ch < '0' || ch > '9') {
            return err.NumericArgRequired;
        }
        rem = (rem * @as(u16, 10) + @as(u16, ch - '0')) % @as(u16, 256);
        idx += 1;
    }
    if (negative) {
        const inv: u16 = (@as(u16, 256) - rem) % @as(u16, 256);
        return @intCast(inv);
    }
    return @intCast(rem);
}

/// 解析 `exit` 内置命令参数列表（对齐 Bun `src/shell/builtin/exit.cp`）：
/// - 0 参数：返回 `last_exit_code`
/// - 1 参数：按模 256 解析退出码
/// - 2+ 参数：返回 `err.TooManyArguments`
pub fn parseExitArgs(args: []const ([]const u8), last_exit_code: u8): u8! {
    if (args.len == 0) {
        return last_exit_code;
    }
    if (args.len > 1) {
        return err.TooManyArguments;
    }
    return parseExitCodeStr(args[0]);
}

/// 将 POSIX 终止信号编号映射为标准退出码 (`128 + signal`)
pub fn exitCodeFromSignal(sig: u8): u8 {
    return SHELL_EXIT_SIGNAL_BASE + (sig & 0x7f);
}

/// 判断退出码是否表示被 POSIX 信号终止 (`> 128`)
pub fn isSignalExitCode(code: u8): bool {
    return code > SHELL_EXIT_SIGNAL_BASE;
}

/// 解析单条 `export` 命令参数（支持 `NAME=VALUE` 与裸 `NAME` 两种形式，严格校验标识符）
pub fn parseExportArg(arg: []const u8): ExportEntry! {
    if (arg.len == 0) {
        return err.InvalidIdentifier;
    }
    let eq_idx: usize = arg.len;
    let i: usize = 0;
    while (i < arg.len) {
        if (arg[i] == '=') {
            eq_idx = i;
            break;
        }
        i += 1;
    }
    if (eq_idx == arg.len) {
        if (!isValidShellIdentifier(arg)) {
            return err.InvalidIdentifier;
        }
        return ExportEntry{
            name: arg,
            value: "",
            has_value: false,
        };
    }
    const name = arg[0..eq_idx];
    if (!isValidShellIdentifier(name)) {
        return err.InvalidIdentifier;
    }
    return ExportEntry{
        name: name,
        value: arg[eq_idx + 1 ..],
        has_value: true,
    };
}

/// 零堆格式化单条 `export` 输出行 `NAME=VALUE\n`（对齐 Bun `export` 无参输出格式）
pub fn formatExportLine(buf: []u8, name: []const u8, value: []const u8): ([]const u8)! {
    if (!isValidShellIdentifier(name)) {
        return err.InvalidIdentifier;
    }
    const needed = name.len + 1 + value.len + 1;
    if (needed > buf.len) {
        return err.BufferTooSmall;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < name.len) {
        buf[pos] = name[i];
        pos += 1;
        i += 1;
    }
    buf[pos] = '=';
    pos += 1;
    i = 0;
    while (i < value.len) {
        buf[pos] = value[i];
        pos += 1;
        i += 1;
    }
    buf[pos] = '\n';
    pos += 1;
    return buf[0..pos];
}
