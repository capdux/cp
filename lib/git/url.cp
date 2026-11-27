// Source: Absorbed from /data/dev/bun/src/install/repository.cp | Synced: 2026-11-23T20:27:02Z (Git Remote URL Parser & Formatter)

/// Git 远程传输协议枚举
pub enum GitProtocol: u8 {
    ssh = 0,
    scp = 1,
    https = 2,
    http = 3,
    git = 4,
    file = 5,

    fn asStr(): string {
        return switch (_) {
            .ssh => "ssh",
            .scp => "scp",
            .https => "https",
            .http => "http",
            .git => "git",
            .file => "file",
        };
    }
}

type Vec16u8 = @Vector(16, u8);

fn makeGitLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

fn makeGitWsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 1;
    lut['\t'] = 1;
    lut['\n'] = 1;
    lut['\r'] = 1;
    return lut;
}

const GIT_LOWER_LUT: [u8; 256] = makeGitLowerLut();
const GIT_WS_LUT: [u8; 256] = makeGitWsLut();

fn isSpace(c: u8): bool {
    return GIT_WS_LUT[@as(usize, c)] != 0;
}

fn trimSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isSpace(s[start])) { start += 1; }
    while (end > start && isSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn hasWhitespace(s: []const u8): bool {
    let i: usize = 0;
    const v_sp: Vec16u8 = @splat(0x20);
    const v_del: Vec16u8 = @splat(0x7F);
    while (i + 16 <= s.len) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((v <= v_sp) | (v == v_del));
        if (mask != 0) { return true; }
        i += 16;
    }
    while (i < s.len) {
        if (s[i] <= 0x20 || s[i] == 0x7F) { return true; }
        i += 1;
    }
    return false;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    const v_A: Vec16u8 = @splat('A');
    const v_Z: Vec16u8 = @splat('Z');
    const v_32: Vec16u8 = @splat(32);
    while (i + 16 <= a.len) {
        const va: Vec16u8 = a[i .. i + 16][0..16].*;
        const vb: Vec16u8 = b[i .. i + 16][0..16].*;
        const la = @select(u8, (va >= v_A) & (va <= v_Z), va + v_32, va);
        const lb = @select(u8, (vb >= v_A) & (vb <= v_Z), vb + v_32, vb);
        const mask: u16 = @bitCast(la == lb);
        if (mask != 0xFFFF) { return false; }
        i += 16;
    }
    while (i < a.len) {
        if (GIT_LOWER_LUT[@as(usize, a[i])] != GIT_LOWER_LUT[@as(usize, b[i])]) { return false; }
        i += 1;
    }
    return true;
}

fn findByteSimd(s: []const u8, start: usize, ch: u8): usize {
    let i = start;
    const v_ch: Vec16u8 = @splat(ch);
    while (i + 16 <= s.len) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(v == v_ch);
        if (mask != 0) {
            return i + @as(usize, @ctz(mask));
        }
        i += 16;
    }
    while (i < s.len && s[i] != ch) {
        i += 1;
    }
    return i;
}

fn findLastByteSimd(s: []const u8, ch: u8): usize? {
    let i = s.len;
    const v_ch: Vec16u8 = @splat(ch);
    while (i >= 16) {
        i -= 16;
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(v == v_ch);
        if (mask != 0) {
            return i + (15 - @as(usize, @clz(mask)));
        }
    }
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) { return i; }
    }
    return null;
}

fn findQueryOrHashSimd(s: []const u8): usize {
    let i: usize = 0;
    const v_q: Vec16u8 = @splat('?');
    const v_h: Vec16u8 = @splat('#');
    while (i + 16 <= s.len) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((v == v_q) | (v == v_h));
        if (mask != 0) {
            return i + @as(usize, @ctz(mask));
        }
        i += 16;
    }
    while (i < s.len && s[i] != '?' && s[i] != '#') {
        i += 1;
    }
    return i;
}

fn startsWithIgnoreCaseAscii(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    return eqlIgnoreCaseAscii(s[0..prefix.len], prefix);
}

fn endsWithIgnoreCaseAscii(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) { return false; }
    return eqlIgnoreCaseAscii(s[(s.len - suffix.len)..s.len], suffix);
}

fn parsePortU16(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5 || s[0] == '0') { return null; }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') { return null; }
        acc = acc * 10 + @as(u32, c - '0');
        i += 1;
    }
    if (acc == 0 || acc > 65535) { return null; }
    return @as(u16, @intCast(acc));
}

