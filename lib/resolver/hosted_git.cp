// Source: /data/dev/bun/src/install/hosted_git_info.cp
// ============================================================================
// lib/resolver/hosted_git.cp — Hosted Git Repository Specifier & URL Builder
// ============================================================================

const std = @import("std");

pub err HostedGitError {
    InvalidHostedGitUrl,
    UnsupportedGitHost,
    OutOfMemory,
}

/// Supported hosted Git service providers.
pub enum GitHost: u8 {
    GitHub = 0,
    GitLab = 1,
    Bitbucket = 2,
    Gist = 3,
    SourceHut = 4,

    fn domain(): []const u8 {
        return switch (_) {
            .GitHub => "github.com",
            .GitLab => "gitlab.com",
            .Bitbucket => "bitbucket.org",
            .Gist => "gist.github.com",
            .SourceHut => "git.sr.ht",
        };
    }

    fn shortcut(): []const u8 {
        return switch (_) {
            .GitHub => "github:",
            .GitLab => "gitlab:",
            .Bitbucket => "bitbucket:",
            .Gist => "gist:",
            .SourceHut => "sourcehut:",
        };
    }

    fn name(): []const u8 {
        return switch (_) {
            .GitHub => "github",
            .GitLab => "gitlab",
            .Bitbucket => "bitbucket",
            .Gist => "gist",
            .SourceHut => "sourcehut",
        };
    }

    static fn fromDomain(domain_str: []const u8): GitHost? {
        let d = domain_str;
        if (std.mem.startsWith(u8, d, "www.")) {
            d = d[4..];
        }
        if (std.mem.eql(u8, d, "github.com")) return .GitHub;
        if (std.mem.eql(u8, d, "gitlab.com")) return .GitLab;
        if (std.mem.eql(u8, d, "bitbucket.org")) return .Bitbucket;
        if (std.mem.eql(u8, d, "gist.github.com")) return .Gist;
        if (std.mem.eql(u8, d, "git.sr.ht")) return .SourceHut;
        return null;
    }

    static fn fromShortcut(shortcut_str: []const u8): GitHost? {
        let s = shortcut_str;
        if (s.len > 0 && s[s.len - 1] == ':') {
            s = s[0 .. s.len - 1];
        }
        if (std.mem.eql(u8, s, "github")) return .GitHub;
        if (std.mem.eql(u8, s, "gitlab")) return .GitLab;
        if (std.mem.eql(u8, s, "bitbucket")) return .Bitbucket;
        if (std.mem.eql(u8, s, "gist")) return .Gist;
        if (std.mem.eql(u8, s, "sourcehut")) return .SourceHut;
        return null;
    }
}

fn trimGitSuffix(s: []const u8): []const u8 {
    if (std.mem.endsWith(u8, s, ".git")) {
        return s[0 .. s.len - 4];
    }
    return s;
}

fn isGitHubShorthandStr(s: []const u8): bool {
    if (s.len == 0) return false;
    if (s[0] == '.' || s[0] == '/') return false;
    let pound_idx: usize? = null;
    let seen_slash = false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        switch (c) {
            ':' | '@' => {
                if (pound_idx == null) return false;
            },
            '#' => {
                if (pound_idx == null) pound_idx = i;
            },
            '/' => {
                if (seen_slash && pound_idx == null) return false;
                seen_slash = true;
            },
            else => {
                if (std.ascii.isWhitespace(c) && pound_idx == null) return false;
            },
        }
    }
    if (!seen_slash) return false;
    if (pi := pound_idx) {
        return pi > 0 && s[pi - 1] != '/';
    }
    return s[s.len - 1] != '/';
}

struct SplitRef {
    base: []const u8 = "",
    committish: ([]const u8)? = null,
}

fn splitCommittish(s: []const u8): SplitRef {
    if (idx := std.mem.indexOfScalar(u8, s, '#')) {
        const git_ref = s[idx + 1 ..];
        return SplitRef{
            base: s[0..idx],
            committish: if (git_ref.len > 0) git_ref else null,
        };
    }
    return SplitRef{ base: s, committish: null };
}

