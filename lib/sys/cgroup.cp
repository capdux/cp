// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:17:28Z (Linux cgroup v2 Resource Limits Parser)

const MAX_U64: u64 = 18446744073709551615;

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
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

fn parseDecU64(s: []const u8): u64? {
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
        const d: u64 = u64(c - '0');
        if (val > (MAX_U64 - d) / 10) {
            return null;
        }
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

pub struct CgroupLimit {
    is_unlimited: bool;
    value: u64;

    static fn unlimited(): CgroupLimit {
        return CgroupLimit {
            is_unlimited: true,
            value: 0,
        };
    }
}

pub fn parseCgroupMax(content: []const u8): CgroupLimit? {
    let s: usize = 0;
    let e = content.len;
    while (s < e && isWhitespace(content[s])) {
        s += 1;
    }
    while (e > s && isWhitespace(content[e - 1])) {
        e -= 1;
    }
    if (s >= e) {
        return null;
    }
    const tok = content[s..e];
    if (bytesEql(tok, "max")) {
        return CgroupLimit {
            is_unlimited: true,
            value: 0,
        };
    }
    const v = parseDecU64(tok) ?? return null;
    return CgroupLimit {
        is_unlimited: false,
        value: v,
    };
}

pub struct CpuMaxQuota {
    quota_us: u64;
    period_us: u64;
    is_unlimited: bool;

    fn effectiveCpusCeil(): u32 {
        if (_.is_unlimited || _.period_us == 0) {
            return 0;
        }
        if (_.quota_us == 0) {
            return 1;
        }
        const num: u128 = u128(_.quota_us) + u128(_.period_us) - 1;
        const ceil_val: u128 = num / u128(_.period_us);
        if (ceil_val < 1) {
            return 1;
        }
        if (ceil_val > 0xFFFFFFFF) {
            return 0xFFFFFFFF;
        }
        return ceil_val as u32;
    }

    fn effectiveMilliCpus(): u64 {
        if (_.is_unlimited || _.period_us == 0) {
            return 0;
        }
        const milli: u128 = (u128(_.quota_us) * 1000) / u128(_.period_us);
        if (milli > u128(MAX_U64)) {
            return MAX_U64;
        }
        return milli as u64;
    }
}

pub fn parseCpuMax(content: []const u8): CpuMaxQuota? {
    let i: usize = 0;

    while (i < content.len && isWhitespace(content[i])) {
        i += 1;
    }
    const q_start = i;
    while (i < content.len && !isWhitespace(content[i])) {
        i += 1;
    }
    const q_tok = content[q_start..i];
    if (q_tok.len == 0) {
        return null;
    }

    while (i < content.len && isWhitespace(content[i])) {
        i += 1;
    }
    const p_start = i;
    while (i < content.len && !isWhitespace(content[i])) {
        i += 1;
    }
    const p_tok = content[p_start..i];
    if (p_tok.len == 0) {
        return null;
    }

    while (i < content.len && isWhitespace(content[i])) {
        i += 1;
    }
    if (i < content.len) {
        // 拒绝多余第 3 个字段
        return null;
    }

    const period = parseDecU64(p_tok) ?? return null;
    if (period == 0) {
        return null;
    }

    if (bytesEql(q_tok, "max")) {
        return CpuMaxQuota {
            quota_us: 0,
            period_us: period,
            is_unlimited: true,
        };
    }

    const quota = parseDecU64(q_tok) ?? return null;
    return CpuMaxQuota {
        quota_us: quota,
        period_us: period,
        is_unlimited: false,
    };
}

pub struct IoMaxEntry {
    major: u32;
    minor: u32;
    rbps: CgroupLimit;
    wbps: CgroupLimit;
    riops: CgroupLimit;
    wiops: CgroupLimit;
}

pub fn parseIoMaxLine(line: []const u8): IoMaxEntry? {
    let i: usize = 0;
    while (i < line.len && isWhitespace(line[i])) {
        i += 1;
    }
    if (i >= line.len) {
        return null;
    }

    // 解析设备号 <major>:<minor>
    const dev_start = i;
    while (i < line.len && !isWhitespace(line[i])) {
        i += 1;
    }
    const dev_tok = line[dev_start..i];

    let colon_idx: usize = 0;
    let found_colon = false;
    while (colon_idx < dev_tok.len) {
        if (dev_tok[colon_idx] == ':') {
            found_colon = true;
            break;
        }
        colon_idx += 1;
    }
    if (!found_colon) {
        return null;
    }

    const major = parseDecU32(dev_tok[0..colon_idx]) ?? return null;
    const minor = parseDecU32(dev_tok[(colon_idx + 1)..dev_tok.len]) ?? return null;

    let entry = IoMaxEntry {
        major: major,
        minor: minor,
        rbps: CgroupLimit.unlimited(),
        wbps: CgroupLimit.unlimited(),
        riops: CgroupLimit.unlimited(),
        wiops: CgroupLimit.unlimited(),
    };

    while (i < line.len) {
        while (i < line.len && isWhitespace(line[i])) {
            i += 1;
        }
        if (i >= line.len) {
            break;
        }
        const tok_start = i;
        while (i < line.len && !isWhitespace(line[i])) {
            i += 1;
        }
        const tok = line[tok_start..i];

        let eq_idx: usize = 0;
        let found_eq = false;
        while (eq_idx < tok.len) {
            if (tok[eq_idx] == '=') {
                found_eq = true;
                break;
            }
            eq_idx += 1;
        }
        if (!found_eq) {
            return null;
        }

        const key = tok[0..eq_idx];
        const val_str = tok[(eq_idx + 1)..tok.len];
        const lim = parseCgroupMax(val_str) ?? return null;

        if (bytesEql(key, "rbps")) {
            entry.rbps = lim;
        } else if (bytesEql(key, "wbps")) {
            entry.wbps = lim;
        } else if (bytesEql(key, "riops")) {
            entry.riops = lim;
        } else if (bytesEql(key, "wiops")) {
            entry.wiops = lim;
        }
    }

    return entry;
}

pub fn effectiveMemoryLimit(cgroup_max: CgroupLimit, host_mem_bytes: u64): u64 {
    if (cgroup_max.is_unlimited || cgroup_max.value > host_mem_bytes) {
        return host_mem_bytes;
    }
    return cgroup_max.value;
}
