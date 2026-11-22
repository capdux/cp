// Production deep regression tests for lib/sys/statFs.cp (cap:sys)

import { test, expect_eq } from "cap:test";
import {
    FS_MAGIC_EXT4,
    FS_MAGIC_TMPFS,
    FS_MAGIC_BTRFS,
    FS_MAGIC_XFS,
    FS_MAGIC_OVERLAYFS,
    FS_MAGIC_NFS,
    FS_MAGIC_PROC,
    FS_MAGIC_SYSFS,
    FS_MAGIC_ZFS,
    StatFsError,
    statFsTypeName,
    StatFsBig,
} from "cap:sys";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("statFs: filesystem magic number naming", () => {
    expect_eq(strEq(statFsTypeName(FS_MAGIC_EXT4), "ext4"), true);
    expect_eq(strEq(statFsTypeName(FS_MAGIC_TMPFS), "tmpfs"), true);
    expect_eq(strEq(statFsTypeName(FS_MAGIC_BTRFS), "btrfs"), true);
    expect_eq(strEq(statFsTypeName(FS_MAGIC_XFS), "xfs"), true);
    expect_eq(strEq(statFsTypeName(FS_MAGIC_OVERLAYFS), "overlayfs"), true);
    expect_eq(strEq(statFsTypeName(FS_MAGIC_NFS), "nfs"), true);
    expect_eq(strEq(statFsTypeName(FS_MAGIC_PROC), "proc"), true);
    expect_eq(strEq(statFsTypeName(FS_MAGIC_SYSFS), "sysfs"), true);
    expect_eq(strEq(statFsTypeName(FS_MAGIC_ZFS), "zfs"), true);
    expect_eq(strEq(statFsTypeName(0x123456), "unknown"), true);
});

test("statFs: byte capacity, utilization ratios & low-space alert", () => {
    const info = StatFsBig{
        fstype: FS_MAGIC_EXT4,
        bsize: 4096,
        blocks: 1000,
        bfree: 250,
        bavail: 100,
        files: 2000,
        ffree: 500,
    };

    const ok = info.validate() catch false;
    expect_eq(ok, true);
    expect_eq(strEq(info.fsTypeName(), "ext4"), true);

    expect_eq(info.totalBytes(), 4_096_000);
    expect_eq(info.freeBytes(), 1_024_000);
    expect_eq(info.availableBytes(), 409_600);
    expect_eq(info.usedBytes(), 3_072_000);

    expect_eq(info.usedPermille(), 750);
    expect_eq(info.availablePermille(), 100);
    expect_eq(info.inodeUsedPermille(), 750);

    // Threshold checks
    expect_eq(info.isLowSpace(100), false);
    expect_eq(info.isLowSpace(101), true);
});

test("statFs: 64-bit large disk & StatFsSmall truncation parity", () => {
    // 16 TiB disk with 4 KiB blocks => 4_294_967_296 blocks (exceeds i32!)
    const big = StatFsBig{
        fstype: FS_MAGIC_XFS,
        bsize: 4096,
        blocks: 4_294_967_296,
        bfree: 1_073_741_824,
        bavail: 1_073_741_824,
        files: 100_000_000,
        ffree: 90_000_000,
    };
    expect_eq(big.validate() catch false, true);
    expect_eq(big.usedPermille(), 750);
    expect_eq(big.availablePermille(), 250);
    expect_eq(big.inodeUsedPermille(), 100);

    // Small truncation preserves lower 32 bits (Bun StatFSSmall parity)
    const small = big.toSmall();
    expect_eq(small.bsize, 4096);
    expect_eq(small.blocks, 0);
    expect_eq(small.bfree, 1_073_741_824);
});

test("statFs: zero-safe calculations on empty/virtual filesystems", () => {
    const proc_fs = StatFsBig{
        fstype: FS_MAGIC_PROC,
        bsize: 4096,
        blocks: 0,
        bfree: 0,
        bavail: 0,
        files: 0,
        ffree: 0,
    };
    expect_eq(proc_fs.validate() catch false, true);
    expect_eq(proc_fs.totalBytes(), 0);
    expect_eq(proc_fs.usedPermille(), 0);
    expect_eq(proc_fs.availablePermille(), 0);
    expect_eq(proc_fs.inodeUsedPermille(), 0);
});

test("statFs: validation error guards (InvalidBlockSize & InconsistentCounts)", () => {
    const zero_bsize = StatFsBig{ bsize: 0, blocks: 10 };
    let e1 = false;
    * = zero_bsize.validate() catch |e| {
        e1 = (e == StatFsError.InvalidBlockSize);
        false
    };
    expect_eq(e1, true);

    const bad_bfree = StatFsBig{ bsize: 4096, blocks: 100, bfree: 120, bavail: 50 };
    let e2 = false;
    * = bad_bfree.validate() catch |e| {
        e2 = (e == StatFsError.InconsistentCounts);
        false
    };
    expect_eq(e2, true);

    const bad_bavail = StatFsBig{ bsize: 4096, blocks: 100, bfree: 50, bavail: 60 };
    let e3 = false;
    * = bad_bavail.validate() catch |e| {
        e3 = (e == StatFsError.InconsistentCounts);
        false
    };
    expect_eq(e3, true);

    const bad_inodes = StatFsBig{ bsize: 4096, blocks: 100, bfree: 50, bavail: 50, files: 10, ffree: 11 };
    let e4 = false;
    * = bad_inodes.validate() catch |e| {
        e4 = (e == StatFsError.InconsistentCounts);
        false
    };
    expect_eq(e4, true);
});