fn stripKnownUrlScheme(s: []const u8): ([]const u8)? {
    if (std.mem.startsWith(u8, s, "git+https://")) return s[12..];
    if (std.mem.startsWith(u8, s, "git+http://")) return s[11..];
    if (std.mem.startsWith(u8, s, "git+ssh://")) return s[10..];
    if (std.mem.startsWith(u8, s, "git+file://")) return s[11..];
    if (std.mem.startsWith(u8, s, "git+rsync://")) return s[12..];
    if (std.mem.startsWith(u8, s, "https://")) return s[8..];
    if (std.mem.startsWith(u8, s, "http://")) return s[7..];
    if (std.mem.startsWith(u8, s, "ssh://")) return s[6..];
    if (std.mem.startsWith(u8, s, "git://")) return s[6..];
    return null;
}

/// Parsed hosted Git repository descriptor (`GitHub`, `GitLab`, `Bitbucket`, `Gist`, `SourceHut`).
pub struct HostedGitInfo {
    host: GitHost = .GitHub,
    user: []const u8 = "",
    repo: []const u8 = "",
    committish: ([]const u8)? = null,

    /// Parses hosted Git specifiers, shortcuts, URLs, and SCP-style SSH strings:
    /// - `"user/repo#main"`
    /// - `"github:user/repo#v1.0"`
    /// - `"https://github.com/user/repo.git#commit"`
    /// - `"git+ssh://git@gitlab.com/org/proj.git"`
    /// - `"git@github.com:user/repo.git#v1.0"`
    pub static fn parse(raw_input: []const u8): HostedGitInfo! {
        const input = std.mem.trim(u8, raw_input, " \t\r\n");
        if (input.len == 0) return error.InvalidHostedGitUrl;

        // 1. Bare GitHub shorthand: "user/repo" or "user/repo#ref"
        if (isGitHubShorthandStr(input)) {
            const split1 = splitCommittish(input);
            const body = split1.base;
            const committish = split1.committish;
            const slash = std.mem.indexOfScalar(u8, body, '/') ?? return error.InvalidHostedGitUrl;
            const user = body[0..slash];
            const repo = trimGitSuffix(body[slash + 1 ..]);
            if (user.len == 0 || repo.len == 0) return error.InvalidHostedGitUrl;
            return HostedGitInfo{
                host: .GitHub,
                user: user,
                repo: repo,
                committish: committish,
            };
        }

        // 2. Shortcut protocol: "github:user/repo#v1.0", "gitlab:org/sub/repo", "gist:id"
        if (colon_idx := std.mem.indexOfScalar(u8, input, ':')) {
            const scheme_part = input[0..colon_idx];
            if (host := GitHost.fromShortcut(scheme_part)) {
                const rest_raw = input[colon_idx + 1 ..];
                if (std.mem.startsWith(u8, rest_raw, "//")) {
                    // Treat "github://user/repo" like shortcut path too
                    return parseShortcutBody(host, rest_raw[2..]);
                }
                return parseShortcutBody(host, rest_raw);
            }
        }

        // 3. Standard URL with scheme: "https://...", "git+ssh://...", "git://..."
        if (after_scheme := stripKnownUrlScheme(input)) {
            const split3 = splitCommittish(after_scheme);
            const authority_and_path = split3.base;
            const frag_committish = split3.committish;
            const first_slash = std.mem.indexOfScalar(u8, authority_and_path, '/') ?? return error.InvalidHostedGitUrl;
            let authority = authority_and_path[0..first_slash];
            let pathname = authority_and_path[first_slash + 1 ..];

            // Strip optional "user@" or "token@" in authority
            if (at_idx := std.mem.lastIndexOfScalar(u8, authority, '@')) {
                authority = authority[at_idx + 1 ..];
            }
            // Strip optional ":port" in authority
            if (port_idx := std.mem.indexOfScalar(u8, authority, ':')) {
                authority = authority[0..port_idx];
            }

            const host = GitHost.fromDomain(authority) ?? return error.UnsupportedGitHost;
            return extractFromHostAndPath(host, pathname, frag_committish);
        }

        // 4. SCP-style SSH: "git@github.com:user/repo.git#v1.0"
        if (at_idx := std.mem.indexOfScalar(u8, input, '@')) {
            const after_at = input[at_idx + 1 ..];
            const colon_idx = std.mem.indexOfScalar(u8, after_at, ':') ?? return error.InvalidHostedGitUrl;
            const domain_part = after_at[0..colon_idx];
            const path_and_frag = after_at[colon_idx + 1 ..];
            const split4 = splitCommittish(path_and_frag);
            const pathname = split4.base;
            const frag_committish = split4.committish;
            const host = GitHost.fromDomain(domain_part) ?? return error.UnsupportedGitHost;
            return extractFromHostAndPath(host, pathname, frag_committish);
        }

        return error.InvalidHostedGitUrl;
    }

