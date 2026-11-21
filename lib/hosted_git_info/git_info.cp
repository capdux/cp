// Source: Absorbed from /data/dev/bun/src/install/hosted_git_info.cp | Synced: 2026-11-21T13:44:46Z

/// 主流托管 Git 平台枚举
pub enum GitProvider: u8 {
    github = 0,
    gitlab = 1,
    bitbucket = 2,
    gist = 3,

    fn domain(): []const u8 {
        return switch (_) {
            .github => "github.com",
            .gitlab => "gitlab.com",
            .bitbucket => "bitbucket.org",
            .gist => "gist.github.com",
        };
    }

    fn shortcutPrefix(): []const u8 {
        return switch (_) {
            .github => "github:",
            .gitlab => "gitlab:",
            .bitbucket => "bitbucket:",
            .gist => "gist:",
        };
    }
}

fn trimAscii(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    return s.len >= prefix.len && std.ascii.eqlIgnoreCase(s[0..prefix.len], prefix);
}

fn stripGitSuffix(s: []const u8): []const u8 {
    let res = s;
    while (res.len > 0 && res[res.len - 1] == '/') {
        res = res[0 .. res.len - 1];
    }
    if (res.len > 4 && std.ascii.eqlIgnoreCase(res[res.len - 4 ..], ".git")) {
        res = res[0 .. res.len - 4];
    }
    return res;
}

fn matchHost(host: []const u8): GitProvider? {
    if (std.ascii.eqlIgnoreCase(host, "github.com") || std.ascii.eqlIgnoreCase(host, "www.github.com")) {
        return .github;
    }
    if (std.ascii.eqlIgnoreCase(host, "gitlab.com") || std.ascii.eqlIgnoreCase(host, "www.gitlab.com")) {
        return .gitlab;
    }
    if (std.ascii.eqlIgnoreCase(host, "bitbucket.org") || std.ascii.eqlIgnoreCase(host, "www.bitbucket.org")) {
        return .bitbucket;
    }
    if (std.ascii.eqlIgnoreCase(host, "gist.github.com")) {
        return .gist;
    }
    return null;
}

fn isValidSlug(s: []const u8): bool {
    if (s.len == 0) { return false; }
    for (c of s) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '/' || c == ':' || c == '@') {
            return false;
        }
    }
    return true;
}

fn parseProviderPath(
    provider: GitProvider,
    raw_path: []const u8,
    initial_committish: ([]const u8)?,
    auth: ([]const u8)?,
): HostedGitInfo? {
    let p = raw_path;
    while (p.len > 0 && p[0] == '/') {
        p = p[1..];
    }
    while (p.len > 0 && p[p.len - 1] == '/') {
        p = p[0 .. p.len - 1];
    }
    if (p.len == 0) {
        return null;
    }

    if (provider == GitProvider.gist) {
        if (slash := std.mem.indexOfScalar(u8, p, '/')) {
            const u = p[0..slash];
            const proj = stripGitSuffix(p[slash + 1 ..]);
            if (!isValidSlug(u) || !isValidSlug(proj)) { return null; }
            return HostedGitInfo{
                provider: .gist,
                user: u,
                project: proj,
                committish: initial_committish,
                auth: auth,
            };
        }
        const proj = stripGitSuffix(p);
        if (!isValidSlug(proj)) { return null; }
        return HostedGitInfo{
            provider: .gist,
            user: null,
            project: proj,
            committish: initial_committish,
            auth: auth,
        };
    }

    // github / gitlab / bitbucket: <user>/<repo>[/tree/<branch>]
    const first_slash = std.mem.indexOfScalar(u8, p, '/') ?? return null;
    const u = p[0..first_slash];
    const after_u = p[first_slash + 1 ..];
    if (!isValidSlug(u) || after_u.len == 0) {
        return null;
    }

    let proj = after_u;
    let comm = initial_committish;
    if (second_slash := std.mem.indexOfScalar(u8, after_u, '/')) {
        proj = after_u[0..second_slash];
        const rest = after_u[second_slash + 1 ..];
        if (startsWithIgnoreCase(rest, "tree/") && rest.len > 5) {
            if (comm == null) {
                comm = rest[5..];
            }
        } else if (startsWithIgnoreCase(rest, "-/tree/") && rest.len > 7) {
            if (comm == null) {
                comm = rest[7..];
            }
        } else if (startsWithIgnoreCase(rest, "src/") && rest.len > 4) {
            if (comm == null) {
                comm = rest[4..];
            }
        } else {
            return null;
        }
    }

    proj = stripGitSuffix(proj);
    if (!isValidSlug(proj)) {
        return null;
    }

    return HostedGitInfo{
        provider: provider,
        user: u,
        project: proj,
        committish: comm,
        auth: auth,
    };
}

/// 托管 Git 平台仓库描述符与多格式 URL 生成器（纯 CP、零外部依赖、零堆分配）
pub struct HostedGitInfo {
    provider: GitProvider,
    user: ([]const u8)? = null,
    project: []const u8,
    committish: ([]const u8)? = null,
    auth: ([]const u8)? = null,

