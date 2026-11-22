// Origin: Pure-CP Stdlib (Zero-Alloc Linux cgroup v2 CPU/Memory Quota, PSI Pressure Parser & Budget Advisor) | Created: 2026-11-23
// Pure CP zero-heap-allocation container resource-awareness and adaptive backpressure engine:
//   - Linux cgroup v2 `cpu.max` parser (`parseCgroupCpuMax`) with millicore & host-clamped worker sizing
//   - Linux cgroup v2 `memory.max` / `memory.high` / `memory.current` parser (`parseCgroupMemoryValue`)
//   - Linux kernel PSI (`/proc/pressure/memory|cpu|io`) pure-integer fixed-point parser (`parsePsiPressure`)
//     representing percentages in basis points (`0..=10000`, e.g. `12.34% -> 1234`) with zero float error
//   - Deterministic container concurrency & load-shedding advisor (`evaluateCgroupBudget`) fusing
//     CPU quota, memory utilization, and PSI `some`/`full` stall pressure into 4 pressure tiers

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Basis-point scale (`10000 = 100.00%`) used for memory utilization and PSI stall averages.
pub const CGROUP_BPS_SCALE: u32 = 10_000;

/// Parsed Linux cgroup v2 `cpu.max` quota descriptor.
pub struct CgroupCpuQuota {
    is_unlimited: bool = true;
    quota_us: u64 = 0;
    period_us: u64 = 100_000;

    /// Return the effective CPU allocation in millicores (`1000 = 1 vCPU`).
    /// Returns `0` when `is_unlimited == true` or `period_us == 0`.
    fn effectiveMillicores(): u64 {
        if (is_unlimited || period_us == 0) return 0;
        const num: u128 = @as(u128, quota_us) * 1000;
        return @as(u64, @intCast(num / @as(u128, period_us)));
    }

    /// Recommend an integer worker count clamped to `[1, max(1, host_cores)]`.
    /// Uses ceiling division (`ceil(quota_us / period_us)`) so fractional-core containers
    /// (e.g. 2.5 vCPU -> 3 workers, 0.5 vCPU -> 1 worker) can saturate their allotted quota.
    fn recommendedWorkers(host_cores: usize): usize {
        const host: usize = if (host_cores == 0) 1 else host_cores;
        if (is_unlimited || period_us == 0) return host;
        if (quota_us == 0) return 1;
        const ceil_cores: u128 = (@as(u128, quota_us) + @as(u128, period_us) - 1) / @as(u128, period_us);
        if (ceil_cores == 0) return 1;
        if (ceil_cores >= @as(u128, host)) return host;
        return @as(usize, @intCast(ceil_cores));
    }
}

/// Parsed Linux cgroup v2 memory quota or current usage value.
pub struct CgroupMemoryLimit {
    is_unlimited: bool = true;
    limit_bytes: u64 = 0;
}

/// Parsed PSI (`some` or `full`) stall averages in basis points (`0..=10000`) and cumulative microseconds.
pub struct PsiMetrics {
    avg10_bps: u32 = 0;
    avg60_bps: u32 = 0;
    avg300_bps: u32 = 0;
    total_us: u64 = 0;
}

/// Combined `some` and `full` PSI snapshot parsed from `/proc/pressure/*`.
pub struct PsiSnapshot {
    has_some: bool = false;
    some: PsiMetrics = PsiMetrics{};
    has_full: bool = false;
    full: PsiMetrics = PsiMetrics{};
}

/// Four-tier container resource pressure classification.
pub enum CgroupPressureLevel {
    nominal,
    elevated,
    critical,
    oom_imminent,
}

/// Deterministic concurrency and backpressure recommendation returned by `evaluateCgroupBudget`.
pub struct CgroupBudgetAdvice {
    pressure_level: CgroupPressureLevel = CgroupPressureLevel.nominal;
    memory_util_bps: u32 = 0;
    effective_workers: usize = 1;
    headroom_bytes: u64 = 0;
    should_shed_load: bool = false;
    should_pause_background: bool = false;
}

// ============================================================================
// 2. Pure Zero-Alloc Parsing Helpers
// ============================================================================

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isAsciiSpace(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && isAsciiSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return bytesEqual(s[0..prefix.len], prefix);
}

fn parseStrictU64(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const digit: u64 = @as(u64, c - '0');
        // Overflow guard: val * 10 + digit <= u64::MAX (18446744073709551615)
        if (val > 1844674407370955161) return null;
        if (val == 1844674407370955161 && digit > 5) return null;
        val = val * 10 + digit;
    }
    return val;
}

