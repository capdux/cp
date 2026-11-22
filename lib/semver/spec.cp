// Source: Absorbed from /data/dev/bun/src/semver/SemverQuery.cp | Synced: 2026-11-22T02:34:23Z

/// 依赖协议/描述符类别
pub enum DependencyKind: u8 {
    npm = 0,
    dist_tag = 1,
    workspace = 2,
    catalog = 3,
    file = 4,
    link = 5,
    folder = 6,
    tarball = 7,
    github = 8,
    git = 9,
    npm_alias = 10,
}

/// Workspace 协议子类型
pub enum WorkspaceSpecKind: u8 {
    star = 0,
    caret = 1,
    tilde = 2,
    semver = 3,
}

/// `workspace:*` / `workspace:^` / `workspace:~` / `workspace:^1.2.3`
pub struct WorkspaceSpec {
    kind: WorkspaceSpecKind = WorkspaceSpecKind.star,
    version: []const u8 = "",
}

/// Git / GitHub 依赖描述符
pub struct GitSpec {
    url: []const u8 = "",
    owner: []const u8 = "",
    repo: []const u8 = "",
    committish: ([]const u8)? = null,
}

/// `npm:@scope/pkg@^2.0.0` 别名描述符
pub struct NpmAliasSpec {
    alias_name: []const u8 = "",
    version_spec: []const u8 = "",
}

fn trimAscii(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    return s.len >= prefix.len && std.mem.eql(u8, s[0..prefix.len], prefix);
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    return s.len >= suffix.len && std.mem.eql(u8, s[s.len - suffix.len ..], suffix);
}

fn parseGitHubOwnerRepo(spec: []const u8): GitSpec {
    let base = spec;
    let committish: ([]const u8)? = null;
    if (hash := std.mem.indexOfScalar(u8, spec, '#')) {
        base = spec[0..hash];
        if (hash + 1 < spec.len) {
            committish = spec[hash + 1 ..];
        }
    }
    if (endsWith(base, ".git")) {
        base = base[0 .. base.len - 4];
    }
    let owner: []const u8 = "";
    let repo: []const u8 = base;
    if (slash := std.mem.indexOfScalar(u8, base, '/')) {
        owner = base[0..slash];
        repo = base[slash + 1 ..];
    }
    return GitSpec{
        url: spec,
        owner: owner,
        repo: repo,
        committish: committish,
    };
}

fn isSemverStart(s: []const u8): bool {
    if (s.len == 0) {
        return true;
    }
    const c = s[0];
    return (c >= '0' && c <= '9')
        || c == '^'
        || c == '~'
        || c == '>'
        || c == '<'
        || c == '='
        || c == '*'
        || c == 'v'
        || c == 'V'
        || c == 'x'
        || c == 'X';
}

/// 统一包管理器依赖描述符（纯 CP、零外部依赖、零堆分配）
pub struct DependencySpec {
    kind: DependencyKind = DependencyKind.npm,
    raw: []const u8 = "",
    value: []const u8 = "",
    workspace: WorkspaceSpec? = null,
    git: GitSpec? = null,
    npm_alias: NpmAliasSpec? = null,