fn findSchemeSep(s: []const u8): usize? {
    let i: usize = 0;
    while (i + 2 < s.len) {
        const pos = findByteSimd(s, i, ':');
        if (pos + 2 >= s.len) { return null; }
        if (s[pos + 1] == '/' && s[pos + 2] == '/') {
            return pos;
        }
        i = pos + 1;
    }
    return null;
}

struct HostPort {
    host: []const u8,
    port: u16?,
}

fn parseHostPort(hp: []const u8): HostPort? {
    if (hp.len == 0) { return null; }
    if (hp[0] == '[') {
        const close_pos = findByteSimd(hp, 1, ']');
        if (close_pos >= hp.len) { return null; }
        if (close_pos <= 1) { return null; }
        const h = hp[0..(close_pos + 1)];
        if (close_pos + 1 == hp.len) {
            return HostPort{ host: h, port: null };
        }
        if (hp[close_pos + 1] != ':') { return null; }
        const p = parsePortU16(hp[(close_pos + 2)..hp.len]) ?? return null;
        return HostPort{ host: h, port: p };
    }

    const first_colon = findByteSimd(hp, 0, ':');
    if (first_colon < hp.len) {
        if (findByteSimd(hp, first_colon + 1, ':') < hp.len) { return null; }
        const h = hp[0..first_colon];
        if (h.len == 0) { return null; }
        const p = parsePortU16(hp[(first_colon + 1)..hp.len]) ?? return null;
        return HostPort{ host: h, port: p };
    }
    return HostPort{ host: hp, port: null };
}

fn normalizeRepoPath(raw_path: []const u8): []const u8 {
    // 剥离查询串/锚点
    let end = findQueryOrHashSimd(raw_path);
    let start: usize = 0;
    while (start < end && raw_path[start] == '/') { start += 1; }
    while (end > start && raw_path[end - 1] == '/') { end -= 1; }
    let s = raw_path[start..end];
    if (endsWithIgnoreCaseAscii(s, ".git")) {
        s = s[0..(s.len - 4)];
    }
    while (s.len > 0 && s[s.len - 1] == '/') {
        s = s[0..(s.len - 1)];
    }
    return s;
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        if (s.len == 0) { return; }
        if (_.pos + s.len <= _.buf.len) {
            @memcpy(_.buf[_.pos .. _.pos + s.len], s);
            _.pos += s.len;
        } else {
            const avail = _.buf.len - _.pos;
            if (avail > 0) {
                @memcpy(_.buf[_.pos .. _.buf.len], s[0..avail]);
                _.pos = _.buf.len;
            }
            _.overflow = true;
        }
    }

    fn writeU16(*_, val: u16) {
        let digits: [8]u8 = [0; 8];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 8) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 零堆分配 Git 远程地址解析结果
pub struct GitRemoteUrl {
    protocol: GitProtocol = .https,
    user: ([]const u8)? = null,
    host: ([]const u8)? = null,
    port: u16? = null,
    path: []const u8 = "",
    owner: ([]const u8)? = null,
    repo: []const u8 = "",

    static fn buildWithPath(
        proto: GitProtocol,
        user: ([]const u8)?,
        host: ([]const u8)?,
        port: u16?,
        raw_path: []const u8,
    ): GitRemoteUrl? {
        const norm = normalizeRepoPath(raw_path);
        if (norm.len == 0) { return null; }

        const last_slash = findLastByteSimd(norm, '/');

        let owner: ([]const u8)? = null;
        let repo = norm;
        if (ls := last_slash) {
            owner = norm[0..ls];
            repo = norm[(ls + 1)..norm.len];
        }
        if (repo.len == 0) { return null; }

        return GitRemoteUrl{
            protocol: proto,
            user: user,
            host: host,
            port: port,
            path: norm,
            owner: owner,
            repo: repo,
        };
    }

