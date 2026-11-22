// Source: Absorbed from /data/dev/bun/src/cli/install_completions_command.cp | Synced: 2026-11-22T07:24:30Z
// Pure-CP zero-allocation shell completions installer path planner:
// - Maps `CompletionShell` -> canonical completion filename (`_bun` / `bun.fish` / `bun.completion.bash`)
// - Generates ordered candidate completion directories for Fish, Zsh (including `$fpath`), and Bash
// - Resolves `$ZDOTDIR/.zshrc` / `$HOME/.zshrc` startup file path & checks whether `source` line is present
// - Generates POSIX `bunx` symlink fallback targets and Windows `bun\bin` uninstaller eligibility checks.

import { CompletionShell } from "./completions.cp";

/// 单个 Shell 补全目录探测链允许的最大候选路径数
pub const COMPLETION_MAX_CANDIDATES: usize = 16;

/// 单个候选路径缓冲最大字节数
pub const COMPLETION_PATH_BUF_CAP: usize = 2048;

/// Shell 补全安装规划错误集
pub err CompletionInstallError {
    UnsupportedShell,
    BufferTooSmall,
    TooManyCandidates,
    InvalidPath,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (bytesEql(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) return false;
    return bytesEql(s[s.len - suffix.len..], suffix);
}

fn stripTrailingSlashes(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 1 && (s[end - 1] == '/' || s[end - 1] == '\\')) {
        end -= 1;
    }
    return s[0..end];
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

/// 补全安装环境快照（零分配纯逻辑输入，不直接读取进程环境变量）
pub struct CompletionInstallEnv {
    home: []const u8 = "",
    xdg_config_home: []const u8 = "",
    xdg_data_home: []const u8 = "",
    fpath: []const u8 = "",
    zdotdir: []const u8 = "",
    install_dir: []const u8 = "",
    is_macos: bool = false,
    is_arm64: bool = false,
}

/// 候选目录列表（所有路径切片连续存储在内部 `buf` 中，零堆分配）
pub struct CompletionCandidateList {
    buf: [COMPLETION_PATH_BUF_CAP]u8 = [0; COMPLETION_PATH_BUF_CAP],
    buf_len: usize = 0,
    offsets: [COMPLETION_MAX_CANDIDATES]usize = [0; COMPLETION_MAX_CANDIDATES],
    lens: [COMPLETION_MAX_CANDIDATES]usize = [0; COMPLETION_MAX_CANDIDATES],
    count: usize = 0,

    fn pushRaw(*_, path: []const u8)! {
        if (path.len == 0) return;
        if (_.count >= COMPLETION_MAX_CANDIDATES) {
            return err.TooManyCandidates;
        }
        const start = _.buf_len;
        const end = appendBytes(_.buf[0..], start, path)?;
        _.offsets[_.count] = start;
        _.lens[_.count] = end - start;
        _.buf_len = end;
        _.count += 1;
    }

    fn pushJoined(*_, base: []const u8, rel: []const u8)! {
        if (base.len == 0) return;
        if (_.count >= COMPLETION_MAX_CANDIDATES) {
            return err.TooManyCandidates;
        }
        const clean = stripTrailingSlashes(base);
        const start = _.buf_len;
        let pos = appendBytes(_.buf[0..], start, clean)?;
        if (rel.len > 0) {
            if (clean.len > 0 && clean[clean.len - 1] != '/') {
                pos = appendBytes(_.buf[0..], pos, "/")?;
            }
            pos = appendBytes(_.buf[0..], pos, rel)?;
        }
        _.offsets[_.count] = start;
        _.lens[_.count] = pos - start;
        _.buf_len = pos;
        _.count += 1;
    }

    fn at(idx: usize): ([]const u8)? {
        if (idx >= _.count) {
            return null;
        }
        const off = _.offsets[idx];
        return _.buf[off..off + _.lens[idx]];
    }
}

/// 返回指定 Shell 对应的标准补全文件名（对齐 Bun：`bun.fish` / `_bun` / `bun.completion.bash`）
pub fn completionFilename(shell: CompletionShell, tool_name: []const u8, out: []u8): usize! {
    if (tool_name.len == 0) {
        return err.InvalidPath;
    }
    return switch (shell) {
        .fish => blk: {
            let p = appendBytes(out, 0, tool_name)?;
            break :blk appendBytes(out, p, ".fish")?;
        },
        .zsh => blk: {
            let p = appendBytes(out, 0, "_")?;
            break :blk appendBytes(out, p, tool_name)?;
        },
        .bash => blk: {
            let p = appendBytes(out, 0, tool_name)?;
            break :blk appendBytes(out, p, ".completion.bash")?;
        },
        else => err.UnsupportedShell,
    };
}

/// 按 Bun 精确探测优先级生成 Shell 补全安装候选目录列表
pub fn collectCompletionDirs(
    shell: CompletionShell,
    env: CompletionInstallEnv,
): CompletionCandidateList! {
    let list = CompletionCandidateList{};

    if (shell == .fish) {
        if (env.xdg_config_home.len > 0) {
            list.pushJoined(env.xdg_config_home, "fish/completions")?;
        }
        if (env.xdg_data_home.len > 0) {
            list.pushJoined(env.xdg_data_home, "fish/completions")?;
        }
        if (env.home.len > 0) {
            list.pushJoined(env.home, ".config/fish/completions")?;
        }
        if (env.is_macos) {
            if (env.is_arm64) {
                list.pushRaw("/opt/homebrew/share/fish/completions")?;
            } else {
                list.pushRaw("/usr/local/share/fish/completions")?;
            }
        }
        list.pushRaw("/etc/fish/completions")?;
        return list;
    }

    if (shell == .zsh) {
        // First: space-separated $fpath entries
        if (env.fpath.len > 0) {
            let i: usize = 0;
            while (i < env.fpath.len) {
                while (i < env.fpath.len && env.fpath[i] == ' ') {
                    i += 1;
                }
                const start = i;
                while (i < env.fpath.len && env.fpath[i] != ' ') {
                    i += 1;
                }
                if (i > start && list.count < COMPLETION_MAX_CANDIDATES) {
                    list.pushRaw(env.fpath[start..i])?;
                }
            }
        }
        if (env.xdg_data_home.len > 0 && list.count < COMPLETION_MAX_CANDIDATES) {
            list.pushJoined(env.xdg_data_home, "zsh-completions")?;
        }
        if (env.install_dir.len > 0 && list.count < COMPLETION_MAX_CANDIDATES) {
            list.pushRaw(env.install_dir)?;
        }
        if (env.home.len > 0) {
            if (list.count < COMPLETION_MAX_CANDIDATES) {
                list.pushJoined(env.home, ".oh-my-zsh/completions")?;
            }
            if (list.count < COMPLETION_MAX_CANDIDATES) {
                list.pushJoined(env.home, ".bun")?;
            }
        }
        if (list.count < COMPLETION_MAX_CANDIDATES) list.pushRaw("/usr/local/share/zsh/site-functions")?;
        if (list.count < COMPLETION_MAX_CANDIDATES) list.pushRaw("/usr/local/share/zsh/completions")?;
        if (list.count < COMPLETION_MAX_CANDIDATES) list.pushRaw("/opt/homebrew/share/zsh/completions")?;
        if (list.count < COMPLETION_MAX_CANDIDATES) list.pushRaw("/opt/homebrew/share/zsh/site-functions")?;
        return list;
    }

    if (shell == .bash) {
        if (env.xdg_data_home.len > 0) {
            list.pushJoined(env.xdg_data_home, "bash-completion/completions")?;
        }
        if (env.xdg_config_home.len > 0) {
            list.pushJoined(env.xdg_config_home, "bash-completion/completions")?;
        }
        if (env.home.len > 0) {
            list.pushJoined(env.home, ".oh-my-bash/custom/completions")?;
            list.pushJoined(env.home, ".bash_completion.d")?;
        }
        list.pushRaw("/opt/homebrew/share/bash-completion/completions/")?;
        list.pushRaw("/opt/local/share/bash-completion/completions/")?;
        return list;
    }

    return err.UnsupportedShell;
}

/// 解析 Zsh 启动配置文件路径（优先 `$ZDOTDIR/.zshrc`，否则 `$HOME/.zshrc`）
pub fn resolveZshrcPath(env: CompletionInstallEnv, out: []u8): usize! {
    const base = if (env.zdotdir.len > 0) env.zdotdir else env.home;
    if (base.len == 0) {
        return err.InvalidPath;
    }
    const clean = stripTrailingSlashes(base);
    let pos = appendBytes(out, 0, clean)?;
    if (clean[clean.len - 1] != '/') {
        pos = appendBytes(out, pos, "/")?;
    }
    return appendBytes(out, pos, ".zshrc");
}

/// 检查 `.zshrc` 内容中是否已经包含指定补全文件路径（决定是否提示用户追加 `source` 行）
pub fn zshrcNeedsCompletionHint(zshrc_contents: []const u8, completions_file_path: []const u8): bool {
    if (completions_file_path.len == 0) {
        return false;
    }
    return !containsSubstr(zshrc_contents, completions_file_path);
}

/// 格式化建议用户追加至 `.zshrc` 的加载语句（`[ -s "<path>" ] && source "<path>"\n`）
pub fn formatZshrcSourceLine(completions_file_path: []const u8, out: []u8): usize! {
    if (completions_file_path.len == 0) {
        return err.InvalidPath;
    }
    let pos = appendBytes(out, 0, "[ -s \"")?;
    pos = appendBytes(out, pos, completions_file_path)?;
    pos = appendBytes(out, pos, "\" ] && source \"")?;
    pos = appendBytes(out, pos, completions_file_path)?;
    return appendBytes(out, pos, "\"\n");
}

/// 生成 POSIX 下 `bunx` / `capx` 软链接的回退安装候选路径（同目录 → `$INSTALL_DIR/bin` → `$HOME/.bun/bin` → `$HOME/.local/bin`）
pub fn collectAliasSymlinkTargets(
    exe_dir: []const u8,
    alias_name: []const u8,
    env: CompletionInstallEnv,
): CompletionCandidateList! {
    if (alias_name.len == 0) {
        return err.InvalidPath;
    }
    let list = CompletionCandidateList{};
    if (exe_dir.len > 0) {
        list.pushJoined(exe_dir, alias_name)?;
    }
    if (env.install_dir.len > 0) {
        let tmp: [256]u8 = [0; 256];
        let p = appendBytes(tmp[0..], 0, "bin/")?;
        p = appendBytes(tmp[0..], p, alias_name)?;
        list.pushJoined(env.install_dir, tmp[0..p])?;
    }
    if (env.home.len > 0) {
        let tmp: [256]u8 = [0; 256];
        let p1 = appendBytes(tmp[0..], 0, ".bun/bin/")?;
        p1 = appendBytes(tmp[0..], p1, alias_name)?;
        list.pushJoined(env.home, tmp[0..p1])?;

        let p2 = appendBytes(tmp[0..], 0, ".local/bin/")?;
        p2 = appendBytes(tmp[0..], p2, alias_name)?;
        list.pushJoined(env.home, tmp[0..p2])?;
    }
    return list;
}

/// 判定 Windows 可执行目录是否位于 `bun\bin` 下（仅此时写入 `uninstall.ps1`）
pub fn isWindowsInstallerBinDir(exe_dirname: []const u8): bool {
    const clean = stripTrailingSlashes(exe_dirname);
    return endsWith(clean, "bun\\bin") || endsWith(clean, "bun/bin");
}
