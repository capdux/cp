// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:22:31Z (Linux /etc/group Parser)
// Pure-CP zero-allocation Linux /etc/group streaming parser and membership lookup utilities.

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
        pos.* = line.len + 1;
    }
    return seg;
}

fn isBlankOrComment(line: []const u8): bool {
    let i: usize = 0;
    while (i < line.len && (line[i] == ' ' || line[i] == '\t')) {
        i += 1;
    }
    return i == line.len || line[i] == '#';
}

/// Zero-allocation iterator over a comma-separated group member list.
pub struct GroupMemberIter {
    members: []const u8,
    pos: usize,

    static fn init(members: []const u8): GroupMemberIter {
        return GroupMemberIter {
            members: members,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.members.len) {
            const start = _.pos;
            while (_.pos < _.members.len && _.members[_.pos] != ',') {
                _.pos += 1;
            }
            const tok = _.members[start.._.pos];
            if (_.pos < _.members.len && _.members[_.pos] == ',') {
                _.pos += 1;
            }
            if (tok.len > 0) {
                return tok;
            }
        }
        return null;
    }
}

/// Represents a single parsed `/etc/group` entry: `name:passwd:gid:member1,member2`.
pub struct GroupEntry {
    name: []const u8,
    passwd: []const u8,
    gid: u32,
    members: []const u8,

    /// Parse a single 4-field colon-delimited `/etc/group` line.
    static fn parseLine(raw: []const u8): GroupEntry? {
        let end = raw.len;
        if (end > 0 && raw[end - 1] == '\r') {
            end -= 1;
        }
        const line = raw[0..end];
        if (isBlankOrComment(line)) return null;

        let pos: usize = 0;
        const f_name = nextColonField(line, &pos) ?? return null;
        const f_passwd = nextColonField(line, &pos) ?? return null;
        const f_gid = nextColonField(line, &pos) ?? return null;
        const f_members = nextColonField(line, &pos) ?? return null;

        // Must have exactly 4 colon-separated fields
        if (pos <= line.len) return null;
        if (f_name.len == 0) return null;

        const gid = parseU32Strict(f_gid) ?? return null;
        return GroupEntry {
            name: f_name,
            passwd: f_passwd,
            gid: gid,
            members: f_members,
        };
    }

    /// Returns an iterator over the comma-separated members of this group.
    fn memberIter(): GroupMemberIter {
        return GroupMemberIter.init(_.members);
    }

    /// Exact comma-delimited membership check (avoids substring false positives).
    fn hasMember(username: []const u8): bool {
        if (username.len == 0 || _.members.len == 0) return false;
        let it = GroupMemberIter.init(_.members);
        while (m := it.next()) {
            if (strEq(m, username)) return true;
        }
        return false;
    }

    /// Counts non-empty members in the comma-separated member list.
    fn memberCount(): usize {
        let count: usize = 0;
        let it = GroupMemberIter.init(_.members);
        while (m := it.next()) {
            if (m.len > 0) {
                count += 1;
            }
        }
        return count;
    }
}

/// Zero-allocation streaming iterator over `/etc/group` file contents.
pub struct EtcGroupIter {
    src: []const u8,
    pos: usize,

    static fn init(content: []const u8): EtcGroupIter {
        return EtcGroupIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): GroupEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            if (entry := GroupEntry.parseLine(_.src[start..end])) {
                return entry;
            }
        }
        return null;
    }
}

/// Lookup the first group entry matching `name`.
pub fn findGroupByName(content: []const u8, name: []const u8): GroupEntry? {
    if (name.len == 0) return null;
    let it = EtcGroupIter.init(content);
    while (entry := it.next()) {
        if (strEq(entry.name, name)) return entry;
    }
    return null;
}

/// Lookup the first group entry matching `gid`.
pub fn findGroupByGid(content: []const u8, gid: u32): GroupEntry? {
    let it = EtcGroupIter.init(content);
    while (entry := it.next()) {
        if (entry.gid == gid) return entry;
    }
    return null;
}

/// Returns true if `username` is listed as a member of `group_name` in `content`.
pub fn isUserInGroup(content: []const u8, group_name: []const u8, username: []const u8): bool {
    if (group_name.len == 0 || username.len == 0) return false;
    let it = EtcGroupIter.init(content);
    while (entry := it.next()) {
        if (strEq(entry.name, group_name) && entry.hasMember(username)) {
            return true;
        }
    }
    return false;
}
