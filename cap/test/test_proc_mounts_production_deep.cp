// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/mounts Zero-Alloc Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MountEntry,
    ProcMountsIter,
    unescapeMountPath,
    findMountForPath,
} from "cap:proc_mounts";

test("1. ProcMountsIter skips blank/comment lines and parses standard 6-column entries and options", () => {
    const sample =
        "# /proc/mounts snapshot\n"
        ++ "\n"
        ++ "/dev/sda1 / ext4 rw,relatime,errors=remount-ro 0 1\n"
        ++ "tmpfs /run/user/1000 tmpfs ro,nosuid,nodev,size=65536k,mode=700 0 0\r\n";

    let it = ProcMountsIter.init(sample);

    const m1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m1.source, "/dev/sda1"), true);
    expect_eq(std.mem.eql(u8, m1.target, "/"), true);
    expect_eq(std.mem.eql(u8, m1.fs_type, "ext4"), true);
    expect_eq(m1.dump_freq, 0);
    expect_eq(m1.pass_num, 1);
    expect_eq(m1.isReadOnly(), false);
    expect_eq(m1.hasOption("rw"), true);
    expect_eq(m1.hasOption("ro"), false);
    expect_eq(m1.hasOption("errors"), true);
    expect_eq(std.mem.eql(u8, m1.getOptionValue("errors") ?? unreachable, "remount-ro"), true);
    expect_eq(m1.getOptionValue("missing") == null, true);

    const m2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m2.fs_type, "tmpfs"), true);
    expect_eq(m2.isReadOnly(), true);
    expect_eq(m2.hasOption("nosuid"), true);
    expect_eq(m2.hasOption("noexec"), false);
    expect_eq(std.mem.eql(u8, m2.getOptionValue("size") ?? unreachable, "65536k"), true);
    expect_eq(std.mem.eql(u8, m2.getOptionValue("mode") ?? unreachable, "700"), true);

    expect_eq(it.next() == null, true);
});

test("2. unescapeMountPath decodes octal escapes (\\040, \\011, \\012, \\134) and guards buffer size", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const p1 = unescapeMountPath("/mnt/my\\040drive\\011tab\\012nl\\134slash", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, p1, "/mnt/my drive\ttab\nnl\\slash"), true);

    // 非合法八进制序列原样保留
    const p2 = unescapeMountPath("/mnt/\\999/\\04", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, p2, "/mnt/\\999/\\04"), true);

    // 缓冲不足返回 null
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(unescapeMountPath("/mnt/my\\040drive", &tiny) == null, true);
});

test("3. findMountForPath performs longest path-boundary prefix matching (/var vs /var2 and root /)", () => {
    const mounts =
        "/dev/root / ext4 rw 0 1\n"
        ++ "/dev/sdb1 /var xfs rw 0 0\n"
        ++ "/dev/sdc1 /var/lib/docker overlay rw 0 0\n"
        ++ "/dev/sdd1 /mnt/my\\040disk vfat ro,noexec 0 0\n";

    // /var2 不应误匹配 /var，应回退到根挂载 /
    const m_var2 = findMountForPath(mounts, "/var2/data") ?? unreachable;
    expect_eq(std.mem.eql(u8, m_var2.target, "/"), true);
    expect_eq(std.mem.eql(u8, m_var2.fs_type, "ext4"), true);

    // /var/log 匹配 /var
    const m_var = findMountForPath(mounts, "/var/log/app.log") ?? unreachable;
    expect_eq(std.mem.eql(u8, m_var.target, "/var"), true);
    expect_eq(std.mem.eql(u8, m_var.fs_type, "xfs"), true);

    // /var/lib/docker/containers 命中更长前缀 /var/lib/docker
    const m_docker = findMountForPath(mounts, "/var/lib/docker/containers") ?? unreachable;
    expect_eq(std.mem.eql(u8, m_docker.fs_type, "overlay"), true);

    // 含八进制空格转义的挂载点也能准确命中
    const m_space = findMountForPath(mounts, "/mnt/my disk/photo.jpg") ?? unreachable;
    expect_eq(std.mem.eql(u8, m_space.fs_type, "vfat"), true);
    expect_eq(m_space.isReadOnly(), true);
});

test("4. Overlay/shadow mounts on the same target prefer the later mount entry", () => {
    const mounts =
        "/dev/sda1 /data ext4 ro 0 0\n"
        ++ "tmpfs /data tmpfs rw,size=128m 0 0\n";

    const m = findMountForPath(mounts, "/data/file.txt") ?? unreachable;
    expect_eq(std.mem.eql(u8, m.fs_type, "tmpfs"), true);
    expect_eq(m.isReadOnly(), false);
    expect_eq(std.mem.eql(u8, m.getOptionValue("size") ?? unreachable, "128m"), true);
});

test("5. Malformed lines, relative paths, and empty inputs are safely rejected", () => {
    const broken =
        "incomplete /only three\n"
        ++ "/dev/bad /bad ext4 rw not_a_num 0\n"
        ++ "/dev/good /good ext4 rw,ro 0 0";

    const m = findMountForPath(broken, "/good/sub") ?? unreachable;
    expect_eq(std.mem.eql(u8, m.source, "/dev/good"), true);
    expect_eq(m.isReadOnly(), true);

    expect_eq(findMountForPath(broken, "") == null, true);
    expect_eq(findMountForPath(broken, "relative/path") == null, true);
    expect_eq(findMountForPath("", "/any") == null, true);
});
