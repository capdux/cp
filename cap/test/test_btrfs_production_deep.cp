// Source: cp/cap/test/test_btrfs_production_deep.cp (Production deep tests for cap:btrfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BTRFS_SUPER_OFFSET,
    BTRFS_SUPER_SIZE,
    BTRFS_MAGIC,
    BTRFS_TREE_HEADER_SIZE,
    BTRFS_KEY_SIZE,
    BTRFS_SYS_CHUNK_ARRAY_OFFSET,
    BTRFS_SYS_CHUNK_ARRAY_MAX,
    BTRFS_CHUNK_FIXED_SIZE,
    BTRFS_STRIPE_SIZE,
    CSUM_CRC32C,
    CSUM_XXHASH64,
    CSUM_SHA256,
    CSUM_BLAKE2B,
    KEY_INODE_ITEM,
    KEY_DIR_ITEM,
    KEY_EXTENT_DATA,
    KEY_ROOT_ITEM,
    KEY_CHUNK_ITEM,
    BLOCK_GROUP_DATA,
    BLOCK_GROUP_SYSTEM,
    BLOCK_GROUP_METADATA,
    BLOCK_GROUP_RAID0,
    BLOCK_GROUP_RAID1,
    BLOCK_GROUP_DUP,
    BLOCK_GROUP_RAID10,
    BLOCK_GROUP_RAID5,
    BLOCK_GROUP_RAID6,
    btrfsCrc32c,
    BtrfsKey,
    BtrfsSysChunk,
    BtrfsSysChunkIter,
    BtrfsSuperblock,
    BtrfsTreeHeader,
    isBtrfsSuperblock,
    parseBtrfsSuperblock,
    parseBtrfsKey,
    parseBtrfsTreeHeader,
} from "cap:btrfs";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU64Le(buf: []u8, off: usize, v: u64) {
    putU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn putBytes(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

fn slicesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn buildValidSuperblock(csum_type: u16): [4096]u8 {
    let sb: [4096]u8 = std.mem.zeroes([4096]u8);
    // fsid [32..48]
    let i: usize = 0;
    while (i < 16) {
        sb[32 + i] = (0xA0 + i) as u8;
        i += 1;
    }
    putU64Le(sb[0..], 48, BTRFS_SUPER_OFFSET as u64); // bytenr
    putU64Le(sb[0..], 56, 1);                         // flags
    putBytes(sb[0..], 64, BTRFS_MAGIC);               // magic "_BHRfS_M"
    putU64Le(sb[0..], 72, 42);                        // generation
    putU64Le(sb[0..], 80, 0x01D00000);                // root
    putU64Le(sb[0..], 88, 0x01400000);                // chunk_root
    putU64Le(sb[0..], 96, 0);                         // log_root
    putU64Le(sb[0..], 112, 10737418240);              // total_bytes (10 GiB)
    putU64Le(sb[0..], 120, 536870912);                // bytes_used (512 MiB)
    putU64Le(sb[0..], 128, 6);                        // root_dir_objectid
    putU64Le(sb[0..], 136, 2);                        // num_devices
    putU32Le(sb[0..], 144, 4096);                     // sectorsize
    putU32Le(sb[0..], 148, 16384);                    // nodesize
    putU32Le(sb[0..], 156, 4096);                     // stripesize
    putU32Le(sb[0..], 160, 0);                        // sys_chunk_array_size
    putU64Le(sb[0..], 164, 41);                       // chunk_root_generation
    putU64Le(sb[0..], 172, 0);                        // compat_flags
    putU64Le(sb[0..], 180, 0x01);                     // compat_ro_flags
    putU64Le(sb[0..], 188, 0x161);                    // incompat_flags
    putU16Le(sb[0..], 196, csum_type);                // csum_type
    sb[198] = 1;                                      // root_level
    sb[199] = 0;                                      // chunk_root_level
    putBytes(sb[0..], 299, "cp-btrfs-vol");           // label

    if (csum_type == CSUM_CRC32C) {
        const crc = btrfsCrc32c(sb[32..4096]);
        putU32Le(sb[0..], 0, crc);
    }
    return sb;
}

test("1. Btrfs CRC32C test vector and 4KB Superblock parsing with CRC32C verification", () => {
    expect_eq(BTRFS_SUPER_OFFSET, 65536 as usize);
    expect_eq(BTRFS_SUPER_SIZE, 4096 as usize);
    expect_eq(BTRFS_TREE_HEADER_SIZE, 101 as usize);
    expect_eq(BTRFS_KEY_SIZE, 17 as usize);

    // Standard iSCSI CRC32C("123456789") == 0xE3069283; Btrfs omits final XOR -> ~0xE3069283 == 0x1CF96D7C
    const raw_crc = btrfsCrc32c("123456789");
    expect_eq(raw_crc, 0x1CF96D7C as u32);
    expect_eq(~raw_crc, 0xE3069283 as u32);

    const sb_raw = buildValidSuperblock(CSUM_CRC32C);
    expect_eq(isBtrfsSuperblock(sb_raw[0..]), true);

    if (sb := parseBtrfsSuperblock(sb_raw[0..])) {
        expect_eq(sb.bytenr, 65536 as u64);
        expect_eq(sb.flags, 1 as u64);
        expect_eq(sb.generation, 42 as u64);
        expect_eq(sb.root, 0x01D00000 as u64);
        expect_eq(sb.chunk_root, 0x01400000 as u64);
        expect_eq(sb.total_bytes, 10737418240 as u64);
        expect_eq(sb.bytes_used, 536870912 as u64);
        expect_eq(sb.root_dir_objectid, 6 as u64);
        expect_eq(sb.num_devices, 2 as u64);
        expect_eq(sb.sectorsize, 4096 as u32);
        expect_eq(sb.nodesize, 16384 as u32);
        expect_eq(sb.stripesize, 4096 as u32);
        expect_eq(sb.chunk_root_generation, 41 as u64);
        expect_eq(sb.compat_ro_flags, 0x01 as u64);
        expect_eq(sb.incompat_flags, 0x161 as u64);
        expect_eq(sb.csum_type, CSUM_CRC32C);
        expect_eq(sb.root_level, 1 as u8);
        expect_eq(sb.chunk_root_level, 0 as u8);
        expect_eq(sb.fsid[0], 0xA0 as u8);
        expect_eq(sb.fsid[15], 0xAF as u8);
        expect_eq(slicesEq(sb.csumName(), "crc32c"), true);
        expect_eq(slicesEq(sb.labelSlice(), "cp-btrfs-vol"), true);
    } else {
        expect_eq(true, false);
    }
});

test("2. Label NUL-termination edge cases and alternative checksum types", () => {
    // 2a. Empty label + XXHASH64
    let sb_xx = buildValidSuperblock(CSUM_XXHASH64);
    let i: usize = 0;
    while (i < 256) {
        sb_xx[299 + i] = 0;
        i += 1;
    }
    if (sb := parseBtrfsSuperblock(sb_xx[0..])) {
        expect_eq(sb.labelSlice().len, 0 as usize);
        expect_eq(slicesEq(sb.csumName(), "xxhash64"), true);
    } else {
        expect_eq(true, false);
    }

    // 2b. Full 256-byte label without NUL terminator + SHA256
    let sb_sha = buildValidSuperblock(CSUM_SHA256);
    i = 0;
    while (i < 256) {
        sb_sha[299 + i] = 'X';
        i += 1;
    }
    if (sb := parseBtrfsSuperblock(sb_sha[0..])) {
        expect_eq(sb.labelSlice().len, 256 as usize);
        expect_eq(sb.labelSlice()[0], 'X' as u8);
        expect_eq(sb.labelSlice()[255], 'X' as u8);
        expect_eq(slicesEq(sb.csumName(), "sha256"), true);
    } else {
        expect_eq(true, false);
    }

    // 2c. BLAKE2B + unknown csum name
    const sb_b2 = buildValidSuperblock(CSUM_BLAKE2B);
    if (sb := parseBtrfsSuperblock(sb_b2[0..])) {
        expect_eq(slicesEq(sb.csumName(), "blake2b"), true);
    } else {
        expect_eq(true, false);
    }

    const sb_unk = buildValidSuperblock(99);
    if (sb := parseBtrfsSuperblock(sb_unk[0..])) {
        expect_eq(slicesEq(sb.csumName(), "unknown"), true);
    } else {
        expect_eq(true, false);
    }
});

test("3. sys_chunk_array zero-copy iteration over multiple System Chunks and multi-stripe RAID1/DUP", () => {
    let sb_raw = buildValidSuperblock(CSUM_CRC32C);

    // Entry 1: 17B key + 48B chunk + 1 * 32B stripe = 97 bytes
    let pos: usize = BTRFS_SYS_CHUNK_ARRAY_OFFSET;
    putU64Le(sb_raw[0..], pos + 0, 256);               // key.objectid (FIRST_CHUNK_TREE_OBJECTID)
    sb_raw[pos + 8] = KEY_CHUNK_ITEM;                  // key.key_type (228)
    putU64Le(sb_raw[0..], pos + 9, 0x00400000);        // key.offset (logical start)
    pos += BTRFS_KEY_SIZE;

    putU64Le(sb_raw[0..], pos + 0, 0x00400000);        // length (4 MiB)
    putU64Le(sb_raw[0..], pos + 8, 2);                 // owner (EXTENT_TREE)
    putU64Le(sb_raw[0..], pos + 16, 65536);            // stripe_len
    putU64Le(sb_raw[0..], pos + 24, BLOCK_GROUP_SYSTEM);
    putU16Le(sb_raw[0..], pos + 44, 1);                // num_stripes = 1
    pos += BTRFS_CHUNK_FIXED_SIZE;

    putU64Le(sb_raw[0..], pos + 0, 1);                 // stripe[0].devid
    putU64Le(sb_raw[0..], pos + 8, 0x00100000);        // stripe[0].offset
    pos += BTRFS_STRIPE_SIZE;

    // Entry 2: 17B key + 48B chunk + 2 * 32B stripes = 129 bytes (SYSTEM | DUP)
    putU64Le(sb_raw[0..], pos + 0, 256);               // key.objectid
    sb_raw[pos + 8] = KEY_CHUNK_ITEM;                  // key.key_type (228)
    putU64Le(sb_raw[0..], pos + 9, 0x01400000);        // key.offset (logical start)
    pos += BTRFS_KEY_SIZE;

    putU64Le(sb_raw[0..], pos + 0, 0x02000000);        // length (32 MiB)
    putU64Le(sb_raw[0..], pos + 8, 2);                 // owner
    putU64Le(sb_raw[0..], pos + 16, 65536);            // stripe_len
    putU64Le(sb_raw[0..], pos + 24, BLOCK_GROUP_SYSTEM | BLOCK_GROUP_DUP);
    putU16Le(sb_raw[0..], pos + 44, 2);                // num_stripes = 2
    pos += BTRFS_CHUNK_FIXED_SIZE;

    putU64Le(sb_raw[0..], pos + 0, 1);                 // stripe[0].devid
    putU64Le(sb_raw[0..], pos + 8, 0x02000000);        // stripe[0].offset
    pos += BTRFS_STRIPE_SIZE;
    putU64Le(sb_raw[0..], pos + 0, 2);                 // stripe[1].devid
    putU64Le(sb_raw[0..], pos + 8, 0x04000000);        // stripe[1].offset
    pos += BTRFS_STRIPE_SIZE;

    const total_arr_size = pos - BTRFS_SYS_CHUNK_ARRAY_OFFSET;
    expect_eq(total_arr_size, (97 + 129) as usize);
    putU32Le(sb_raw[0..], 160, total_arr_size as u32);
    // Recompute CRC32C after populating sys_chunk_array
    putU32Le(sb_raw[0..], 0, btrfsCrc32c(sb_raw[32..4096]));

    if (sb := parseBtrfsSuperblock(sb_raw[0..])) {
        expect_eq(sb.sys_chunk_array_size, 226 as u32);
        expect_eq(sb.sys_chunk_array.len, 226 as usize);

        let iter = sb.sysChunks();
        if (c1 := iter.next()) {
            expect_eq(c1.key.objectid, 256 as u64);
            expect_eq(c1.key.key_type, KEY_CHUNK_ITEM);
            expect_eq(c1.key.offset, 0x00400000 as u64);
            expect_eq(c1.length, 0x00400000 as u64);
            expect_eq(c1.owner, 2 as u64);
            expect_eq(c1.stripe_len, 65536 as u64);
            expect_eq(c1.chunk_type, BLOCK_GROUP_SYSTEM);
            expect_eq(c1.num_stripes, 1 as u16);
            expect_eq(c1.first_devid, 1 as u64);
            expect_eq(c1.first_offset, 0x00100000 as u64);
        } else {
            expect_eq(true, false);
        }

        if (c2 := iter.next()) {
            expect_eq(c2.key.objectid, 256 as u64);
            expect_eq(c2.key.key_type, KEY_CHUNK_ITEM);
            expect_eq(c2.key.offset, 0x01400000 as u64);
            expect_eq(c2.length, 0x02000000 as u64);
            expect_eq(c2.chunk_type, BLOCK_GROUP_SYSTEM | BLOCK_GROUP_DUP);
            expect_eq(c2.num_stripes, 2 as u16);
            expect_eq(c2.first_devid, 1 as u64);
            expect_eq(c2.first_offset, 0x02000000 as u64);
        } else {
            expect_eq(true, false);
        }

        // Exhausted
        if (_ := iter.next()) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. 101B B-Tree Leaf/Node Header and 17B B-Tree Disk Key parsing", () => {
    // 4a. B-Tree Leaf Header (level == 0)
    let leaf_raw: [128]u8 = std.mem.zeroes([128]u8);
    leaf_raw[0] = 0x11;
    leaf_raw[31] = 0x22;
    leaf_raw[32] = 0x33;
    leaf_raw[47] = 0x44;
    putU64Le(leaf_raw[0..], 48, 0x01D04000); // bytenr
    putU64Le(leaf_raw[0..], 56, 0x03);       // flags
    leaf_raw[64] = 0x55;
    leaf_raw[79] = 0x66;
    putU64Le(leaf_raw[0..], 80, 99);         // generation
    putU64Le(leaf_raw[0..], 88, 5);          // owner (FS_TREE)
    putU32Le(leaf_raw[0..], 96, 128);        // nritems
    leaf_raw[100] = 0;                       // level = 0 (leaf)

    if (hdr := parseBtrfsTreeHeader(leaf_raw[0..])) {
        expect_eq(hdr.csum[0], 0x11 as u8);
        expect_eq(hdr.csum[31], 0x22 as u8);
        expect_eq(hdr.fsid[0], 0x33 as u8);
        expect_eq(hdr.fsid[15], 0x44 as u8);
        expect_eq(hdr.bytenr, 0x01D04000 as u64);
        expect_eq(hdr.flags, 0x03 as u64);
        expect_eq(hdr.chunk_tree_uuid[0], 0x55 as u8);
        expect_eq(hdr.chunk_tree_uuid[15], 0x66 as u8);
        expect_eq(hdr.generation, 99 as u64);
        expect_eq(hdr.owner, 5 as u64);
        expect_eq(hdr.nritems, 128 as u32);
        expect_eq(hdr.level, 0 as u8);
        expect_eq(hdr.isLeaf(), true);
    } else {
        expect_eq(true, false);
    }

    // 4b. B-Tree Internal Node Header (level == 2)
    leaf_raw[100] = 2;
    if (hdr := parseBtrfsTreeHeader(leaf_raw[0..])) {
        expect_eq(hdr.level, 2 as u8);
        expect_eq(hdr.isLeaf(), false);
    } else {
        expect_eq(true, false);
    }

    // 4c. 17B B-Tree Disk Key
    let key_raw: [17]u8 = std.mem.zeroes([17]u8);
    putU64Le(key_raw[0..], 0, 256);           // objectid
    key_raw[8] = KEY_INODE_ITEM;              // key_type = 1
    putU64Le(key_raw[0..], 9, 0x123456789ABC); // offset

    if (k := parseBtrfsKey(key_raw[0..])) {
        expect_eq(k.objectid, 256 as u64);
        expect_eq(k.key_type, KEY_INODE_ITEM);
        expect_eq(k.offset, 0x123456789ABC as u64);
    } else {
        expect_eq(true, false);
    }

    // Sanity check remaining key & block group flag constants
    expect_eq(KEY_DIR_ITEM, 84 as u8);
    expect_eq(KEY_EXTENT_DATA, 108 as u8);
    expect_eq(KEY_ROOT_ITEM, 132 as u8);
    expect_eq(BLOCK_GROUP_DATA, 0x01 as u64);
    expect_eq(BLOCK_GROUP_METADATA, 0x04 as u64);
    expect_eq(BLOCK_GROUP_RAID0, 0x08 as u64);
    expect_eq(BLOCK_GROUP_RAID1, 0x10 as u64);
    expect_eq(BLOCK_GROUP_RAID10, 0x40 as u64);
    expect_eq(BLOCK_GROUP_RAID5, 0x80 as u64);
    expect_eq(BLOCK_GROUP_RAID6, 0x100 as u64);
});

test("5. Malformed input rejection: bad magic, bad CRC32C, invalid sizes, truncation, and corrupt chunk entries", () => {
    // 5a. Wrong magic
    let bad_magic = buildValidSuperblock(CSUM_CRC32C);
    bad_magic[64] = 'X';
    expect_eq(isBtrfsSuperblock(bad_magic[0..]), false);
    if (_ := parseBtrfsSuperblock(bad_magic[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Bad CRC32C (flip 1 bit in payload)
    let bad_crc = buildValidSuperblock(CSUM_CRC32C);
    bad_crc[100] ^= 0x01;
    expect_eq(isBtrfsSuperblock(bad_crc[0..]), true);
    if (_ := parseBtrfsSuperblock(bad_crc[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Non-power-of-two sectorsize and nodesize
    let bad_sector = buildValidSuperblock(CSUM_XXHASH64);
    putU32Le(bad_sector[0..], 144, 3000);
    if (_ := parseBtrfsSuperblock(bad_sector[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let bad_node = buildValidSuperblock(CSUM_XXHASH64);
    putU32Le(bad_node[0..], 148, 0);
    if (_ := parseBtrfsSuperblock(bad_node[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. sys_chunk_array_size > 2048
    let bad_arr = buildValidSuperblock(CSUM_XXHASH64);
    putU32Le(bad_arr[0..], 160, (BTRFS_SYS_CHUNK_ARRAY_MAX + 1) as u32);
    if (_ := parseBtrfsSuperblock(bad_arr[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5e. Truncated buffers for superblock, key, and tree header
    const short_sb = buildValidSuperblock(CSUM_CRC32C);
    expect_eq(isBtrfsSuperblock(short_sb[0..4095]), false);
    if (_ := parseBtrfsSuperblock(short_sb[0..1024])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let short_key: [16]u8 = std.mem.zeroes([16]u8);
    if (_ := parseBtrfsKey(short_key[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let short_hdr: [100]u8 = std.mem.zeroes([100]u8);
    if (_ := parseBtrfsTreeHeader(short_hdr[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5f. Corrupt sys_chunk_array entry with num_stripes == 0 terminates safely
    let zero_stripe_sb = buildValidSuperblock(CSUM_XXHASH64);
    const p = BTRFS_SYS_CHUNK_ARRAY_OFFSET;
    zero_stripe_sb[p + 8] = KEY_CHUNK_ITEM;
    putU16Le(zero_stripe_sb[0..], p + BTRFS_KEY_SIZE + 44, 0); // num_stripes = 0
    putU32Le(zero_stripe_sb[0..], 160, 97);
    if (sb := parseBtrfsSuperblock(zero_stripe_sb[0..])) {
        let iter = sb.sysChunks();
        if (_ := iter.next()) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }
    } else {
        expect_eq(true, false);
    }
});