    static fn parse(input: []const u8): DependencySpec {
        const s = trimAscii(input);

        // 1. workspace:
        if (startsWith(s, "workspace:")) {
            const rest = s[10..];
            let ws = WorkspaceSpec{ kind: WorkspaceSpecKind.semver, version: rest };
            if (std.mem.eql(u8, rest, "*")) {
                ws = WorkspaceSpec{ kind: WorkspaceSpecKind.star, version: "*" };
            } else if (std.mem.eql(u8, rest, "^")) {
                ws = WorkspaceSpec{ kind: WorkspaceSpecKind.caret, version: "^" };
            } else if (std.mem.eql(u8, rest, "~")) {
                ws = WorkspaceSpec{ kind: WorkspaceSpecKind.tilde, version: "~" };
            }
            return DependencySpec{
                kind: DependencyKind.workspace,
                raw: s,
                value: rest,
                workspace: ws,
            };
        }

        // 2. catalog:
        if (startsWith(s, "catalog:")) {
            const rest = s[8..];
            const cname = if (rest.len == 0) "default" else rest;
            return DependencySpec{
                kind: DependencyKind.catalog,
                raw: s,
                value: cname,
            };
        }

        // 3. npm: alias (e.g., npm:@scope/pkg@^2.0.0 or npm:lodash@4.17.21)
        if (startsWith(s, "npm:")) {
            const rest = s[4..];
            let alias_name = rest;
            let version_spec: []const u8 = "*";
            // Skip leading '@' for scoped packages when searching for version separator '@'
            const search_from: usize = if (rest.len > 0 && rest[0] == '@') usize(1) else usize(0);
            if (at_idx := std.mem.indexOfScalarPos(u8, rest, search_from, '@')) {
                alias_name = rest[0..at_idx];
                version_spec = if (at_idx + 1 < rest.len) rest[at_idx + 1 ..] else "*";
            }
            return DependencySpec{
                kind: DependencyKind.npm_alias,
                raw: s,
                value: rest,
                npm_alias: NpmAliasSpec{
                    alias_name: alias_name,
                    version_spec: version_spec,
                },
            };
        }

        // 4. file: / link: / local folder
        if (startsWith(s, "file:")) {
            return DependencySpec{ kind: DependencyKind.file, raw: s, value: s[5..] };
        }
        if (startsWith(s, "link:")) {
            return DependencySpec{ kind: DependencyKind.link, raw: s, value: s[5..] };
        }
        if (startsWith(s, "./") || startsWith(s, "../") || startsWith(s, "/") || startsWith(s, "~/")) {
            return DependencySpec{ kind: DependencyKind.folder, raw: s, value: s };
        }

        // 5. github:
        if (startsWith(s, "github:")) {
            const rest = s[7..];
            const gs = parseGitHubOwnerRepo(rest);
            return DependencySpec{
                kind: DependencyKind.github,
                raw: s,
                value: rest,
                git: gs,
            };
        }

        // 6. git+ / git:// / ssh://
        if (startsWith(s, "git+") || startsWith(s, "git://") || startsWith(s, "ssh://")) {
            let base = s;
            let committish: ([]const u8)? = null;
            if (hash := std.mem.indexOfScalar(u8, s, '#')) {
                base = s[0..hash];
                if (hash + 1 < s.len) {
                    committish = s[hash + 1 ..];
                }
            }
            return DependencySpec{
                kind: DependencyKind.git,
                raw: s,
                value: base,
                git: GitSpec{
                    url: base,
                    committish: committish,
                },
            };
        }

        // 7. http:// or https:// -> tarball or git
        if (startsWith(s, "https://") || startsWith(s, "http://")) {
            let base = s;
            let committish: ([]const u8)? = null;
            if (hash := std.mem.indexOfScalar(u8, s, '#')) {
                base = s[0..hash];
                if (hash + 1 < s.len) {
                    committish = s[hash + 1 ..];
                }
            }
            if (endsWith(base, ".git")) {
                return DependencySpec{
                    kind: DependencyKind.git,
                    raw: s,
                    value: base,
                    git: GitSpec{ url: base, committish: committish },
                };
            }
            return DependencySpec{
                kind: DependencyKind.tarball,
                raw: s,
                value: s,
            };
        }

        // 8. GitHub shorthand `owner/repo` or `owner/repo#ref`
        if (s.len > 2 && s[0] != '@' && std.mem.indexOfScalar(u8, s, ' ') == null) {
            if (slash := std.mem.indexOfScalar(u8, s, '/')) {
                if (slash > 0 && slash + 1 < s.len && std.mem.indexOfScalarPos(u8, s, slash + 1, '/') == null) {
                    const gs = parseGitHubOwnerRepo(s);
                    return DependencySpec{
                        kind: DependencyKind.github,
                        raw: s,
                        value: s,
                        git: gs,
                    };
                }
            }
        }

        // 9. npm semver range vs dist-tag
        if (isSemverStart(s)) {
            return DependencySpec{
                kind: DependencyKind.npm,
                raw: s,
                value: if (s.len == 0) "*" else s,
            };
        }
        return DependencySpec{
            kind: DependencyKind.dist_tag,
            raw: s,
            value: s,
        };
    }