    /// 解析 shorthand / HTTPS / HTTP / git+https / git+ssh / SSH SCP / tree URL
    static fn parse(input: []const u8): HostedGitInfo? {
        const trimmed = trimAscii(input);
        if (trimmed.len == 0) {
            return null;
        }

        // 剥离可选 `#committish`
        let base = trimmed;
        let comm: ([]const u8)? = null;
        if (hash := std.mem.indexOfScalar(u8, trimmed, '#')) {
            base = trimmed[0..hash];
            if (hash + 1 < trimmed.len) {
                comm = trimmed[hash + 1 ..];
            }
        }
        if (base.len == 0) {
            return null;
        }

        // 1. 显式 scheme URL (`git+https://`, `https://`, `http://`, `git+ssh://`, `ssh://`, `git://`)
        let rest_opt: ([]const u8)? = null;
        let is_ssh_scheme = false;
        if (startsWithIgnoreCase(base, "git+https://")) {
            rest_opt = base[12..];
        } else if (startsWithIgnoreCase(base, "git+http://")) {
            rest_opt = base[11..];
        } else if (startsWithIgnoreCase(base, "git+ssh://")) {
            rest_opt = base[10..];
            is_ssh_scheme = true;
        } else if (startsWithIgnoreCase(base, "https://")) {
            rest_opt = base[8..];
        } else if (startsWithIgnoreCase(base, "http://")) {
            rest_opt = base[7..];
        } else if (startsWithIgnoreCase(base, "ssh://")) {
            rest_opt = base[6..];
            is_ssh_scheme = true;
        } else if (startsWithIgnoreCase(base, "git://")) {
            rest_opt = base[6..];
        }

        if (after_scheme := rest_opt) {
            const path_slash = std.mem.indexOfScalar(u8, after_scheme, '/') ?? return null;
            let authority = after_scheme[0..path_slash];
            const path = after_scheme[path_slash + 1 ..];
            let auth: ([]const u8)? = null;
            if (at := std.mem.indexOfScalar(u8, authority, '@')) {
                const candidate = authority[0..at];
                if (!is_ssh_scheme || !std.mem.eql(u8, candidate, "git")) {
                    auth = candidate;
                }
                authority = authority[at + 1 ..];
            }
            const prov = matchHost(authority) ?? return null;
            return parseProviderPath(prov, path, comm, auth);
        }

        // 2. 显式 provider shortcut (`github:`, `gitlab:`, `bitbucket:`, `gist:`)
        if (startsWithIgnoreCase(base, "github:")) {
            return parseProviderPath(.github, base[7..], comm, null);
        }
        if (startsWithIgnoreCase(base, "gitlab:")) {
            return parseProviderPath(.gitlab, base[7..], comm, null);
        }
        if (startsWithIgnoreCase(base, "bitbucket:")) {
            return parseProviderPath(.bitbucket, base[10..], comm, null);
        }
        if (startsWithIgnoreCase(base, "gist:")) {
            return parseProviderPath(.gist, base[5..], comm, null);
        }

        // 3. SSH SCP 语法 (`git@github.com:owner/repo.git`)
        if (at := std.mem.indexOfScalar(u8, base, '@')) {
            if (colon := std.mem.indexOfScalarPos(u8, base, at + 1, ':')) {
                const host = base[at + 1 .. colon];
                const path = base[colon + 1 ..];
                const prov = matchHost(host) ?? return null;
                return parseProviderPath(prov, path, comm, null);
            }
            return null;
        }

        // 4. 隐式 GitHub shorthand (`owner/repo`)
        if (base[0] != '.' && base[0] != '/' && std.mem.indexOfScalar(u8, base, ':') == null) {
            return parseProviderPath(.github, base, comm, null);
        }
        return null;
    }

    fn toHttps(out: []u8): ([]const u8)? {
        const dom = _.provider.domain();
        if (_.provider == GitProvider.gist && _.user == null) {
            if (comm := _.committish) {
                return std.fmt.bufPrint(out, "https://{s}/{s}.git#{s}", .{ dom, _.project, comm }) catch null;
            }
            return std.fmt.bufPrint(out, "https://{s}/{s}.git", .{ dom, _.project }) catch null;
        }
        const u = _.user ?? "";
        if (a := _.auth) {
            if (comm := _.committish) {
                return std.fmt.bufPrint(out, "https://{s}@{s}/{s}/{s}.git#{s}", .{ a, dom, u, _.project, comm }) catch null;
            }
            return std.fmt.bufPrint(out, "https://{s}@{s}/{s}/{s}.git", .{ a, dom, u, _.project }) catch null;
        }
        if (comm := _.committish) {
            return std.fmt.bufPrint(out, "https://{s}/{s}/{s}.git#{s}", .{ dom, u, _.project, comm }) catch null;
        }
        return std.fmt.bufPrint(out, "https://{s}/{s}/{s}.git", .{ dom, u, _.project }) catch null;
    }