    /// Generates an HTTPS clone URL (`"https://<domain>/<user>/<repo>.git[#<ref>]"`).
    pub fn httpsUrlAlloc(allocator: std.mem.Allocator): []u8! {
        zig {
            if (self.host == .Gist and self.user.len == 0) {
                if (self.committish) |c| {
                    return std.fmt.allocPrint(allocator, "https://{s}/{s}.git#{s}", .{ self.host.domain(), self.repo, c }) catch return error.OutOfMemory;
                }
                return std.fmt.allocPrint(allocator, "https://{s}/{s}.git", .{ self.host.domain(), self.repo }) catch return error.OutOfMemory;
            }
            if (self.committish) |c| {
                return std.fmt.allocPrint(allocator, "https://{s}/{s}/{s}.git#{s}", .{ self.host.domain(), self.user, self.repo, c }) catch return error.OutOfMemory;
            }
            return std.fmt.allocPrint(allocator, "https://{s}/{s}/{s}.git", .{ self.host.domain(), self.user, self.repo }) catch return error.OutOfMemory;
        }
    }

    /// Generates an archive tarball URL for the configured host and committish (defaults to `"HEAD"`).
    pub fn tarballUrlAlloc(allocator: std.mem.Allocator): []u8! {
        const git_ref = _.committish ?? "HEAD";
        zig {
            return switch (self.host) {
                .GitHub => std.fmt.allocPrint(allocator, "https://codeload.github.com/{s}/{s}/tar.gz/{s}", .{ self.user, self.repo, git_ref }) catch return error.OutOfMemory,
                .GitLab => std.fmt.allocPrint(allocator, "https://gitlab.com/{s}/{s}/-/archive/{s}/{s}-{s}.tar.gz", .{ self.user, self.repo, git_ref, self.repo, git_ref }) catch return error.OutOfMemory,
                .Bitbucket => std.fmt.allocPrint(allocator, "https://bitbucket.org/{s}/{s}/get/{s}.tar.gz", .{ self.user, self.repo, git_ref }) catch return error.OutOfMemory,
                .Gist => std.fmt.allocPrint(allocator, "https://codeload.github.com/gist/{s}/tar.gz/{s}", .{ self.repo, git_ref }) catch return error.OutOfMemory,
                .SourceHut => std.fmt.allocPrint(allocator, "https://git.sr.ht/{s}/{s}/archive/{s}.tar.gz", .{ self.user, self.repo, git_ref }) catch return error.OutOfMemory,
            };
        }
    }

    /// Generates an SCP-style SSH URL (`"git@<domain>:<user>/<repo>.git[#<ref>]"`).
    pub fn sshUrlAlloc(allocator: std.mem.Allocator): []u8! {
        zig {
            if (self.host == .Gist and self.user.len == 0) {
                if (self.committish) |c| {
                    return std.fmt.allocPrint(allocator, "git@{s}:{s}.git#{s}", .{ self.host.domain(), self.repo, c }) catch return error.OutOfMemory;
                }
                return std.fmt.allocPrint(allocator, "git@{s}:{s}.git", .{ self.host.domain(), self.repo }) catch return error.OutOfMemory;
            }
            if (self.committish) |c| {
                return std.fmt.allocPrint(allocator, "git@{s}:{s}/{s}.git#{s}", .{ self.host.domain(), self.user, self.repo, c }) catch return error.OutOfMemory;
            }
            return std.fmt.allocPrint(allocator, "git@{s}:{s}/{s}.git", .{ self.host.domain(), self.user, self.repo }) catch return error.OutOfMemory;
        }
    }

