// Source: Absorbed from /data/dev/bun/src/cli/list-of-yarn-commands.cp | Synced: 2026-11-22T06:02:57Z
// ============================================================================
// Pure-CP Zero-Allocation Yarn v1/v2 Command Classifier & Package Manager Progress Labels
// Absorbed from Bun: src/cli/list-of-yarn-commands.cp & src/install/PackageManager/ProgressStrings.cp
// ============================================================================

/// 包管理 CLI 进度标签格式化错误集
pub err PmCommandsError {
    BufferTooSmall,
}

/// Yarn CLI 命令代际分类（覆盖 Yarn v1 与 Yarn v2.3+ 全量内置命令表）
pub enum YarnCommandEra: u8 {
    none = 0,
    v1_only = 1,
    v2_only = 2,
    both = 3,

    fn isKnown(): bool {
        return _ != YarnCommandEra.none;
    }

    fn supportsV1(): bool {
        return _ == YarnCommandEra.v1_only || _ == YarnCommandEra.both;
    }

    fn supportsV2(): bool {
        return _ == YarnCommandEra.v2_only || _ == YarnCommandEra.both;
    }
}

/// 包管理器安装流水线阶段（对齐 Bun `ProgressStrings.cp`）
pub enum PmStage: u8 {
    resolve = 0,
    extract = 1,
    install = 2,
    save_lockfile = 3,
    run_script = 4,
}

