// Source: Absorbed from /data/dev/bun/src/cli/shell_completions.cp | Synced: 2026-11-22T03:58:25Z
// Pure-CP zero-allocation CLI shell completion target detector & formatter:
// - Detects bash / zsh / fish / pwsh (including Windows `.exe` and full POSIX/NT paths)
// - Formats command & description completion payloads using shell-specific delimiters
//   (space for fish, newline for bash/zsh/pwsh, tab-separated descriptions)
// - Supports prefix-filtered `CompletionItem` formatting directly into caller buffers.

/// CLI Shell 补全格式化错误集
pub err CompletionError {
    BufferTooSmall,
}

/// 支持的交互式 Shell 类型（对齐 Bun `Shell`）
pub enum CompletionShell: u8 {
    unknown = 0,
    bash = 1,
    zsh = 2,
    fish = 3,
    pwsh = 4,

    /// 从 `$SHELL` 环境变量路径或可执行文件名探测目标 Shell（兼容 `/`、`\`、`.exe` 与大小写）
    static fn fromShellPath(shell_path: []const u8): CompletionShell {
        let end = shell_path.len;
        while (end > 0 && (shell_path[end - 1] == '/' || shell_path[end - 1] == '\\')) {
            end -= 1;
        }
        let start = end;
        while (start > 0 && shell_path[start - 1] != '/' && shell_path[start - 1] != '\\') {
            start -= 1;
        }
        let base = shell_path[start..end];
        if (base.len >= 4 && eqlAsciiIgnoreCase(base[base.len - 4 ..], ".exe")) {
            base = base[0 .. base.len - 4];
        }
        if (eqlAsciiIgnoreCase(base, "bash")) {
            return CompletionShell.bash;
        }
        if (eqlAsciiIgnoreCase(base, "zsh")) {
            return CompletionShell.zsh;
        }
        if (eqlAsciiIgnoreCase(base, "fish")) {
            return CompletionShell.fish;
        }
        if (eqlAsciiIgnoreCase(base, "pwsh") || eqlAsciiIgnoreCase(base, "powershell")) {
            return CompletionShell.pwsh;
        }
        return CompletionShell.unknown;
    }

    /// 返回规范 Shell 名称字符串
    fn name(): []const u8 {
        return switch (_) {
            .bash => "bash",
            .zsh => "zsh",
            .fish => "fish",
            .pwsh => "pwsh",
            .unknown => "unknown",
        };
    }

    /// 返回当前 Shell 补全条目分隔符（`fish` 使用空格，其余使用换行，对齐 Bun `ShellCompletions`）
    fn delimiter(): []const u8 {
        if (_ == CompletionShell.fish) {
            return " ";
        }
        return "\n";
    }
}

/// 带可选描述的单个 CLI 补全候选项
pub struct CompletionItem {
    command: []const u8 = "",
    description: []const u8 = "",
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWithBytes(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (haystack[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// 按 Bun `ShellCompletions.print()` 语义格式化命令与描述列表至调用方缓冲区，返回写入字节数
pub fn formatCompletions(
    shell: CompletionShell,
    commands: []const ([]const u8),
    descriptions: []const ([]const u8),
    out: []u8,
): usize! {
    if (commands.len == 0) {
        return 0;
    }
    const delim = shell.delimiter();
    let pos: usize = 0;
    let i: usize = 0;
    while (i < commands.len) {
        if (i > 0) {
            pos = appendBytes(out, pos, delim)?;
        }
        pos = appendBytes(out, pos, commands[i])?;
        if (i < descriptions.len && descriptions[i].len > 0) {
            pos = appendBytes(out, pos, "\t")?;
            pos = appendBytes(out, pos, descriptions[i])?;
        }
        i += 1;
    }
    return pos;
}

/// 按前缀 `prefix` 过滤 `CompletionItem` 并按目标 Shell 规范格式化输出，返回写入字节数
pub fn formatCompletionItems(
    shell: CompletionShell,
    items: []const CompletionItem,
    prefix: []const u8,
    out: []u8,
): usize! {
    const delim = shell.delimiter();
    let pos: usize = 0;
    let written_count: usize = 0;
    let i: usize = 0;
    while (i < items.len) {
        const item = items[i];
        if (startsWithBytes(item.command, prefix)) {
            if (written_count > 0) {
                pos = appendBytes(out, pos, delim)?;
            }
            pos = appendBytes(out, pos, item.command)?;
            if (item.description.len > 0) {
                pos = appendBytes(out, pos, "\t")?;
                pos = appendBytes(out, pos, item.description)?;
            }
            written_count += 1;
        }
        i += 1;
    }
    return pos;
}
