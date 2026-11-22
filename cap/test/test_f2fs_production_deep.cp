// Source: cp/cap/test/test_f2fs_production_deep.cp (Production deep tests for cap:f2fs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    F2FS_SUPER_OFFSET,
    F2FS_SUPER_MAGIC,
    F2FS_BLKSIZE,
    F2FS_SB_MIN_SIZE,
    F2FS_CP_MIN_SIZE,
    F2FS_SB_FEATURE_OFFSET,
    F2FS_SB_CRC_OFFSET,
    FEAT_ENCRYPT,
    FEAT_SB_CHKSUM,
    FEAT_COMPRESSION,
    CP_UMOUNT_FLAG,
    CP_ORPHAN_PRESENT_FLAG,
    CP_ERROR_FLAG,
    CP_FASTBOOT_FLAG,
    f2fsCrc32,
    isF2fsSuperblock,
    parseF2fsSuperblock,
    parseF2fsCheckpoint,
    selectActiveCheckpoint,
} from "cap:devops";

fn writeU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64Le(buf: []u8, off: usize, v: u64) {
    writeU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    writeU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn buildF2fsSuperblock(buf: []u8, volume_name: []const u8, feature: u32, block_count: u64) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
    writeU32Le(buf, 0, F2FS_SUPER_MAGIC);
    writeU16Le(buf, 4, 1);  // major_ver
    writeU16Le(buf, 6, 16); // minor_ver
    writeU32Le(buf, 8, 9);  // log_sectorsize = 9 (512B)
    writeU32Le(buf, 12, 3); // log_sectors_per_block = 3 (8 sectors/block)
    writeU32Le(buf, 16, 12); // log_blocksize = 12 (4096B)
    writeU32Le(buf, 20, 9); // log_blocks_per_seg = 9 (512 blocks/seg = 2MB)
    writeU32Le(buf, 24, 1); // segs_per_sec
    writeU32Le(buf, 28, 1); // secs_per_zone
    writeU32Le(buf, 32, F2FS_SB_CRC_OFFSET as u32);
    writeU64Le(buf, 36, block_count);
    writeU32Le(buf, 44, 500); // section_count
    writeU32Le(buf, 48, 512); // segment_count
    writeU32Le(buf, 52, 2);   // segment_count_ckpt
    writeU32Le(buf, 56, 2);   // segment_count_sit
    writeU32Le(buf, 60, 4);   // segment_count_nat
    writeU32Le(buf, 64, 4);   // segment_count_ssa
    writeU32Le(buf, 68, 500); // segment_count_main
    writeU32Le(buf, 72, 512); // segment0_blkaddr
    writeU32Le(buf, 76, 512); // cp_blkaddr
    writeU32Le(buf, 80, 1536); // sit_blkaddr
    writeU32Le(buf, 84, 2560); // nat_blkaddr
    writeU32Le(buf, 88, 4608); // ssa_blkaddr
    writeU32Le(buf, 92, 6656); // main_blkaddr
    writeU32Le(buf, 96, 3);    // root_ino
    writeU32Le(buf, 100, 1);   // node_ino
    writeU32Le(buf, 104, 2);   // meta_ino

    let g: usize = 0;
    while (g < 16) {
        buf[108 + g] = (0xC0 + g) as u8;
        g += 1;
    }

    let c: usize = 0;
    while (c < volume_name.len) {
        writeU16Le(buf, 124 + c * 2, volume_name[c] as u16);
        c += 1;
    }

    if (buf.len >= F2FS_SB_FEATURE_OFFSET + 4) {
        writeU32Le(buf, F2FS_SB_FEATURE_OFFSET, feature);
    }
    if ((feature & FEAT_SB_CHKSUM) != 0 && buf.len >= F2FS_SB_CRC_OFFSET + 4) {
        const crc = f2fsCrc32(F2FS_SUPER_MAGIC, buf[0..F2FS_SB_CRC_OFFSET]);
        writeU32Le(buf, F2FS_SB_CRC_OFFSET, crc);
    }
}

