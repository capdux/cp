pub const HOST_UNKNOWN: u8 = 0;
pub const HOST_GITHUB: u8 = 1;
pub const HOST_GITLAB: u8 = 2;
pub const HOST_BITBUCKET: u8 = 3;

pub struct HostedGitInfo {
    pub host_type: u8,
    pub domain: string,
    pub user: string,
    pub project: string,
    pub committish: string,
    pub valid: bool,

    pub static fn invalid(): HostedGitInfo {
        return HostedGitInfo {
            host_type: HOST_UNKNOWN,
            domain: "",
            user: "",
            project: "",
            committish: "",
            valid: false,
        };
    }

    pub fn formatHttps(out: []u8): usize {
        if (!_.valid) { return 0; }
        let p: usize = 0;
        p = hgiPush(out, p, "git+https://");
        p = hgiPush(out, p, _.domain);
        p = hgiPush(out, p, "/");
        p = hgiPush(out, p, _.user);
        p = hgiPush(out, p, "/");
        p = hgiPush(out, p, _.project);
        p = hgiPush(out, p, ".git");
        if (_.committish.len > 0) {
            p = hgiPush(out, p, "#");
            p = hgiPush(out, p, _.committish);
        }
        return p;
    }

    pub fn formatSsh(out: []u8): usize {
        if (!_.valid) { return 0; }
        let p: usize = 0;
        p = hgiPush(out, p, "git@");
        p = hgiPush(out, p, _.domain);
        p = hgiPush(out, p, ":");
        p = hgiPush(out, p, _.user);
        p = hgiPush(out, p, "/");
        p = hgiPush(out, p, _.project);
        p = hgiPush(out, p, ".git");
        if (_.committish.len > 0) {
            p = hgiPush(out, p, "#");
            p = hgiPush(out, p, _.committish);
        }
        return p;
    }

    pub fn formatBrowse(out: []u8): usize {
        if (!_.valid) { return 0; }
        let p: usize = 0;
        p = hgiPush(out, p, "https://");
        p = hgiPush(out, p, _.domain);
        p = hgiPush(out, p, "/");
        p = hgiPush(out, p, _.user);
        p = hgiPush(out, p, "/");
        p = hgiPush(out, p, _.project);
        if (_.committish.len > 0) {
            p = hgiPush(out, p, "/tree/");
            p = hgiPush(out, p, _.committish);
        }
        return p;
    }

    pub fn formatBugs(out: []u8): usize {
        if (!_.valid) { return 0; }
        let p: usize = 0;
        p = hgiPush(out, p, "https://");
        p = hgiPush(out, p, _.domain);
        p = hgiPush(out, p, "/");
        p = hgiPush(out, p, _.user);
        p = hgiPush(out, p, "/");
        p = hgiPush(out, p, _.project);
        p = hgiPush(out, p, "/issues");
        return p;
    }
}

fn hgiPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn hgiStartsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) { return false; }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn hgiEndsWith(s: string, suffix: string): bool {
    if (s.len < suffix.len) { return false; }
    let off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[off + i] != suffix[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn hgiFindChar(s: string, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) { return i; }
        i = i + 1;
    }
    return null;
}

fn hgiBuildFromPath(host_type: u8, domain: string, path_and_hash: string): HostedGitInfo {
    let hash_pos = hgiFindChar(path_and_hash, 35); // '#'
    let path_part = path_and_hash;
    let committish = "";
    if (hash_pos != null) {
        let hp = hash_pos ?? 0;
        path_part = path_and_hash[0..hp];
        committish = path_and_hash[(hp + 1)..path_and_hash.len];
    }
    if (path_part.len > 0 && path_part[0] == 47) {
        path_part = path_part[1..path_part.len];
    }
    let slash = hgiFindChar(path_part, 47); // '/'
    if (slash == null) { return HostedGitInfo.invalid(); }
    let sp = slash ?? 0;
    let user = path_part[0..sp];
    let rest = path_part[(sp + 1)..path_part.len];
    if (user.len == 0 || rest.len == 0) { return HostedGitInfo.invalid(); }
    // Reject extra path segments beyond user/project
    if (hgiFindChar(rest, 47) != null) { return HostedGitInfo.invalid(); }
    let proj = if (hgiEndsWith(rest, ".git")) { rest[0..(rest.len - 4)] } else { rest };
    if (proj.len == 0) { return HostedGitInfo.invalid(); }

    return HostedGitInfo {
        host_type: host_type,
        domain: domain,
        user: user,
        project: proj,
        committish: committish,
        valid: true,
    };
}

pub fn fromUrl(raw: string): HostedGitInfo {
    if (raw.len == 0) { return HostedGitInfo.invalid(); }

    // Strip optional git+ prefix
    let s = if (hgiStartsWith(raw, "git+")) { raw[4..raw.len] } else { raw };

    // Shorthand prefixes: github:, gitlab:, bitbucket:
    if (hgiStartsWith(s, "github:")) {
        return hgiBuildFromPath(HOST_GITHUB, "github.com", s[7..s.len]);
    }
    if (hgiStartsWith(s, "gitlab:")) {
        return hgiBuildFromPath(HOST_GITLAB, "gitlab.com", s[7..s.len]);
    }
    if (hgiStartsWith(s, "bitbucket:")) {
        return hgiBuildFromPath(HOST_BITBUCKET, "bitbucket.org", s[10..s.len]);
    }

    // SSH syntax: git@github.com:user/repo.git
    if (hgiStartsWith(s, "git@github.com:")) {
        return hgiBuildFromPath(HOST_GITHUB, "github.com", s[15..s.len]);
    }
    if (hgiStartsWith(s, "git@gitlab.com:")) {
        return hgiBuildFromPath(HOST_GITLAB, "gitlab.com", s[15..s.len]);
    }
    if (hgiStartsWith(s, "git@bitbucket.org:")) {
        return hgiBuildFromPath(HOST_BITBUCKET, "bitbucket.org", s[18..s.len]);
    }

    // HTTPS / HTTP / GIT URLs
    if (hgiStartsWith(s, "https://github.com/")) {
        return hgiBuildFromPath(HOST_GITHUB, "github.com", s[19..s.len]);
    }
    if (hgiStartsWith(s, "https://gitlab.com/")) {
        return hgiBuildFromPath(HOST_GITLAB, "gitlab.com", s[19..s.len]);
    }
    if (hgiStartsWith(s, "https://bitbucket.org/")) {
        return hgiBuildFromPath(HOST_BITBUCKET, "bitbucket.org", s[22..s.len]);
    }
    if (hgiStartsWith(s, "git://github.com/")) {
        return hgiBuildFromPath(HOST_GITHUB, "github.com", s[17..s.len]);
    }

    // Bare GitHub shorthand: user/repo(#branch) without ':'
    if (hgiFindChar(s, 58) == null && hgiFindChar(s, 47) != null) {
        return hgiBuildFromPath(HOST_GITHUB, "github.com", s);
    }

    return HostedGitInfo.invalid();
}
