// Source: Absorbed from /data/dev/bun/src/install/repository.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/install/repository.cp & /data/dev/bun/docs/pm/cli/add.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Git/GitHub/GitLab/Bitbucket dependency specifier parser,
// SCP-style path detector, canonical & isolated store path formatter, and `.gitconfig`
// non-interactive `GIT_ASKPASS` / `GIT_SSH_COMMAND` anti-hang policy detector.

/// 默认非交互式防密码提示 `GIT_ASKPASS` 回退值
pub const GIT_DEFAULT_ASKPASS: []const u8 = "echo";

/// 默认非交互式 SSH 主机密钥策略 `GIT_SSH_COMMAND` 回退值
pub const GIT_DEFAULT_SSH_COMMAND: []const u8 = "ssh -oStrictHostKeyChecking=accept-new";

/// Git 依赖规范解析与格式化错误集
pub err GitRepoError {
    EmptyInput,
    InvalidShorthand,
    MissingRepoName,
    BufferTooSmall,
}

/// 托管仓库平台分类（对应 Bun `Repository.Hosts`）
pub enum GitRepoHost: u8 {
    generic = 0,
    github = 1,
    gitlab = 2,
    bitbucket = 3,

    fn domainSuffix(): []const u8 {
        return switch (_) {
            .github => ".com",
            .gitlab => ".com",
            .bitbucket => ".org",
            .generic => "",
        };
    }

    fn defaultLabel(): []const u8 {
        return switch (_) {
            .github => "github:",
            .gitlab => "gitlab:",
            .bitbucket => "bitbucket:",
            .generic => "git+",
        };
    }
}

/// `.gitconfig` 轻量扫描结果（对齐 Bun `SloppyGlobalGitConfig`）
pub struct GitConfigFlags {
    has_askpass: bool = false,
    has_ssh_command: bool = false,
}

/// 非交互式 Git 子进程防挂起环境推导结果（对齐 Bun `SharedEnv`）
pub struct GitNonInteractiveEnv {
    set_askpass: bool = false,
    askpass_value: []const u8 = "",
    set_ssh_command: bool = false,
    ssh_command_value: []const u8 = "",
}

/// Git 依赖规范（零拷贝切片视图，对齐 Bun `Repository`）
pub struct GitRepoSpec {
    host: GitRepoHost = GitRepoHost.generic,
    owner: []const u8 = "",
    repo: []const u8 = "",
    committish: []const u8 = "",
    resolved: []const u8 = "",

    /// 解析 `git+...` 或通用 Git URL（如 `git+https://github.com/org/repo.git#v1.0.0`）
    static fn parseGit(input: []const u8): GitRepoSpec! {
        let remain = trimWs(input);
        if (remain.len == 0) {
            return GitRepoError.EmptyInput;
        }
        if (startsWith(remain, "git+")) {
            remain = remain[4..];
        }
        if (remain.len == 0) {
            return GitRepoError.MissingRepoName;
        }
        if (hash := lastIndexOfByte(remain, '#')) {
            const repo_part = remain[0..hash];
            if (repo_part.len == 0) {
                return GitRepoError.MissingRepoName;
            }
            return GitRepoSpec{
                host: GitRepoHost.generic,
                repo: repo_part,
                committish: remain[hash + 1..],
            };
        }
        return GitRepoSpec{
            host: GitRepoHost.generic,
            repo: remain,
        };
    }

    /// 解析 `github:owner/repo#committish`（或裸 `owner/repo#committish`）简写
    static fn parseGithub(input: []const u8): GitRepoSpec! {
        return GitRepoSpec.parseHostedShorthand(input, "github:", GitRepoHost.github);
    }

    /// 统一解析任意 Git 依赖写法（`github:` / `gitlab:` / `bitbucket:` / `git+...` / SCP / `owner/repo`）
    static fn parseAny(input: []const u8): GitRepoSpec! {
        const s = trimWs(input);
        if (s.len == 0) {
            return GitRepoError.EmptyInput;
        }
        if (startsWith(s, "github:")) {
            return GitRepoSpec.parseHostedShorthand(s, "github:", GitRepoHost.github);
        }
        if (startsWith(s, "gitlab:")) {
            return GitRepoSpec.parseHostedShorthand(s, "gitlab:", GitRepoHost.gitlab);
        }
        if (startsWith(s, "bitbucket:")) {
            return GitRepoSpec.parseHostedShorthand(s, "bitbucket:", GitRepoHost.bitbucket);
        }
        if (startsWith(s, "git+") ||
            startsWith(s, "https://") ||
            startsWith(s, "http://") ||
            startsWith(s, "ssh://") ||
            startsWith(s, "git://") ||
            isScpLikeGitPath(s))
        {
            return GitRepoSpec.parseGit(s);
        }
        // 回退为默认 GitHub `owner/repo[#ref]` 简写
        return GitRepoSpec.parseHostedShorthand(s, "", GitRepoHost.github);
    }

