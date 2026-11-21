import std from "std";
import { test, expect_eq } from "cap:test";
import { MountEntry, MountIter, unescapeMountPath, findMountForPath } from "cap:fstab_parse";

test("1. MountEntry.parseLine handles 4-field and 6-field lines, comments, and options", () => {
    const e1 = MountEntry.parseLine("/dev/nvme0n1p2  /  ext4  rw,relatime,errors=remount-ro  0  1") ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.device, "/dev/nvme0n1p2"), true);
    expect_eq(std.mem.eql(u8, e1.mount_point, "/"), true);
    expect_eq(std.mem.eql(u8, e1.fs_type, "ext4"), true);
    expect_eq(e1.dump_freq, u32(0));
    expect_eq(e1.pass_no, u32(1));
    expect_eq(e1.isReadOnly(), false);
    expect_eq(e1.hasOption("relatime"), true);
    expect_eq(e1.hasOption("errors"), true);
    expect_eq(std.mem.eql(u8, e1.getOptionValue("errors") ?? unreachable, "remount-ro"), true);
    expect_eq(e1.getOptionValue("missing") == null, true);

    // 省略最后两个数字字段时默认 0 0
    const e2 = MountEntry.parseLine("tmpfs /tmp tmpfs ro,noexec,nosuid,size=512m") ?? unreachable;
    expect_eq(e2.dump_freq, u32(0));
    expect_eq(e2.pass_no, u32(0));
    expect_eq(e2.isReadOnly(), true);
    expect_eq(e2.hasOption("noexec"), true);
    expect_eq(e2.hasOption("nosuid"), true);
    expect_eq(std.mem.eql(u8, e2.getOptionValue("size") ?? unreachable, "512m"), true);

    // 空行、注释行、少于 4 字段、非法数字拒绝
    expect_eq(MountEntry.parseLine("") == null, true);
    expect_eq(MountEntry.parseLine("   # comment line") == null, true);
    expect_eq(MountEntry.parseLine("/dev/sda1 / ext4") == null, true);
    expect_eq(MountEntry.parseLine("/dev/sda1 / ext4 rw badnum 1") == null, true);
});

test("2. MountIter streams multi-line /etc/fstab ignoring comments and blank lines", () => {
    const content =
        "# /etc/fstab\n"
        ++ "\n"
        ++ "UUID=1111 / ext4 rw,noatime 0 1\n"
        ++ "  # data disk\n"
        ++ "/dev/sdb1 /data xfs ro,noexec 0 2\n";

    let it = MountIter.init(content);
    const m0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m0.mount_point, "/"), true);
    const m1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m1.mount_point, "/data"), true);
    expect_eq(m1.isReadOnly(), true);
    expect_eq(it.next() == null, true);
});

test("3. unescapeMountPath decodes octal escapes (\\040, \\011, \\012, \\134)", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const decoded = unescapeMountPath("/mnt/My\\040Drive\\011Tab\\012NL\\134Slash", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, decoded, "/mnt/My Drive\tTab\nNL\\Slash"), true);

    // 缓冲区不足返回 null
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(unescapeMountPath("/mnt/data", &tiny) == null, true);
});

test("4. findMountForPath picks longest matching mount prefix and avoids false prefix matches", () => {
    const mounts =
        "/dev/sda1 / ext4 rw 0 1\n"
        ++ "/dev/sdb1 /data xfs rw 0 2\n"
        ++ "/dev/sdc1 /data/dev btrfs ro 0 0\n";

    // /data/dev/cp 命中 /data/dev（最长前缀）
    const m1 = findMountForPath(mounts, "/data/dev/cp") ?? unreachable;
    expect_eq(std.mem.eql(u8, m1.mount_point, "/data/dev"), true);
    expect_eq(m1.isReadOnly(), true);

    // /data/other 命中 /data
    const m2 = findMountForPath(mounts, "/data/other") ?? unreachable;
    expect_eq(std.mem.eql(u8, m2.mount_point, "/data"), true);

    // /database 不能误匹配 /data，应回退到 /
    const m3 = findMountForPath(mounts, "/database/file") ?? unreachable;
    expect_eq(std.mem.eql(u8, m3.mount_point, "/"), true);

    // 非绝对路径返回 null
    expect_eq(findMountForPath(mounts, "relative/path") == null, true);
});

test("5. ro/rw last-wins option precedence", () => {
    const e1 = MountEntry.parseLine("none /mnt tmpfs ro,rw 0 0") ?? unreachable;
    expect_eq(e1.isReadOnly(), false);

    const e2 = MountEntry.parseLine("none /mnt tmpfs rw,ro 0 0") ?? unreachable;
    expect_eq(e2.isReadOnly(), true);
});
