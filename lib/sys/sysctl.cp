// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux sysctl.conf Zero-Alloc Parser)

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isSysctlSep(c: u8): bool {
    return c == '.' || c == '/';
}

fn isValidSysctlKey(key: []const u8): bool {
    if (key.len == 0) {
        return false;
    }
    if (isSysctlSep(key[0]) || isSysctlSep(key[key.len - 1])) {
        return false;
    }
    let prev_sep = false;
    let i: usize = 0;
    while (i < key.len) {
        const c = key[i];
        if (c <= 0x20 || c == 0x7F) {
            return false;
        }
        if (isSysctlSep(c)) {
            if (prev_sep) {
                return false;
            }
            prev_sep = true;
        } else {
            prev_sep = false;
        }
        i += 1;
    }
    return true;
}

pub fn normalizeSysctlKey(key: []const u8, out: []u8): ([]const u8)? {
    if (!isValidSysctlKey(key) || key.len > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < key.len) {
        const c = key[i];
        if (c == '/') {
            out[i] = '.';
        } else {
            out[i] = c;
        }
        i += 1;
    }
    return out[0..key.len];
}

pub fn sysctlKeysEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len || !isValidSysctlKey(a) || !isValidSysctlKey(b)) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        const ca: u8 = if (a[i] == '/') '.' else a[i];
        const cb: u8 = if (b[i] == '/') '.' else b[i];
        if (ca != cb) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct SysctlEntry {
    key: []const u8;
    value: []const u8;
    ignore_errors: bool;
}

pub struct SysctlIter {
    src: []const u8;
    pos: usize;

    fn init(content: []const u8): SysctlIter {
        let start: usize = 0;
        if (content.len >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF) {
            start = 3;
        }
        return SysctlIter {
            src: content,
            pos: start,
        };
    }

    fn next(*_): SysctlEntry? {
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.src.len) {
                if (_.src[_.pos] == '\r') {
                    _.pos += 1;
                    if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                        _.pos += 1;
                    }
                } else if (_.src[_.pos] == '\n') {
                    _.pos += 1;
                }
            }

            let s = line_start;
            let e = line_end;
            while (s < e && isHWhitespace(_.src[s])) {
                s += 1;
            }
            while (e > s && isHWhitespace(_.src[e - 1])) {
                e -= 1;
            }
            if (s >= e || _.src[s] == '#' || _.src[s] == ';') {
                continue;
            }

            // 查找首个 '='
            let eq_idx = s;
            let found_eq = false;
            while (eq_idx < e) {
                if (_.src[eq_idx] == '=') {
                    found_eq = true;
                    break;
                }
                eq_idx += 1;
            }
            if (!found_eq) {
                continue;
            }

            let ignore_errors = false;
            let key_start = s;
            if (_.src[key_start] == '-') {
                ignore_errors = true;
                key_start += 1;
                while (key_start < eq_idx && isHWhitespace(_.src[key_start])) {
                    key_start += 1;
                }
            }

            let key_end = eq_idx;
            while (key_end > key_start && isHWhitespace(_.src[key_end - 1])) {
                key_end -= 1;
            }
            const key = _.src[key_start..key_end];
            if (!isValidSysctlKey(key)) {
                continue;
            }

            let val_start = eq_idx + 1;
            while (val_start < e && isHWhitespace(_.src[val_start])) {
                val_start += 1;
            }

            return SysctlEntry {
                key: key,
                value: _.src[val_start..e],
                ignore_errors: ignore_errors,
            };
        }
        return null;
    }
}

pub fn getSysctlValue(content: []const u8, key: []const u8): ([]const u8)? {
    if (!isValidSysctlKey(key)) {
        return null;
    }
    let it = SysctlIter.init(content);
    let result: ([]const u8)? = null;
    while (entry := it.next()) {
        if (sysctlKeysEqual(entry.key, key)) {
            result = entry.value;
        }
    }
    return result;
}
