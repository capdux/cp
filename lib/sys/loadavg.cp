// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T15:19:45Z (Linux /proc/loadavg & /proc/uptime Zero-Alloc Parser)
// Pure-CP zero-allocation Linux /proc/loadavg and /proc/uptime parsers using exact integer arithmetic.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn nextTok(s: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < s.len && isWs(s[pos.*])) pos.* += 1;
    if (pos.* >= s.len) return null;
    const start = pos.*;
    while (pos.* < s.len && !isWs(s[pos.*])) pos.* += 1;
    return s[start..pos.*];
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

/// Parse a non-negative decimal number into scaled hundredths (`"1.25"` -> `125`, `"0.05"` -> `5`, `"2"` -> `200`).
fn parseHundredths(s: []const u8): u64? {
    if (s.len == 0) return null;
    let dot: usize? = null;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '.') {
            if (dot != null) return null;
            dot = i;
        }
        i += 1;
    }
    if (dp := dot) {
        const whole = parseU64(s[0..dp]) ?? return null;
        const frac = s[dp + 1..];
        if (frac.len == 0) return null;
        let f_hundredths: u64 = 0;
        let fi: usize = 0;
        while (fi < frac.len) {
            if (frac[fi] < '0' || frac[fi] > '9') return null;
            if (fi == 0) f_hundredths += @as(u64, frac[fi] - '0') * 10;
            if (fi == 1) f_hundredths += @as(u64, frac[fi] - '0');
            fi += 1;
        }
        if (whole > 184467440737095516) return null;
        return whole * 100 + f_hundredths;
    }
    const whole = parseU64(s) ?? return null;
    if (whole > 184467440737095516) return null;
    return whole * 100;
}

export struct LoadAvg {
    load1_centi: u64,   // e.g. 1.25 -> 125
    load5_centi: u64,
    load15_centi: u64,
    running_procs: u64,
    total_procs: u64,
    last_pid: u64,

    /// Parse Linux `/proc/loadavg` (e.g. `"0.52 0.58 0.59 2/412 12345\n"`).
    pub fn parse(content: []const u8): LoadAvg? {
        let pos: usize = 0;
        const t1 = nextTok(content, &pos) ?? return null;
        const t5 = nextTok(content, &pos) ?? return null;
        const t15 = nextTok(content, &pos) ?? return null;
        const tprocs = nextTok(content, &pos) ?? return null;
        const tpid = nextTok(content, &pos) ?? return null;

        const l1 = parseHundredths(t1) ?? return null;
        const l5 = parseHundredths(t5) ?? return null;
        const l15 = parseHundredths(t15) ?? return null;

        let slash: usize? = null;
        let k: usize = 0;
        while (k < tprocs.len) {
            if (tprocs[k] == '/') {
                slash = k;
                break;
            }
            k += 1;
        }
        const sp = slash ?? return null;
        const running = parseU64(tprocs[0..sp]) ?? return null;
        const total = parseU64(tprocs[sp + 1..]) ?? return null;
        const pid = parseU64(tpid) ?? return null;

        return LoadAvg {
            load1_centi: l1,
            load5_centi: l5,
            load15_centi: l15,
            running_procs: running,
            total_procs: total,
            last_pid: pid,
        };
    }

    /// True if 1-minute load average exceeds `num_cpus` (`load1_centi > num_cpus * 100`).
    pub fn isOverloaded(num_cpus: u64): bool {
        if (num_cpus == 0) return true;
        return _.load1_centi > num_cpus * 100;
    }
}

export struct SystemUptime {
    uptime_ms: u64,
    idle_ms: u64,

    /// Parse Linux `/proc/uptime` (e.g. `"12345.67 98765.43\n"` -> milliseconds).
    pub fn parse(content: []const u8): SystemUptime? {
        let pos: usize = 0;
        const t_up = nextTok(content, &pos) ?? return null;
        const t_idle = nextTok(content, &pos) ?? return null;

        const up_cs = parseHundredths(t_up) ?? return null;
        const idle_cs = parseHundredths(t_idle) ?? return null;

        return SystemUptime {
            uptime_ms: up_cs * 10,
            idle_ms: idle_cs * 10,
        };
    }

    pub fn uptimeSeconds(): u64 {
        return _.uptime_ms / 1000;
    }
}
