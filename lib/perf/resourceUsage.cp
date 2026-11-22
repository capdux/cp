// Source: Absorbed from /data/dev/bun/src/runtime/api/bun/subprocess/ResourceUsage.cp | Synced: 2026-11-22T15:26:00Z
// Zero-allocation subprocess & process resource usage metrics (CPU time, RSS, I/O ops, IPC, context switches).

/// Microseconds per second for `timeval` normalization.
pub const RUSAGE_USEC_PER_SEC: u64 = 1_000_000;

/// Bytes per kibibyte (used to normalize Linux `ru_maxrss` KiB to bytes).
pub const RUSAGE_BYTES_PER_KB: u64 = 1_024;

/// Error set for `ResourceUsage` formatting.
pub err ResourceUsageError {
    BufferTooSmall,
}

/// Aggregated CPU time in microseconds (mirrors Bun `usage.cpuTime`).
pub struct CpuTimeUs {
    user_us: u64 = 0,
    system_us: u64 = 0,
    total_us: u64 = 0,

    static fn fromTimevals(u_sec: u64, u_usec: u32, s_sec: u64, s_usec: u32): CpuTimeUs {
        const u = u_sec * RUSAGE_USEC_PER_SEC + u64(u_usec);
        const s = s_sec * RUSAGE_USEC_PER_SEC + u64(s_usec);
        return CpuTimeUs{
            user_us: u,
            system_us: s,
            total_us: u + s,
        };
    }

    /// Return user CPU share in permille (`0..=1000`, 0 when total == 0).
    fn userSharePermille(): u32 {
        if (_.total_us == 0) return 0;
        return u32((u128(_.user_us) * 1000) / u128(_.total_us));
    }
}

/// Block I/O operation counts (mirrors Bun `usage.ops`).
pub struct IoBlockOps {
    in_blocks: u64 = 0,
    out_blocks: u64 = 0,

    fn total(): u64 {
        return _.in_blocks + _.out_blocks;
    }
}

/// IPC message counts (mirrors Bun `usage.messages`).
pub struct IpcMessages {
    sent: u64 = 0,
    received: u64 = 0,

    fn total(): u64 {
        return _.sent + _.received;
    }
}

/// Context switch counters (mirrors Bun `usage.contextSwitches`).
pub struct ContextSwitches {
    voluntary: u64 = 0,
    involuntary: u64 = 0,

    fn total(): u64 {
        return _.voluntary + _.involuntary;
    }

    /// Return involuntary context switch ratio in permille (`0..=1000`).
    fn involuntaryRatioPermille(): u32 {
        const tot = _.total();
        if (tot == 0) return 0;
        return u32((u128(_.involuntary) * 1000) / u128(tot));
    }
}

fn satSubU64(a: u64, b: u64): u64 {
    return if (a > b) (a - b) else 0;
}

fn maxU64(a: u64, b: u64): u64 {
    return if (a >= b) a else b;
}

fn writeStr(out: []u8, pos: *usize, s: []const u8): usize ! ResourceUsageError {
    if (pos.* + s.len > out.len) return err.BufferTooSmall;
    for (i of 0..s.len) {
        out[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return pos.*;
}

fn writeU64(out: []u8, pos: *usize, val: u64): usize ! ResourceUsageError {
    let tmp: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = u8('0') + u8(v % 10);
            v = v / 10;
            len += 1;
        }
    }
    if (pos.* + len > out.len) return err.BufferTooSmall;
    for (i of 0..len) {
        out[pos.* + i] = tmp[len - 1 - i];
    }
    pos.* += len;
    return pos.*;
}

/// Subprocess / process resource usage snapshot absorbed from Bun `ResourceUsage.cp`.
/// All counters are stored in normalized platform-independent units (`max_rss_bytes` in bytes,
/// CPU time normalized to microseconds).
pub struct ResourceUsage {
    utime_sec: u64 = 0,
    utime_usec: u32 = 0,
    stime_sec: u64 = 0,
    stime_usec: u32 = 0,
    max_rss_bytes: u64 = 0,
    shared_memory_size: u64 = 0,
    swap_count: u64 = 0,
    in_block_ops: u64 = 0,
    out_block_ops: u64 = 0,
    ipc_sent: u64 = 0,
    ipc_received: u64 = 0,
    signal_count: u64 = 0,
    voluntary_ctx_switches: u64 = 0,
    involuntary_ctx_switches: u64 = 0,

    /// Convert Linux `ru_maxrss` (reported in KiB) to normalized bytes.
    static fn linuxMaxRssKbToBytes(maxrss_kb: u64): u64 {
        return maxrss_kb * RUSAGE_BYTES_PER_KB;
    }

    /// Return structured CPU time in microseconds (user / system / total).
    fn cpuTime(): CpuTimeUs {
        return CpuTimeUs.fromTimevals(_.utime_sec, _.utime_usec, _.stime_sec, _.stime_usec);
    }

    /// Return block I/O operations (`in` / `out`).
    fn ops(): IoBlockOps {
        return IoBlockOps{
            in_blocks: _.in_block_ops,
            out_blocks: _.out_block_ops,
        };
    }

