// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/status Zero-Alloc Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ProcStatus,
    parseProcStatus,
    findProcStatusField,
} from "cap:sys";

test("1. parseProcStatus decodes full /proc/<pid>/status including memory, threads, UID/GID, and ctxt switches", () => {
    const sample =
        "Name:\tcp-worker\n"
        ++ "Umask:\t0022\n"
        ++ "State:\tR (running)\n"
        ++ "Pid:\t4242\n"
        ++ "PPid:\t100\n"
        ++ "Uid:\t1000\t0\t0\t0\n"
        ++ "Gid:\t1000\t1001\t1001\t1001\n"
        ++ "VmPeak:\t  256000 kB\n"
        ++ "VmSize:\t  200000 kB\n"
        ++ "VmHWM:\t   65536 kB\n"
        ++ "VmRSS:\t   40960 kB\n"
        ++ "Threads:\t12\n"
        ++ "Cpus_allowed_list:\t0-7\n"
        ++ "voluntary_ctxt_switches:\t98765\n"
        ++ "nonvoluntary_ctxt_switches:\t4321\r\n";

    const st = parseProcStatus(sample) ?? unreachable;
    expect_eq(std.mem.eql(u8, st.name, "cp-worker"), true);
    expect_eq(st.state_code, 'R');
    expect_eq(st.isRunning(), true);
    expect_eq(st.isZombie(), false);
    expect_eq(st.pid, 4242);
    expect_eq(st.ppid, 100);
    expect_eq(st.threads, 12);
    expect_eq(st.uid_real, 1000);
    expect_eq(st.uid_effective, 0);
    expect_eq(st.gid_real, 1000);
    expect_eq(st.gid_effective, 1001);
    expect_eq(st.vm_peak_kb, 256000);
    expect_eq(st.vm_size_kb, 200000);
    expect_eq(st.vm_hwm_kb, 65536);
    expect_eq(st.vm_rss_kb, 40960);
    expect_eq(st.rssBytes(), 40960 * 1024);
    expect_eq(st.voluntary_ctxt_switches, 98765);
    expect_eq(st.nonvoluntary_ctxt_switches, 4321);
});

test("2. Zombie and kernel-thread minimal status (missing Vm* fields default to 0)", () => {
    const kthread =
        "Name:\tkworker/0:1\n"
        ++ "State:\tZ (zombie)\n"
        ++ "Pid:\t99\n"
        ++ "PPid:\t2\n"
        ++ "Threads:\t1\n";

    const st = parseProcStatus(kthread) ?? unreachable;
    expect_eq(std.mem.eql(u8, st.name, "kworker/0:1"), true);
    expect_eq(st.state_code, 'Z');
    expect_eq(st.isZombie(), true);
    expect_eq(st.isRunning(), false);
    expect_eq(st.vm_rss_kb, 0);
    expect_eq(st.rssBytes(), 0);
    expect_eq(st.voluntary_ctxt_switches, 0);
});

test("3. findProcStatusField extracts arbitrary custom fields zero-copy", () => {
    const sample =
        "Name:\tapp\n"
        ++ "Cpus_allowed_list:\t0-3,8-11   \n"
        ++ "Mems_allowed_list:\t0\n"
        ++ "Seccomp:\t2\n";

    expect_eq(std.mem.eql(u8, findProcStatusField(sample, "Cpus_allowed_list") ?? unreachable, "0-3,8-11"), true);
    expect_eq(std.mem.eql(u8, findProcStatusField(sample, "Seccomp") ?? unreachable, "2"), true);
    expect_eq(findProcStatusField(sample, "NonExistent") == null, true);
    expect_eq(findProcStatusField(sample, "") == null, true);
});

test("4. Supports space-separated values and MB/GB memory unit normalization", () => {
    const sample =
        "Name:   demo\n"
        ++ "State:  S (sleeping)\n"
        ++ "VmRSS:  16 MB\n"
        ++ "VmSize: 1 GB\n";

    const st = parseProcStatus(sample) ?? unreachable;
    expect_eq(st.vm_rss_kb, 16384);
    expect_eq(st.vm_size_kb, 1048576);
    expect_eq(st.rssBytes(), 16384 * 1024);
});

test("5. Empty content and garbage without recognized status fields return null", () => {
    expect_eq(parseProcStatus("") == null, true);
    expect_eq(parseProcStatus("   \n\t\n") == null, true);
    expect_eq(parseProcStatus("not a key value line\nUnknownKey: 123\n") == null, true);

    // 部分非法数值行被忽略，但合法字段仍正常解析
    const partial =
        "Pid:\tabc\n"
        ++ "Uid:\t1000\n"
        ++ "Name:\tsurvivor\n"
        ++ "Threads:\t4\n";
    const st = parseProcStatus(partial) ?? unreachable;
    expect_eq(std.mem.eql(u8, st.name, "survivor"), true);
    expect_eq(st.pid, 0);
    expect_eq(st.threads, 4);
});
