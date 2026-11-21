// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux sysctl.conf Zero-Alloc Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SysctlEntry,
    SysctlIter,
    normalizeSysctlKey,
    sysctlKeysEqual,
    getSysctlValue,
} from "cap:sysctl_conf";

test("1. SysctlIter skips blank and #/; comment lines, handles - ignore_errors prefix and multi-word values", () => {
    const conf =
        "# Linux sysctl config\n"
        ++ "  ; semicolon comment\n"
        ++ "\n"
        ++ "net.ipv4.ip_forward = 1\n"
        ++ "-net.ipv6.conf.all.disable_ipv6 = 0\r\n"
        ++ "net.ipv4.ip_local_port_range = 1024 65535\n"
        ++ "kernel.core_pattern = |/usr/bin/handler %p=%t\n";

    let it = SysctlIter.init(conf);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "net.ipv4.ip_forward"), true);
    expect_eq(std.mem.eql(u8, e1.value, "1"), true);
    expect_eq(e1.ignore_errors, false);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "net.ipv6.conf.all.disable_ipv6"), true);
    expect_eq(std.mem.eql(u8, e2.value, "0"), true);
    expect_eq(e2.ignore_errors, true);

    const e3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "net.ipv4.ip_local_port_range"), true);
    expect_eq(std.mem.eql(u8, e3.value, "1024 65535"), true);

    const e4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.key, "kernel.core_pattern"), true);
    expect_eq(std.mem.eql(u8, e4.value, "|/usr/bin/handler %p=%t"), true);

    expect_eq(it.next() == null, true);
});

test("2. normalizeSysctlKey converts slashes to dots and rejects empty segments or small buffers", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    expect_eq(
        std.mem.eql(u8, normalizeSysctlKey("net/ipv4/ip_forward", &buf) ?? unreachable, "net.ipv4.ip_forward"),
        true,
    );
    expect_eq(
        std.mem.eql(u8, normalizeSysctlKey("vm.swappiness", &buf) ?? unreachable, "vm.swappiness"),
        true,
    );

    expect_eq(normalizeSysctlKey("", &buf) == null, true);
    expect_eq(normalizeSysctlKey(".leading.dot", &buf) == null, true);
    expect_eq(normalizeSysctlKey("trailing.slash/", &buf) == null, true);
    expect_eq(normalizeSysctlKey("net..ipv4", &buf) == null, true);
    expect_eq(normalizeSysctlKey("net//ipv4", &buf) == null, true);
    expect_eq(normalizeSysctlKey("net./ipv4", &buf) == null, true);
    expect_eq(normalizeSysctlKey("bad key", &buf) == null, true);

    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(normalizeSysctlKey("vm.swappiness", &tiny) == null, true);
});

test("3. sysctlKeysEqual treats . and / as equivalent separators zero-alloc", () => {
    expect_eq(sysctlKeysEqual("net.ipv4.ip_forward", "net/ipv4/ip_forward"), true);
    expect_eq(sysctlKeysEqual("net/ipv4.tcp_syncookies", "net.ipv4/tcp_syncookies"), true);
    expect_eq(sysctlKeysEqual("vm.swappiness", "vm.swappiness"), true);

    expect_eq(sysctlKeysEqual("vm.swappiness", "vm.dirty_ratio"), false);
    expect_eq(sysctlKeysEqual("net..ipv4", "net..ipv4"), false);
    expect_eq(sysctlKeysEqual("", ""), false);
});

test("4. getSysctlValue applies last-wins override across dot and slash key spellings", () => {
    const conf =
        "\xEF\xBB\xBF"
        ++ "net.ipv4.ip_forward = 0\n"
        ++ "vm.swappiness = 60\n"
        ++ "-net/ipv4/ip_forward = 1\n"
        ++ "vm/swappiness = 10\n";

    expect_eq(std.mem.eql(u8, getSysctlValue(conf, "net.ipv4.ip_forward") ?? unreachable, "1"), true);
    expect_eq(std.mem.eql(u8, getSysctlValue(conf, "net/ipv4/ip_forward") ?? unreachable, "1"), true);
    expect_eq(std.mem.eql(u8, getSysctlValue(conf, "vm.swappiness") ?? unreachable, "10"), true);
    expect_eq(getSysctlValue(conf, "kernel.missing") == null, true);
    expect_eq(getSysctlValue(conf, "bad..key") == null, true);
});

test("5. Malformed lines without =, empty keys, or invalid separators are safely skipped", () => {
    const weird =
        "no_equals_here\n"
        ++ "= orphan_value\n"
        ++ "- = empty_after_dash\n"
        ++ "net..broken = 1\n"
        ++ "fs.file-max = 2097152";

    let it = SysctlIter.init(weird);
    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "fs.file-max"), true);
    expect_eq(std.mem.eql(u8, e1.value, "2097152"), true);
    expect_eq(it.next() == null, true);
});
