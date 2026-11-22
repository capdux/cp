// Origin: Pure-CP Stdlib Test Suite (lib/sys/cgroupLimit.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Linux cgroup v2 CPU/Memory Quota, PSI Parser & Budget Advisor:
//   1. `parseCgroupCpuMax`: bounded, unlimited, fractional millicores & malformed/zero-period rejection
//   2. `recommendedWorkers`: fractional-core ceiling, host-core clamping & zero-host-core floor
//   3. `parseCgroupMemoryValue`: `"max"`, integer bytes, whitespace trimming & malformed rejection
//   4. `parsePsiPressure`: `some`+`full` pure-integer fixed-point bps parsing & malformed rejection
//   5. `evaluateCgroupBudget`: four-tier pressure classification (`nominal`, `elevated`, `critical`, `oom_imminent`)
//   6. Unlimited memory + PSI-triggered pressure escalation & `cap.sys` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CGROUP_BPS_SCALE,
    CgroupCpuQuota,
    CgroupMemoryLimit,
    PsiSnapshot,
    CgroupPressureLevel,
    parseCgroupCpuMax,
    parseCgroupMemoryValue,
    parsePsiPressure,
    evaluateCgroupBudget,
} from "cap:sys";

test("CgroupLimit: parseCgroupCpuMax bounded, unlimited, millicores & malformed guards") {
    // 2.5 vCPU (250000 / 100000)
    if (q := parseCgroupCpuMax("  250000 100000 \n")) {
        expect_true(!q.is_unlimited);
        expect_eq(q.quota_us, @as(u64, 250_000));
        expect_eq(q.period_us, @as(u64, 100_000));
        expect_eq(q.effectiveMillicores(), @as(u64, 2500));
    } else {
        expect_true(false);
    }

    // 0.5 vCPU (50000 / 100000)
    if (q_half := parseCgroupCpuMax("50000 100000")) {
        expect_eq(q_half.effectiveMillicores(), @as(u64, 500));
    } else {
        expect_true(false);
    }

    // Unlimited ("max 100000")
    if (q_max := parseCgroupCpuMax("max 100000\n")) {
        expect_true(q_max.is_unlimited);
        expect_eq(q_max.period_us, @as(u64, 100_000));
        expect_eq(q_max.effectiveMillicores(), @as(u64, 0));
    } else {
        expect_true(false);
    }

    // Malformed / invalid inputs must return null
    expect_true(parseCgroupCpuMax("") == null);
    expect_true(parseCgroupCpuMax("max") == null);
    expect_true(parseCgroupCpuMax("100000 0") == null);
    expect_true(parseCgroupCpuMax("0 100000") == null);
    expect_true(parseCgroupCpuMax("-100 100000") == null);
    expect_true(parseCgroupCpuMax("100000 100000 extra") == null);
    expect_true(parseCgroupCpuMax("abc 100000") == null);
}

test("CgroupLimit: recommendedWorkers fractional ceiling & host_cores clamping") {
    const q_2_5 = parseCgroupCpuMax("250000 100000") ?? CgroupCpuQuota{};
    expect_eq(q_2_5.recommendedWorkers(16), @as(usize, 3));
    expect_eq(q_2_5.recommendedWorkers(2), @as(usize, 2)); // clamped to host_cores=2
    expect_eq(q_2_5.recommendedWorkers(0), @as(usize, 1)); // host_cores=0 safe floor

    const q_half = parseCgroupCpuMax("50000 100000") ?? CgroupCpuQuota{};
    expect_eq(q_half.recommendedWorkers(8), @as(usize, 1));

    const q_unlim = parseCgroupCpuMax("max 100000") ?? CgroupCpuQuota{};
    expect_eq(q_unlim.recommendedWorkers(12), @as(usize, 12));
}

test("CgroupLimit: parseCgroupMemoryValue max, integer bytes & malformed guards") {
    if (m_max := parseCgroupMemoryValue("  max\n")) {
        expect_true(m_max.is_unlimited);
        expect_eq(m_max.limit_bytes, @as(u64, 0));
    } else {
        expect_true(false);
    }

    if (m_1g := parseCgroupMemoryValue("1073741824\r\n")) {
        expect_true(!m_1g.is_unlimited);
        expect_eq(m_1g.limit_bytes, @as(u64, 1_073_741_824));
    } else {
        expect_true(false);
    }

    if (m_zero := parseCgroupMemoryValue("0")) {
        expect_true(!m_zero.is_unlimited);
        expect_eq(m_zero.limit_bytes, @as(u64, 0));
    } else {
        expect_true(false);
    }

    expect_true(parseCgroupMemoryValue("") == null);
    expect_true(parseCgroupMemoryValue("   \n") == null);
    expect_true(parseCgroupMemoryValue("-1024") == null);
    expect_true(parseCgroupMemoryValue("1024MB") == null);
    expect_true(parseCgroupMemoryValue("123 456") == null);
}

