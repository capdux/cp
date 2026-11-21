// Source: cp/cap/test/test_zfs_production_deep.cp (Production deep tests for cap:zfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ZFS_UBERBLOCK_MAGIC,
    ZFS_UBERBLOCK_MAGIC_SWAPPED,
    ZFS_UBERBLOCK_SIZE,
    ZFS_BLKPTR_SIZE,
    ZFS_SECTOR_SHIFT,
    ZFS_VDEV_LABEL_SIZE,
    ZFS_VDEV_BOOT_OFFSET,
    COMP_INHERIT,
    COMP_ON,
    COMP_OFF,
    COMP_LZJB,
    COMP_EMPTY,
    COMP_GZIP_1,
    COMP_LZ4,
    COMP_ZSTD,
    CKSUM_INHERIT,
    CKSUM_ON,
    CKSUM_OFF,
    CKSUM_FLETCHER_2,
    CKSUM_FLETCHER_4,
    CKSUM_SHA256,
    CKSUM_SHA512,
    CKSUM_SKEIN,
    CKSUM_EDONR,
    CKSUM_BLAKE3,
    ZfsChecksum,
    zfsFletcher4Le,
    zfsFletcher4Be,
    ZfsDva,
    ZfsBlkPtr,
    ZfsUberblock,
    parseZfsBlkPtr,
    isZfsUberblock,
    parseZfsUberblock,
    selectActiveUberblock,
} from "cap:zfs";

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off + 0] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off + 0] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putU64Le(buf: []u8, off: usize, v: u64) {
    putU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn putU64Be(buf: []u8, off: usize, v: u64) {
    putU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    putU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn putU64Endian(buf: []u8, off: usize, v: u64, is_be: bool) {
    if (is_be) {
        putU64Be(buf, off, v);
    } else {
        putU64Le(buf, off, v);
    }
}

fn buildUberblock(is_be: bool, version: u64, txg: u64): [1024]u8 {
    let ub: [1024]u8 = std.mem.zeroes([1024]u8);
    putU64Endian(ub[0..], 0, ZFS_UBERBLOCK_MAGIC, is_be);
    putU64Endian(ub[0..], 8, version, is_be);
    putU64Endian(ub[0..], 16, txg, is_be);
    putU64Endian(ub[0..], 24, 0xCAFEBABE12345678, is_be); // guid_sum
    putU64Endian(ub[0..], 32, 1710000000, is_be);         // timestamp

    // Embedded rootbp at [40..168]
    // DVA 0: vdev=1, grid=0, asize=8 sectors (4096B), gang=false, offset=1024 sectors
    const dva0_w0: u64 = ((1 as u64) << 32) | 8;
    const dva0_w1: u64 = 1024;
    putU64Endian(ub[0..], 40 + 0, dva0_w0, is_be);
    putU64Endian(ub[0..], 40 + 8, dva0_w1, is_be);

    // DVA 1: vdev=2, grid=3, asize=8 sectors, gang=true, offset=2048 sectors
    const dva1_w0: u64 = ((2 as u64) << 32) | ((3 as u64) << 24) | 8;
    const dva1_w1: u64 = ((1 as u64) << 63) | 2048;
    putU64Endian(ub[0..], 40 + 16, dva1_w0, is_be);
    putU64Endian(ub[0..], 40 + 24, dva1_w1, is_be);

    // blk_prop: lsize=7 (4096B), psize=3 (2048B), comp=COMP_LZ4, cksum=CKSUM_FLETCHER_4, dmu=10, level=1, enc=0, dedup=1
    let order_bit: u64 = (1 as u64) << 63; // 1 = LE
    if (is_be) {
        order_bit = 0; // 0 = BE
    }
    const prop: u64 =
        order_bit |
        ((1 as u64) << 62) |                    // is_dedup
        ((1 as u64) << 56) |                    // level = 1
        ((10 as u64) << 48) |                   // dmu_type = 10
        ((CKSUM_FLETCHER_4 as u64) << 40) |     // checksum_type
        ((COMP_LZ4 as u64) << 32) |             // compression
        ((3 as u64) << 16) |                    // psize = (3+1)*512 = 2048
        7;                                      // lsize = (7+1)*512 = 4096
    putU64Endian(ub[0..], 40 + 48, prop, is_be);
    putU64Endian(ub[0..], 40 + 72, txg - 1, is_be); // phys_birth_txg
    putU64Endian(ub[0..], 40 + 80, txg, is_be);     // logical_birth_txg
    putU64Endian(ub[0..], 40 + 88, 64, is_be);      // fill_count
    putU64Endian(ub[0..], 40 + 96, 0x1111, is_be);  // cksum[0]
    putU64Endian(ub[0..], 40 + 104, 0x2222, is_be); // cksum[1]
    putU64Endian(ub[0..], 40 + 112, 0x3333, is_be); // cksum[2]
    putU64Endian(ub[0..], 40 + 120, 0x4444, is_be); // cksum[3]

    putU64Endian(ub[0..], 168, 5000, is_be); // software_version
    return ub;
}

test("1. Dual-endian Uberblock auto-detection (LE & BE) and selectActiveUberblock election", () => {
    expect_eq(ZFS_UBERBLOCK_MAGIC, 0x00BAB10C as u64);
    expect_eq(ZFS_UBERBLOCK_MAGIC_SWAPPED, 0x0CB1BA00 as u64);
    expect_eq(ZFS_UBERBLOCK_SIZE, 1024 as usize);
    expect_eq(ZFS_BLKPTR_SIZE, 128 as usize);
    expect_eq(ZFS_VDEV_LABEL_SIZE, 262144 as usize);

    const ub_le_raw = buildUberblock(false, 5000, 100);
    const ub_be_raw = buildUberblock(true, 5000, 250);

    expect_eq(isZfsUberblock(ub_le_raw[0..]), true);
    expect_eq(isZfsUberblock(ub_be_raw[0..]), true);

    const u_le = parseZfsUberblock(ub_le_raw[0..]);
    const u_be = parseZfsUberblock(ub_be_raw[0..]);

    if (a := u_le) {
        expect_eq(a.is_big_endian, false);
        expect_eq(a.version, 5000 as u64);
        expect_eq(a.txg, 100 as u64);
        expect_eq(a.guid_sum, 0xCAFEBABE12345678 as u64);
        expect_eq(a.timestamp, 1710000000 as u64);
        expect_eq(a.software_version, 5000 as u64);
        expect_eq(a.rootbp.is_big_endian, false);
    } else {
        expect_eq(true, false);
    }

    if (b := u_be) {
        expect_eq(b.is_big_endian, true);
        expect_eq(b.version, 5000 as u64);
        expect_eq(b.txg, 250 as u64);
        expect_eq(b.rootbp.is_big_endian, true);
    } else {
        expect_eq(true, false);
    }

    // selectActiveUberblock picks higher txg (250 > 100)
    if (best := selectActiveUberblock(u_le, u_be)) {
        expect_eq(best.txg, 250 as u64);
        expect_eq(best.is_big_endian, true);
    } else {
        expect_eq(true, false);
    }

    // One-sided null handling
    const none_ub: ZfsUberblock? = null;
    if (only_le := selectActiveUberblock(u_le, none_ub)) {
        expect_eq(only_le.txg, 100 as u64);
    } else {
        expect_eq(true, false);
    }
});

test("2. 128B ZfsBlkPtr DVA decoding, 4MiB VDEV Label offset math, and compression/checksum flags", () => {
    const ub_raw = buildUberblock(false, 5000, 42);
    if (ub := parseZfsUberblock(ub_raw[0..])) {
        const bp = ub.rootbp;
        expect_eq(bp.isHole(), false);
        expect_eq(bp.lsize_bytes, 4096 as u32);
        expect_eq(bp.psize_bytes, 2048 as u32);
        expect_eq(bp.compression, COMP_LZ4);
        expect_eq(bp.checksum_type, CKSUM_FLETCHER_4);
        expect_eq(bp.dmu_type, 10 as u8);
        expect_eq(bp.level, 1 as u8);
        expect_eq(bp.is_encrypted, false);
        expect_eq(bp.is_dedup, true);
        expect_eq(bp.is_embedded, false);
        expect_eq(bp.phys_birth_txg, 41 as u64);
        expect_eq(bp.logical_birth_txg, 42 as u64);
        expect_eq(bp.fill_count, 64 as u64);
        expect_eq(bp.cksum.words[0], 0x1111 as u64);
        expect_eq(bp.cksum.words[3], 0x4444 as u64);

        // DVA 0
        expect_eq(bp.dvas[0].isValid(), true);
        expect_eq(bp.dvas[0].vdev, 1 as u32);
        expect_eq(bp.dvas[0].asize_sectors, 8 as u32);
        expect_eq(bp.dvas[0].asizeBytes(), 4096 as u64);
        expect_eq(bp.dvas[0].gang, false);
        expect_eq(bp.dvas[0].byteOffset(), ((1024 as u64) << ZFS_SECTOR_SHIFT) + ZFS_VDEV_BOOT_OFFSET);

        // DVA 1 (gang=true, grid=3)
        expect_eq(bp.dvas[1].isValid(), true);
        expect_eq(bp.dvas[1].vdev, 2 as u32);
        expect_eq(bp.dvas[1].grid, 3 as u8);
        expect_eq(bp.dvas[1].gang, true);
        expect_eq(bp.dvas[1].byteOffset(), ((2048 as u64) << 9) + 0x400000);

        // DVA 2 (empty)
        expect_eq(bp.dvas[2].isValid(), false);
    } else {
        expect_eq(true, false);
    }
});

test("3. ZfsBlkPtr Hole, ZSTD/SHA256, and Encrypted flag decoding", () => {
    // 3a. All-zero hole blkptr
    let hole_raw: [128]u8 = std.mem.zeroes([128]u8);
    if (bp := parseZfsBlkPtr(hole_raw[0..], false)) {
        expect_eq(bp.isHole(), true);
        expect_eq(bp.lsize_bytes, 0 as u32);
        expect_eq(bp.psize_bytes, 0 as u32);
        expect_eq(bp.dvas[0].isValid(), false);
    } else {
        expect_eq(true, false);
    }

    // 3b. ZSTD + SHA256 + Encrypted block
    let enc_raw: [128]u8 = std.mem.zeroes([128]u8);
    const prop: u64 =
        ((1 as u64) << 63) |                // LE
        ((1 as u64) << 61) |                // is_encrypted
        ((CKSUM_SHA256 as u64) << 40) |
        ((COMP_ZSTD as u64) << 32) |
        ((15 as u64) << 16) |               // psize = 8192
        31;                                 // lsize = 16384
    putU64Le(enc_raw[0..], 48, prop);
    putU64Le(enc_raw[0..], 80, 999);        // logical_birth_txg

    if (bp := parseZfsBlkPtr(enc_raw[0..], false)) {
        expect_eq(bp.isHole(), false);
        expect_eq(bp.compression, COMP_ZSTD);
        expect_eq(bp.checksum_type, CKSUM_SHA256);
        expect_eq(bp.is_encrypted, true);
        expect_eq(bp.lsize_bytes, 16384 as u32);
        expect_eq(bp.psize_bytes, 8192 as u32);
    } else {
        expect_eq(true, false);
    }

    // Sanity check remaining compression & checksum constants
    expect_eq(COMP_INHERIT, 0 as u8);
    expect_eq(COMP_ON, 1 as u8);
    expect_eq(COMP_OFF, 2 as u8);
    expect_eq(COMP_LZJB, 3 as u8);
    expect_eq(COMP_EMPTY, 4 as u8);
    expect_eq(COMP_GZIP_1, 5 as u8);
    expect_eq(CKSUM_INHERIT, 0 as u8);
    expect_eq(CKSUM_ON, 1 as u8);
    expect_eq(CKSUM_OFF, 2 as u8);
    expect_eq(CKSUM_FLETCHER_2, 6 as u8);
    expect_eq(CKSUM_SHA512, 11 as u8);
    expect_eq(CKSUM_SKEIN, 12 as u8);
    expect_eq(CKSUM_EDONR, 13 as u8);
    expect_eq(CKSUM_BLAKE3, 14 as u8);
});

test("4. OpenZFS Fletcher-4 256-bit checksum (LE and BE exact arithmetic)", () => {
    // 4 words: [1, 2, 3, 4]
    // Step 1 (w=1): a=1,  b=1,  c=1,  d=1
    // Step 2 (w=2): a=3,  b=4,  c=5,  d=6
    // Step 3 (w=3): a=6,  b=10, c=15, d=21
    // Step 4 (w=4): a=10, b=20, c=35, d=56
    let buf_le: [16]u8 = std.mem.zeroes([16]u8);
    putU32Le(buf_le[0..], 0, 1);
    putU32Le(buf_le[0..], 4, 2);
    putU32Le(buf_le[0..], 8, 3);
    putU32Le(buf_le[0..], 12, 4);

    const f_le = zfsFletcher4Le(buf_le[0..]);
    expect_eq(f_le.words[0], 10 as u64);
    expect_eq(f_le.words[1], 20 as u64);
    expect_eq(f_le.words[2], 35 as u64);
    expect_eq(f_le.words[3], 56 as u64);

    let buf_be: [16]u8 = std.mem.zeroes([16]u8);
    putU32Be(buf_be[0..], 0, 1);
    putU32Be(buf_be[0..], 4, 2);
    putU32Be(buf_be[0..], 8, 3);
    putU32Be(buf_be[0..], 12, 4);

    const f_be = zfsFletcher4Be(buf_be[0..]);
    expect_eq(f_be.words[0], 10 as u64);
    expect_eq(f_be.words[1], 20 as u64);
    expect_eq(f_be.words[2], 35 as u64);
    expect_eq(f_be.words[3], 56 as u64);
});

test("5. Malformed input rejection: bad magic, version == 0, txg == 0, and truncated buffers", () => {
    // 5a. Wrong magic
    let bad_magic = buildUberblock(false, 5000, 10);
    bad_magic[0] = 0xFF;
    expect_eq(isZfsUberblock(bad_magic[0..]), false);
    if (_ := parseZfsUberblock(bad_magic[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. version == 0
    const bad_ver = buildUberblock(false, 0, 10);
    expect_eq(isZfsUberblock(bad_ver[0..]), true);
    if (_ := parseZfsUberblock(bad_ver[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. txg == 0
    const bad_txg = buildUberblock(true, 5000, 0);
    if (_ := parseZfsUberblock(bad_txg[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. Truncated Uberblock (< 1024B) and truncated BlkPtr (< 128B)
    const ok_ub = buildUberblock(false, 5000, 10);
    expect_eq(isZfsUberblock(ok_ub[0..1023]), false);
    if (_ := parseZfsUberblock(ok_ub[0..512])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let short_bp: [127]u8 = std.mem.zeroes([127]u8);
    if (_ := parseZfsBlkPtr(short_bp[0..], false)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
