// Source: cp/cap/test/test_apfs_production_deep.cp (Production deep tests for cap:apfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    APFS_OBJ_HEADER_SIZE,
    APFS_NX_MAGIC,
    APFS_VOL_MAGIC,
    APFS_MIN_BLOCK_SIZE,
    APFS_MAX_FILE_SYSTEMS,
    OBJ_TYPE_NX_SUPERBLOCK,
    OBJ_TYPE_BTREE,
    OBJ_TYPE_BTREE_NODE,
    OBJ_TYPE_SPACEMAN,
    OBJ_TYPE_OMAP,
    OBJ_TYPE_CHECKPOINT_MAP,
    OBJ_TYPE_FS,
    OBJ_VIRTUAL,
    OBJ_PHYSICAL,
    OBJ_EPHEMERAL,
    APFS_FS_UNENCRYPTED,
    APFS_FS_ONEKEY,
    APFS_FS_CASE_INSENSITIVE,
    APFS_FS_CASE_SENSITIVE,
    apfsFletcher64,
    verifyApfsChecksum,
    parseApfsObjHeader,
    isApfsContainer,
    parseApfsContainer,
    isApfsVolume,
    parseApfsVolume,
} from "cap:sys";

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

fn copyInto(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn buildContainerBlock(buf: []u8, block_size: u32, block_count: u64, xid: u64) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
    // Object header: oid=1, xid, type=NX_SUPERBLOCK | EPHEMERAL
    writeU64Le(buf, 8, 1);
    writeU64Le(buf, 16, xid);
    writeU16Le(buf, 24, OBJ_TYPE_NX_SUPERBLOCK);
    writeU16Le(buf, 26, OBJ_EPHEMERAL);
    writeU32Le(buf, 28, 0);

    writeU32Le(buf, 32, APFS_NX_MAGIC);
    writeU32Le(buf, 36, block_size);
    writeU64Le(buf, 40, block_count);
    writeU64Le(buf, 48, 0x02); // features
    writeU64Le(buf, 56, 0x00); // ro_compat
    writeU64Le(buf, 64, 0x00); // incompat
    let g: usize = 0;
    while (g < 16) {
        buf[72 + g] = (0x50 + g) as u8;
        g += 1;
    }
    writeU64Le(buf, 88, 1024); // next_oid
    writeU64Le(buf, 96, xid + 1); // next_xid
    writeU32Le(buf, 104, 64); // xp_desc_blocks
    writeU32Le(buf, 108, 128); // xp_data_blocks
    writeU64Le(buf, 160, 42); // omap_oid
    writeU32Le(buf, 180, APFS_MAX_FILE_SYSTEMS);

    const cksum = apfsFletcher64(buf);
    writeU64Le(buf, 0, cksum);
}

fn buildVolumeBlock(buf: []u8, volname: []const u8, num_files: u64, num_dirs: u64, num_symlinks: u64) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
    // Object header: oid=0x400, xid=55, type=FS | VIRTUAL
    writeU64Le(buf, 8, 0x400);
    writeU64Le(buf, 16, 55);
    writeU16Le(buf, 24, OBJ_TYPE_FS);
    writeU16Le(buf, 26, OBJ_VIRTUAL);
    writeU32Le(buf, 28, 0);

    writeU32Le(buf, 32, APFS_VOL_MAGIC);
    writeU32Le(buf, 36, 0); // fs_index
    writeU64Le(buf, 40, APFS_FS_UNENCRYPTED | APFS_FS_ONEKEY);
    writeU64Le(buf, 48, 0);
    writeU64Le(buf, 56, APFS_FS_CASE_INSENSITIVE);
    writeU64Le(buf, 128, 1001); // omap_oid
    writeU64Le(buf, 136, 1002); // root_tree_oid
    writeU64Le(buf, 144, 1003); // extentref_tree_oid
    writeU64Le(buf, 152, 1004); // snap_meta_tree_oid
    writeU64Le(buf, 184, num_files);
    writeU64Le(buf, 192, num_dirs);
    writeU64Le(buf, 200, num_symlinks);
    let g: usize = 0;
    while (g < 16) {
        buf[240 + g] = (0x90 + g) as u8;
        g += 1;
    }
    copyInto(buf, 704, volname);

    const cksum = apfsFletcher64(buf);
    writeU64Le(buf, 0, cksum);
}

