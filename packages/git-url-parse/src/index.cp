// @cap/git-url — Zero-allocation Git repository URL parser & normalizer for CP
// Clean-room MIT implementation inspired by npm `hosted-git-info` and `git-url-parse`.

pub enum GitProtocol: u8 {
    Https = 0,
    Http = 1,
    Ssh = 2,
    Git = 3,
    File = 4,
    Shorthand = 5,
}

pub enum GitHostKind: u8 {
    GitHub = 0,
    GitLab = 1,
    Bitbucket = 2,
    Gitee = 3,
    Codeberg = 4,
    Other = 5,
}

pub struct ParsedGitUrl {
    pub valid: bool,
    pub protocol: GitProtocol,
    pub host_kind: GitHostKind,
    pub user: string,
    pub host: string,
    pub port: u16,
    pub owner: string,
    pub repo: string,
    pub ref_name: string,
    pub subpath: string,

    pub static fn invalid() -> ParsedGitUrl {
        return ParsedGitUrl{
            valid: false,
            protocol: GitProtocol.Https,
            host_kind: GitHostKind.Other,
            user: "",
            host: "",
            port: 0,
            owner: "",
            repo: "",
            ref_name: "",
            subpath: "",
        };
    }
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isWs(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimWs(s: string) -> string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && isWs(s[a])) {
        a += 1;
    }
    while (b > a && isWs(s[b - 1])) {
        b -= 1;
    }
    return s[a..b];
}

fn eqIgnoreCase(a: string, b: string) -> bool {
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

fn startsWithIgnoreCase(s: string, prefix: string) -> bool {
    if (s.len < prefix.len) {
        return false;
    }
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

fn indexOfChar(s: string, ch: u8) -> usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn lastIndexOfChar(s: string, ch: u8) -> usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) {
            return i;
        }
    }
    return null;
}