    /// Generates a `git+ssh://` URL (`"git+ssh://git@<domain>/<user>/<repo>.git[#<ref>]"`).
    pub fn gitSshUrlAlloc(allocator: std.mem.Allocator): []u8! {
        zig {
            if (self.host == .Gist and self.user.len == 0) {
                if (self.committish) |c| {
                    return std.fmt.allocPrint(allocator, "git+ssh://git@{s}/{s}.git#{s}", .{ self.host.domain(), self.repo, c }) catch return error.OutOfMemory;
                }
                return std.fmt.allocPrint(allocator, "git+ssh://git@{s}/{s}.git", .{ self.host.domain(), self.repo }) catch return error.OutOfMemory;
            }
            if (self.committish) |c| {
                return std.fmt.allocPrint(allocator, "git+ssh://git@{s}/{s}/{s}.git#{s}", .{ self.host.domain(), self.user, self.repo, c }) catch return error.OutOfMemory;
            }
            return std.fmt.allocPrint(allocator, "git+ssh://git@{s}/{s}/{s}.git", .{ self.host.domain(), self.user, self.repo }) catch return error.OutOfMemory;
        }
    }

    /// Generates a canonical shortcut string (`"<host>:<user>/<repo>[#<ref>]"`).
    pub fn shortcutAlloc(allocator: std.mem.Allocator): []u8! {
        zig {
            if (self.host == .Gist and self.user.len == 0) {
                if (self.committish) |c| {
                    return std.fmt.allocPrint(allocator, "{s}{s}#{s}", .{ self.host.shortcut(), self.repo, c }) catch return error.OutOfMemory;
                }
                return std.fmt.allocPrint(allocator, "{s}{s}", .{ self.host.shortcut(), self.repo }) catch return error.OutOfMemory;
            }
            if (self.committish) |c| {
                return std.fmt.allocPrint(allocator, "{s}{s}/{s}#{s}", .{ self.host.shortcut(), self.user, self.repo, c }) catch return error.OutOfMemory;
            }
            return std.fmt.allocPrint(allocator, "{s}{s}/{s}", .{ self.host.shortcut(), self.user, self.repo }) catch return error.OutOfMemory;
        }
    }
}

fn parseShortcutBody(host: GitHost, rest_raw: []const u8): HostedGitInfo! {
    let rest = std.mem.trimStart(u8, rest_raw, "/");
    if (at_idx := std.mem.indexOfScalar(u8, rest, '@')) {
        rest = rest[at_idx + 1 ..];
    }
    const sp = splitCommittish(rest);
    const body = sp.base;
    const committish = sp.committish;
    if (host == .Gist) {
        if (slash_idx := std.mem.lastIndexOfScalar(u8, body, '/')) {
            const user = body[0..slash_idx];
            const repo = trimGitSuffix(body[slash_idx + 1 ..]);
            if (repo.len == 0) return error.InvalidHostedGitUrl;
            return HostedGitInfo{
                host: .Gist,
                user: user,
                repo: repo,
                committish: committish,
            };
        }
        const repo = trimGitSuffix(body);
        if (repo.len == 0) return error.InvalidHostedGitUrl;
        return HostedGitInfo{
            host: .Gist,
            user: "",
            repo: repo,
            committish: committish,
        };
    }

    const last_slash = std.mem.lastIndexOfScalar(u8, body, '/') ?? return error.InvalidHostedGitUrl;
    const user = body[0..last_slash];
    const repo = trimGitSuffix(body[last_slash + 1 ..]);
    if (user.len == 0 || repo.len == 0) return error.InvalidHostedGitUrl;
    return HostedGitInfo{
        host: host,
        user: user,
        repo: repo,
        committish: committish,
    };
}

