// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T15:17:03Z (Linux /proc/<pid>/stat & /proc/<pid>/status Parser)
// Pure-CP zero-allocation Linux /proc/<pid>/stat parser (safe for comm containing spaces or parentheses).

fn parseI64(s: []const u8): i64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
        if (i >= s.len) return null;
    }
    let val: i64 = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        if (val > 922337203685477580) return null;
        val = val * 10 + @as(i64, c - '0');
        i += 1;
    }
    return if (neg) 0 - val else val;
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

fn nextSpaceTok(s: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < s.len && (s[pos.*] == ' ' || s[pos.*] == '\t' || s[pos.*] == '\r' || s[pos.*] == '\n')) {
        pos.* += 1;
    }
    if (pos.* >= s.len) return null;
    const start = pos.*;
    while (pos.* < s.len && s[pos.*] != ' ' && s[pos.*] != '\t' && s[pos.*] != '\r' && s[pos.*] != '\n') {
        pos.* += 1;
    }
    return s[start..pos.*];
}

export enum ProcState {
    running,       // R
    sleeping,      // S
    disk_sleep,    // D
    zombie,        // Z
    stopped,       // T
    tracing_stop,  // t
    dead,          // X / x
    idle,          // I
    unknown,
}

export fn parseProcState(ch: u8): ProcState {
    if (ch == 'R') return ProcState.running;
    if (ch == 'S') return ProcState.sleeping;
    if (ch == 'D') return ProcState.disk_sleep;
    if (ch == 'Z') return ProcState.zombie;
    if (ch == 'T') return ProcState.stopped;
    if (ch == 't') return ProcState.tracing_stop;
    if (ch == 'X' || ch == 'x') return ProcState.dead;
    if (ch == 'I') return ProcState.idle;
    return ProcState.unknown;
}

export struct ProcStat {
    pid: i64,
    comm: []const u8,
    state: ProcState,
    ppid: i64,
    pgrp: i64,
    session: i64,
    utime_ticks: u64,
    stime_ticks: u64,
    num_threads: i64,
    starttime_ticks: u64,
    vsize_bytes: u64,
    rss_pages: i64,

    /// Parse a Linux `/proc/<pid>/stat` line (handles `comm` containing spaces or `)` via last-`)` scan).
    pub static fn parse(line: []const u8): ProcStat? {
        // 1. Find first '(' and last ')'
        let open_p: usize? = null;
        let close_p: usize? = null;
        let i: usize = 0;
        while (i < line.len) {
            if (line[i] == '(' && open_p == null) {
                open_p = i;
            }
            if (line[i] == ')') {
                close_p = i;
            }
            i += 1;
        }
        const op = open_p ?? return null;
        const cp = close_p ?? return null;
        if (cp <= op) return null;

        // Parse pid before '('
        let pid_end = op;
        while (pid_end > 0 && line[pid_end - 1] == ' ') pid_end -= 1;
        let pid_start: usize = 0;
        while (pid_start < pid_end && line[pid_start] == ' ') pid_start += 1;
        const pid = parseI64(line[pid_start..pid_end]) ?? return null;
        const comm = line[op + 1..cp];

        // Fields after ')' start at field 3 (state)
        const after = line[cp + 1..];
        let pos: usize = 0;

        // Field 3: state
        const f3 = nextSpaceTok(after, &pos) ?? return null;
        if (f3.len != 1) return null;
        const state = parseProcState(f3[0]);

        // Field 4: ppid, 5: pgrp, 6: session
        const ppid = parseI64(nextSpaceTok(after, &pos) ?? return null) ?? return null;
        const pgrp = parseI64(nextSpaceTok(after, &pos) ?? return null) ?? return null;
        const session = parseI64(nextSpaceTok(after, &pos) ?? return null) ?? return null;

        // Skip fields 7..13 (tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt)
        let skip1: usize = 0;
        while (skip1 < 7) {
            if (nextSpaceTok(after, &pos) == null) return null;
            skip1 += 1;
        }

        // Field 14: utime, 15: stime
        const utime = parseU64(nextSpaceTok(after, &pos) ?? return null) ?? return null;
        const stime = parseU64(nextSpaceTok(after, &pos) ?? return null) ?? return null;

        // Skip fields 16..19 (cutime, cstime, priority, nice)
        let skip2: usize = 0;
        while (skip2 < 4) {
            if (nextSpaceTok(after, &pos) == null) return null;
            skip2 += 1;
        }

        // Field 20: num_threads
        const num_threads = parseI64(nextSpaceTok(after, &pos) ?? return null) ?? return null;

        // Skip field 21 (itrealvalue)
        if (nextSpaceTok(after, &pos) == null) return null;

        // Field 22: starttime, 23: vsize, 24: rss
        const starttime = parseU64(nextSpaceTok(after, &pos) ?? return null) ?? return null;
        const vsize = parseU64(nextSpaceTok(after, &pos) ?? return null) ?? return null;
        const rss = parseI64(nextSpaceTok(after, &pos) ?? return null) ?? return null;

        return ProcStat {
            pid: pid,
            comm: comm,
            state: state,
            ppid: ppid,
            pgrp: pgrp,
            session: session,
            utime_ticks: utime,
            stime_ticks: stime,
            num_threads: num_threads,
            starttime_ticks: starttime,
            vsize_bytes: vsize,
            rss_pages: rss,
        };
    }

    pub fn totalCpuTicks(): u64 {
        return _.utime_ticks + _.stime_ticks;
    }

    pub fn rssBytes(page_size: u64): u64 {
        if (_.rss_pages <= 0) return 0;
        return @as(u64, _.rss_pages) * page_size;
    }

    pub fn isZombie(): bool {
        return _.state == ProcState.zombie;
    }
}
