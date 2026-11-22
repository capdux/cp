// Source: Absorbed from /data/dev/bun/src/shell/builtin/rm.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/shell/builtin/rm.cp & /data/dev/bun/docs/runtime/shell.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation POSIX `rm` flag parser (`-f`/`-r`/`-R`/`-v`/`-d`/`-i`/`-I`,
// `--preserve-root`/`--no-preserve-root`/`--recursive`/`--verbose`/`--dir`/`--interactive=*`,
// combined short flags, `--` sentinel), root & dot-segment deletion guard, and removal planner.

/// 单次 `rm` 命令支持的最大目标路径数量
pub const SHELL_RM_MAX_TARGETS: usize = 64;

/// 内部规范化路径最大段数
const RM_MAX_PATH_SEGS: usize = 64;

/// 内部规范化路径缓冲区容量
const RM_PATH_BUF_CAP: usize = 512;

/// POSIX `rm` 标志解析与安全删除校验错误集
pub err ShellRmError {
    MissingOperand,
    IllegalOption,
    TooManyTargets,
    RefusingToRemoveRoot,
    RefusingToRemoveDotOrDotDot,
    CannotRemoveDirectoryWithoutFlag,
    BufferTooSmall,
}

/// `rm` 交互确认策略（对应 Bun `OptsPromptBehaviour`）
pub enum RmPromptBehaviour: u8 {
    Never = 0,
    Once = 1,
    Always = 2,
}

/// 目标路径类型提示（用于安全删除预检）
pub enum RmPathKind: u8 {
    File = 0,
    EmptyDir = 1,
    NonEmptyDir = 2,
    Missing = 3,
}

/// 路径分隔风格（对应 Bun `ShellJoinStyle`）
pub enum RmJoinStyle: u8 {
    Posix = 0,
    Windows = 1,

    static fn fromPath(p: []const u8): RmJoinStyle {
        let backslash: usize = p.len;
        let forwardslash: usize = p.len;
        let i: usize = 0;
        while (i < p.len) {
            if (p[i] == '\\' && backslash == p.len) {
                backslash = i;
            } else if (p[i] == '/' && forwardslash == p.len) {
                forwardslash = i;
            }
            i += 1;
        }
        if (forwardslash <= backslash) {
            return RmJoinStyle.Posix;
        }
        return RmJoinStyle.Windows;
    }
}