/// Parse a non-negative fixed-point percentage string (e.g. `"12.34"`, `"0.50"`, `"100.00"`)
/// into basis points (`0..=10000`) using pure integer arithmetic.
fn parsePercentBps(s: []const u8): u32? {
    if (s.len == 0) return null;
    let dot_idx: usize = s.len;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '.') {
            if (dot_idx != s.len) return null; // multiple dots
            dot_idx = i;
        }
    }

    const int_str: []const u8 = s[0..dot_idx];
    const int_val = parseStrictU64(int_str) ?? return null;
    if (int_val > 100) return null;

    let frac_bps: u32 = 0;
    if (dot_idx < s.len) {
        const frac_str: []const u8 = s[dot_idx + 1 .. s.len];
        if (frac_str.len == 0 || frac_str.len > 2) return null;
        const f0 = frac_str[0];
        if (f0 < '0' || f0 > '9') return null;
        if (frac_str.len == 1) {
            frac_bps = @as(u32, f0 - '0') * 10;
        } else {
            const f1 = frac_str[1];
            if (f1 < '0' || f1 > '9') return null;
            frac_bps = @as(u32, f0 - '0') * 10 + @as(u32, f1 - '0');
        }
    }

    const total: u32 = @as(u32, @intCast(int_val)) * 100 + frac_bps;
    if (total > CGROUP_BPS_SCALE) return null;
    return total;
}

fn parsePsiLine(line: []const u8): PsiMetrics? {
    let m = PsiMetrics{};
    let got_avg10 = false;
    let got_avg60 = false;
    let got_avg300 = false;
    let got_total = false;

    let pos: usize = 0;
    while (pos < line.len) {
        while (pos < line.len && isAsciiSpace(line[pos])) : (pos += 1) {}
        if (pos >= line.len) break;
        const start = pos;
        while (pos < line.len && !isAsciiSpace(line[pos])) : (pos += 1) {}
        const tok: []const u8 = line[start..pos];

        if (startsWith(tok, "avg10=")) {
            m.avg10_bps = parsePercentBps(tok[6..]) ?? return null;
            got_avg10 = true;
        } else if (startsWith(tok, "avg60=")) {
            m.avg60_bps = parsePercentBps(tok[6..]) ?? return null;
            got_avg60 = true;
        } else if (startsWith(tok, "avg300=")) {
            m.avg300_bps = parsePercentBps(tok[7..]) ?? return null;
            got_avg300 = true;
        } else if (startsWith(tok, "total=")) {
            m.total_us = parseStrictU64(tok[6..]) ?? return null;
            got_total = true;
        } else {
            return null;
        }
    }

    if (!got_avg10 || !got_avg60 || !got_avg300 || !got_total) return null;
    return m;
}

// ============================================================================
// 3. Public Parsers & Adaptive Container Budget Evaluator
// ============================================================================

/// Parse Linux cgroup v2 `cpu.max` content (`"max 100000"` or `"250000 100000"`).
/// Rejects empty input, zero/negative quota or period, and malformed trailing tokens.
pub fn parseCgroupCpuMax(content: []const u8): CgroupCpuQuota? {
    const trimmed = trimAscii(content);
    if (trimmed.len == 0) return null;

    // First token: "max" or positive quota_us
    let p: usize = 0;
    while (p < trimmed.len && !isAsciiSpace(trimmed[p])) : (p += 1) {}
    const quota_tok: []const u8 = trimmed[0..p];

    // Skip whitespace between tokens
    while (p < trimmed.len && isAsciiSpace(trimmed[p])) : (p += 1) {}
    if (p >= trimmed.len) return null;

    // Second token: positive period_us
    const p2_start = p;
    while (p < trimmed.len && !isAsciiSpace(trimmed[p])) : (p += 1) {}
    const period_tok: []const u8 = trimmed[p2_start..p];

    // Reject extra trailing tokens
    if (p != trimmed.len) return null;

    const period_us = parseStrictU64(period_tok) ?? return null;
    if (period_us == 0) return null;

    if (bytesEqual(quota_tok, "max")) {
        return CgroupCpuQuota{
            is_unlimited: true,
            quota_us: 0,
            period_us: period_us,
        };
    }

    const quota_us = parseStrictU64(quota_tok) ?? return null;
    if (quota_us == 0) return null;

    return CgroupCpuQuota{
        is_unlimited: false,
        quota_us: quota_us,
        period_us: period_us,
    };
}