fn sliceEql(a: []const u8, b: []const u8): bool {
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

fn isYarnV2Command(name: []const u8): bool {
    return sliceEql(name, "add") ||
        sliceEql(name, "bin") ||
        sliceEql(name, "cache") ||
        sliceEql(name, "config") ||
        sliceEql(name, "dedupe") ||
        sliceEql(name, "dlx") ||
        sliceEql(name, "exec") ||
        sliceEql(name, "explain") ||
        sliceEql(name, "info") ||
        sliceEql(name, "init") ||
        sliceEql(name, "install") ||
        sliceEql(name, "link") ||
        sliceEql(name, "node") ||
        sliceEql(name, "npm") ||
        sliceEql(name, "pack") ||
        sliceEql(name, "patch") ||
        sliceEql(name, "plugin") ||
        sliceEql(name, "rebuild") ||
        sliceEql(name, "remove") ||
        sliceEql(name, "run") ||
        sliceEql(name, "set") ||
        sliceEql(name, "unplug") ||
        sliceEql(name, "up") ||
        sliceEql(name, "why") ||
        sliceEql(name, "workspace") ||
        sliceEql(name, "workspaces");
}

fn isYarnV1Command(name: []const u8): bool {
    return sliceEql(name, "access") ||
        sliceEql(name, "add") ||
        sliceEql(name, "audit") ||
        sliceEql(name, "autoclean") ||
        sliceEql(name, "bin") ||
        sliceEql(name, "cache") ||
        sliceEql(name, "check") ||
        sliceEql(name, "config") ||
        sliceEql(name, "create") ||
        sliceEql(name, "exec") ||
        sliceEql(name, "generate-lock-entry") ||
        sliceEql(name, "generateLockEntry") ||
        sliceEql(name, "global") ||
        sliceEql(name, "help") ||
        sliceEql(name, "import") ||
        sliceEql(name, "info") ||
        sliceEql(name, "init") ||
        sliceEql(name, "install") ||
        sliceEql(name, "licenses") ||
        sliceEql(name, "link") ||
        sliceEql(name, "list") ||
        sliceEql(name, "login") ||
        sliceEql(name, "logout") ||
        sliceEql(name, "node") ||
        sliceEql(name, "outdated") ||
        sliceEql(name, "owner") ||
        sliceEql(name, "pack") ||
        sliceEql(name, "policies") ||
        sliceEql(name, "publish") ||
        sliceEql(name, "remove") ||
        sliceEql(name, "run") ||
        sliceEql(name, "tag") ||
        sliceEql(name, "team") ||
        sliceEql(name, "unlink") ||
        sliceEql(name, "unplug") ||
        sliceEql(name, "upgrade") ||
        sliceEql(name, "upgrade-interactive") ||
        sliceEql(name, "upgradeInteractive") ||
        sliceEql(name, "version") ||
        sliceEql(name, "versions") ||
        sliceEql(name, "why") ||
        sliceEql(name, "workspace") ||
        sliceEql(name, "workspaces");
}

/// 分类命令所属 Yarn 代际（`v1_only` / `v2_only` / `both` / `none`）
pub fn yarnCommandEra(name: []const u8): YarnCommandEra {
    const v1 = isYarnV1Command(name);
    const v2 = isYarnV2Command(name);
    if (v1 && v2) {
        return YarnCommandEra.both;
    }
    if (v1) {
        return YarnCommandEra.v1_only;
    }
    if (v2) {
        return YarnCommandEra.v2_only;
    }
    return YarnCommandEra.none;
}

/// 判断命令名是否属于 Yarn v1/v2 内置命令集合（对齐 Bun `all_yarn_commands`）
pub fn isYarnCommand(name: []const u8): bool {
    return yarnCommandEra(name).isKnown();
}

/// 将常见 Yarn CLI 子命令映射为标准 `cap`/`bun` 包管理子命令（无法直接等价映射时返回 `null`）
pub fn mapYarnToCapSubcommand(name: []const u8): ([]const u8)? {
    if (sliceEql(name, "add")) return "add";
    if (sliceEql(name, "remove")) return "remove";
    if (sliceEql(name, "install")) return "install";
    if (sliceEql(name, "up") || sliceEql(name, "upgrade") || sliceEql(name, "upgrade-interactive") || sliceEql(name, "upgradeInteractive")) {
        return "update";
    }
    if (sliceEql(name, "dlx")) return "x";
    if (sliceEql(name, "run")) return "run";
    if (sliceEql(name, "init")) return "init";
    if (sliceEql(name, "create")) return "create";
    if (sliceEql(name, "link")) return "link";
    if (sliceEql(name, "unlink")) return "unlink";
    if (sliceEql(name, "outdated")) return "outdated";
    if (sliceEql(name, "audit")) return "audit";
    if (sliceEql(name, "patch")) return "patch";
    if (sliceEql(name, "publish")) return "publish";
    if (sliceEql(name, "info")) return "info";
    if (sliceEql(name, "why")) return "why";
    if (sliceEql(name, "pack")) return "pm pack";
    if (sliceEql(name, "bin")) return "pm bin";
    if (sliceEql(name, "cache")) return "pm cache";
    if (sliceEql(name, "list")) return "pm ls";
    return null;
}

/// 返回包管理阶段纯文本名称（不含 emoji 与换行，对齐 Bun `ProgressStrings.*_no_emoji_`）
pub fn pmStagePlainName(stage: PmStage): []const u8 {
    return switch (stage) {
        .resolve => "Resolving",
        .extract => "Resolving & extracting",
        .install => "Installing",
        .save_lockfile => "Saving lockfile",
        .run_script => "Running script",
    };
}

/// 返回包管理阶段终端展示前缀（对齐 Bun `ProgressStrings.cp`：ANSI 终端带 emoji，非 ANSI 降级纯文本）
pub fn pmStageLabel(stage: PmStage, ansi_enabled: bool): []const u8 {
    if (ansi_enabled) {
        return switch (stage) {
            .resolve => "  🔍 Resolving",
            .extract => "  🚚 Resolving & extracting",
            .install => "  📦 Installing",
            .save_lockfile => "  🔒 Saving lockfile",
            .run_script => "  ⚙️  Running script",
        };
    }
    return switch (stage) {
        .resolve => "Resolving\n",
        .extract => "Resolving & extracting\n",
        .install => "Installing\n",
        .save_lockfile => "Saving lockfile",
        .run_script => "Running script\n",
    };
}

/// 零堆分配拼接包管理阶段进度横幅（如 `"  📦 Installing lodash@4.17.21"` 或 `"Installing lodash@4.17.21"`）
pub fn formatPmStageBanner(
    buf: []u8,
    stage: PmStage,
    detail: []const u8,
    ansi_enabled: bool,
): ([]const u8)! {
    const prefix = if (ansi_enabled) pmStageLabel(stage, true) else pmStagePlainName(stage);
    const sep_len: usize = if (detail.len > 0) 1 else 0;
    const needed = prefix.len + sep_len + detail.len;
    if (needed > buf.len) {
        return err.BufferTooSmall;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len) {
        buf[pos] = prefix[i];
        pos += 1;
        i += 1;
    }
    if (detail.len > 0) {
        buf[pos] = ' ';
        pos += 1;
        i = 0;
        while (i < detail.len) {
            buf[pos] = detail[i];
            pos += 1;
            i += 1;
        }
    }
    return buf[0..pos];
}