/// 单个目标路径的预检决策结果
pub enum RmTargetDecision: u8 {
    Remove = 0,
    SkipMissing = 1,
    PromptBeforeRemove = 2,
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

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return ShellRmError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn isDrivePrefix(s: []const u8): bool {
    if (s.len < 2) {
        return false;
    }
    const c = s[0];
    const is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    return is_alpha && s[1] == ':';
}

/// 判断目标路径去除尾部斜杠后的最后一段是否为 `.` 或 `..`（POSIX 严禁删除）
pub fn isRmDotOrDotDotTarget(target: []const u8): bool {
    if (target.len == 0) {
        return false;
    }
    let end = target.len;
    while (end > 0 && (target[end - 1] == '/' || target[end - 1] == '\\')) {
        end -= 1;
    }
    if (end == 0) {
        return false;
    }
    let start = end;
    while (start > 0 && target[start - 1] != '/' && target[start - 1] != '\\') {
        start -= 1;
    }
    const last = target[start..end];
    if (last.len == 1 && last[0] == '.') {
        return true;
    }
    if (last.len == 2 && last[0] == '.' && last[1] == '.') {
        return true;
    }
    return false;
}

struct SegRange {
    start: usize = 0,
    end: usize = 0,
}

/// 词法规范化 `cwd` + `target`，判断最终目标是否指向系统根目录（`/` 或 Windows `X:/`）
pub fn isRmRootPath(cwd: []const u8, target: []const u8): bool {
    if (target.len == 0) {
        return false;
    }

    let raw: [RM_PATH_BUF_CAP]u8 = [0; RM_PATH_BUF_CAP];
    let raw_len: usize = 0;

    const tgt_is_drive = isDrivePrefix(target);
    const tgt_is_abs = tgt_is_drive || target[0] == '/' || target[0] == '\\';
    let has_drive = tgt_is_drive;
    let is_abs = tgt_is_abs;

    if (!tgt_is_abs && cwd.len > 0) {
        if (isDrivePrefix(cwd)) {
            has_drive = true;
            is_abs = true;
        } else if (cwd[0] == '/' || cwd[0] == '\\') {
            is_abs = true;
        }
        let i: usize = 0;
        while (i < cwd.len && raw_len < RM_PATH_BUF_CAP) {
            raw[raw_len] = if (cwd[i] == '\\') '/' else cwd[i];
            raw_len += 1;
            i += 1;
        }
        if (raw_len > 0 && raw[raw_len - 1] != '/' && raw_len < RM_PATH_BUF_CAP) {
            raw[raw_len] = '/';
            raw_len += 1;
        }
    }

    let k: usize = 0;
    while (k < target.len && raw_len < RM_PATH_BUF_CAP) {
        raw[raw_len] = if (target[k] == '\\') '/' else target[k];
        raw_len += 1;
        k += 1;
    }

    if (!is_abs) {
        return false;
    }

    let pos: usize = 0;
    if (has_drive && raw_len >= 2) {
        pos = 2;
    }

    let segs: [RM_MAX_PATH_SEGS]SegRange = [SegRange{}; RM_MAX_PATH_SEGS];
    let seg_count: usize = 0;

    while (pos < raw_len) {
        while (pos < raw_len && raw[pos] == '/') {
            pos += 1;
        }
        if (pos >= raw_len) {
            break;
        }
        const start = pos;
        while (pos < raw_len && raw[pos] != '/') {
            pos += 1;
        }
        const seg = raw[start..pos];
        if (seg.len == 1 && seg[0] == '.') {
            continue;
        }
        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
            if (seg_count > 0) {
                seg_count -= 1;
            }
            continue;
        }
        if (seg_count < RM_MAX_PATH_SEGS) {
            segs[seg_count] = SegRange{ start: start, end: pos };
            seg_count += 1;
        }
    }

    return seg_count == 0;
}

/// POSIX `rm` 解析后的标志选项集合（严格对齐 Bun `shell/builtin/rm.cp` `Opts`）
pub struct RmOpts {
    preserve_root: bool = true,
    force: bool = false,
    prompt_behaviour: RmPromptBehaviour = RmPromptBehaviour.Never,
    recursive: bool = false,
    verbose: bool = false,
    remove_empty_dirs: bool = false,

    static fn defaults(): RmOpts {
        return RmOpts{};
    }

    /// 当前标志组合是否允许删除目录
    fn canRemoveDirectory(is_empty: bool): bool {
        if (_.recursive) {
            return true;
        }
        return is_empty && _.remove_empty_dirs;
    }

    /// 解析单个参数是否为标志；返回 `true` 表示已消费为标志，`false` 表示操作数开始
    fn parseFlag(*_, arg: []const u8): bool! {
        if (arg.len <= 1 || arg[0] != '-') {
            return false;
        }
        if (arg[1] == '-') {
            if (arg.len == 2) {
                // `--` 由调用方作为标志终止哨兵处理
                return false;
            }
            if (bytesEql(arg, "--preserve-root")) {
                _.preserve_root = true;
                return true;
            }
            if (bytesEql(arg, "--no-preserve-root")) {
                _.preserve_root = false;
                return true;
            }
            if (bytesEql(arg, "--recursive")) {
                _.recursive = true;
                return true;
            }
            if (bytesEql(arg, "--force")) {
                _.force = true;
                _.prompt_behaviour = RmPromptBehaviour.Never;
                return true;
            }
            if (bytesEql(arg, "--verbose")) {
                _.verbose = true;
                return true;
            }
            if (bytesEql(arg, "--dir")) {
                _.remove_empty_dirs = true;
                return true;
            }
            if (bytesEql(arg, "--interactive=never")) {
                _.prompt_behaviour = RmPromptBehaviour.Never;
                return true;
            }
            if (bytesEql(arg, "--interactive=once")) {
                _.prompt_behaviour = RmPromptBehaviour.Once;
                return true;
            }
            if (bytesEql(arg, "--interactive=always")) {
                _.prompt_behaviour = RmPromptBehaviour.Always;
                return true;
            }
            return ShellRmError.IllegalOption;
        }

        let i: usize = 1;
        while (i < arg.len) {
            const c = arg[i];
            if (c == 'f') {
                _.force = true;
                _.prompt_behaviour = RmPromptBehaviour.Never;
            } else if (c == 'r' || c == 'R') {
                _.recursive = true;
            } else if (c == 'v') {
                _.verbose = true;
            } else if (c == 'd') {
                _.remove_empty_dirs = true;
            } else if (c == 'i') {
                _.prompt_behaviour = RmPromptBehaviour.Always;
            } else if (c == 'I') {
                _.prompt_behaviour = RmPromptBehaviour.Once;
            } else {
                return ShellRmError.IllegalOption;
            }
            i += 1;
        }
        return true;
    }
}