    /// 解析 Git 远程地址（支持 SCP `git@host:org/repo.git`、`ssh://`、`https://`、`http://`、`git://`、`file://`）
    static fn parse(raw: []const u8): GitRemoteUrl? {
        const s = trimSpace(raw);
        if (s.len == 0 || hasWhitespace(s)) { return null; }

        if (scheme_end := findSchemeSep(s)) {
            const scheme = s[0..scheme_end];
            const after = s[(scheme_end + 3)..s.len];

            let proto: GitProtocol = .https;
            if (eqlIgnoreCaseAscii(scheme, "https")) {
                proto = .https;
            } else if (eqlIgnoreCaseAscii(scheme, "http")) {
                proto = .http;
            } else if (eqlIgnoreCaseAscii(scheme, "ssh")) {
                proto = .ssh;
            } else if (eqlIgnoreCaseAscii(scheme, "git")) {
                proto = .git;
            } else if (eqlIgnoreCaseAscii(scheme, "file")) {
                proto = .file;
            } else {
                return null;
            }

            if (proto == .file) {
                if (after.len == 0) { return null; }
                if (after[0] == '/') {
                    return GitRemoteUrl.buildWithPath(.file, null, null, null, after);
                }
                let sl: usize? = null;
                let k: usize = 0;
                while (k < after.len) {
                    if (after[k] == '/') { sl = k; break; }
                    k += 1;
                }
                const slash = sl ?? return null;
                const h = after[0..slash];
                const h_opt: ([]const u8)? = if (h.len > 0) h else null;
                return GitRemoteUrl.buildWithPath(.file, null, h_opt, null, after[slash..after.len]);
            }

            // 拆分 authority 与 path
            let slash_idx: usize? = null;
            let i: usize = 0;
            while (i < after.len) {
                if (after[i] == '/') {
                    slash_idx = i;
                    break;
                }
                i += 1;
            }
            const slash = slash_idx ?? return null;
            const authority = after[0..slash];
            const raw_path = after[slash..after.len];
            if (authority.len == 0) { return null; }

            let user: ([]const u8)? = null;
            let hostport = authority;
            let at_idx: usize? = null;
            let k: usize = 0;
            while (k < authority.len) {
                if (authority[k] == '@') { at_idx = k; }
                k += 1;
            }
            if (at := at_idx) {
                if (at == 0 || at + 1 >= authority.len) { return null; }
                user = authority[0..at];
                hostport = authority[(at + 1)..authority.len];
            }

            const hp = parseHostPort(hostport) ?? return null;
            return GitRemoteUrl.buildWithPath(proto, user, hp.host, hp.port, raw_path);
        }

        // SCP 简写: [user@]host:path
        let colon_idx: usize? = null;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '/') { break; }
            if (s[i] == ':') {
                colon_idx = i;
                break;
            }
            i += 1;
        }
        const colon = colon_idx ?? return null;
        if (colon == 0 || colon + 1 >= s.len) { return null; }

        const left = s[0..colon];
        const raw_path = s[(colon + 1)..s.len];

        let user: ([]const u8)? = null;
        let host = left;
        let at_idx: usize? = null;
        let k: usize = 0;
        while (k < left.len) {
            if (left[k] == '@') {
                if (at_idx != null) { return null; }
                at_idx = k;
            }
            k += 1;
        }
        if (at := at_idx) {
            if (at == 0 || at + 1 >= left.len) { return null; }
            user = left[0..at];
            host = left[(at + 1)..left.len];
        }
        if (host.len == 0) { return null; }

        return GitRemoteUrl.buildWithPath(.scp, user, host, null, raw_path);
    }

    /// 规范化转换为 HTTPS 远程地址（如 `https://github.com/org/repo.git`）
    fn formatHttps(out: []u8): ([]const u8)? {
        const h = _.host ?? return null;
        let w = OutWriter{ buf: out };
        w.writeStr("https://");
        w.writeStr(h);
        if (_.protocol == .https || _.protocol == .http) {
            if (p := _.port) {
                if (p != 443) {
                    w.writeByte(':');
                    w.writeU16(p);
                }
            }
        }
        w.writeByte('/');
        w.writeStr(_.path);
        w.writeStr(".git");
        return w.finish();
    }

    /// 规范化转换为 SCP 简写远程地址（如 `git@github.com:org/repo.git`）
    fn formatScp(out: []u8): ([]const u8)? {
        const h = _.host ?? return null;
        let w = OutWriter{ buf: out };
        let u: []const u8 = "git";
        if (_.protocol == .ssh || _.protocol == .scp) {
            if (existing_u := _.user) {
                u = existing_u;
            }
        }
        w.writeStr(u);
        w.writeByte('@');
        w.writeStr(h);
        w.writeByte(':');
        w.writeStr(_.path);
        w.writeStr(".git");
        return w.finish();
    }
}