fn extractFromHostAndPath(host: GitHost, raw_path: []const u8, frag_committish: ([]const u8)?): HostedGitInfo! {
    let path = std.mem.trim(u8, raw_path, "/");
    if (path.len == 0) return error.InvalidHostedGitUrl;

    switch (host) {
        .GitHub => {
            let iter = std.mem.splitScalar(u8, path, '/');
            const user = iter.next() ?? return error.InvalidHostedGitUrl;
            const repo_raw = iter.next() ?? return error.InvalidHostedGitUrl;
            const repo = trimGitSuffix(repo_raw);
            if (user.len == 0 || repo.len == 0) return error.InvalidHostedGitUrl;

            let committish = frag_committish;
            if (seg3 := iter.next()) {
                if (!std.mem.eql(u8, seg3, "tree")) {
                    return error.InvalidHostedGitUrl;
                }
                if (tree_ref := iter.next()) {
                    if (tree_ref.len > 0) {
                        committish = tree_ref;
                    }
                }
            }
            return HostedGitInfo{
                host: .GitHub,
                user: user,
                repo: repo,
                committish: committish,
            };
        },
        .GitLab => {
            if (std.mem.indexOf(u8, path, "/-/") != null || std.mem.indexOf(u8, path, "/archive.tar.gz") != null) {
                return error.InvalidHostedGitUrl;
            }
            const last_slash = std.mem.lastIndexOfScalar(u8, path, '/') ?? return error.InvalidHostedGitUrl;
            const user = path[0..last_slash];
            const repo = trimGitSuffix(path[last_slash + 1 ..]);
            if (user.len == 0 || repo.len == 0) return error.InvalidHostedGitUrl;
            return HostedGitInfo{
                host: .GitLab,
                user: user,
                repo: repo,
                committish: frag_committish,
            };
        },
        .Bitbucket => {
            let iter = std.mem.splitScalar(u8, path, '/');
            const user = iter.next() ?? return error.InvalidHostedGitUrl;
            const repo_raw = iter.next() ?? return error.InvalidHostedGitUrl;
            if (aux := iter.next()) {
                if (std.mem.eql(u8, aux, "get")) return error.InvalidHostedGitUrl;
            }
            const repo = trimGitSuffix(repo_raw);
            if (user.len == 0 || repo.len == 0) return error.InvalidHostedGitUrl;
            return HostedGitInfo{
                host: .Bitbucket,
                user: user,
                repo: repo,
                committish: frag_committish,
            };
        },
        .Gist => {
            let iter = std.mem.splitScalar(u8, path, '/');
            const first = iter.next() ?? return error.InvalidHostedGitUrl;
            const second_opt = iter.next();
            if (aux := iter.next()) {
                if (std.mem.eql(u8, aux, "raw")) return error.InvalidHostedGitUrl;
            }
            let user: []const u8 = "";
            let repo_raw = first;
            if (second := second_opt) {
                if (second.len > 0) {
                    user = first;
                    repo_raw = second;
                }
            }
            const repo = trimGitSuffix(repo_raw);
            if (repo.len == 0) return error.InvalidHostedGitUrl;
            return HostedGitInfo{
                host: .Gist,
                user: user,
                repo: repo,
                committish: frag_committish,
            };
        },
        .SourceHut => {
            let iter = std.mem.splitScalar(u8, path, '/');
            const user = iter.next() ?? return error.InvalidHostedGitUrl;
            const repo_raw = iter.next() ?? return error.InvalidHostedGitUrl;
            if (aux := iter.next()) {
                if (std.mem.eql(u8, aux, "archive")) return error.InvalidHostedGitUrl;
            }
            const repo = trimGitSuffix(repo_raw);
            if (user.len == 0 || repo.len == 0) return error.InvalidHostedGitUrl;
            return HostedGitInfo{
                host: .SourceHut,
                user: user,
                repo: repo,
                committish: frag_committish,
            };
        },
    }
}