    static fn parseHostedShorthand(input: []const u8, prefix: []const u8, host: GitRepoHost): GitRepoSpec! {
        let remain = trimWs(input);
        if (prefix.len > 0 && startsWith(remain, prefix)) {
            remain = remain[prefix.len..];
        }
        if (remain.len == 0) {
            return GitRepoError.InvalidShorthand;
        }

        let hash_idx: usize? = null;
        let slash_idx: usize? = null;
        let i: usize = 0;
        while (i < remain.len) {
            const c = remain[i];
            if (c == '#' && hash_idx == null) {
                hash_idx = i;
            } else if (c == '/' && hash_idx == null) {
                slash_idx = i;
            }
            i += 1;
        }

        const slash = slash_idx ?? return GitRepoError.InvalidShorthand;
        if (slash == 0) {
            return GitRepoError.InvalidShorthand;
        }
        const owner = remain[0..slash];
        const repo_end = hash_idx ?? remain.len;
        if (repo_end <= slash + 1) {
            return GitRepoError.MissingRepoName;
        }
        const repo = remain[slash + 1..repo_end];
        const committish = if (h := hash_idx) remain[h + 1..] else "";

        return GitRepoSpec{
            host: host,
            owner: owner,
            repo: repo,
            committish: committish,
        };
    }

    /// 从 `repo` 字段提取包名（剥离 `#ref`、取最后一个 `/` 之后并可选剥离 `.git` 后缀）
    fn derivePackageName(): []const u8 {
        let remain = _.repo;
        if (h := indexOfByte(remain, '#')) {
            remain = remain[0..h];
        }
        if (remain.len == 0) {
            return "";
        }
        if (sl := lastIndexOfByte(remain, '/')) {
            remain = remain[sl + 1..];
        } else if (col := lastIndexOfByte(remain, ':')) {
            remain = remain[col + 1..];
        }
        if (remain.len > 4 && bytesEql(remain[remain.len - 4..], ".git")) {
            remain = remain[0..remain.len - 4];
        }
        return remain;
    }

    /// 比较两个 `GitRepoSpec` 是否等价（对齐 Bun `Repository.eql`：任一侧 `resolved` 为空时按 `committish` 比较）
    fn eql(other: GitRepoSpec): bool {
        if (!bytesEql(_.owner, other.owner)) {
            return false;
        }
        if (!bytesEql(_.repo, other.repo)) {
            return false;
        }
        if (_.resolved.len == 0 || other.resolved.len == 0) {
            return bytesEql(_.committish, other.committish);
        }
        return bytesEql(_.resolved, other.resolved);
    }

    /// 按 `(owner, repo, committish)` 字典序比较（返回 `-1` / `0` / `1`）
    fn compare(other: GitRepoSpec): i32 {
        const c1 = cmpBytes(_.owner, other.owner);
        if (c1 != 0) {
            return c1;
        }
        const c2 = cmpBytes(_.repo, other.repo);
        if (c2 != 0) {
            return c2;
        }
        return cmpBytes(_.committish, other.committish);
    }

    /// 规范化输出（对齐 Bun `TopFormatter`：`label + [owner/|ssh://] + repo + [#resolved_tail|#committish]`）
    fn formatCanonical(label: []const u8, out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, label)?;
        if (_.owner.len > 0) {
            pos = appendBytes(out, pos, _.owner)?;
            pos = appendByte(out, pos, '/')?;
        } else if (isScpLikeGitPath(_.repo)) {
            pos = appendBytes(out, pos, "ssh://")?;
        }
        pos = appendBytes(out, pos, _.repo)?;

        if (_.resolved.len > 0) {
            pos = appendByte(out, pos, '#')?;
            let tail = _.resolved;
            if (dash := lastIndexOfByte(tail, '-')) {
                tail = tail[dash + 1..];
            }
            pos = appendBytes(out, pos, tail)?;
        } else if (_.committish.len > 0) {
            pos = appendByte(out, pos, '#')?;
            pos = appendBytes(out, pos, _.committish)?;
        }
        return pos;
    }

    /// 格式化隔离存储安全路径（对齐 Bun `TopStorePathFormatter`：`/` 与 `\` 替换为 `+`，SCP 补 `ssh++`）
    fn formatStorePath(label: []const u8, out: []u8): usize! {
        let pos: usize = 0;
        pos = appendStoreEscaped(out, pos, label)?;
        if (_.owner.len > 0) {
            pos = appendStoreEscaped(out, pos, _.owner)?;
            pos = appendByte(out, pos, '+')?;
        } else if (isScpLikeGitPath(_.repo)) {
            pos = appendBytes(out, pos, "ssh++")?;
        }
        pos = appendStoreEscaped(out, pos, _.repo)?;

        if (_.resolved.len > 0) {
            pos = appendByte(out, pos, '+')?;
            let tail = _.resolved;
            if (dash := lastIndexOfByte(tail, '-')) {
                tail = tail[dash + 1..];
            }
            pos = appendStoreEscaped(out, pos, tail)?;
        } else if (_.committish.len > 0) {
            pos = appendByte(out, pos, '+')?;
            pos = appendStoreEscaped(out, pos, _.committish)?;
        }
        return pos;
    }
}

