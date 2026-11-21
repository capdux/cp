// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/status Zero-Alloc Parser)

const MAX_U64: u64 = 18446744073709551615;

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

// 解析 "40960 kB" / "16 MB" / "40960" -> kB
fn parseKbValue(s: []const u8): u64? {
    let i: usize = 0;
    while (i < s.len && isHWhitespace(s[i])) {
        i += 1;
    }
    const num_start = i;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        i += 1;
    }
    const base = parseDecU64(s[num_start..i]) ?? return null;

    while (i < s.len && isHWhitespace(s[i])) {
        i += 1;
    }
    const unit_start = i;
    while (i < s.len && !isHWhitespace(s[i])) {
        i += 1;
    }
    const unit = s[unit_start..i];
    if (unit.len == 0 || asciiEqlIgnoreCase(unit, "kb") || asciiEqlIgnoreCase(unit, "kib")) {
        return base;
    }
    if (asciiEqlIgnoreCase(unit, "mb") || asciiEqlIgnoreCase(unit, "mib")) {
        if (base > MAX_U64 / 1024) {
            return MAX_U64;
        }
        return base * 1024;
    }
    if (asciiEqlIgnoreCase(unit, "gb") || asciiEqlIgnoreCase(unit, "gib")) {
        if (base > MAX_U64 / 1048576) {
            return MAX_U64;
        }
        return base * 1048576;
    }
    return base;
}

fn parseTwoU32(s: []const u8, first_out: *u32, second_out: *u32): bool {
    let i: usize = 0;
    while (i < s.len && isHWhitespace(s[i])) {
        i += 1;
    }
    const t1_start = i;
    while (i < s.len && !isHWhitespace(s[i])) {
        i += 1;
    }
    const v1 = parseDecU32(s[t1_start..i]) ?? return false;

    while (i < s.len && isHWhitespace(s[i])) {
        i += 1;
    }
    const t2_start = i;
    while (i < s.len && !isHWhitespace(s[i])) {
        i += 1;
    }
    const v2 = parseDecU32(s[t2_start..i]) ?? return false;

    first_out.* = v1;
    second_out.* = v2;
    return true;
}

pub struct ProcStatus {
    name: []const u8;
    state_code: u8;
    pid: u32;
    ppid: u32;
    threads: u32;
    uid_real: u32;
    uid_effective: u32;
    gid_real: u32;
    gid_effective: u32;
    vm_peak_kb: u64;
    vm_size_kb: u64;
    vm_rss_kb: u64;
    vm_hwm_kb: u64;
    voluntary_ctxt_switches: u64;
    nonvoluntary_ctxt_switches: u64;

    fn isRunning(): bool {
        return _.state_code == 'R';
    }

    fn isZombie(): bool {
        return _.state_code == 'Z';
    }

    fn rssBytes(): u64 {
        if (_.vm_rss_kb > MAX_U64 / 1024) {
            return MAX_U64;
        }
        return _.vm_rss_kb * 1024;
    }
}

