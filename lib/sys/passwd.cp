// Source: Absorbed from /data/dev/bun/src/sys.zig (POSIX /etc/passwd & /etc/group Zero-Alloc Parser)
// Pure-CP zero-allocation POSIX /etc/passwd and /etc/group streaming parser and UID/GID/name lookups.

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn parseU32Strict(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        if (val > 429496729) return null;
        const d: u32 = @as(u32, c - '0');
        if (val == 429496729 && d > 5) return null;
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

fn nextColonField(line: []const u8, pos: *usize): ([]const u8)? {
    if (pos.* > line.len) return null;
    const start = pos.*;
    while (pos.* < line.len && line[pos.*] != ':') {
        pos.* += 1;
    }
    const seg = line[start..pos.*];
    if (pos.* < line.len && line[pos.*] == ':') {
        pos.* += 1;
    } else {
        pos.* = line.len + 1; // mark exhausted
    }
    return seg;
}

export struct PasswdEntry {
    username: []const u8,
    password: []const u8,
    uid: u32,
    gid: u32,
    gecos: []const u8,
    home_dir: []const u8,
    shell: []const u8,

    /// Parse a single 7-field colon-delimited `/etc/passwd` line.
    pub fn parseLine(raw: []const u8): PasswdEntry? {
        let end = raw.len;
        if (end > 0 && raw[end - 1] == '\r') end -= 1;
        const line = raw[0..end];
        if (line.len == 0 || line[0] == '#') return null;

        let pos: usize = 0;
        const f0 = nextColonField(line, &pos) ?? return null;
        const f1 = nextColonField(line, &pos) ?? return null;
        const f2 = nextColonField(line, &pos) ?? return null;
        const f3 = nextColonField(line, &pos) ?? return null;
        const f4 = nextColonField(line, &pos) ?? return null;
        const f5 = nextColonField(line, &pos) ?? return null;
        const f6 = nextColonField(line, &pos) ?? return null;
        // Must have exactly 7 fields
        if (pos <= line.len) return null;
        if (f0.len == 0) return null;

        const uid = parseU32Strict(f2) ?? return null;
        const gid = parseU32Strict(f3) ?? return null;
        return PasswdEntry {
            username: f0,
            password: f1,
            uid: uid,
            gid: gid,
            gecos: f4,
            home_dir: f5,
            shell: f6,
        };
    }

    pub fn isNologinShell(): bool {
        return strEq(_.shell, "/usr/sbin/nologin") ||
            strEq(_.shell, "/sbin/nologin") ||
            strEq(_.shell, "/bin/false") ||
            strEq(_.shell, "/usr/bin/false");
    }
}

export struct PasswdIter {
    src: []const u8,
    pos: usize,

    pub fn init(content: []const u8): PasswdIter {
        return PasswdIter { src: content, pos: 0 };
    }

    pub fn next(*_): PasswdEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') _.pos += 1;
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
            if (e := PasswdEntry.parseLine(_.src[start..end])) return e;
        }
        return null;
    }
}

export fn lookupUserByName(content: []const u8, username: []const u8): PasswdEntry? {
    let it = PasswdIter.init(content);
    while (e := it.next()) {
        if (strEq(e.username, username)) return e;
    }
    return null;
}

export fn lookupUserByUid(content: []const u8, uid: u32): PasswdEntry? {
    let it = PasswdIter.init(content);
    while (e := it.next()) {
        if (e.uid == uid) return e;
    }
    return null;
}

export struct GroupEntry {
    name: []const u8,
    password: []const u8,
    gid: u32,
    members_csv: []const u8,

    /// Parse a single 4-field colon-delimited `/etc/group` line.
    pub fn parseLine(raw: []const u8): GroupEntry? {
        let end = raw.len;
        if (end > 0 && raw[end - 1] == '\r') end -= 1;
        const line = raw[0..end];
        if (line.len == 0 || line[0] == '#') return null;

        let pos: usize = 0;
        const f0 = nextColonField(line, &pos) ?? return null;
        const f1 = nextColonField(line, &pos) ?? return null;
        const f2 = nextColonField(line, &pos) ?? return null;
        const f3 = nextColonField(line, &pos) ?? return null;
        if (pos <= line.len) return null;
        if (f0.len == 0) return null;

        const gid = parseU32Strict(f2) ?? return null;
        return GroupEntry {
            name: f0,
            password: f1,
            gid: gid,
            members_csv: f3,
        };
    }

    pub fn hasMember(username: []const u8): bool {
        if (username.len == 0 || _.members_csv.len == 0) return false;
        let pos: usize = 0;
        while (pos < _.members_csv.len) {
            const start = pos;
            while (pos < _.members_csv.len && _.members_csv[pos] != ',') pos += 1;
            const tok = _.members_csv[start..pos];
            if (pos < _.members_csv.len && _.members_csv[pos] == ',') pos += 1;
            if (strEq(tok, username)) return true;
        }
        return false;
    }
}

export struct GroupIter {
    src: []const u8,
    pos: usize,

    pub fn init(content: []const u8): GroupIter {
        return GroupIter { src: content, pos: 0 };
    }

    pub fn next(*_): GroupEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') _.pos += 1;
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
            if (g := GroupEntry.parseLine(_.src[start..end])) return g;
        }
        return null;
    }
}

export fn lookupGroupByName(content: []const u8, name: []const u8): GroupEntry? {
    let it = GroupIter.init(content);
    while (g := it.next()) {
        if (strEq(g.name, name)) return g;
    }
    return null;
}

export fn lookupGroupByGid(content: []const u8, gid: u32): GroupEntry? {
    let it = GroupIter.init(content);
    while (g := it.next()) {
        if (g.gid == gid) return g;
    }
    return null;
}
