// Source: Absorbed from /data/dev/bun/src/shell/states/Assigns.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation POSIX / Bun Shell Environment Assignment Expansion & Scope Commit State Machine
// Aligned with Bun src/shell/states/Assigns.cp & docs/runtime/shell.mdx:
// - Models all 3 POSIX assignment contexts: `Cmd` (per-command prefix), `Shell` (local shell var), `Exported` (`export`)
// - Implements Bun's pipeline no-op optimization (`FOO=bar | echo hi` has no effect on parent shell env and can be skipped)
// - Joins multi-word expansion outputs with single spaces (`0 words -> ""`, `1 word -> word`, `N words -> w0 w1 ...`)
// - Evaluates assignments strictly left-to-right; aborts immediately on any expansion failure (exit code 1) without applying remaining assignments
// - 100% zero heap allocation with fixed-capacity label/value storage and last-write-wins deduplication.

/// 单组赋值列表支持的最大赋值条数
pub const SHELL_ASSIGNS_MAX_ITEMS: usize = 16;

/// 变量名最大长度
pub const SHELL_ASSIGN_MAX_LABEL_LEN: usize = 64;

/// 展开合并后的变量值最大长度
pub const SHELL_ASSIGN_MAX_VALUE_LEN: usize = 256;

/// 环境变量赋值状态机错误集
pub err ShellAssignsError {
    TooManyAssigns,
    InvalidLabel,
    LabelTooLong,
    ValueTooLong,
    BufferTooSmall,
    AlreadyStarted,
    ExpansionStillPending,
    NotAwaitingExpansion,
}

/// 赋值作用域上下文（对齐 Bun `Assigns.AssignCtx`）
pub enum AssignScopeCtx: u8 {
    /// 命令前缀赋值（如 `NODE_ENV=production bun run app.ts`），仅作用于该命令
    Cmd = 0,
    /// 当前 Shell 局部赋值（如 `FOO=bar`）
    Shell = 1,
    /// 导出赋值（如 `export FOO=bar`）
    Exported = 2,
}

/// 赋值状态机步进动作类型
pub enum AssignStepKind: u8 {
    /// 调度器应展开第 `assign_index` 条赋值的右侧表达式，完成后调用 `onExpansionDone` 或 `onExpansionFailed`
    ExpandValue = 0,
    /// 全部赋值已成功提交（或因管道优化被安全跳过），`exit_code == 0`
    Completed = 1,
    /// 某条赋值右侧展开失败，状态机中止，`exit_code == 1`
    Failed = 2,
}

/// 单步调度结果
pub struct AssignStepResult {
    kind: AssignStepKind = .Completed,
    assign_index: usize = 0,
    exit_code: u8 = 0,
    skipped_in_pipeline: bool = false,
}

/// 已提交的环境变量绑定快照
pub struct CommittedAssign {
    label_buf: [SHELL_ASSIGN_MAX_LABEL_LEN]u8 = [0; SHELL_ASSIGN_MAX_LABEL_LEN],
    label_len: usize = 0,
    value_buf: [SHELL_ASSIGN_MAX_VALUE_LEN]u8 = [0; SHELL_ASSIGN_MAX_VALUE_LEN],
    value_len: usize = 0,
    ctx: AssignScopeCtx = .Shell,

    fn label(): []const u8 {
        return _.label_buf[0.._.label_len];
    }

    fn value(): []const u8 {
        return _.value_buf[0.._.value_len];
    }
}

