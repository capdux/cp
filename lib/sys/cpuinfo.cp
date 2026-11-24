// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:10:25Z (Linux /proc/cpuinfo Zero-Alloc Parser)

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

// 解析 "2899.998" -> 2899998，"2400" -> 2400000（零浮点误差）
fn parseMhzX1000(s: []const u8): u64 {
    if (s.len == 0) {
        return 0;
    }
    let i: usize = 0;
    let int_part: u64 = 0;
    let saw_digit = false;

    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        saw_digit = true;
        const d: u64 = u64(s[i] - '0');
        if (int_part > (MAX_U64 - d) / 10) {
            return 0;
        }
        int_part = int_part * 10 + d;
        i += 1;
    }
    if (!saw_digit) {
        return 0;
    }
    if (int_part > MAX_U64 / 1000) {
        return 0;
    }

    let frac_part: u64 = 0;
    let frac_digits: usize = 0;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            if (frac_digits < 3) {
                frac_part = frac_part * 10 + u64(s[i] - '0');
                frac_digits += 1;
            }
            i += 1;
        }
    }
    while (frac_digits < 3) {
        frac_part *= 10;
        frac_digits += 1;
    }

    return int_part * 1000 + frac_part;
}

// 解析 "55296 KB" / "16 MB" / "1024" -> KB 数值
fn parseCacheKb(s: []const u8): u64 {
    let i: usize = 0;
    while (i < s.len && isHWhitespace(s[i])) {
        i += 1;
    }
    const num_start = i;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        i += 1;
    }
    const base = parseDecU64(s[num_start..i]) ?? return 0;

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

pub fn hasCpuFlag(flags: []const u8, flag: []const u8): bool {
    if (flag.len == 0 || flags.len < flag.len) {
        return false;
    }
    let i: usize = 0;
    while (i < flags.len) {
        while (i < flags.len && isHWhitespace(flags[i])) {
            i += 1;
        }
        if (i >= flags.len) {
            break;
        }
        const tok_start = i;
        while (i < flags.len && !isHWhitespace(flags[i])) {
            i += 1;
        }
        if (bytesEql(flags[tok_start..i], flag)) {
            return true;
        }
    }
    return false;
}

pub struct CpuBlock {
    processor: u32;
    physical_id: u32;
    cpu_cores: u32;
    model_name: []const u8;
    vendor_id: []const u8;
    cpu_mhz_x1000: u64;
    cache_kb: u64;
    flags: []const u8;

    fn hasFlag(flag: []const u8): bool {
        return hasCpuFlag(_.flags, flag);
    }
}

pub struct CpuSummary {
    logical_cpus: u32;
    physical_cores: u32;
    model_name: []const u8;
    cache_kb: u64;
}

pub struct ProcCpuInfoIter {
    src: []const u8;
    pos: usize;

    static fn init(content: []const u8): ProcCpuInfoIter {
        return ProcCpuInfoIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): CpuBlock? {
        let block = CpuBlock {
            processor: 0,
            physical_id: 0,
            cpu_cores: 0,
            model_name: "",
            vendor_id: "",
            cpu_mhz_x1000: 0,
            cache_kb: 0,
            flags: "",
        };
        let has_processor = false;

        while (_.pos < _.src.len) {
            const saved_pos = _.pos;
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

            // 裁剪行首尾空白
            let s = line_start;
            let e = line_end;
            while (s < e && isHWhitespace(_.src[s])) {
                s += 1;
            }
            while (e > s && isHWhitespace(_.src[e - 1])) {
                e -= 1;
            }

            // 空行结束当前 processor 块
            if (s >= e) {
                if (has_processor) {
                    return block;
                }
                continue;
            }

            // 查找首个 ':' 分隔 key 与 value
            let colon_idx = s;
            let found_colon = false;
            while (colon_idx < e) {
                if (_.src[colon_idx] == ':') {
                    found_colon = true;
                    break;
                }
                colon_idx += 1;
            }
            if (!found_colon) {
                continue;
            }

            let key_end = colon_idx;
            while (key_end > s && isHWhitespace(_.src[key_end - 1])) {
                key_end -= 1;
            }
            const key = _.src[s..key_end];

            let val_start = colon_idx + 1;
            while (val_start < e && isHWhitespace(_.src[val_start])) {
                val_start += 1;
            }
            const val = _.src[val_start..e];

            if (bytesEql(key, "processor")) {
                if (has_processor) {
                    // 遇到无空行分隔的新 processor 块：回退游标留给下一次 next()
                    _.pos = saved_pos;
                    return block;
                }
                if (proc_id := parseDecU32(val)) {
                    block.processor = proc_id;
                    has_processor = true;
                }
            } else if (has_processor) {
                if (bytesEql(key, "physical id")) {
                    if (pid := parseDecU32(val)) {
                        block.physical_id = pid;
                    }
                } else if (bytesEql(key, "cpu cores")) {
                    if (cores := parseDecU32(val)) {
                        block.cpu_cores = cores;
                    }
                } else if (bytesEql(key, "model name") || (block.model_name.len == 0 && (bytesEql(key, "Model") || bytesEql(key, "Hardware")))) {
                    block.model_name = val;
                } else if (bytesEql(key, "vendor_id") || (block.vendor_id.len == 0 && bytesEql(key, "CPU implementer"))) {
                    block.vendor_id = val;
                } else if (bytesEql(key, "cpu MHz")) {
                    block.cpu_mhz_x1000 = parseMhzX1000(val);
                } else if (bytesEql(key, "cache size")) {
                    block.cache_kb = parseCacheKb(val);
                } else if (bytesEql(key, "flags") || bytesEql(key, "Features")) {
                    block.flags = val;
                }
            }
        }

        if (has_processor) {
            return block;
        }
        return null;
    }
}

pub fn summarizeCpuInfo(content: []const u8): CpuSummary {
    let it = ProcCpuInfoIter.init(content);
    let logical_cpus: u32 = 0;
    let physical_cores: u32 = 0;
    let model_name: []const u8 = "";
    let cache_kb: u64 = 0;

    // 零分配栈上 socket 去重表（最多 64 路物理插槽）
    let seen_sockets: [64]u32 = [0; 64];
    let seen_count: usize = 0;

    while (blk := it.next()) {
        logical_cpus += 1;
        if (model_name.len == 0 && blk.model_name.len > 0) {
            model_name = blk.model_name;
        }
        if (cache_kb == 0 && blk.cache_kb > 0) {
            cache_kb = blk.cache_kb;
        }
        if (blk.cpu_cores > 0) {
            let already_seen = false;
            let i: usize = 0;
            while (i < seen_count) {
                if (seen_sockets[i] == blk.physical_id) {
                    already_seen = true;
                    break;
                }
                i += 1;
            }
            if (!already_seen) {
                physical_cores += blk.cpu_cores;
                if (seen_count < 64) {
                    seen_sockets[seen_count] = blk.physical_id;
                    seen_count += 1;
                }
            }
        }
    }

    if (physical_cores == 0) {
        physical_cores = logical_cpus;
    }

    return CpuSummary {
        logical_cpus: logical_cpus,
        physical_cores: physical_cores,
        model_name: model_name,
        cache_kb: cache_kb,
    };
}