fn buildF2fsCheckpoint(buf: []u8, ver: u64, flags: u32, valid_blocks: u64, cksum_off: u32) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
    writeU64Le(buf, 0, ver);
    writeU64Le(buf, 8, 250000);       // user_block_count
    writeU64Le(buf, 16, valid_blocks);
    writeU32Le(buf, 24, 20);          // rsvd_segment_count
    writeU32Le(buf, 28, 30);          // overprov_segment_count
    writeU32Le(buf, 32, 420);         // free_segment_count
    writeU32Le(buf, 132, flags);
    writeU32Le(buf, 136, 4);          // cp_pack_total_block_count
    writeU32Le(buf, 140, 2);          // cp_pack_start_sum
    writeU32Le(buf, 144, 1500);       // valid_node_count
    writeU32Le(buf, 148, 1200);       // valid_inode_count
    writeU32Le(buf, 152, 8192);       // next_free_nid
    writeU32Le(buf, 164, cksum_off);

    const off: usize = cksum_off as usize;
    if (off + 4 <= buf.len) {
        const crc = f2fsCrc32(F2FS_SUPER_MAGIC, buf[0..off]);
        writeU32Le(buf, off, crc);
    }
}

test("1. F2FS Superblock full parsing, UTF-16LE volume name decode, geometry, and FEAT_SB_CHKSUM CRC32", () => {
    expect_eq(F2FS_SUPER_OFFSET, 1024);
    expect_eq(F2FS_SUPER_MAGIC, 0xF2F52010 as u32);
    expect_eq(F2FS_BLKSIZE, 4096);
    expect_eq(F2FS_SB_MIN_SIZE, 1024);
    expect_eq(F2FS_CP_MIN_SIZE, 192);

    // Standard IEEE 802.3 CRC32 check: crc32_le(0xFFFFFFFF, "123456789") ^ 0xFFFFFFFF == 0xCBF43926
    expect_eq(f2fsCrc32(0xFFFFFFFF, "123456789") ^ 0xFFFFFFFF, 0xCBF43926 as u32);

    let sb_buf: [3072]u8 = [0; 3072];
    const feats = FEAT_ENCRYPT | FEAT_COMPRESSION | FEAT_SB_CHKSUM;
    buildF2fsSuperblock(sb_buf[0..], "AndroidData", feats, 262144);

    expect_eq(isF2fsSuperblock(sb_buf[0..]), true);
    const sb = parseF2fsSuperblock(sb_buf[0..]) ?? unreachable;
    expect_eq(sb.major_ver, 1 as u16);
    expect_eq(sb.minor_ver, 16 as u16);
    expect_eq(sb.sectorSize(), 512 as u32);
    expect_eq(sb.blockSize(), 4096 as u32);
    expect_eq(sb.segmentSizeBytes(), 2097152 as u64); // 512 * 4096 = 2MB
    expect_eq(sb.totalBytes(), (262144 as u128) * (4096 as u128)); // 1 GiB
    expect_eq(sb.root_ino, 3 as u32);
    expect_eq(sb.node_ino, 1 as u32);
    expect_eq(sb.meta_ino, 2 as u32);
    expect_eq(sb.uuid[0], 0xC0 as u8);
    expect_eq(sb.hasEncrypt(), true);
    expect_eq(sb.hasCompression(), true);
    expect_eq(sb.hasSbChecksum(), true);

    let vol_out: [32]u8 = [0; 32];
    const n = sb.decodeVolumeNameAscii(vol_out[0..]);
    expect_eq(n, 11);
    expect_eq(std.mem.eql(u8, vol_out[0..n], "AndroidData"), true);
});

test("2. Dual F2FS Checkpoint parsing, CRC32 verification, flags, and selectActiveCheckpoint election", () => {
    let cp1_buf: [4096]u8 = [0; 4096];
    let cp2_buf: [4096]u8 = [0; 4096];

    buildF2fsCheckpoint(cp1_buf[0..], 42, CP_UMOUNT_FLAG | CP_FASTBOOT_FLAG, 10000, 4092);
    buildF2fsCheckpoint(cp2_buf[0..], 43, CP_ORPHAN_PRESENT_FLAG | CP_ERROR_FLAG, 10500, 4092);

    const cp1 = parseF2fsCheckpoint(cp1_buf[0..]);
    const cp2 = parseF2fsCheckpoint(cp2_buf[0..]);
    expect_eq(cp1 != null, true);
    expect_eq(cp2 != null, true);

    const c1 = cp1 ?? unreachable;
    expect_eq(c1.checkpoint_ver, 42 as u64);
    expect_eq(c1.valid_block_count, 10000 as u64);
    expect_eq(c1.isCleanUnmount(), true);
    expect_eq(c1.isFastboot(), true);
    expect_eq(c1.hasOrphans(), false);
    expect_eq(c1.hasError(), false);

    const c2 = cp2 ?? unreachable;
    expect_eq(c2.isCleanUnmount(), false);
    expect_eq(c2.hasOrphans(), true);
    expect_eq(c2.hasError(), true);

    // Higher checkpoint_ver wins
    const active = selectActiveCheckpoint(cp1, cp2) ?? unreachable;
    expect_eq(active.checkpoint_ver, 43 as u64);
    expect_eq(active.valid_block_count, 10500 as u64);

    // Reverse argument order
    const active_rev = selectActiveCheckpoint(cp2, cp1) ?? unreachable;
    expect_eq(active_rev.checkpoint_ver, 43 as u64);

    // Single valid / both invalid
    expect_eq((selectActiveCheckpoint(cp1, null) ?? unreachable).checkpoint_ver, 42 as u64);
    expect_eq((selectActiveCheckpoint(null, cp2) ?? unreachable).checkpoint_ver, 43 as u64);
    expect_eq(selectActiveCheckpoint(null, null) == null, true);
});