test("1. Apple APFS Fletcher-64 checksum calculation and single-byte bit-flip detection", () => {
    expect_eq(APFS_OBJ_HEADER_SIZE, 32);
    expect_eq(APFS_MIN_BLOCK_SIZE, 4096);
    expect_eq(APFS_NX_MAGIC, 0x4253584E as u32);
    expect_eq(APFS_VOL_MAGIC, 0x42535041 as u32);

    let blk: [64]u8 = [0; 64];
    writeU32Le(blk[0..], 8, 0x11223344);
    writeU32Le(blk[0..], 12, 0x55667788);
    writeU32Le(blk[0..], 60, 0xAABBCCDD);

    const cksum = apfsFletcher64(blk[0..]);
    writeU64Le(blk[0..], 0, cksum);
    expect_eq(verifyApfsChecksum(blk[0..]), true);

    // Single-bit flip in payload invalidates checksum
    blk[33] ^= 0x01;
    expect_eq(verifyApfsChecksum(blk[0..]), false);

    // Restoring bit restores validity
    blk[33] ^= 0x01;
    expect_eq(verifyApfsChecksum(blk[0..]), true);

    // Short / non-4B-aligned buffers rejected
    expect_eq(verifyApfsChecksum("short"), false);
    let unaligned: [10]u8 = [0; 10];
    expect_eq(verifyApfsChecksum(unaligned[0..]), false);
});

test("2. 32-byte Object Header storage flags: Physical, Ephemeral, and Virtual", () => {
    let raw: [32]u8 = [0; 32];
    writeU64Le(raw[0..], 0, 0xCAFEBABE12345678);
    writeU64Le(raw[0..], 8, 99);
    writeU64Le(raw[0..], 16, 777);
    writeU16Le(raw[0..], 24, OBJ_TYPE_OMAP);
    writeU16Le(raw[0..], 26, OBJ_PHYSICAL);
    writeU32Le(raw[0..], 28, OBJ_TYPE_BTREE as u32);

    const h_phys = parseApfsObjHeader(raw[0..]) ?? unreachable;
    expect_eq(h_phys.oid, 99 as u64);
    expect_eq(h_phys.xid, 777 as u64);
    expect_eq(h_phys.obj_type, OBJ_TYPE_OMAP);
    expect_eq(h_phys.isPhysical(), true);
    expect_eq(h_phys.isEphemeral(), false);
    expect_eq(h_phys.isVirtual(), false);

    writeU16Le(raw[0..], 26, OBJ_EPHEMERAL);
    const h_eph = parseApfsObjHeader(raw[0..]) ?? unreachable;
    expect_eq(h_eph.isEphemeral(), true);
    expect_eq(h_eph.isPhysical(), false);
    expect_eq(h_eph.isVirtual(), false);

    writeU16Le(raw[0..], 26, OBJ_VIRTUAL);
    const h_virt = parseApfsObjHeader(raw[0..]) ?? unreachable;
    expect_eq(h_virt.isVirtual(), true);
    expect_eq(h_virt.isPhysical(), false);
    expect_eq(h_virt.isEphemeral(), false);

    // Verify remaining constants
    expect_eq(OBJ_TYPE_BTREE_NODE, 0x0003 as u16);
    expect_eq(OBJ_TYPE_SPACEMAN, 0x0005 as u16);
    expect_eq(OBJ_TYPE_CHECKPOINT_MAP, 0x000C as u16);
    expect_eq(APFS_FS_CASE_SENSITIVE, 0x08 as u64);
});

test("3. 4KB APFS Container Superblock (NXSB) full parsing and u128 totalBytes()", () => {
    let nx_buf: [4096]u8 = [0; 4096];
    // 256,000,000 blocks * 4096 = 1,048,576,000,000 bytes (~1 TB)
    buildContainerBlock(nx_buf[0..], 4096, 256000000, 12345);

    expect_eq(isApfsContainer(nx_buf[0..]), true);
    expect_eq(isApfsVolume(nx_buf[0..]), false);

    const nx = parseApfsContainer(nx_buf[0..]) ?? unreachable;
    expect_eq(nx.header.oid, 1 as u64);
    expect_eq(nx.header.xid, 12345 as u64);
    expect_eq(nx.header.isEphemeral(), true);
    expect_eq(nx.block_size, 4096 as u32);
    expect_eq(nx.block_count, 256000000 as u64);
    expect_eq(nx.totalBytes(), (256000000 as u128) * (4096 as u128));
    expect_eq(nx.uuid[0], 0x50 as u8);
    expect_eq(nx.next_oid, 1024 as u64);
    expect_eq(nx.next_xid, 12346 as u64);
    expect_eq(nx.xp_desc_blocks, 64 as u32);
    expect_eq(nx.xp_data_blocks, 128 as u32);
    expect_eq(nx.omap_oid, 42 as u64);
    expect_eq(nx.max_file_systems, 100 as u32);
});

