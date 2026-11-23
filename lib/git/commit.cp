// Source: Absorbed from /data/dev/bun/src/cli.zig | Synced: 2026-11-21T15:06:30Z (Conventional Commits 1.0.0 Parser & Bump Advisor)
// Pure-CP zero-allocation Conventional Commits 1.0.0 parser, breaking-change detector, and SemVer bump advisor.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn isTypeChar(b: u8): bool {
    return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '-' || b == '_';
}

export enum ReleaseBump {
    none,
    patch,
    minor,
    major,
}

export struct ConventionalCommit {
    commit_type: []const u8,
    scope: ([]const u8)?,
    is_breaking: bool,
    description: []const u8,
    body_and_footers: []const u8,

    /// Parse a Conventional Commits 1.0.0 message (`type[(scope)][!]: description[\n\nbody/footers]`).
    pub static fn parse(msg: []const u8): ConventionalCommit? {
        // Split first line (header) and remainder
        let nl: usize = 0;
        while (nl < msg.len && msg[nl] != '\n') nl += 1;
        let hdr_end = nl;
        if (hdr_end > 0 && msg[hdr_end - 1] == '\r') hdr_end -= 1;
        const header = msg[0..hdr_end];
        const rest = if (nl < msg.len) msg[nl + 1..] else "";

        if (header.len == 0) return null;

        // Parse commit_type
        let pos: usize = 0;
        while (pos < header.len && isTypeChar(header[pos])) {
            pos += 1;
        }
        if (pos == 0) return null;
        const ctype = header[0..pos];

        // Optional (scope)
        let scope: ([]const u8)? = null;
        if (pos < header.len && header[pos] == '(') {
            pos += 1;
            const sstart = pos;
            while (pos < header.len && header[pos] != ')') {
                pos += 1;
            }
            if (pos >= header.len || pos == sstart) return null;
            scope = header[sstart..pos];
            pos += 1; // skip ')'
        }

        // Optional '!' breaking marker
        let is_breaking = false;
        if (pos < header.len && header[pos] == '!') {
            is_breaking = true;
            pos += 1;
        }

        // Must be followed by ": "
        if (pos + 1 >= header.len || header[pos] != ':' || header[pos + 1] != ' ') {
            return null;
        }
        pos += 2;
        const desc = trimWs(header[pos..]);
        if (desc.len == 0) return null;

        // Scan body/footers for `BREAKING CHANGE:` or `BREAKING-CHANGE:` at start of any line
        if (!is_breaking && rest.len > 0) {
            let li: usize = 0;
            while (li < rest.len) {
                const lstart = li;
                while (li < rest.len && rest[li] != '\n') li += 1;
                let lend = li;
                if (li < rest.len && rest[li] == '\n') li += 1;
                if (lend > lstart && rest[lend - 1] == '\r') lend -= 1;
                const line = rest[lstart..lend];
                if (startsWith(line, "BREAKING CHANGE:") || startsWith(line, "BREAKING-CHANGE:") ||
                    startsWith(line, "BREAKING CHANGE #") || startsWith(line, "BREAKING-CHANGE #"))
                {
                    is_breaking = true;
                    break;
                }
            }
        }

        return ConventionalCommit {
            commit_type: ctype,
            scope: scope,
            is_breaking: is_breaking,
            description: desc,
            body_and_footers: rest,
        };
    }

    /// Recommend SemVer bump level per Conventional Commits 1.0.0:
    /// breaking -> major; feat -> minor; fix/perf/revert -> patch; others -> none.
    pub fn recommendedBump(): ReleaseBump {
        if (_.is_breaking) return ReleaseBump.major;
        if (eqIgnoreCase(_.commit_type, "feat")) return ReleaseBump.minor;
        if (eqIgnoreCase(_.commit_type, "fix") || eqIgnoreCase(_.commit_type, "perf") || eqIgnoreCase(_.commit_type, "revert")) {
            return ReleaseBump.patch;
        }
        return ReleaseBump.none;
    }

    pub fn isFeature(): bool {
        return eqIgnoreCase(_.commit_type, "feat");
    }

    pub fn isFix(): bool {
        return eqIgnoreCase(_.commit_type, "fix");
    }
}