    fn toSsh(out: []u8): ([]const u8)? {
        const dom = _.provider.domain();
        if (_.provider == GitProvider.gist && _.user == null) {
            if (comm := _.committish) {
                return std.fmt.bufPrint(out, "git@{s}:{s}.git#{s}", .{ dom, _.project, comm }) catch null;
            }
            return std.fmt.bufPrint(out, "git@{s}:{s}.git", .{ dom, _.project }) catch null;
        }
        const u = _.user ?? "";
        if (comm := _.committish) {
            return std.fmt.bufPrint(out, "git@{s}:{s}/{s}.git#{s}", .{ dom, u, _.project, comm }) catch null;
        }
        return std.fmt.bufPrint(out, "git@{s}:{s}/{s}.git", .{ dom, u, _.project }) catch null;
    }

    fn toSshUrl(out: []u8): ([]const u8)? {
        const dom = _.provider.domain();
        if (_.provider == GitProvider.gist && _.user == null) {
            if (comm := _.committish) {
                return std.fmt.bufPrint(out, "git+ssh://git@{s}/{s}.git#{s}", .{ dom, _.project, comm }) catch null;
            }
            return std.fmt.bufPrint(out, "git+ssh://git@{s}/{s}.git", .{ dom, _.project }) catch null;
        }
        const u = _.user ?? "";
        if (comm := _.committish) {
            return std.fmt.bufPrint(out, "git+ssh://git@{s}/{s}/{s}.git#{s}", .{ dom, u, _.project, comm }) catch null;
        }
        return std.fmt.bufPrint(out, "git+ssh://git@{s}/{s}/{s}.git", .{ dom, u, _.project }) catch null;
    }

    fn toShortcut(out: []u8): ([]const u8)? {
        const pre = _.provider.shortcutPrefix();
        if (_.provider == GitProvider.gist && _.user == null) {
            if (comm := _.committish) {
                return std.fmt.bufPrint(out, "{s}{s}#{s}", .{ pre, _.project, comm }) catch null;
            }
            return std.fmt.bufPrint(out, "{s}{s}", .{ pre, _.project }) catch null;
        }
        const u = _.user ?? "";
        if (comm := _.committish) {
            return std.fmt.bufPrint(out, "{s}{s}/{s}#{s}", .{ pre, u, _.project, comm }) catch null;
        }
        return std.fmt.bufPrint(out, "{s}{s}/{s}", .{ pre, u, _.project }) catch null;
    }

    fn toTarball(out: []u8): ([]const u8)? {
        const comm = _.committish ?? "HEAD";
        const u = _.user ?? "";
        return switch (_.provider) {
            .github => std.fmt.bufPrint(out, "https://codeload.github.com/{s}/{s}/tar.gz/{s}", .{ u, _.project, comm }) catch null,
            .gitlab => std.fmt.bufPrint(out, "https://gitlab.com/{s}/{s}/repository/archive.tar.gz?ref={s}", .{ u, _.project, comm }) catch null,
            .bitbucket => std.fmt.bufPrint(out, "https://bitbucket.org/{s}/{s}/get/{s}.tar.gz", .{ u, _.project, comm }) catch null,
            .gist => std.fmt.bufPrint(out, "https://codeload.github.com/gist/{s}/tar.gz/{s}", .{ _.project, comm }) catch null,
        };
    }

    fn toBrowse(out: []u8): ([]const u8)? {
        const u = _.user ?? "";
        if (comm := _.committish) {
            return switch (_.provider) {
                .github => std.fmt.bufPrint(out, "https://github.com/{s}/{s}/tree/{s}", .{ u, _.project, comm }) catch null,
                .gitlab => std.fmt.bufPrint(out, "https://gitlab.com/{s}/{s}/-/tree/{s}", .{ u, _.project, comm }) catch null,
                .bitbucket => std.fmt.bufPrint(out, "https://bitbucket.org/{s}/{s}/src/{s}", .{ u, _.project, comm }) catch null,
                .gist => std.fmt.bufPrint(out, "https://gist.github.com/{s}/{s}", .{ _.project, comm }) catch null,
            };
        }
        return switch (_.provider) {
            .github => std.fmt.bufPrint(out, "https://github.com/{s}/{s}", .{ u, _.project }) catch null,
            .gitlab => std.fmt.bufPrint(out, "https://gitlab.com/{s}/{s}", .{ u, _.project }) catch null,
            .bitbucket => std.fmt.bufPrint(out, "https://bitbucket.org/{s}/{s}", .{ u, _.project }) catch null,
            .gist => std.fmt.bufPrint(out, "https://gist.github.com/{s}", .{_.project}) catch null,
        };
    }

    fn toBugs(out: []u8): ([]const u8)? {
        const u = _.user ?? "";
        return switch (_.provider) {
            .github => std.fmt.bufPrint(out, "https://github.com/{s}/{s}/issues", .{ u, _.project }) catch null,
            .gitlab => std.fmt.bufPrint(out, "https://gitlab.com/{s}/{s}/-/issues", .{ u, _.project }) catch null,
            .bitbucket => std.fmt.bufPrint(out, "https://bitbucket.org/{s}/{s}/issues", .{ u, _.project }) catch null,
            .gist => std.fmt.bufPrint(out, "https://gist.github.com/{s}", .{_.project}) catch null,
        };
    }
}
