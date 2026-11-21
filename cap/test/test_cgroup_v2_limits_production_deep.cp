// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux cgroup v2 Resource Limits Parser)
import { test, expect_eq } from "cap:test";
import {
    CgroupLimit,
    CpuMaxQuota,
    IoMaxEntry,
    parseCgroupMax,
    parseCpuMax,
    parseIoMaxLine,
    effectiveMemoryLimit,
} from "cap:sys";

test("1. parseCgroupMax handles max, numeric limits, whitespace, and rejects invalid inputs", () => {
    const lim_max = parseCgroupMax("max\n") ?? unreachable;
    expect_eq(lim_max.is_unlimited, true);
    expect_eq(lim_max.value, 0);

    const lim_num = parseCgroupMax("  536870912 \r\n") ?? unreachable;
    expect_eq(lim_num.is_unlimited, false);
    expect_eq(lim_num.value, 536870912);

    const lim_zero = parseCgroupMax("0\n") ?? unreachable;
    expect_eq(lim_zero.is_unlimited, false);
    expect_eq(lim_zero.value, 0);

    expect_eq(parseCgroupMax("") == null, true);
    expect_eq(parseCgroupMax("   \n") == null, true);
    expect_eq(parseCgroupMax("-1\n") == null, true);
    expect_eq(parseCgroupMax("123abc\n") == null, true);
    expect_eq(parseCgroupMax("100 200\n") == null, true);
    expect_eq(parseCgroupMax("99999999999999999999999\n") == null, true);
});

test("2. parseCpuMax computes effectiveCpusCeil and effectiveMilliCpus accurately", () => {
    // 2.0 CPUs
    const c1 = parseCpuMax("200000 100000\n") ?? unreachable;
    expect_eq(c1.is_unlimited, false);
    expect_eq(c1.quota_us, 200000);
    expect_eq(c1.period_us, 100000);
    expect_eq(c1.effectiveCpusCeil(), 2);
    expect_eq(c1.effectiveMilliCpus(), 2000);

    // 1.5 CPUs -> ceil = 2, milli = 1500
    const c2 = parseCpuMax("150000 100000\n") ?? unreachable;
    expect_eq(c2.effectiveCpusCeil(), 2);
    expect_eq(c2.effectiveMilliCpus(), 1500);

    // Sub-1 CPU (0.25 CPU) -> ceil = 1, milli = 250
    const c3 = parseCpuMax("25000 100000\n") ?? unreachable;
    expect_eq(c3.effectiveCpusCeil(), 1);
    expect_eq(c3.effectiveMilliCpus(), 250);

    // Unlimited ("max 100000") -> ceil = 0, milli = 0
    const c_unlim = parseCpuMax("max 100000\n") ?? unreachable;
    expect_eq(c_unlim.is_unlimited, true);
    expect_eq(c_unlim.period_us, 100000);
    expect_eq(c_unlim.effectiveCpusCeil(), 0);
    expect_eq(c_unlim.effectiveMilliCpus(), 0);
});

test("3. parseCpuMax rejects zero period, missing fields, extra fields, and negative values", () => {
    expect_eq(parseCpuMax("100000 0\n") == null, true);
    expect_eq(parseCpuMax("max 0\n") == null, true);
    expect_eq(parseCpuMax("100000\n") == null, true);
    expect_eq(parseCpuMax("100000 100000 extra\n") == null, true);
    expect_eq(parseCpuMax("-100 100000\n") == null, true);
    expect_eq(parseCpuMax("") == null, true);
});

test("4. parseIoMaxLine parses device major:minor and rbps/wbps/riops/wiops limits", () => {
    const io1 = parseIoMaxLine("8:0 rbps=1048576 wbps=max riops=max wiops=1000\n") ?? unreachable;
    expect_eq(io1.major, 8);
    expect_eq(io1.minor, 0);
    expect_eq(io1.rbps.is_unlimited, false);
    expect_eq(io1.rbps.value, 1048576);
    expect_eq(io1.wbps.is_unlimited, true);
    expect_eq(io1.riops.is_unlimited, true);
    expect_eq(io1.wiops.is_unlimited, false);
    expect_eq(io1.wiops.value, 1000);

    // 部分字段省略时默认为 unlimited
    const io2 = parseIoMaxLine("259:16 wbps=2097152\r\n") ?? unreachable;
    expect_eq(io2.major, 259);
    expect_eq(io2.minor, 16);
    expect_eq(io2.rbps.is_unlimited, true);
    expect_eq(io2.wbps.value, 2097152);

    // 畸形 io.max 行返回 null
    expect_eq(parseIoMaxLine("") == null, true);
    expect_eq(parseIoMaxLine("8 rbps=100") == null, true);
    expect_eq(parseIoMaxLine("8:0 badtoken") == null, true);
    expect_eq(parseIoMaxLine("8:0 rbps=-5") == null, true);
});

test("5. effectiveMemoryLimit clamps container memory quota against host physical memory", () => {
    const host_16g: u64 = 17179869184;
    const lim_512m = parseCgroupMax("536870912\n") ?? unreachable;
    const lim_64g = parseCgroupMax("68719476736\n") ?? unreachable;
    const lim_unlim = parseCgroupMax("max\n") ?? unreachable;

    expect_eq(effectiveMemoryLimit(lim_512m, host_16g), 536870912);
    expect_eq(effectiveMemoryLimit(lim_64g, host_16g), host_16g);
    expect_eq(effectiveMemoryLimit(lim_unlim, host_16g), host_16g);
});