/// Parse Linux cgroup v2 `memory.max`, `memory.high`, or `memory.current` content.
/// Accepts `"max"` or a non-negative integer with optional surrounding whitespace/newlines.
pub fn parseCgroupMemoryValue(content: []const u8): CgroupMemoryLimit? {
    const trimmed = trimAscii(content);
    if (trimmed.len == 0) return null;
    if (bytesEqual(trimmed, "max")) {
        return CgroupMemoryLimit{
            is_unlimited: true,
            limit_bytes: 0,
        };
    }
    const bytes = parseStrictU64(trimmed) ?? return null;
    return CgroupMemoryLimit{
        is_unlimited: false,
        limit_bytes: bytes,
    };
}

/// Parse Linux kernel PSI (`/proc/pressure/memory`, `cpu`, or `io`) content.
/// Uses pure-integer fixed-point arithmetic (`avg10_bps`, `avg60_bps`, `avg300_bps` in `0..=10000`).
/// Returns `null` on malformed lines or empty input.
pub fn parsePsiPressure(content: []const u8): PsiSnapshot? {
    let snap = PsiSnapshot{};
    let pos: usize = 0;

    while (pos < content.len) {
        const line_start = pos;
        while (pos < content.len && content[pos] != '\n') : (pos += 1) {}
        const raw_line: []const u8 = content[line_start..pos];
        if (pos < content.len && content[pos] == '\n') pos += 1;

        const line = trimAscii(raw_line);
        if (line.len == 0) continue;

        if (startsWith(line, "some ") || startsWith(line, "some\t")) {
            snap.some = parsePsiLine(line[5..]) ?? return null;
            snap.has_some = true;
        } else if (startsWith(line, "full ") || startsWith(line, "full\t")) {
            snap.full = parsePsiLine(line[5..]) ?? return null;
            snap.has_full = true;
        } else {
            return null;
        }
    }

    if (!snap.has_some && !snap.has_full) return null;
    return snap;
}

/// Evaluate container CPU quota, memory utilization, and PSI pressure stall metrics
/// into a deterministic concurrency and backpressure recommendation.
pub fn evaluateCgroupBudget(
    cpu: CgroupCpuQuota,
    mem_max: CgroupMemoryLimit,
    mem_current_bytes: u64,
    psi: PsiSnapshot,
    host_cores: usize,
): CgroupBudgetAdvice {
    let adv = CgroupBudgetAdvice{};
    const base_workers = cpu.recommendedWorkers(host_cores);

    if (!mem_max.is_unlimited && mem_max.limit_bytes > 0) {
        adv.headroom_bytes = if (mem_max.limit_bytes > mem_current_bytes) mem_max.limit_bytes - mem_current_bytes else 0;
        const util_u128: u128 = (@as(u128, mem_current_bytes) * @as(u128, CGROUP_BPS_SCALE)) / @as(u128, mem_max.limit_bytes);
        adv.memory_util_bps = if (util_u128 > @as(u128, CGROUP_BPS_SCALE)) CGROUP_BPS_SCALE else @as(u32, @intCast(util_u128));
    } else {
        adv.memory_util_bps = 0;
        adv.headroom_bytes = 0xFFFFFFFFFFFFFFFF;
    }

    const some10: u32 = if (psi.has_some) psi.some.avg10_bps else 0;
    const full10: u32 = if (psi.has_full) psi.full.avg10_bps else 0;

    if (adv.memory_util_bps >= 9500 || full10 >= 2500) {
        adv.pressure_level = CgroupPressureLevel.oom_imminent;
        adv.effective_workers = 1;
        adv.should_pause_background = true;
        adv.should_shed_load = true;
    } else if (adv.memory_util_bps >= 8500 || full10 >= 1000 || some10 >= 4000) {
        adv.pressure_level = CgroupPressureLevel.critical;
        const half: usize = base_workers / 2;
        adv.effective_workers = if (half == 0) 1 else half;
        adv.should_pause_background = true;
        adv.should_shed_load = true;
    } else if (adv.memory_util_bps >= 7000 || full10 >= 200 || some10 >= 1500) {
        adv.pressure_level = CgroupPressureLevel.elevated;
        const scaled: usize = (base_workers * 3) / 4;
        adv.effective_workers = if (scaled == 0) 1 else scaled;
        adv.should_pause_background = true;
        adv.should_shed_load = false;
    } else {
        adv.pressure_level = CgroupPressureLevel.nominal;
        adv.effective_workers = base_workers;
        adv.should_pause_background = false;
        adv.should_shed_load = false;
    }

    return adv;
}
