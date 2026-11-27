// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T15:18:36Z (Linux /proc/meminfo Zero-Alloc Parser)
// Pure-CP zero-allocation Linux /proc/meminfo parser converting kB fields to exact byte metrics.

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

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn parseU64(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = @as(u64, c - '0');
        if (val > 1844674407370955161) return null;
        if (val == 1844674407370955161 && d > 5) return null;
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

export struct MemInfoField {
    key: []const u8,
    value_bytes: u64,
    is_kb: bool,
}

export struct MemInfoIter {
    src: []const u8,
    pos: usize,

    pub static fn init(content: []const u8): MemInfoIter {
        return MemInfoIter { src: content, pos: 0 };
    }

    pub fn next(*_): MemInfoField? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') _.pos += 1;
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;
            const line = trimWs(_.src[start..end]);
            if (line.len == 0) continue;

            let colon: usize? = null;
            let k: usize = 0;
            while (k < line.len) {
                if (line[k] == ':') {
                    colon = k;
                    break;
                }
                k += 1;
            }
            if (colon == null) continue;
            const cp = colon.?;
            const key = trimWs(line[0..cp]);
            const rhs = trimWs(line[cp + 1..]);
            if (key.len == 0 || rhs.len == 0) continue;

            let num_end: usize = 0;
            while (num_end < rhs.len && rhs[num_end] >= '0' && rhs[num_end] <= '9') {
                num_end += 1;
            }
            const raw_num = parseU64(rhs[0..num_end]) ?? continue;
            const unit = trimWs(rhs[num_end..]);
            const is_kb = strEq(unit, "kB") || strEq(unit, "KB");
            let bytes = raw_num;
            if (is_kb) {
                if (raw_num > 18014398509481983) continue; // overflow guard for * 1024
                bytes = raw_num * 1024;
            }
            return MemInfoField { key: key, value_bytes: bytes, is_kb: is_kb };
        }
        return null;
    }
}

export struct MemInfo {
    mem_total: u64,
    mem_free: u64,
    mem_available: u64,
    has_mem_available: bool,
    buffers: u64,
    cached: u64,
    swap_total: u64,
    swap_free: u64,
    dirty: u64,
    shmem: u64,

    pub static fn parse(content: []const u8): MemInfo {
        let info = MemInfo {
            mem_total: 0,
            mem_free: 0,
            mem_available: 0,
            has_mem_available: false,
            buffers: 0,
            cached: 0,
            swap_total: 0,
            swap_free: 0,
            dirty: 0,
            shmem: 0,
        };
        let it = MemInfoIter.init(content);
        while (f := it.next()) {
            if (strEq(f.key, "MemTotal")) {
                info.mem_total = f.value_bytes;
            } else if (strEq(f.key, "MemFree")) {
                info.mem_free = f.value_bytes;
            } else if (strEq(f.key, "MemAvailable")) {
                info.mem_available = f.value_bytes;
                info.has_mem_available = true;
            } else if (strEq(f.key, "Buffers")) {
                info.buffers = f.value_bytes;
            } else if (strEq(f.key, "Cached")) {
                info.cached = f.value_bytes;
            } else if (strEq(f.key, "SwapTotal")) {
                info.swap_total = f.value_bytes;
            } else if (strEq(f.key, "SwapFree")) {
                info.swap_free = f.value_bytes;
            } else if (strEq(f.key, "Dirty")) {
                info.dirty = f.value_bytes;
            } else if (strEq(f.key, "Shmem")) {
                info.shmem = f.value_bytes;
            }
        }
        return info;
    }

    /// Effective available bytes (uses `MemAvailable` when present, else `MemFree + Buffers + Cached`).
    pub fn availableBytes(): u64 {
        if (_.has_mem_available) return _.mem_available;
        return _.mem_free + _.buffers + _.cached;
    }

    /// Effective used RAM bytes (`MemTotal - availableBytes`, clamped to 0).
    pub fn usedBytes(): u64 {
        const avail = _.availableBytes();
        if (avail >= _.mem_total) return 0;
        return _.mem_total - avail;
    }

    pub fn swapUsedBytes(): u64 {
        if (_.swap_free >= _.swap_total) return 0;
        return _.swap_total - _.swap_free;
    }

    /// Memory usage percentage in `0..=100` (pure integer).
    pub fn usedPercent(): u8 {
        if (_.mem_total == 0) return 0;
        const pct: u64 = @as(u64, (@as(u128, _.usedBytes()) * 100) / @as(u128, _.mem_total));
        if (pct > 100) return 100;
        return @as(u8, pct);
    }
}