fn indexOfSubstr(s: string, sub: string) -> usize? {
    if (sub.len == 0 || sub.len > s.len) {
        return null;
    }
    let i: usize = 0;
    while (i + sub.len <= s.len) {
        let j: usize = 0;
        let ok = true;
        while (j < sub.len) {
            if (s[i + j] != sub[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn hasWhitespace(s: string) -> bool {
    let i: usize = 0;
    while (i < s.len) {
        if (isWs(s[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn parsePort(s: string) -> u16? {
    if (s.len == 0 || s.len > 5) {
        return null;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        val = val * 10 + ((c - '0') as u32);
        if (val > 65535) {
            return null;
        }
        i += 1;
    }
    if (val == 0) {
        return null;
    }
    return val as u16;
}

fn detectHostKind(host: string) -> GitHostKind {
    if (eqIgnoreCase(host, "github.com")) return GitHostKind.GitHub;
    if (eqIgnoreCase(host, "gitlab.com")) return GitHostKind.GitLab;
    if (eqIgnoreCase(host, "bitbucket.org")) return GitHostKind.Bitbucket;
    if (eqIgnoreCase(host, "gitee.com")) return GitHostKind.Gitee;
    if (eqIgnoreCase(host, "codeberg.org")) return GitHostKind.Codeberg;
    return GitHostKind.Other;
}

fn stripDotGit(s: string) -> string {
    if (s.len > 4 && eqIgnoreCase(s[(s.len - 4)..s.len], ".git")) {
        return s[0..(s.len - 4)];
    }
    return s;
}

fn isValidSegment(s: string) -> bool {
    if (s.len == 0) return false;
    if (eqIgnoreCase(s, ".") || eqIgnoreCase(s, "..")) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Public API: Parser
// ---------------------------------------------------------------------------

/// Parses a Git repository URL (SCP SSH, HTTPS, HTTP, SSH+port, `git+https`, hosted shorthands,
/// or bare `owner/repo`) with zero heap allocation.
pub fn parseGitUrl(raw: string) -> ParsedGitUrl {
    let s = trimWs(raw);
    if (s.len == 0 || hasWhitespace(s)) {
        return ParsedGitUrl.invalid();
    }

    let ref_name: string = "";
    let subpath: string = "";

    // 1. Extract optional '#ref' fragment
    const hash_pos = indexOfChar(s, '#');
    if (hash_pos != null) {
        ref_name = s[(hash_pos.? + 1)..s.len];
        s = s[0..hash_pos.?];
    }

    // 2. Strip optional 'git+' transport prefix (e.g. git+https://, git+ssh://)
    if (startsWithIgnoreCase(s, "git+") && indexOfSubstr(s, "://") != null) {
        s = s[4..s.len];
    }

    let protocol = GitProtocol.Https;
    let user: string = "";
    let host: string = "";
    let port: u16 = 0;
    let path: string = "";

    const scheme_sep = indexOfSubstr(s, "://");
    if (scheme_sep != null) {
        const scheme = s[0..scheme_sep.?];
        if (eqIgnoreCase(scheme, "https")) {
            protocol = GitProtocol.Https;
        } else if (eqIgnoreCase(scheme, "http")) {
            protocol = GitProtocol.Http;
        } else if (eqIgnoreCase(scheme, "ssh")) {
            protocol = GitProtocol.Ssh;
        } else if (eqIgnoreCase(scheme, "git")) {
            protocol = GitProtocol.Git;
        } else if (eqIgnoreCase(scheme, "file")) {
            protocol = GitProtocol.File;
        } else {
            return ParsedGitUrl.invalid();
        }

        const after_scheme = s[(scheme_sep.? + 3)..s.len];
        if (protocol == GitProtocol.File) {
            path = after_scheme;
        } else {
            const first_slash = indexOfChar(after_scheme, '/');
            if (first_slash == null) {
                return ParsedGitUrl.invalid();
            }
            let authority = after_scheme[0..first_slash.?];
            path = after_scheme[(first_slash.? + 1)..after_scheme.len];

            const at_pos = indexOfChar(authority, '@');
            if (at_pos != null) {
                let userinfo = authority[0..at_pos.?];
                const pass_colon = indexOfChar(userinfo, ':');
                if (pass_colon != null) {
                    userinfo = userinfo[0..pass_colon.?];
                }
                user = userinfo;
                authority = authority[(at_pos.? + 1)..authority.len];
            }

            const port_colon = indexOfChar(authority, ':');
            if (port_colon != null) {
                host = authority[0..port_colon.?];
                const parsed_port = parsePort(authority[(port_colon.? + 1)..authority.len]);
                if (parsed_port == null) {
                    return ParsedGitUrl.invalid();
                }
                port = parsed_port.?;
            } else {
                host = authority;
            }
            if (host.len == 0) {
                return ParsedGitUrl.invalid();
            }
        }
    } else if (startsWithIgnoreCase(s, "github:")) {
        protocol = GitProtocol.Shorthand;
        host = "github.com";
        path = s[7..s.len];
    } else if (startsWithIgnoreCase(s, "gitlab:")) {
        protocol = GitProtocol.Shorthand;
        host = "gitlab.com";
        path = s[7..s.len];
    } else if (startsWithIgnoreCase(s, "bitbucket:")) {
        protocol = GitProtocol.Shorthand;
        host = "bitbucket.org";
        path = s[10..s.len];
    } else if (startsWithIgnoreCase(s, "gitee:")) {
        protocol = GitProtocol.Shorthand;
        host = "gitee.com";
        path = s[6..s.len];
    } else if (startsWithIgnoreCase(s, "codeberg:")) {
        protocol = GitProtocol.Shorthand;
        host = "codeberg.org";
        path = s[9..s.len];
    } else {
        // Check SCP-style SSH: `[user@]host:owner/repo.git`
        const colon_pos = indexOfChar(s, ':');
        const slash_pos = indexOfChar(s, '/');
        if (colon_pos != null && (slash_pos == null || colon_pos.? < slash_pos.?)) {
            protocol = GitProtocol.Ssh;
            let left = s[0..colon_pos.?];
            path = s[(colon_pos.? + 1)..s.len];
            const at_pos = indexOfChar(left, '@');
            if (at_pos != null) {
                user = left[0..at_pos.?];
                host = left[(at_pos.? + 1)..left.len];
            } else {
                host = left;
            }
            if (host.len == 0) {
                return ParsedGitUrl.invalid();
            }
        } else {
            // Bare GitHub shorthand: `owner/repo`
            if (slash_pos == null) {
                return ParsedGitUrl.invalid();
            }
            protocol = GitProtocol.Shorthand;
            host = "github.com";
            path = s;
        }
    }

    // 3. Clean path: strip query string and leading/trailing slashes
    const q_pos = indexOfChar(path, '?');
    if (q_pos != null) {
        path = path[0..q_pos.?];
    }
    let ps: usize = 0;
    let pe: usize = path.len;
    while (ps < pe && path[ps] == '/') {
        ps += 1;
    }
    while (pe > ps && path[pe - 1] == '/') {
        pe -= 1;
    }
    path = path[ps..pe];
    if (path.len == 0) {
        return ParsedGitUrl.invalid();
    }

    // 4. Extract optional browser `/tree/<ref>[/<subpath>]` or `/blob/<ref>[/<subpath>]`
    let repo_path = path;
    let marker_pos = indexOfSubstr(path, "/-/tree/");
    let marker_len: usize = 8;
    if (marker_pos == null) {
        marker_pos = indexOfSubstr(path, "/-/blob/");
        marker_len = 8;
    }
    if (marker_pos == null) {
        marker_pos = indexOfSubstr(path, "/tree/");
        marker_len = 6;
    }
    if (marker_pos == null) {
        marker_pos = indexOfSubstr(path, "/blob/");
        marker_len = 6;
    }
    if (marker_pos != null) {
        repo_path = path[0..marker_pos.?];
        const tail = path[(marker_pos.? + marker_len)..path.len];
        const sub_slash = indexOfChar(tail, '/');
        if (sub_slash != null) {
            if (ref_name.len == 0) {
                ref_name = tail[0..sub_slash.?];
            }
            subpath = tail[(sub_slash.? + 1)..tail.len];
        } else {
            if (ref_name.len == 0) {
                ref_name = tail;
            }
        }
    }

    // 5. Split `owner` and `repo` (supports nested GitLab groups like `group/sub/repo`)
    const last_slash = lastIndexOfChar(repo_path, '/');
    if (last_slash == null) {
        return ParsedGitUrl.invalid();
    }
    const owner = repo_path[0..last_slash.?];
    const raw_repo = repo_path[(last_slash.? + 1)..repo_path.len];
    const repo = stripDotGit(raw_repo);

    if (!isValidSegment(owner) || !isValidSegment(repo)) {
        return ParsedGitUrl.invalid();
    }
    // Reject relative path shorthands like `./foo` or `../foo`
    if (startsWithIgnoreCase(owner, "./") || startsWithIgnoreCase(owner, "../")) {
        return ParsedGitUrl.invalid();
    }

    return ParsedGitUrl{
        valid: true,
        protocol: protocol,
        host_kind: detectHostKind(host),
        user: user,
        host: host,
        port: port,
        owner: owner,
        repo: repo,
        ref_name: ref_name,
        subpath: subpath,
    };
}

// ---------------------------------------------------------------------------
// Public API: Normalizers (Zero Heap Allocation)
// ---------------------------------------------------------------------------

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendU16(out: []u8, pos: usize, val: u16) -> usize {
    if (val == 0) {
        return appendByte(out, pos, '0');
    }
    let digits: [8]u8 = undefined;
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        digits[len] = ('0' as u16 + (v % 10)) as u8;
        v /= 10;
        len += 1;
    }
    let p = pos;
    let k = len;
    while (k > 0) {
        k -= 1;
        p = appendByte(out, p, digits[k]);
    }
    return p;
}

/// Formats a normalized HTTPS clone URL: `https://<host>/<owner>/<repo>.git`.
pub fn toHttpsUrl(parsed: ParsedGitUrl, out: []u8) -> string {
    if (!parsed.valid || parsed.host.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = appendStr(out, pos, "https://");
    pos = appendStr(out, pos, parsed.host);
    if (parsed.port != 0 && parsed.port != 443 && parsed.port != 22) {
        pos = appendByte(out, pos, ':');
        pos = appendU16(out, pos, parsed.port);
    }
    pos = appendByte(out, pos, '/');
    pos = appendStr(out, pos, parsed.owner);
    pos = appendByte(out, pos, '/');
    pos = appendStr(out, pos, parsed.repo);
    pos = appendStr(out, pos, ".git");
    return out[0..pos];
}

/// Formats a normalized SSH clone URL:
/// - Default port: `<user>@<host>:<owner>/<repo>.git` (user defaults to `"git"`)
/// - Custom port: `ssh://<user>@<host>:<port>/<owner>/<repo>.git`
pub fn toSshUrl(parsed: ParsedGitUrl, out: []u8) -> string {
    if (!parsed.valid || parsed.host.len == 0) {
        return out[0..0];
    }
    const user = if (parsed.user.len > 0) parsed.user else "git";
    let pos: usize = 0;
    if (parsed.port != 0 && parsed.port != 22) {
        pos = appendStr(out, pos, "ssh://");
        pos = appendStr(out, pos, user);
        pos = appendByte(out, pos, '@');
        pos = appendStr(out, pos, parsed.host);
        pos = appendByte(out, pos, ':');
        pos = appendU16(out, pos, parsed.port);
        pos = appendByte(out, pos, '/');
        pos = appendStr(out, pos, parsed.owner);
        pos = appendByte(out, pos, '/');
        pos = appendStr(out, pos, parsed.repo);
        pos = appendStr(out, pos, ".git");
        return out[0..pos];
    }
    pos = appendStr(out, pos, user);
    pos = appendByte(out, pos, '@');
    pos = appendStr(out, pos, parsed.host);
    pos = appendByte(out, pos, ':');
    pos = appendStr(out, pos, parsed.owner);
    pos = appendByte(out, pos, '/');
    pos = appendStr(out, pos, parsed.repo);
    pos = appendStr(out, pos, ".git");
    return out[0..pos];
}

/// Formats a browser URL: `https://<host>/<owner>/<repo>[/tree/<ref>[/<subpath>]]`.
pub fn toBrowseUrl(parsed: ParsedGitUrl, out: []u8) -> string {
    if (!parsed.valid || parsed.host.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = appendStr(out, pos, "https://");
    pos = appendStr(out, pos, parsed.host);
    pos = appendByte(out, pos, '/');
    pos = appendStr(out, pos, parsed.owner);
    pos = appendByte(out, pos, '/');
    pos = appendStr(out, pos, parsed.repo);
    if (parsed.ref_name.len > 0) {
        pos = appendStr(out, pos, "/tree/");
        pos = appendStr(out, pos, parsed.ref_name);
        if (parsed.subpath.len > 0) {
            pos = appendByte(out, pos, '/');
            pos = appendStr(out, pos, parsed.subpath);
        }
    }
    return out[0..pos];
}