test("4. 4KB APFS Volume Superblock (APSB) parsing, volnameSlice(), UUID, and object counters", () => {
    let vol_buf: [4096]u8 = [0; 4096];
    buildVolumeBlock(vol_buf[0..], "Macintosh HD", 450123, 81920, 4096);

    expect_eq(isApfsVolume(vol_buf[0..]), true);
    expect_eq(isApfsContainer(vol_buf[0..]), false);

    const vol = parseApfsVolume(vol_buf[0..]) ?? unreachable;
    expect_eq(vol.header.oid, 0x400 as u64);
    expect_eq(vol.header.xid, 55 as u64);
    expect_eq(vol.header.isVirtual(), true);
    expect_eq(vol.fs_index, 0 as u32);
    expect_eq(vol.omap_oid, 1001 as u64);
    expect_eq(vol.root_tree_oid, 1002 as u64);
    expect_eq(vol.extentref_tree_oid, 1003 as u64);
    expect_eq(vol.snap_meta_tree_oid, 1004 as u64);
    expect_eq(vol.num_files, 450123 as u64);
    expect_eq(vol.num_directories, 81920 as u64);
    expect_eq(vol.num_symlinks, 4096 as u64);
    expect_eq(vol.vol_uuid[0], 0x90 as u8);
    expect_eq(vol.isUnencrypted(), true);
    expect_eq(vol.isCaseInsensitive(), true);
    expect_eq(std.mem.eql(u8, vol.volnameSlice(), "Macintosh HD"), true);
});

test("5. Malformed APFS block rejection: bad magic, bad Fletcher-64, non-power-of-two block_size, wrong obj_type, truncation", () => {
    // Truncated buffers
    let short_buf: [16]u8 = [0; 16];
    expect_eq(parseApfsObjHeader(short_buf[0..]) == null, true);
    expect_eq(isApfsContainer(short_buf[0..]), false);
    expect_eq(isApfsVolume(short_buf[0..]), false);
    expect_eq(parseApfsContainer(short_buf[0..]) == null, true);
    expect_eq(parseApfsVolume(short_buf[0..]) == null, true);

    let nx_buf: [4096]u8 = [0; 4096];

    // Bad magic
    buildContainerBlock(nx_buf[0..], 4096, 1000, 1);
    writeU32Le(nx_buf[0..], 32, 0xDEADBEEF);
    writeU64Le(nx_buf[0..], 0, apfsFletcher64(nx_buf[0..]));
    expect_eq(parseApfsContainer(nx_buf[0..]) == null, true);

    // Bad Fletcher-64 checksum
    buildContainerBlock(nx_buf[0..], 4096, 1000, 1);
    nx_buf[200] ^= 0xFF;
    expect_eq(parseApfsContainer(nx_buf[0..]) == null, true);

    // Non-power-of-two block_size (6000) and < 4096 block_size (2048)
    buildContainerBlock(nx_buf[0..], 6000, 1000, 1);
    expect_eq(parseApfsContainer(nx_buf[0..]) == null, true);
    buildContainerBlock(nx_buf[0..], 2048, 1000, 1);
    expect_eq(parseApfsContainer(nx_buf[0..]) == null, true);

    // Declared block_size (8192) exceeds available buffer (4096)
    buildContainerBlock(nx_buf[0..], 8192, 1000, 1);
    expect_eq(parseApfsContainer(nx_buf[0..]) == null, true);

    // Wrong obj_type in Volume Superblock
    let vol_buf: [4096]u8 = [0; 4096];
    buildVolumeBlock(vol_buf[0..], "BadType", 1, 1, 0);
    writeU16Le(vol_buf[0..], 24, OBJ_TYPE_BTREE);
    writeU64Le(vol_buf[0..], 0, apfsFletcher64(vol_buf[0..]));
    expect_eq(parseApfsVolume(vol_buf[0..]) == null, true);

    // Corrupted Fletcher-64 in Volume Superblock
    buildVolumeBlock(vol_buf[0..], "BadCksum", 1, 1, 0);
    vol_buf[705] ^= 0x04;
    expect_eq(parseApfsVolume(vol_buf[0..]) == null, true);
});
