// Source: Absorbed from /data/dev/bun/src/install/PackageManager/ProgressStrings.cp | Synced: 2026-11-23T01:39:20Z
// Pure-CP zero-allocation Package Manager 5-Stage Progress Renderer & Node Counter
// Aligned with Bun src/install/PackageManager/ProgressStrings.cp & docs/pm/cli/install.mdx:
// - Models all 5 install lifecycle stages (`Download`, `Extract`, `Install`, `Save`, `Script`)
// - Formats ANSI TTY labels with emoji prefixes and plain non-TTY/CI log labels (with exact newline rules)
// - Implements Bun's incremental `setNodeName` prefix-preserving buffer update with strict bounds checking
// - Tracks estimated total (`total_tasks + extracted_count`) and completed (`total_tasks - pending_tasks`) items
// - Supports `--silent` suppression, idempotent `startIfNone()`, and clean `finish()` completion snap.

/// 进度节点名称固定缓冲区容量
pub const INSTALL_PROGRESS_NAME_CAP: usize = 128;

/// 安装进度渲染与追踪错误集
pub err InstallProgressError {
    BufferTooSmall,
    NameTooLong,
    PendingExceedsTotal,
    NotStarted,
}

/// 包安装生命周期 5 阶段（对齐 Bun `ProgressStrings`）
pub enum InstallProgressStage: u8 {
    Download = 0,
    Extract = 1,
    Install = 2,
    Save = 3,
    Script = 4,
}

/// 返回指定阶段的纯文本标题（不含 Emoji 与换行）
pub fn stagePlainLabel(stage: InstallProgressStage): []const u8 {
    return switch (stage) {
        .Download => "Resolving",
        .Extract => "Resolving & extracting",
        .Install => "Installing",
        .Save => "Saving lockfile",
        .Script => "Running script",
    };
}

/// 返回指定阶段的 Emoji 前缀（TTY/ANSI 模式）
pub fn stageEmojiPrefix(stage: InstallProgressStage): []const u8 {
    return switch (stage) {
        .Download => "  🔍 ",
        .Extract => "  🚚 ",
        .Install => "  📦 ",
        .Save => "  🔒 ",
        .Script => "  ⚙️  ",
    };
}

/// 返回指定阶段对应的完整阶段标签（对齐 Bun `ProgressStrings.{download,extract,install,save,script}`）
pub fn stageBannerLabel(stage: InstallProgressStage, enable_ansi_colors: bool): []const u8 {
    if (enable_ansi_colors) {
        return switch (stage) {
            .Download => "  🔍 Resolving",
            .Extract => "  🚚 Resolving & extracting",
            .Install => "  📦 Installing",
            .Save => "  🔒 Saving lockfile",
            .Script => "  ⚙️  Running script",
        };
    }
    return switch (stage) {
        .Download => "Resolving\n",
        .Extract => "Resolving & extracting\n",
        .Install => "Installing\n",
        .Save => "Saving lockfile",
        .Script => "Running script\n",
    };
}

fn ipWriteU64(val: u64, out: []u8, start: usize): InstallProgressError!usize {
    let tmp: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = ('0' as u8) + ((v % 10) as u8);
            v /= 10;
            len += 1;
        }
    }
    if (start + len > out.len) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < len) {
        out[start + i] = tmp[len - 1 - i];
        i += 1;
    }
    return start + len;
}

/// 零堆分配包安装进度条状态追踪器（对齐 Bun `ProgressStrings` + `startProgressBar` / `endProgressBar`）
pub struct InstallProgressTracker {
    enable_ansi_colors: bool = false,
    silent: bool = false,
    active: bool = false,
    finished: bool = false,
    stage: InstallProgressStage = .Download,
    name_buf: [INSTALL_PROGRESS_NAME_CAP]u8 = [0; INSTALL_PROGRESS_NAME_CAP],
    name_len: usize = 0,
    prefix_len: usize = 0,
    estimated_total_items: u64 = 0,
    completed_items: u64 = 0,
    refresh_count: u64 = 0,

    static fn init(enable_ansi_colors: bool, silent: bool): InstallProgressTracker {
        let t = InstallProgressTracker{};
        t.enable_ansi_colors = enable_ansi_colors;
        t.silent = silent;
        return t;
    }

    fn isActive(): bool {
        return _.active;
    }

    fn isFinished(): bool {
        return _.finished;
    }

    fn currentStage(): InstallProgressStage {
        return _.stage;
    }