/// 解析后的 `rm` 执行计划（零堆分配）
pub struct RmPlan {
    opts: RmOpts = RmOpts{},
    targets: [SHELL_RM_MAX_TARGETS]([]const u8) = [""; SHELL_RM_MAX_TARGETS],
    target_count: usize = 0,

    /// 解析 `rm` 参数列表（不包含命令名 `"rm"` 本身）
    static fn parse(args: []const []const u8): RmPlan! {
        let plan = RmPlan{};
        let parsing_flags = true;

        let i: usize = 0;
        while (i < args.len) {
            const arg = args[i];
            if (parsing_flags) {
                if (bytesEql(arg, "--")) {
                    parsing_flags = false;
                    i += 1;
                    continue;
                }
                const consumed = plan.opts.parseFlag(arg)?;
                if (consumed) {
                    i += 1;
                    continue;
                }
                parsing_flags = false;
            }

            if (plan.target_count >= SHELL_RM_MAX_TARGETS) {
                return ShellRmError.TooManyTargets;
            }
            plan.targets[plan.target_count] = arg;
            plan.target_count += 1;
            i += 1;
        }

        if (plan.opts.recursive) {
            plan.opts.remove_empty_dirs = true;
        }

        if (plan.target_count == 0 && !plan.opts.force) {
            return ShellRmError.MissingOperand;
        }

        return plan;
    }

    fn targetAt(idx: usize): ([]const u8)? {
        if (idx >= _.target_count) {
            return null;
        }
        return _.targets[idx];
    }

    /// 对单个待删除目标执行完整安全预检（`.`/`..` 拦截、`--preserve-root` 根目录防线、目录标志检查与缺失静默策略）
    fn validateTarget(cwd: []const u8, target: []const u8, kind: RmPathKind): RmTargetDecision! {
        if (isRmDotOrDotDotTarget(target)) {
            return ShellRmError.RefusingToRemoveDotOrDotDot;
        }
        if (_.opts.preserve_root && isRmRootPath(cwd, target)) {
            return ShellRmError.RefusingToRemoveRoot;
        }
        if (kind == RmPathKind.Missing) {
            if (_.opts.force) {
                return RmTargetDecision.SkipMissing;
            }
            return RmTargetDecision.Remove;
        }
        if (kind == RmPathKind.EmptyDir) {
            if (!_.opts.canRemoveDirectory(true)) {
                return ShellRmError.CannotRemoveDirectoryWithoutFlag;
            }
        } else if (kind == RmPathKind.NonEmptyDir) {
            if (!_.opts.recursive) {
                return ShellRmError.CannotRemoveDirectoryWithoutFlag;
            }
        }

        if (_.opts.prompt_behaviour != RmPromptBehaviour.Never) {
            return RmTargetDecision.PromptBeforeRemove;
        }
        return RmTargetDecision.Remove;
    }

    /// 格式化 `-v` / `--verbose` 删除日志行
    fn formatVerboseLine(target: []const u8, is_dir: bool, out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, if (is_dir) "removed directory '" else "removed '")?;
        pos = appendBytes(out, pos, target)?;
        pos = appendBytes(out, pos, "'\n")?;
        return pos;
    }
}
