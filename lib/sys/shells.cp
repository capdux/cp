// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:25:50Z (Linux /etc/shells Parser)
// Pure-CP zero-allocation Linux /etc/shells parser, validator, and lookup utilities.

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWs(s[start])) {
        start += 1;
    }
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn pathBasename(path: []const u8): []const u8 {
    let i = path.len;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/') {
            return path[i + 1..path.len];
        }
    }
    return path;
}

/// Validates and extracts a single `/etc/shells` line.
/// Rejects blank lines, `#` comments, non-absolute paths, trailing `/`, and paths containing whitespace or control bytes.
pub fn parseShellLine(raw: []const u8): ([]const u8)? {
    let end = raw.len;
    if (end > 0 && raw[end - 1] == '\r') {
        end -= 1;
    }
    const line = trimWs(raw[0..end]);
    if (line.len == 0 || line[0] == '#') return null;

    // Must be an absolute executable path (starts with `/`, not root `/` or directory ending in `/`)
    if (line[0] != '/' || line.len < 2 || line[line.len - 1] == '/') {
        return null;
    }

    // Reject internal whitespace (0x20, 0x09), control characters (< 0x20), and DEL (0x7f)
    let i: usize = 0;
    while (i < line.len) {
        const c = line[i];
        if (c <= 0x20 || c == 0x7f) {
            return null;
        }
        i += 1;
    }
    return line;
}

/// Zero-allocation streaming iterator over valid login shell paths in `/etc/shells`.
pub struct EtcShellsIter {
    src: []const u8,
    pos: usize,

    fn init(content: []const u8): EtcShellsIter {
        return EtcShellsIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            if (shell := parseShellLine(_.src[start..end])) {
                return shell;
            }
        }
        return null;
    }
}

/// Returns true if `shell_path` exactly matches a valid entry in `/etc/shells` content.
pub fn isAllowedShell(content: []const u8, shell_path: []const u8): bool {
    if (shell_path.len == 0) return false;
    let it = EtcShellsIter.init(content);
    while (entry := it.next()) {
        if (strEq(entry, shell_path)) return true;
    }
    return false;
}

/// Finds the first shell path in `/etc/shells` whose basename matches `basename` (e.g. `"bash"` -> `"/bin/bash"`).
pub fn findShellByBasename(content: []const u8, basename: []const u8): ([]const u8)? {
    if (basename.len == 0) return null;
    let i: usize = 0;
    while (i < basename.len) {
        if (basename[i] == '/') return null;
        i += 1;
    }

    let it = EtcShellsIter.init(content);
    while (entry := it.next()) {
        if (strEq(pathBasename(entry), basename)) {
            return entry;
        }
    }
    return null;
}

/// Counts the total number of valid shell entries in `/etc/shells` content.
pub fn countShells(content: []const u8): usize {
    let count: usize = 0;
    let it = EtcShellsIter.init(content);
    while (entry := it.next()) {
        if (entry.len > 0) {
            count += 1;
        }
    }
    return count;
}

/// Returns true if `shell_path` is a standard non-interactive / nologin shell.
pub fn isNologinShell(shell_path: []const u8): bool {
    return strEq(shell_path, "/sbin/nologin") ||
        strEq(shell_path, "/usr/sbin/nologin") ||
        strEq(shell_path, "/bin/false") ||
        strEq(shell_path, "/usr/bin/false");
}