    fn nodeName(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    fn estimatedTotal(): u64 {
        return _.estimated_total_items;
    }

    fn completed(): u64 {
        return _.completed_items;
    }

    fn refreshCount(): u64 {
        return _.refresh_count;
    }

    /// 写入节点名称缓冲区（对齐 Bun `setNodeName`：`is_first=false` 时复用已有 emoji 前缀槽位）
    fn setNodeName(*_, name: []const u8, emoji: []const u8, is_first: bool): InstallProgressError!usize {
        if (_.enable_ansi_colors) {
            const p_len = if (is_first) emoji.len else _.prefix_len;
            if (p_len + name.len > INSTALL_PROGRESS_NAME_CAP) return err.NameTooLong;
            if (is_first) {
                let i: usize = 0;
                while (i < emoji.len) {
                    _.name_buf[i] = emoji[i];
                    i += 1;
                }
                _.prefix_len = emoji.len;
            }
            let j: usize = 0;
            while (j < name.len) {
                _.name_buf[p_len + j] = name[j];
                j += 1;
            }
            _.name_len = p_len + name.len;
            return _.name_len;
        }

        if (name.len > INSTALL_PROGRESS_NAME_CAP) return err.NameTooLong;
        let k: usize = 0;
        while (k < name.len) {
            _.name_buf[k] = name[k];
            k += 1;
        }
        _.prefix_len = 0;
        _.name_len = name.len;
        return _.name_len;
    }

    /// 幂等启动进度条（对齐 Bun `startProgressBarIfNone`；`silent == true` 时不激活）
    fn startIfNone(*_, total_tasks: u64, pending_tasks: u64, extracted_count: u64): InstallProgressError!bool {
        if (_.silent) return false;
        if (_.active) return false;
        if (pending_tasks > total_tasks) return err.PendingExceedsTotal;

        _.stage = .Download;
        * = _.setNodeName(stagePlainLabel(.Download), stageEmojiPrefix(.Download), true)?;
        _.estimated_total_items = total_tasks + extracted_count;
        _.completed_items = total_tasks - pending_tasks;
        _.active = true;
        _.finished = false;
        _.refresh_count += 1;
        return true;
    }

    /// 切换到新阶段并刷新节点名称与计数
    fn transitionStage(
        *_,
        next_stage: InstallProgressStage,
        total_tasks: u64,
        pending_tasks: u64,
        extracted_count: u64,
    ): InstallProgressError!usize {
        if (!_.active) return err.NotStarted;
        if (pending_tasks > total_tasks) return err.PendingExceedsTotal;

        const is_first = next_stage != _.stage;
        _.stage = next_stage;
        const n = _.setNodeName(stagePlainLabel(next_stage), stageEmojiPrefix(next_stage), is_first)?;
        _.estimated_total_items = total_tasks + extracted_count;
        _.completed_items = total_tasks - pending_tasks;
        _.refresh_count += 1;
        return n;
    }

    /// 计算当前整数进度百分比（0..100，防除零与越界）
    fn percentComplete(): u8 {
        if (_.estimated_total_items == 0) {
            return if (_.finished) (100 as u8) else (0 as u8);
        }
        if (_.completed_items >= _.estimated_total_items) return 100;
        return ((_.completed_items * 100) / _.estimated_total_items) as u8;
    }

    /// 结束进度条：将完成数对齐至总预估数并刷新（对齐 Bun `endProgressBar`）
    fn finish(*_): bool {
        if (!_.active) return false;
        _.completed_items = _.estimated_total_items;
        _.refresh_count += 1;
        _.active = false;
        _.finished = true;
        return true;
    }

    /// 将当前进度渲染为单行摘要：`<nodeName> [completed/total]`
    fn formatStatusLine(out: []u8): InstallProgressError!usize {
        const nm = _.nodeName();
        if (nm.len + 4 > out.len) return err.BufferTooSmall;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < nm.len) {
            out[pos] = nm[i];
            pos += 1;
            i += 1;
        }
        out[pos] = ' ';
        pos += 1;
        out[pos] = '[';
        pos += 1;
        pos = ipWriteU64(_.completed_items, out, pos)?;
        if (pos >= out.len) return err.BufferTooSmall;
        out[pos] = '/';
        pos += 1;
        pos = ipWriteU64(_.estimated_total_items, out, pos)?;
        if (pos >= out.len) return err.BufferTooSmall;
        out[pos] = ']';
        pos += 1;
        return pos;
    }
}