pub fn parseProcStatus(content: []const u8): ProcStatus? {
    let st = ProcStatus {
        name: "",
        state_code: 0,
        pid: 0,
        ppid: 0,
        threads: 0,
        uid_real: 0,
        uid_effective: 0,
        gid_real: 0,
        gid_effective: 0,
        vm_peak_kb: 0,
        vm_size_kb: 0,
        vm_rss_kb: 0,
        vm_hwm_kb: 0,
        voluntary_ctxt_switches: 0,
        nonvoluntary_ctxt_switches: 0,
    };

    let recognized_fields: usize = 0;
    let pos: usize = 0;

    while (pos < content.len) {
        const line_start = pos;
        while (pos < content.len && content[pos] != '\n' && content[pos] != '\r') {
            pos += 1;
        }
        const line_end = pos;
        if (pos < content.len) {
            if (content[pos] == '\r') {
                pos += 1;
                if (pos < content.len && content[pos] == '\n') {
                    pos += 1;
                }
            } else if (content[pos] == '\n') {
                pos += 1;
            }
        }

        let s = line_start;
        let e = line_end;
        while (s < e && isHWhitespace(content[s])) {
            s += 1;
        }
        while (e > s && isHWhitespace(content[e - 1])) {
            e -= 1;
        }
        if (s >= e) {
            continue;
        }

        let colon_idx = s;
        let found_colon = false;
        while (colon_idx < e) {
            if (content[colon_idx] == ':') {
                found_colon = true;
                break;
            }
            colon_idx += 1;
        }
        if (!found_colon) {
            continue;
        }

        let key_end = colon_idx;
        while (key_end > s && isHWhitespace(content[key_end - 1])) {
            key_end -= 1;
        }
        const key = content[s..key_end];

        let val_start = colon_idx + 1;
        while (val_start < e && isHWhitespace(content[val_start])) {
            val_start += 1;
        }
        const val = content[val_start..e];

        if (bytesEql(key, "Name")) {
            st.name = val;
            recognized_fields += 1;
        } else if (bytesEql(key, "State")) {
            if (val.len > 0) {
                st.state_code = val[0];
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "Pid")) {
            if (v := parseDecU32(val)) {
                st.pid = v;
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "PPid")) {
            if (v := parseDecU32(val)) {
                st.ppid = v;
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "Threads")) {
            if (v := parseDecU32(val)) {
                st.threads = v;
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "Uid")) {
            if (parseTwoU32(val, *st.uid_real, *st.uid_effective)) {
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "Gid")) {
            if (parseTwoU32(val, *st.gid_real, *st.gid_effective)) {
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "VmPeak")) {
            if (v := parseKbValue(val)) {
                st.vm_peak_kb = v;
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "VmSize")) {
            if (v := parseKbValue(val)) {
                st.vm_size_kb = v;
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "VmRSS")) {
            if (v := parseKbValue(val)) {
                st.vm_rss_kb = v;
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "VmHWM")) {
            if (v := parseKbValue(val)) {
                st.vm_hwm_kb = v;
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "voluntary_ctxt_switches")) {
            if (v := parseDecU64(val)) {
                st.voluntary_ctxt_switches = v;
                recognized_fields += 1;
            }
        } else if (bytesEql(key, "nonvoluntary_ctxt_switches")) {
            if (v := parseDecU64(val)) {
                st.nonvoluntary_ctxt_switches = v;
                recognized_fields += 1;
            }
        }
    }

    if (recognized_fields == 0) {
        return null;
    }
    return st;
}

pub fn findProcStatusField(content: []const u8, key: []const u8): ([]const u8)? {
    if (key.len == 0) {
        return null;
    }
    let pos: usize = 0;
    while (pos < content.len) {
        const line_start = pos;
        while (pos < content.len && content[pos] != '\n' && content[pos] != '\r') {
            pos += 1;
        }
        const line_end = pos;
        if (pos < content.len) {
            if (content[pos] == '\r') {
                pos += 1;
                if (pos < content.len && content[pos] == '\n') {
                    pos += 1;
                }
            } else if (content[pos] == '\n') {
                pos += 1;
            }
        }

        let s = line_start;
        let e = line_end;
        while (s < e && isHWhitespace(content[s])) {
            s += 1;
        }
        while (e > s && isHWhitespace(content[e - 1])) {
            e -= 1;
        }
        if (s >= e) {
            continue;
        }

        let colon_idx = s;
        let found_colon = false;
        while (colon_idx < e) {
            if (content[colon_idx] == ':') {
                found_colon = true;
                break;
            }
            colon_idx += 1;
        }
        if (!found_colon) {
            continue;
        }

        let key_end = colon_idx;
        while (key_end > s && isHWhitespace(content[key_end - 1])) {
            key_end -= 1;
        }
        if (bytesEql(content[s..key_end], key)) {
            let val_start = colon_idx + 1;
            while (val_start < e && isHWhitespace(content[val_start])) {
                val_start += 1;
            }
            return content[val_start..e];
        }
    }
    return null;
}
