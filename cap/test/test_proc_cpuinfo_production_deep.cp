// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/cpuinfo Zero-Alloc Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CpuBlock,
    CpuSummary,
    ProcCpuInfoIter,
    summarizeCpuInfo,
    hasCpuFlag,
} from "cap:sys";

test("1. ProcCpuInfoIter parses x86_64 multi-socket /proc/cpuinfo blocks with MHz and cache units", () => {
    const sample =
        "processor\t: 0\n"
        ++ "vendor_id\t: GenuineIntel\n"
        ++ "model name\t: Intel(R) Xeon(R) Platinum 8375C CPU @ 2.90GHz\n"
        ++ "cpu MHz\t\t: 2899.998\n"
        ++ "cache size\t: 55296 KB\n"
        ++ "physical id\t: 0\n"
        ++ "cpu cores\t: 4\n"
        ++ "flags\t\t: fpu vme de pse tsc sse4_2 avx2 aes\n"
        ++ "\n"
        ++ "processor\t: 1\n"
        ++ "vendor_id\t: GenuineIntel\n"
        ++ "model name\t: Intel(R) Xeon(R) Platinum 8375C CPU @ 2.90GHz\n"
        ++ "cpu MHz\t\t: 3100.5\n"
        ++ "cache size\t: 16 MB\n"
        ++ "physical id\t: 1\n"
        ++ "cpu cores\t: 4\n"
        ++ "flags\t\t: fpu sse4_2 avx2\r\n";

    let it = ProcCpuInfoIter.init(sample);

    const c0 = it.next() ?? unreachable;
    expect_eq(c0.processor, 0);
    expect_eq(c0.physical_id, 0);
    expect_eq(c0.cpu_cores, 4);
    expect_eq(std.mem.eql(u8, c0.vendor_id, "GenuineIntel"), true);
    expect_eq(std.mem.eql(u8, c0.model_name, "Intel(R) Xeon(R) Platinum 8375C CPU @ 2.90GHz"), true);
    expect_eq(c0.cpu_mhz_x1000, 2899998);
    expect_eq(c0.cache_kb, 55296);
    expect_eq(c0.hasFlag("avx2"), true);
    expect_eq(c0.hasFlag("aes"), true);
    expect_eq(c0.hasFlag("avx"), false);

    const c1 = it.next() ?? unreachable;
    expect_eq(c1.processor, 1);
    expect_eq(c1.physical_id, 1);
    expect_eq(c1.cpu_mhz_x1000, 3100500);
    expect_eq(c1.cache_kb, 16384);
    expect_eq(c1.hasFlag("aes"), false);

    expect_eq(it.next() == null, true);
});

test("2. summarizeCpuInfo deduplicates physical_id across hyperthreaded multi-socket CPUs", () => {
    const sample =
        "processor : 0\nphysical id : 0\ncpu cores : 2\nmodel name : AMD EPYC\ncache size : 512 KB\n\n"
        ++ "processor : 1\nphysical id : 0\ncpu cores : 2\nmodel name : AMD EPYC\ncache size : 512 KB\n\n"
        ++ "processor : 2\nphysical id : 1\ncpu cores : 2\nmodel name : AMD EPYC\ncache size : 512 KB\n\n"
        ++ "processor : 3\nphysical id : 1\ncpu cores : 2\nmodel name : AMD EPYC\ncache size : 512 KB\n";

    const s = summarizeCpuInfo(sample);
    expect_eq(s.logical_cpus, 4);
    expect_eq(s.physical_cores, 4);
    expect_eq(std.mem.eql(u8, s.model_name, "AMD EPYC"), true);
    expect_eq(s.cache_kb, 512);
});

test("3. ARM64 /proc/cpuinfo Features, CPU implementer, and fallback physical_cores", () => {
    const arm64 =
        "processor\t: 0\n"
        ++ "BogoMIPS\t: 48.00\n"
        ++ "Features\t: fp asimd evtstrm aes pmull sha1 sha2 crc32 neon\n"
        ++ "CPU implementer\t: 0x41\n"
        ++ "CPU architecture: 8\n"
        ++ "Model\t\t: Apple M2 / Neoverse-N1\n"
        ++ "\n"
        ++ "processor\t: 1\n"
        ++ "Features\t: fp asimd aes neon\n"
        ++ "CPU implementer\t: 0x41\n";

    let it = ProcCpuInfoIter.init(arm64);
    const a0 = it.next() ?? unreachable;
    expect_eq(a0.processor, 0);
    expect_eq(std.mem.eql(u8, a0.vendor_id, "0x41"), true);
    expect_eq(std.mem.eql(u8, a0.model_name, "Apple M2 / Neoverse-N1"), true);
    expect_eq(a0.hasFlag("neon"), true);
    expect_eq(a0.hasFlag("sha2"), true);
    expect_eq(a0.hasFlag("sha"), false);

    const summary = summarizeCpuInfo(arm64);
    expect_eq(summary.logical_cpus, 2);
    expect_eq(summary.physical_cores, 2);
    expect_eq(std.mem.eql(u8, summary.model_name, "Apple M2 / Neoverse-N1"), true);
});

test("4. hasCpuFlag performs exact whitespace-delimited token matching without substring false positives", () => {
    const flags = "fpu sse sse2 sse4_1 sse4_2 avx2 avx512f aes-ni";
    expect_eq(hasCpuFlag(flags, "fpu"), true);
    expect_eq(hasCpuFlag(flags, "sse4_2"), true);
    expect_eq(hasCpuFlag(flags, "avx2"), true);
    expect_eq(hasCpuFlag(flags, "aes-ni"), true);

    expect_eq(hasCpuFlag(flags, "avx"), false);
    expect_eq(hasCpuFlag(flags, "avx512"), false);
    expect_eq(hasCpuFlag(flags, "sse4"), false);
    expect_eq(hasCpuFlag(flags, "aes"), false);
    expect_eq(hasCpuFlag(flags, ""), false);
    expect_eq(hasCpuFlag("", "fpu"), false);
});

test("5. Compact blocks without blank separators, trailing non-processor metadata, and malformed inputs", () => {
    const compact =
        "garbage line without colon\n"
        ++ "processor : 0\n"
        ++ "cpu MHz : 2400\n"
        ++ "processor : 1\n"
        ++ "cpu MHz : 2500.123456\n"
        ++ "\n"
        ++ "Hardware\t: Generic DT\n"
        ++ "Serial\t\t: 00000000\n";

    let it = ProcCpuInfoIter.init(compact);
    const b0 = it.next() ?? unreachable;
    expect_eq(b0.processor, 0);
    expect_eq(b0.cpu_mhz_x1000, 2400000);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.processor, 1);
    expect_eq(b1.cpu_mhz_x1000, 2500123);

    expect_eq(it.next() == null, true);

    const empty_sum = summarizeCpuInfo("");
    expect_eq(empty_sum.logical_cpus, 0);
    expect_eq(empty_sum.physical_cores, 0);
    expect_eq(empty_sum.model_name.len, 0);
    expect_eq(empty_sum.cache_kb, 0);
});