/// 检测是否为 SCP 风格 Git 路径（如 `git@github.com:oven-sh/bun.git`：含 `:` 且不含 `://`，且 `:` 先于 `/`）
pub fn isScpLikeGitPath(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '/') {
            return false;
        }
        if (c == ':') {
            if (i == 0) {
                return false;
            }
            if (i + 2 < s.len && s[i + 1] == '/' && s[i + 2] == '/') {
                return false;
            }
            // 排除 Windows 盘符如 `C:\...`
            if (i == 1 && i + 1 < s.len && s[i + 1] == '\\') {
                return false;
            }
            return i + 1 < s.len;
        }
        i += 1;
    }
    return false;
}

/// 零堆分配轻量扫描 `.gitconfig` 文本，检测是否已配置 `askpass` 或 `sshCommand`（对齐 `SloppyGlobalGitConfig`）
pub fn parseGitConfigFlags(gitconfig_text: []const u8): GitConfigFlags {
    let flags = GitConfigFlags{};
    let in_core_section = false;
    let i: usize = 0;

    while (i < gitconfig_text.len) {
        if (flags.has_askpass && flags.has_ssh_command) {
            break;
        }
        const line_start = i;
        while (i < gitconfig_text.len && gitconfig_text[i] != '\n') {
            i += 1;
        }
        const raw_line = gitconfig_text[line_start..i];
        if (i < gitconfig_text.len && gitconfig_text[i] == '\n') {
            i += 1;
        }

        const line = trimWs(raw_line);
        if (line.len == 0 || line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line[0] == '[') {
            if (end_b := indexOfByte(line, ']')) {
                in_core_section = asciiEqlIgnoreCase(line[0..end_b + 1], "[core]");
            } else {
                in_core_section = false;
            }
            continue;
        }

        if (in_core_section) {
            if (!flags.has_askpass && hasGitConfigKey(line, "askpass")) {
                flags.has_askpass = true;
                continue;
            }
            if (!flags.has_ssh_command && hasGitConfigKey(line, "sshCommand")) {
                flags.has_ssh_command = true;
            }
        } else {
            if (!flags.has_askpass && hasGitConfigKey(line, "core.askpass")) {
                flags.has_askpass = true;
                continue;
            }
            if (!flags.has_ssh_command && hasGitConfigKey(line, "core.sshCommand")) {
                flags.has_ssh_command = true;
            }
        }
    }

    return flags;
}

/// 根据 `.gitconfig` 标志与现有环境变量，推导非交互式防挂起 `GIT_ASKPASS` / `GIT_SSH_COMMAND` 覆盖策略
pub fn resolveNonInteractiveGitEnv(
    cfg: GitConfigFlags,
    has_env_askpass: bool,
    has_env_ssh_command: bool,
): GitNonInteractiveEnv {
    let out = GitNonInteractiveEnv{};
    if (!has_env_askpass && !cfg.has_askpass) {
        out.set_askpass = true;
        out.askpass_value = GIT_DEFAULT_ASKPASS;
    }
    if (!has_env_ssh_command && !cfg.has_ssh_command) {
        out.set_ssh_command = true;
        out.ssh_command_value = GIT_DEFAULT_SSH_COMMAND;
    }
    return out;
}

// ============================================================================
// 内部零分配辅助函数
// ============================================================================

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn hasGitConfigKey(line: []const u8, key: []const u8): bool {
    if (line.len <= key.len) {
        return false;
    }
    if (!asciiEqlIgnoreCase(line[0..key.len], key)) {
        return false;
    }
    const next = line[key.len];
    return next == ' ' || next == '\t' || next == '=';
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

fn cmpBytes(a: []const u8, b: []const u8): i32 {
    const n = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < n) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEql(s[0..prefix.len], prefix);
}

fn indexOfByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn lastIndexOfByte(s: []const u8, needle: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == needle) {
            return i;
        }
    }
    return null;
}

fn appendByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return GitRepoError.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return GitRepoError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendStoreEscaped(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return GitRepoError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        out[pos + i] = if (c == '/' || c == '\\' || c == ':') '+' else c;
        i += 1;
    }
    return pos + src.len;
}
