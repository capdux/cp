// Source: Absorbed from /data/dev/bun/src/runtime/node/StatFS.cp | Synced: 2026-11-22T15:30:49Z
// Zero-allocation POSIX / Node `fs.statfs` filesystem capacity & inode statistics model.

/// Common Linux `statfs(2)` filesystem magic numbers (`f_type`).
pub const FS_MAGIC_EXT4: i64 = 0xEF53;
pub const FS_MAGIC_TMPFS: i64 = 0x01021994;
pub const FS_MAGIC_BTRFS: i64 = 0x9123683E;
pub const FS_MAGIC_XFS: i64 = 0x58465342;
pub const FS_MAGIC_OVERLAYFS: i64 = 0x794C7630;
pub const FS_MAGIC_NFS: i64 = 0x6969;
pub const FS_MAGIC_PROC: i64 = 0x9FA0;
pub const FS_MAGIC_SYSFS: i64 = 0x62656572;
pub const FS_MAGIC_ZFS: i64 = 0x2FC12FC1;

/// Validation error set for `StatFsBig`.
pub err StatFsError {
    InvalidBlockSize,
    InconsistentCounts,
}

/// Map a Linux `statfs(2)` `f_type` magic number to a human-readable filesystem name.
pub fn statFsTypeName(fstype: i64): []const u8 {
    return switch (fstype) {
        FS_MAGIC_EXT4 => "ext4",
        FS_MAGIC_TMPFS => "tmpfs",
        FS_MAGIC_BTRFS => "btrfs",
        FS_MAGIC_XFS => "xfs",
        FS_MAGIC_OVERLAYFS => "overlayfs",
        FS_MAGIC_NFS => "nfs",
        FS_MAGIC_PROC => "proc",
        FS_MAGIC_SYSFS => "sysfs",
        FS_MAGIC_ZFS => "zfs",
        else => "unknown",
    };
}

/// 32-bit compact `StatFS` snapshot (mirrors Bun `StatFSSmall`).
pub struct StatFsSmall {
    fstype: i32 = 0,
    bsize: i32 = 0,
    blocks: i32 = 0,
    bfree: i32 = 0,
    bavail: i32 = 0,
    files: i32 = 0,
    ffree: i32 = 0,
}

/// 64-bit `StatFS` snapshot (mirrors Bun `StatFSBig` / Node `BigIntStatsFs`).
/// Common cross-platform fields shared between Linux and macOS.
pub struct StatFsBig {
    /// Filesystem type magic (`f_type`).
    fstype: i64 = 0,
    /// Optimal transfer block size in bytes (`f_bsize`).
    bsize: i64 = 0,
    /// Total data blocks in filesystem (`f_blocks`).
    blocks: i64 = 0,
    /// Free blocks in filesystem (`f_bfree`).
    bfree: i64 = 0,
    /// Free blocks available to unprivileged users (`f_bavail`).
    bavail: i64 = 0,
    /// Total file nodes / inodes in filesystem (`f_files`).
    files: i64 = 0,
    /// Free file nodes / inodes in filesystem (`f_ffree`).
    ffree: i64 = 0,

    /// Validate structural invariants (`bsize > 0`, non-negative counts, `bavail <= bfree <= blocks`, `ffree <= files`).
    fn validate(): bool ! StatFsError {
        if (_.bsize <= 0) return err.InvalidBlockSize;
        if (_.blocks < 0 || _.bfree < 0 || _.bavail < 0 || _.files < 0 || _.ffree < 0) {
            return err.InconsistentCounts;
        }
        if (_.bfree > _.blocks || _.bavail > _.bfree || _.ffree > _.files) {
            return err.InconsistentCounts;
        }
        return true;
    }

    /// Truncate to 32-bit `StatFsSmall` (matches Bun `StatFSType(false)` truncation semantics).
    fn toSmall(): StatFsSmall {
        return StatFsSmall{
            fstype: @truncate(_.fstype),
            bsize: @truncate(_.bsize),
            blocks: @truncate(_.blocks),
            bfree: @truncate(_.bfree),
            bavail: @truncate(_.bavail),
            files: @truncate(_.files),
            ffree: @truncate(_.ffree),
        };
    }

    /// Human-readable filesystem name derived from `fstype`.
    fn fsTypeName(): []const u8 {
        return statFsTypeName(_.fstype);
    }

    /// Total filesystem capacity in bytes (`blocks * bsize`).
    fn totalBytes(): u128 {
        if (_.bsize <= 0 || _.blocks <= 0) return 0;
        return u128(_.blocks) * u128(_.bsize);
    }

    /// Total free bytes including root-reserved blocks (`bfree * bsize`).
    fn freeBytes(): u128 {
        if (_.bsize <= 0 || _.bfree <= 0) return 0;
        return u128(_.bfree) * u128(_.bsize);
    }

    /// Free bytes available to unprivileged users (`bavail * bsize`).
    fn availableBytes(): u128 {
        if (_.bsize <= 0 || _.bavail <= 0) return 0;
        return u128(_.bavail) * u128(_.bsize);
    }

    /// Used bytes (`(blocks - bfree) * bsize`, saturating at 0).
    fn usedBytes(): u128 {
        if (_.bsize <= 0 || _.blocks <= _.bfree) return 0;
        return u128(_.blocks - _.bfree) * u128(_.bsize);
    }

    /// Block utilization in permille (`0..=1000`, 0 when `blocks <= 0`).
    fn usedPermille(): u32 {
        if (_.blocks <= 0) return 0;
        const used_blocks: i64 = if (_.blocks > _.bfree) (_.blocks - _.bfree) else 0;
        return u32((u128(used_blocks) * 1000) / u128(_.blocks));
    }

    /// Unprivileged available space ratio in permille (`0..=1000`, 0 when `blocks <= 0`).
    fn availablePermille(): u32 {
        if (_.blocks <= 0 || _.bavail <= 0) return 0;
        const clamped: i64 = if (_.bavail > _.blocks) _.blocks else _.bavail;
        return u32((u128(clamped) * 1000) / u128(_.blocks));
    }

    /// Inode utilization in permille (`0..=1000`, 0 when `files <= 0`).
    fn inodeUsedPermille(): u32 {
        if (_.files <= 0) return 0;
        const used_files: i64 = if (_.files > _.ffree) (_.files - _.ffree) else 0;
        return u32((u128(used_files) * 1000) / u128(_.files));
    }

    /// Return true when unprivileged available space is strictly below `min_avail_permille`.
    fn isLowSpace(min_avail_permille: u32): bool {
        return _.availablePermille() < min_avail_permille;
    }
}
