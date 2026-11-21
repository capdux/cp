// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/limits Zero-Alloc Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LimitValue,
    ProcLimitEntry,
    ProcLimitsIter,
    parseLimitValue,
    findProcLimit,
    maxOpenFiles,
} from "cap:proc_limits";

test("1. parseLimitValue parses unlimited, integers, and rejects negative/overflow inputs", () => {
    const u = parseLimitValue("unlimited") ?? unreachable;
    expect_eq(u.is_unlimited, true);
    expect_eq(u.value, 0);

    const n = parseLimitValue(" 1048576 ") ?? unreachable;
    expect_eq(n.is_unlimited, false);
    expect_eq(n.value, 1048576);

    const z = parseLimitValue("0") ?? unreachable;
    expect_eq(z.is_unlimited, false);
    expect_eq(z.value, 0);

    expect_eq(parseLimitValue("") == null, true);
    expect_eq(parseLimitValue("-1") == null, true);
    expect_eq(parseLimitValue("123k") == null, true);
    expect_eq(parseLimitValue("99999999999999999999999") == null, true);
});

test("2. ProcLimitsIter skips header and parses spaced names, soft/hard limits, and optional units", () => {
    const sample =
        "Limit                     Soft Limit           Hard Limit           Units     \n"
        ++ "Max cpu time              unlimited            unlimited            seconds   \n"
        ++ "Max stack size            8388608              unlimited            bytes     \n"
        ++ "Max open files            1048576              1048576              files     \n"
        ++ "Max nice priority         0                    0                    \r\n"
        ++ "Max pending signals       63172                63172\n";

    let it = ProcLimitsIter.init(sample);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.name, "Max cpu time"), true);
    expect_eq(e1.soft.is_unlimited, true);
    expect_eq(e1.hard.is_unlimited, true);
    expect_eq(std.mem.eql(u8, e1.units, "seconds"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.name, "Max stack size"), true);
    expect_eq(e2.soft.is_unlimited, false);
    expect_eq(e2.soft.value, 8388608);
    expect_eq(e2.hard.is_unlimited, true);
    expect_eq(std.mem.eql(u8, e2.units, "bytes"), true);

    const e3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.name, "Max open files"), true);
    expect_eq(e3.soft.value, 1048576);
    expect_eq(e3.hard.value, 1048576);
    expect_eq(std.mem.eql(u8, e3.units, "files"), true);

    const e4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.name, "Max nice priority"), true);
    expect_eq(e4.soft.value, 0);
    expect_eq(e4.hard.value, 0);
    expect_eq(e4.units.len, 0);

    const e5 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e5.name, "Max pending signals"), true);
    expect_eq(e5.soft.value, 63172);
    expect_eq(e5.hard.value, 63172);
    expect_eq(e5.units.len, 0);

    expect_eq(it.next() == null, true);
});

test("3. findProcLimit matches case-insensitively and returns null when absent", () => {
    const sample =
        "Limit                     Soft Limit           Hard Limit           Units\n"
        ++ "Max locked memory         65536                131072               bytes\n"
        ++ "Max core file size        0                    unlimited            bytes\n";

    const mem = findProcLimit(sample, "max locked memory") ?? unreachable;
    expect_eq(mem.soft.value, 65536);
    expect_eq(mem.hard.value, 131072);

    const core = findProcLimit(sample, "MAX CORE FILE SIZE") ?? unreachable;
    expect_eq(core.soft.value, 0);
    expect_eq(core.hard.is_unlimited, true);

    expect_eq(findProcLimit(sample, "Max unknown") == null, true);
    expect_eq(findProcLimit(sample, "") == null, true);
});

test("4. maxOpenFiles extracts soft open-files limit directly", () => {
    const sample =
        "Limit                     Soft Limit           Hard Limit           Units\n"
        ++ "Max processes             63172                63172                processes\n"
        ++ "Max open files            65535                1048576              files\n";

    const nofile = maxOpenFiles(sample) ?? unreachable;
    expect_eq(nofile.is_unlimited, false);
    expect_eq(nofile.value, 65535);

    expect_eq(maxOpenFiles("Limit  Soft Limit  Hard Limit\n") == null, true);
});

test("5. Tab-separated rows, malformed rows, and empty inputs are safely handled", () => {
    const mixed =
        "Limit\tSoft Limit\tHard Limit\tUnits\n"
        ++ "Broken single column line\n"
        ++ "Bad values\tnot_a_num\t100\tbytes\n"
        ++ "Max open files\tunlimited\tunlimited\tfiles";

    const nofile = maxOpenFiles(mixed) ?? unreachable;
    expect_eq(nofile.is_unlimited, true);
    expect_eq(maxOpenFiles("") == null, true);
});