    /// Return IPC message counts (`sent` / `received`).
    fn messages(): IpcMessages {
        return IpcMessages{
            sent: _.ipc_sent,
            received: _.ipc_received,
        };
    }

    /// Return voluntary & involuntary context switch counts.
    fn contextSwitches(): ContextSwitches {
        return ContextSwitches{
            voluntary: _.voluntary_ctx_switches,
            involuntary: _.involuntary_ctx_switches,
        };
    }

    /// Compute saturating delta `self - earlier` across cumulative counters,
    /// keeping the peak `max_rss_bytes` and `shared_memory_size`.
    fn diff(earlier: ResourceUsage): ResourceUsage {
        const cur_cpu = _.cpuTime();
        const prev_cpu = earlier.cpuTime();
        const du = satSubU64(cur_cpu.user_us, prev_cpu.user_us);
        const ds = satSubU64(cur_cpu.system_us, prev_cpu.system_us);

        return ResourceUsage{
            utime_sec: du / RUSAGE_USEC_PER_SEC,
            utime_usec: u32(du % RUSAGE_USEC_PER_SEC),
            stime_sec: ds / RUSAGE_USEC_PER_SEC,
            stime_usec: u32(ds % RUSAGE_USEC_PER_SEC),
            max_rss_bytes: maxU64(_.max_rss_bytes, earlier.max_rss_bytes),
            shared_memory_size: maxU64(_.shared_memory_size, earlier.shared_memory_size),
            swap_count: satSubU64(_.swap_count, earlier.swap_count),
            in_block_ops: satSubU64(_.in_block_ops, earlier.in_block_ops),
            out_block_ops: satSubU64(_.out_block_ops, earlier.out_block_ops),
            ipc_sent: satSubU64(_.ipc_sent, earlier.ipc_sent),
            ipc_received: satSubU64(_.ipc_received, earlier.ipc_received),
            signal_count: satSubU64(_.signal_count, earlier.signal_count),
            voluntary_ctx_switches: satSubU64(_.voluntary_ctx_switches, earlier.voluntary_ctx_switches),
            involuntary_ctx_switches: satSubU64(_.involuntary_ctx_switches, earlier.involuntary_ctx_switches),
        };
    }

    /// Merge two independent subprocess usage snapshots (sums cumulative counters, takes max RSS).
    fn merge(other: ResourceUsage): ResourceUsage {
        const a_cpu = _.cpuTime();
        const b_cpu = other.cpuTime();
        const u = a_cpu.user_us + b_cpu.user_us;
        const s = a_cpu.system_us + b_cpu.system_us;

        return ResourceUsage{
            utime_sec: u / RUSAGE_USEC_PER_SEC,
            utime_usec: u32(u % RUSAGE_USEC_PER_SEC),
            stime_sec: s / RUSAGE_USEC_PER_SEC,
            stime_usec: u32(s % RUSAGE_USEC_PER_SEC),
            max_rss_bytes: maxU64(_.max_rss_bytes, other.max_rss_bytes),
            shared_memory_size: maxU64(_.shared_memory_size, other.shared_memory_size),
            swap_count: _.swap_count + other.swap_count,
            in_block_ops: _.in_block_ops + other.in_block_ops,
            out_block_ops: _.out_block_ops + other.out_block_ops,
            ipc_sent: _.ipc_sent + other.ipc_sent,
            ipc_received: _.ipc_received + other.ipc_received,
            signal_count: _.signal_count + other.signal_count,
            voluntary_ctx_switches: _.voluntary_ctx_switches + other.voluntary_ctx_switches,
            involuntary_ctx_switches: _.involuntary_ctx_switches + other.involuntary_ctx_switches,
        };
    }

    /// Format a compact zero-allocation diagnostic summary into `out`.
    /// Example: `"rss=1048576B cpu=1500us(u=1000,s=500) io=4/2 csw=12/3"`
    fn formatSummary(out: []u8): usize ! ResourceUsageError {
        const cpu = _.cpuTime();
        let pos: usize = 0;
        * = writeStr(out, *pos, "rss=")?;
        * = writeU64(out, *pos, _.max_rss_bytes)?;
        * = writeStr(out, *pos, "B cpu=")?;
        * = writeU64(out, *pos, cpu.total_us)?;
        * = writeStr(out, *pos, "us(u=")?;
        * = writeU64(out, *pos, cpu.user_us)?;
        * = writeStr(out, *pos, ",s=")?;
        * = writeU64(out, *pos, cpu.system_us)?;
        * = writeStr(out, *pos, ") io=")?;
        * = writeU64(out, *pos, _.in_block_ops)?;
        * = writeStr(out, *pos, "/")?;
        * = writeU64(out, *pos, _.out_block_ops)?;
        * = writeStr(out, *pos, " csw=")?;
        * = writeU64(out, *pos, _.voluntary_ctx_switches)?;
        * = writeStr(out, *pos, "/")?;
        * = writeU64(out, *pos, _.involuntary_ctx_switches)?;
        return pos;
    }
}