    fn isLocal(): bool {
        return _.kind == DependencyKind.file
            || _.kind == DependencyKind.link
            || _.kind == DependencyKind.folder
            || _.kind == DependencyKind.workspace;
    }

    fn isWorkspace(): bool {
        return _.kind == DependencyKind.workspace;
    }

    fn isGit(): bool {
        return _.kind == DependencyKind.git || _.kind == DependencyKind.github;
    }

    /// 规范化输出到调用方缓冲区（缓冲区不足时返回空切片）
    fn toCanonical(out: []u8): []const u8 {
        const res = switch (_.kind) {
            .npm, .dist_tag, .folder, .tarball => std.fmt.bufPrint(out, "{s}", .{_.value}),
            .workspace => std.fmt.bufPrint(out, "workspace:{s}", .{_.value}),
            .catalog => std.fmt.bufPrint(out, "catalog:{s}", .{_.value}),
            .file => std.fmt.bufPrint(out, "file:{s}", .{_.value}),
            .link => std.fmt.bufPrint(out, "link:{s}", .{_.value}),
            .npm_alias => {
                const a = _.npm_alias.?;
                return std.fmt.bufPrint(out, "npm:{s}@{s}", .{ a.alias_name, a.version_spec }) catch "";
            },
            .github => {
                const g = _.git.?;
                if (comm := g.committish) {
                    return std.fmt.bufPrint(out, "github:{s}/{s}#{s}", .{ g.owner, g.repo, comm }) catch "";
                }
                return std.fmt.bufPrint(out, "github:{s}/{s}", .{ g.owner, g.repo }) catch "";
            },
            .git => {
                const g = _.git.?;
                if (comm := g.committish) {
                    return std.fmt.bufPrint(out, "{s}#{s}", .{ g.url, comm }) catch "";
                }
                return std.fmt.bufPrint(out, "{s}", .{g.url}) catch "";
            },
        } catch "";
        return res;
    }
}

fn isValidPkgSegment(seg: []const u8): bool {
    if (seg.len == 0) {
        return false;
    }
    // 禁止前导点或下划线
    if (seg[0] == '.' || seg[0] == '_') {
        return false;
    }
    for (c of seg) {
        // 仅允许小写字母、数字、'-'、'_'、'.'、'~'（禁止大写、空格与 URL 不安全字符）
        const ok = (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9')
            || c == '-'
            || c == '_'
            || c == '.'
            || c == '~';
        if (!ok) {
            return false;
        }
    }
    return true;
}

/// npm / cap 包名解析与合法性校验（长度 <= 214，支持 `@scope/name` 与非作用域包名）
pub struct PackageName {
    full: []const u8,
    scope: ([]const u8)? = null,
    name: []const u8,

    static fn isValid(input: []const u8): bool {
        return PackageName.parse(input) != null;
    }

    static fn parse(input: []const u8): PackageName? {
        if (input.len == 0 || input.len > 214) {
            return null;
        }
        if (input[0] == '@') {
            const slash = std.mem.indexOfScalar(u8, input, '/') ?? return null;
            // 不允许出现多个 '/'
            if (std.mem.indexOfScalarPos(u8, input, slash + 1, '/') != null) {
                return null;
            }
            const sc = input[1..slash];
            const nm = input[slash + 1 ..];
            if (!isValidPkgSegment(sc) || !isValidPkgSegment(nm)) {
                return null;
            }
            return PackageName{
                full: input,
                scope: sc,
                name: nm,
            };
        }
        if (!isValidPkgSegment(input)) {
            return null;
        }
        return PackageName{
            full: input,
            scope: null,
            name: input,
        };
    }

    fn isScoped(): bool {
        return _.scope != null;
    }
}
