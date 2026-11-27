// Source: Absorbed from /data/dev/bun/src/shell/builtin/cd.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation POSIX `cd` target parser, `~`/`-` expansion,
// lexical POSIX path normalizer, and `(cwd, prev_cwd)` directory state machine.

/// 单个规范化目录路径的最大字节长度
pub const SHELL_CD_MAX_PATH: usize = 1024;

/// 路径规范化支持的最大路径分段深度
pub const SHELL_CD_MAX_SEGMENTS: usize = 64;

/// POSIX `cd` 参数解析与路径规范化错误集
pub err ShellCdError {
    TooManyArguments,
    EmptyHome,
    NoPreviousDir,
    PathTooLong,
    BufferTooSmall,
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    if (src.len > 0) {
        @memcpy(out[pos .. pos + src.len], src);
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

/// `cd` 目标类型分类
pub enum CdTargetKind: u8 {
    Home = 0,
    Previous = 1,
    Path = 2,
}

/// 解析后的 `cd` 目标描述符
pub struct CdTarget {
    kind: CdTargetKind = .Home,
    raw_path: []const u8 = "",

    /// 解析 `cd` 命令参数切片（0 参 -> Home，`>1` 参 -> `TooManyArguments`，`-` -> Previous，`~`/`~/sub` -> Home）
    static fn parseArgs(args: []const []const u8): CdTarget! {
        if (args.len > 1) {
            return err.TooManyArguments;
        }
        if (args.len == 0) {
            return CdTarget{ kind: .Home, raw_path: "" };
        }
        const arg = args[0];
        if (arg.len == 0) {
            return CdTarget{ kind: .Path, raw_path: "" };
        }
        if (arg.len == 1 && arg[0] == '-') {
            return CdTarget{ kind: .Previous, raw_path: "-" };
        }
        if (arg.len == 1 && arg[0] == '~') {
            return CdTarget{ kind: .Home, raw_path: "" };
        }
        if (arg.len >= 2 && arg[0] == '~' && arg[1] == '/') {
            return CdTarget{ kind: .Home, raw_path: arg[2..] };
        }
        return CdTarget{ kind: .Path, raw_path: arg };
    }
}

/// 零堆分配 POSIX 词法路径规范化（折叠多余 `/`，解析 `.` 与 `..`，根路径 `/` 下 `..` 不越界）
pub fn normalizePosixPath(path: []const u8, out: []u8): usize! {
    if (path.len == 0) {
        return appendChar(out, 0, '.');
    }

    const is_abs = path[0] == '/';
    let segs: [SHELL_CD_MAX_SEGMENTS][]const u8 = undefined;
    let seg_len: usize = 0;

    let i: usize = 0;
    while (i < path.len) {
        while (i < path.len && path[i] == '/') {
            i += 1;
        }
        if (i >= path.len) {
            break;
        }
        const start = i;
        while (i < path.len && path[i] != '/') {
            i += 1;
        }
        const seg = path[start..i];
        if (seg.len == 1 && seg[0] == '.') {
            continue;
        }
        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
            if (seg_len > 0 && !(segs[seg_len - 1].len == 2 && segs[seg_len - 1][0] == '.' && segs[seg_len - 1][1] == '.')) {
                seg_len -= 1;
            } else if (!is_abs) {
                if (seg_len >= SHELL_CD_MAX_SEGMENTS) {
                    return err.PathTooLong;
                }
                segs[seg_len] = seg;
                seg_len += 1;
            }
            continue;
        }
        if (seg_len >= SHELL_CD_MAX_SEGMENTS) {
            return err.PathTooLong;
        }
        segs[seg_len] = seg;
        seg_len += 1;
    }

    let pos: usize = 0;
    if (is_abs) {
        pos = appendChar(out, pos, '/')?;
    } else if (seg_len == 0) {
        return appendChar(out, 0, '.');
    }

    let k: usize = 0;
    while (k < seg_len) {
        if (k > 0) {
            pos = appendChar(out, pos, '/')?;
        }
        pos = appendBytes(out, pos, segs[k])?;
        k += 1;
    }
    return pos;
}

/// 结合当前目录 `cwd`、上一目录 `prev_cwd` 与 `home` 将 `CdTarget` 解析为规范化 POSIX 路径
pub fn resolveCdTarget(
    cwd: []const u8,
    prev_cwd: []const u8,
    home: []const u8,
    target: CdTarget,
    out: []u8,
): usize! {
    let joined: [SHELL_CD_MAX_PATH]u8 = undefined;
    let jlen: usize = 0;

    switch (target.kind) {
        .Home => {
            if (home.len == 0) {
                return err.EmptyHome;
            }
            jlen = appendBytes(joined[0..], 0, home) catch return err.PathTooLong;
            if (target.raw_path.len > 0) {
                jlen = appendChar(joined[0..], jlen, '/') catch return err.PathTooLong;
                jlen = appendBytes(joined[0..], jlen, target.raw_path) catch return err.PathTooLong;
            }
            return normalizePosixPath(joined[0..jlen], out);
        },
        .Previous => {
            if (prev_cwd.len == 0) {
                return err.NoPreviousDir;
            }
            return normalizePosixPath(prev_cwd, out);
        },
        .Path => {
            if (target.raw_path.len == 0) {
                return normalizePosixPath(cwd, out);
            }
            if (target.raw_path[0] == '/') {
                return normalizePosixPath(target.raw_path, out);
            }
            jlen = appendBytes(joined[0..], 0, cwd) catch return err.PathTooLong;
            jlen = appendChar(joined[0..], jlen, '/') catch return err.PathTooLong;
            jlen = appendBytes(joined[0..], jlen, target.raw_path) catch return err.PathTooLong;
            return normalizePosixPath(joined[0..jlen], out);
        },
    }
}

/// 格式化对齐 Bun `cd.cp` 的标准错误消息
pub fn formatCdError(err_kind: ShellCdError, target_path: []const u8, out: []u8): usize! {
    let pos = appendBytes(out, 0, "cd: ")?;
    switch (err_kind) {
        .TooManyArguments => return appendBytes(out, pos, "too many arguments\n"),
        .EmptyHome => return appendBytes(out, pos, "HOME not set\n"),
        .NoPreviousDir => return appendBytes(out, pos, "OLDPWD not set\n"),
        .PathTooLong => return appendBytes(out, pos, "file name too long\n"),
        .BufferTooSmall => {
            if (target_path.len > 0) {
                pos = appendBytes(out, pos, "not a directory: ")?;
                pos = appendBytes(out, pos, target_path)?;
                return appendChar(out, pos, '\n');
            }
            return appendBytes(out, pos, "buffer too small\n");
        },
    }
}

/// 零堆分配 Shell `(cwd, prev_cwd)` 目录切换状态机
pub struct ShellDirState {
    cwd_buf: [SHELL_CD_MAX_PATH]u8 = undefined,
    cwd_len: usize = 0,
    prev_buf: [SHELL_CD_MAX_PATH]u8 = undefined,
    prev_len: usize = 0,

    static fn init(initial_cwd: []const u8): ShellDirState! {
        let st = ShellDirState{};
        const base = if (initial_cwd.len == 0) "/" else initial_cwd;
        st.cwd_len = normalizePosixPath(base, st.cwd_buf[0..]) catch return err.PathTooLong;
        return st;
    }

    fn cwd(): []const u8 {
        return _.cwd_buf[0.._.cwd_len];
    }

    fn prevCwd(): []const u8 {
        return _.prev_buf[0.._.prev_len];
    }

    /// 执行一次 `cd` 切换：更新 `prev_cwd` 为旧 `cwd`，写入规范化后的新 `cwd` 并返回其切片
    fn applyCd(*_, args: []const []const u8, home: []const u8): ([]const u8)! {
        const target = CdTarget.parseArgs(args)?;
        let next_buf: [SHELL_CD_MAX_PATH]u8 = undefined;
        const next_len = resolveCdTarget(
            _.cwd_buf[0.._.cwd_len],
            _.prev_buf[0.._.prev_len],
            home,
            target,
            next_buf[0..],
        )?;

        // 保存旧 cwd -> prev_cwd
        if (_.cwd_len > 0) {
            @memcpy(_.prev_buf[0.._.cwd_len], _.cwd_buf[0.._.cwd_len]);
        }
        _.prev_len = _.cwd_len;

        // 写入新 cwd
        if (next_len > 0) {
            @memcpy(_.cwd_buf[0..next_len], next_buf[0..next_len]);
        }
        _.cwd_len = next_len;
        return _.cwd_buf[0.._.cwd_len];
    }
}