test("CgroupLimit: parsePsiPressure pure-integer fixed-point bps parsing & malformed guards") {
    const sample =
        "some avg10=12.34 avg60=0.50 avg300=1.2 total=987654\n" ++
        "full avg10=100.00 avg60=0.00 avg300=0 total=42\n";

    if (snap := parsePsiPressure(sample)) {
        expect_true(snap.has_some);
        expect_true(snap.has_full);
        expect_eq(snap.some.avg10_bps, @as(u32, 1234));
        expect_eq(snap.some.avg60_bps, @as(u32, 50));
        expect_eq(snap.some.avg300_bps, @as(u32, 120));
        expect_eq(snap.some.total_us, @as(u64, 987_654));
        expect_eq(snap.full.avg10_bps, CGROUP_BPS_SCALE);
        expect_eq(snap.full.avg60_bps, @as(u32, 0));
        expect_eq(snap.full.avg300_bps, @as(u32, 0));
        expect_eq(snap.full.total_us, @as(u64, 42));
    } else {
        expect_true(false);
    }

    // CPU PSI only has a `some` line
    if (cpu_snap := parsePsiPressure("some avg10=3.05 avg60=1.00 avg300=0.25 total=500\n")) {
        expect_true(cpu_snap.has_some);
        expect_true(!cpu_snap.has_full);
        expect_eq(cpu_snap.some.avg10_bps, @as(u32, 305));
    } else {
        expect_true(false);
    }

    // Malformed PSI inputs rejected
    expect_true(parsePsiPressure("") == null);
    expect_true(parsePsiPressure("some avg10=101.00 avg60=0.00 avg300=0.00 total=0") == null);
    expect_true(parsePsiPressure("some avg10=1.234 avg60=0.00 avg300=0.00 total=0") == null);
    expect_true(parsePsiPressure("some avg10=1.00 avg60=0.00 total=0") == null); // missing avg300
    expect_true(parsePsiPressure("other avg10=1.00 avg60=0.00 avg300=0.00 total=0") == null);
}

test("CgroupLimit: evaluateCgroupBudget four-tier pressure & backpressure decisions") {
    const cpu = parseCgroupCpuMax("800000 100000") ?? CgroupCpuQuota{}; // 8 vCPU
    const mem = parseCgroupMemoryValue("1000000") ?? CgroupMemoryLimit{};
    const calm_psi = parsePsiPressure(
        "some avg10=0.50 avg60=0.10 avg300=0.00 total=10\nfull avg10=0.00 avg60=0.00 avg300=0.00 total=0\n",
    ) ?? PsiSnapshot{};

    // 1. Nominal (50% memory, low PSI)
    const a_nom = evaluateCgroupBudget(cpu, mem, 500_000, calm_psi, 16);
    expect_eq(a_nom.pressure_level, CgroupPressureLevel.nominal);
    expect_eq(a_nom.memory_util_bps, @as(u32, 5000));
    expect_eq(a_nom.headroom_bytes, @as(u64, 500_000));
    expect_eq(a_nom.effective_workers, @as(usize, 8));
    expect_true(!a_nom.should_pause_background);
    expect_true(!a_nom.should_shed_load);

    // 2. Elevated (75% memory) -> pause background, scale workers to 75% (6)
    const a_elev = evaluateCgroupBudget(cpu, mem, 750_000, calm_psi, 16);
    expect_eq(a_elev.pressure_level, CgroupPressureLevel.elevated);
    expect_eq(a_elev.effective_workers, @as(usize, 6));
    expect_true(a_elev.should_pause_background);
    expect_true(!a_elev.should_shed_load);

    // 3. Critical (88% memory) -> pause background + shed load, halve workers (4)
    const a_crit = evaluateCgroupBudget(cpu, mem, 880_000, calm_psi, 16);
    expect_eq(a_crit.pressure_level, CgroupPressureLevel.critical);
    expect_eq(a_crit.effective_workers, @as(usize, 4));
    expect_true(a_crit.should_pause_background);
    expect_true(a_crit.should_shed_load);

    // 4. OOM imminent (96% memory) -> clamp workers to 1, shed load + pause background
    const a_oom = evaluateCgroupBudget(cpu, mem, 960_000, calm_psi, 16);
    expect_eq(a_oom.pressure_level, CgroupPressureLevel.oom_imminent);
    expect_eq(a_oom.effective_workers, @as(usize, 1));
    expect_eq(a_oom.headroom_bytes, @as(u64, 40_000));
    expect_true(a_oom.should_pause_background);
    expect_true(a_oom.should_shed_load);
}

test("CgroupLimit: unlimited memory with PSI-driven escalation & cap.sys facade parity") {
    const cpu = cap.sys.parseCgroupCpuMax("400000 100000") ?? CgroupCpuQuota{};
    const mem_unlim = cap.sys.parseCgroupMemoryValue("max") ?? CgroupMemoryLimit{};
    const heavy_psi = cap.sys.parsePsiPressure(
        "some avg10=45.00 avg60=30.00 avg300=10.00 total=99999\nfull avg10=28.50 avg60=15.00 avg300=5.00 total=50000\n",
    ) ?? PsiSnapshot{};

    // Even with unlimited memory, full avg10=28.50% (>= 25%) escalates to oom_imminent
    const adv = cap.sys.evaluateCgroupBudget(cpu, mem_unlim, 512_000_000, heavy_psi, 8);
    expect_eq(adv.pressure_level, cap.sys.CgroupPressureLevel.oom_imminent);
    expect_eq(adv.memory_util_bps, @as(u32, 0));
    expect_eq(adv.effective_workers, @as(usize, 1));
    expect_true(adv.should_shed_load);
    expect_true(adv.should_pause_background);
}
