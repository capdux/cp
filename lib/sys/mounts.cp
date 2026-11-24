// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:12:14Z (Linux /proc/mounts Zero-Alloc Parser)

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseDecU32(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        val = val * 10 + u64(c - '0');
        if (val > 0xFFFFFFFF) {
            return null;
        }
        i += 1;
    }
    return val as u32;
}

fn isOctalDigit(c: u8): bool {
    return c >= '0' && c <= '7';
}

pub fn unescapeMountPath(raw: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let out_len: usize = 0;

    while (i < raw.len) {
        if (raw[i] == '\\' && i + 3 < raw.len
            && raw[i + 1] >= '0' && raw[i + 1] <= '3'
            && isOctalDigit(raw[i + 2])
            && isOctalDigit(raw[i + 3]))
        {
            const d0: u8 = raw[i + 1] - '0';
            const d1: u8 = raw[i + 2] - '0';
            const d2: u8 = raw[i + 3] - '0';
            const decoded: u8 = (d0 << 6) | (d1 << 3) | d2;
            if (out_len >= out.len) {
                return null;
            }
            out[out_len] = decoded;
            out_len += 1;
            i += 4;
            continue;
        }
        if (out_len >= out.len) {
            return null;
        }
        out[out_len] = raw[i];
        out_len += 1;
        i += 1;
    }

    return out[0..out_len];
}

pub struct MountEntry {
    source: []const u8;
    target: []const u8;
    fs_type: []const u8;
    options: []const u8;
    dump_freq: u32;
    pass_num: u32;

    fn hasOption(key: []const u8): bool {
        if (key.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.options.len) {
            const start = i;
            while (i < _.options.len && _.options[i] != ',') {
                i += 1;
            }
            const end = i;
            if (i < _.options.len && _.options[i] == ',') {
                i += 1;
            }
            const opt_len = end - start;
            if (opt_len == key.len && bytesEql(_.options[start..end], key)) {
                return true;
            }
            if (opt_len > key.len && _.options[start + key.len] == '=' && bytesEql(_.options[start..(start + key.len)], key)) {
                return true;
            }
        }
        return false;
    }

    fn getOptionValue(key: []const u8): ([]const u8)? {
        if (key.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.options.len) {
            const start = i;
            while (i < _.options.len && _.options[i] != ',') {
                i += 1;
            }
            const end = i;
            if (i < _.options.len && _.options[i] == ',') {
                i += 1;
            }
            const opt_len = end - start;
            if (opt_len > key.len && _.options[start + key.len] == '=' && bytesEql(_.options[start..(start + key.len)], key)) {
                return _.options[(start + key.len + 1)..end];
            }
        }
        return null;
    }

    fn isReadOnly(): bool {
        let ro = false;
        let i: usize = 0;
        while (i < _.options.len) {
            const start = i;
            while (i < _.options.len && _.options[i] != ',') {
                i += 1;
            }
            const end = i;
            if (i < _.options.len && _.options[i] == ',') {
                i += 1;
            }
            if (bytesEql(_.options[start..end], "ro")) {
                ro = true;
            } else if (bytesEql(_.options[start..end], "rw")) {
                ro = false;
            }
        }
        return ro;
    }
}

pub struct ProcMountsIter {
    src: []const u8;
    pos: usize;

    static fn init(content: []const u8): ProcMountsIter {
        return ProcMountsIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): MountEntry? {
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

            let p = line_start;
            while (p < line_end && isHWhitespace(_.src[p])) {
                p += 1;
            }
            if (p >= line_end || _.src[p] == '#') {
                continue;
            }

            let col_start: [6]usize = [0; 6];
            let col_end: [6]usize = [0; 6];
            let count: usize = 0;

            while (p < line_end && count < 6) {
                while (p < line_end && isHWhitespace(_.src[p])) {
                    p += 1;
                }
                if (p >= line_end) {
                    break;
                }
                col_start[count] = p;
                while (p < line_end && !isHWhitespace(_.src[p])) {
                    p += 1;
                }
                col_end[count] = p;
                count += 1;
            }

            if (count < 6) {
                continue;
            }

            const dump_freq = parseDecU32(_.src[col_start[4]..col_end[4]]) ?? continue;
            const pass_num = parseDecU32(_.src[col_start[5]..col_end[5]]) ?? continue;

            return MountEntry {
                source: _.src[col_start[0]..col_end[0]],
                target: _.src[col_start[1]..col_end[1]],
                fs_type: _.src[col_start[2]..col_end[2]],
                options: _.src[col_start[3]..col_end[3]],
                dump_freq: dump_freq,
                pass_num: pass_num,
            };
        }
        return null;
    }
}

fn matchMountPrefixLen(clean_path: []const u8, target: []const u8): usize {
    if (target.len == 0 || target[0] != '/') {
        return 0;
    }
    let t_end = target.len;
    while (t_end > 1 && target[t_end - 1] == '/') {
        t_end -= 1;
    }
    const clean_target = target[0..t_end];

    if (clean_target.len == 1 && clean_target[0] == '/') {
        return 1;
    }
    if (clean_path.len < clean_target.len) {
        return 0;
    }
    if (!bytesEql(clean_path[0..clean_target.len], clean_target)) {
        return 0;
    }
    if (clean_path.len == clean_target.len || clean_path[clean_target.len] == '/') {
        return clean_target.len;
    }
    return 0;
}

pub fn findMountForPath(content: []const u8, path: []const u8): MountEntry? {
    if (path.len == 0 || path[0] != '/') {
        return null;
    }
    let p_end = path.len;
    while (p_end > 1 && path[p_end - 1] == '/') {
        p_end -= 1;
    }
    const clean_path = path[0..p_end];

    let it = ProcMountsIter.init(content);
    let best: MountEntry? = null;
    let best_len: usize = 0;
    let unesc_buf: [512]u8 = [0; 512];

    while (entry := it.next()) {
        let target_view = entry.target;
        if (decoded := unescapeMountPath(entry.target, &unesc_buf)) {
            target_view = decoded;
        }
        const mlen = matchMountPrefixLen(clean_path, target_view);
        if (mlen > 0 && mlen >= best_len) {
            best = entry;
            best_len = mlen;
        }
    }

    return best;
}