test("3. Minimal 1024B superblock without FEAT_SB_CHKSUM and non-ASCII volume name fallback", () => {
    let min_sb: [1024]u8 = [0; 1024];
    buildF2fsSuperblock(min_sb[0..], "Disk", 0, 65536);
    // Append non-ASCII UTF-16LE code unit 0x3042 after "Disk"
    writeU16Le(min_sb[0..], 124 + 8, 0x3042);

    const sb = parseF2fsSuperblock(min_sb[0..]) ?? unreachable;
    expect_eq(sb.hasSbChecksum(), false);
    expect_eq(sb.hasEncrypt(), false);
    expect_eq(sb.hasCompression(), false);

    let out: [16]u8 = [0; 16];
    const n = sb.decodeVolumeNameAscii(out[0..]);
    expect_eq(n, 5);
    expect_eq(std.mem.eql(u8, out[0..n], "Disk?"), true);
});

test("4. Malformed F2FS Superblock rejection: bad magic, bad log_blocksize, sector mismatch, bad SB CRC32, truncation", () => {
    // Truncated buffer
    let short_buf: [64]u8 = [0; 64];
    expect_eq(isF2fsSuperblock(short_buf[0..]), false);
    expect_eq(parseF2fsSuperblock(short_buf[0..]) == null, true);

    let sb_buf: [3072]u8 = [0; 3072];

    // Bad magic
    buildF2fsSuperblock(sb_buf[0..], "Test", FEAT_SB_CHKSUM, 10000);
    writeU32Le(sb_buf[0..], 0, 0xDEADBEEF);
    expect_eq(isF2fsSuperblock(sb_buf[0..]), false);
    expect_eq(parseF2fsSuperblock(sb_buf[0..]) == null, true);

    // Invalid log_blocksize != 12
    buildF2fsSuperblock(sb_buf[0..], "Test", 0, 10000);
    writeU32Le(sb_buf[0..], 16, 13);
    expect_eq(parseF2fsSuperblock(sb_buf[0..]) == null, true);

    // log_sectorsize + log_sectors_per_block != log_blocksize (9 + 2 != 12)
    buildF2fsSuperblock(sb_buf[0..], "Test", 0, 10000);
    writeU32Le(sb_buf[0..], 12, 2);
    expect_eq(parseF2fsSuperblock(sb_buf[0..]) == null, true);

    // Corrupted SB CRC32 when FEAT_SB_CHKSUM is set
    buildF2fsSuperblock(sb_buf[0..], "Test", FEAT_SB_CHKSUM, 10000);
    sb_buf[100] ^= 0x01;
    expect_eq(parseF2fsSuperblock(sb_buf[0..]) == null, true);
});

test("5. Malformed F2FS Checkpoint rejection: bad CRC32, checksum_offset < 192, out-of-bounds offset, truncation", () => {
    let short_cp: [128]u8 = [0; 128];
    expect_eq(parseF2fsCheckpoint(short_cp[0..]) == null, true);

    let cp_buf: [4096]u8 = [0; 4096];

    // Corrupted CRC32
    buildF2fsCheckpoint(cp_buf[0..], 10, CP_UMOUNT_FLAG, 500, 4092);
    cp_buf[20] ^= 0x80;
    expect_eq(parseF2fsCheckpoint(cp_buf[0..]) == null, true);

    // checksum_offset < 192
    buildF2fsCheckpoint(cp_buf[0..], 10, CP_UMOUNT_FLAG, 500, 188);
    expect_eq(parseF2fsCheckpoint(cp_buf[0..]) == null, true);

    // checksum_offset + 4 > cp_block.len (4094 + 4 = 4098 > 4096)
    buildF2fsCheckpoint(cp_buf[0..], 10, CP_UMOUNT_FLAG, 500, 4094);
    expect_eq(parseF2fsCheckpoint(cp_buf[0..]) == null, true);
});