fn asnBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// 校验 POSIX 环境变量名合法性：`[A-Za-z_][A-Za-z0-9_]*`
pub fn isValidAssignLabel(label: []const u8): bool {
    if (label.len == 0) return false;
    const c0 = label[0];
    if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '_')) {
        return false;
    }
    let i: usize = 1;
    while (i < label.len) {
        const c = label[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 判断该组赋值在管道上下文中是否无副作用可安全跳过（对齐 Bun `Assigns.cp` 头部规范）
pub fn canSkipPipelineAssigns(in_pipeline: bool, ctx: AssignScopeCtx): bool {
    return in_pipeline && (ctx == .Shell || ctx == .Exported);
}

/// 将赋值右侧展开产生的多词列表以单空格拼接到 `out`（0 词 -> 0 字节，1 词 -> 原样，N 词 -> 空格分隔）
pub fn joinExpandedWords(words: []const ([]const u8), out: []u8): ShellAssignsError!usize {
    if (words.len == 0) return 0;
    let total: usize = 0;
    let i: usize = 0;
    while (i < words.len) {
        total += words[i].len;
        if (i + 1 < words.len) total += 1;
        i += 1;
    }
    if (total > out.len) return err.BufferTooSmall;

    let pos: usize = 0;
    i = 0;
    while (i < words.len) {
        const w = words[i];
        let j: usize = 0;
        while (j < w.len) {
            out[pos] = w[j];
            pos += 1;
            j += 1;
        }
        if (i + 1 < words.len) {
            out[pos] = ' ';
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

struct PendingLabelSlot {
    buf: [SHELL_ASSIGN_MAX_LABEL_LEN]u8 = [0; SHELL_ASSIGN_MAX_LABEL_LEN],
    len: usize = 0,

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

/// 零堆分配 POSIX / Bun Shell 赋值展开与作用域提交状态机
pub struct ShellAssignsStateMachine {
    ctx: AssignScopeCtx = .Shell,
    in_pipeline: bool = false,
    pending_labels: [SHELL_ASSIGNS_MAX_ITEMS]PendingLabelSlot = [PendingLabelSlot{}; SHELL_ASSIGNS_MAX_ITEMS],
    pending_count: usize = 0,
    cursor: usize = 0,
    started: bool = false,
    awaiting_expansion: bool = false,
    failed: bool = false,
    done: bool = false,
    exit_code: u8 = 0,

    committed: [SHELL_ASSIGNS_MAX_ITEMS]CommittedAssign = [CommittedAssign{}; SHELL_ASSIGNS_MAX_ITEMS],
    committed_count: usize = 0,

    static fn init(ctx: AssignScopeCtx, in_pipeline: bool): ShellAssignsStateMachine {
        let m = ShellAssignsStateMachine{};
        m.ctx = ctx;
        m.in_pipeline = in_pipeline;
        return m;
    }

    fn scopeCtx(): AssignScopeCtx {
        return _.ctx;
    }

    fn isDone(): bool {
        return _.done;
    }

    fn isFailed(): bool {
        return _.failed;
    }

    fn exitCode(): u8 {
        return _.exit_code;
    }

    fn committedCount(): usize {
        return _.committed_count;
    }

    /// 添加一条待展开的赋值变量名（仅允许在启动前调用）
    fn addAssignLabel(*_, label: []const u8): ShellAssignsError!usize {
        if (_.started) return err.AlreadyStarted;
        if (!isValidAssignLabel(label)) return err.InvalidLabel;
        if (label.len > SHELL_ASSIGN_MAX_LABEL_LEN) return err.LabelTooLong;
        if (_.pending_count >= SHELL_ASSIGNS_MAX_ITEMS) return err.TooManyAssigns;

        let slot = PendingLabelSlot{};
        slot.len = label.len;
        let i: usize = 0;
        while (i < label.len) {
            slot.buf[i] = label[i];
            i += 1;
        }
        const idx = _.pending_count;
        _.pending_labels[idx] = slot;
        _.pending_count += 1;
        return idx;
    }

    /// 获取待展开列表中第 `idx` 项的变量名
    fn pendingLabelAt(idx: usize): ([]const u8)? {
        if (idx >= _.pending_count) return null;
        return _.pending_labels[idx].slice();
    }

    /// 推进一步展开状态机
    fn nextStep(*_): ShellAssignsError!AssignStepResult {
        if (_.awaiting_expansion) return err.ExpansionStillPending;
        if (_.failed) {
            return AssignStepResult{ kind: .Failed, assign_index: _.cursor, exit_code: 1, skipped_in_pipeline: false };
        }
        if (_.done) {
            return AssignStepResult{ kind: .Completed, assign_index: _.cursor, exit_code: 0, skipped_in_pipeline: false };
        }

        _.started = true;

        // 管道中独立的 Shell/Exported 赋值无副作用，直接跳过
        if (canSkipPipelineAssigns(_.in_pipeline, _.ctx)) {
            _.done = true;
            _.exit_code = 0;
            return AssignStepResult{
                kind: .Completed,
                assign_index: 0,
                exit_code: 0,
                skipped_in_pipeline: true,
            };
        }

        if (_.cursor >= _.pending_count) {
            _.done = true;
            _.exit_code = 0;
            return AssignStepResult{
                kind: .Completed,
                assign_index: _.cursor,
                exit_code: 0,
                skipped_in_pipeline: false,
            };
        }

        _.awaiting_expansion = true;
        return AssignStepResult{
            kind: .ExpandValue,
            assign_index: _.cursor,
            exit_code: 0,
            skipped_in_pipeline: false,
        };
    }

    /// 提交当前赋值右侧的展开结果词列表（同名变量按 last-write-wins 覆盖更新）
    fn onExpansionDone(*_, expanded_words: []const ([]const u8)): ShellAssignsError!usize {
        if (!_.awaiting_expansion) return err.NotAwaitingExpansion;

        let merged_buf: [SHELL_ASSIGN_MAX_VALUE_LEN]u8 = [0; SHELL_ASSIGN_MAX_VALUE_LEN];
        const merged_len = joinExpandedWords(expanded_words, merged_buf[0..]) catch {
            return err.ValueTooLong;
        };

        const lbl = _.pending_labels[_.cursor].slice();

        // 若已存在同名绑定，原地覆盖（last-write-wins）
        let target_idx: usize = _.committed_count;
        let i: usize = 0;
        while (i < _.committed_count) {
            if (asnBytesEql(_.committed[i].label(), lbl)) {
                target_idx = i;
                break;
            }
            i += 1;
        }

        let item = CommittedAssign{};
        item.label_len = lbl.len;
        i = 0;
        while (i < lbl.len) {
            item.label_buf[i] = lbl[i];
            i += 1;
        }
        item.value_len = merged_len;
        i = 0;
        while (i < merged_len) {
            item.value_buf[i] = merged_buf[i];
            i += 1;
        }
        item.ctx = _.ctx;

        _.committed[target_idx] = item;
        if (target_idx == _.committed_count) {
            _.committed_count += 1;
        }

        _.awaiting_expansion = false;
        _.cursor += 1;
        return merged_len;
    }

    /// 展开失败回调：立即终止后续赋值并将退出码置为 1（对齐 Bun `Assigns.childDone`）
    fn onExpansionFailed(*_): ShellAssignsError!u8 {
        if (!_.awaiting_expansion) return err.NotAwaitingExpansion;
        _.awaiting_expansion = false;
        _.failed = true;
        _.done = true;
        _.exit_code = 1;
        return 1;
    }

    /// 按变量名查找已提交的绑定快照
    fn lookupCommitted(label: []const u8): CommittedAssign? {
        let i: usize = 0;
        while (i < _.committed_count) {
            if (asnBytesEql(_.committed[i].label(), label)) {
                return _.committed[i];
            }
            i += 1;
        }
        return null;
    }
}
