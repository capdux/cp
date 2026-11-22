// Source: cp/cap/test/test_xfs_production_deep.cp (Production deep tests for cap:xfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    XFS_SB_MAGIC,
    XFS_SB_MIN_SIZE,
    XFS_SB_SECTOR_SIZE,
    XFS_INODE_MAGIC,
    XFS_INODE_CORE_SIZE,
    XFS_AGF_MAGIC,
    XFS_AGI_MAGIC,
    XFS_SB_CRC_OFFSET,
    XFS_BMBT_REC_SIZE,
    XFS_SB_VERSION_4,
    XFS_SB_VERSION_5,
    FMT_DEV,
    FMT_LOCAL,
    FMT_EXTENTS,
    FMT_BTREE,
    RO_COMPAT_FINOBT,
    RO_COMPAT_RMAPBT,
    RO_COMPAT_REFLINK,
    INCOMPAT_FTYPE,
    INCOMPAT_BIGTIME,
    XFS_S_IFMT,
    XFS_S_IFREG,
    XFS_S_IFDIR,
    xfsCrc32c,
    xfsCrc32cWithZeroField,
    XfsSuperblock,
    XfsInodeCore,
    XfsBmbtRec,
    isXfsSuperblock,
    parseXfsSuperblock,
    parseXfsInodeCore,
    decodeXfsBmbtRec,
    encodeXfsBmbtRec,
} from "cap:sys";

fn putU16Be(buf: []u8, off: usize, v: u16) {
    buf[off + 0] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off + 0] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putU64Be(buf: []u8, off: usize, v: u64) {
    putU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    putU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
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

fn buildSuperblock(ver: u8, ro_compat: u32, incompat: u32): [512]u8 {
    let sb: [512]u8 = std.mem.zeroes([512]u8);
    putU32Be(sb[0..], 0, XFS_SB_MAGIC);       // "XFSB"
    putU32Be(sb[0..], 4, 4096);               // blocksize (1 << 12)
    putU64Be(sb[0..], 8, 2621440);            // dblocks (10 GiB)
    putU64Be(sb[0..], 16, 0);                 // rblocks
    putU64Be(sb[0..], 24, 0);                 // rextents
    let i: usize = 0;
    while (i < 16) {
        sb[32 + i] = (0x10 + i) as u8;        // uuid
        i += 1;
    }
    putU64Be(sb[0..], 48, 0x80000007);        // logstart
    putU64Be(sb[0..], 56, 128);               // rootino
    putU32Be(sb[0..], 84, 655360);            // agblocks
    putU32Be(sb[0..], 88, 4);                 // agcount
    putU16Be(sb[0..], 100, (0xB4A0 as u16) | (ver as u16)); // versionnum
    putU16Be(sb[0..], 102, 512);              // sectsize (1 << 9)
    putU16Be(sb[0..], 104, 512);              // inodesize (1 << 9)
    putBytes(sb[0..], 108, "cp-xfs-root");    // fname (11 bytes + NUL)
    sb[120] = 12;                             // blocklog
    sb[121] = 9;                              // sectlog
    sb[122] = 9;                              // inodelog
    sb[124] = 20;                             // agblklog
    putU64Be(sb[0..], 128, 4096);             // icount
    putU64Be(sb[0..], 136, 1024);             // ifree
    putU64Be(sb[0..], 144, 2000000);          // fdblocks

    if (ver == XFS_SB_VERSION_5) {
        putU32Be(sb[0..], 208, 0);            // features_compat
        putU32Be(sb[0..], 212, ro_compat);    // features_ro_compat
        putU32Be(sb[0..], 216, incompat);     // features_incompat
        const crc = xfsCrc32cWithZeroField(sb[0..512], XFS_SB_CRC_OFFSET);
        putU32Be(sb[0..], XFS_SB_CRC_OFFSET, crc);
    }
    return sb;
}

test("1. XFS v5 Superblock parsing, 512B CRC32C verification, and Reflink/Ftype feature flags", () => {
    expect_eq(XFS_SB_MAGIC, 0x58465342 as u32);
    expect_eq(XFS_SB_MIN_SIZE, 264 as usize);
    expect_eq(XFS_SB_SECTOR_SIZE, 512 as usize);
    expect_eq(XFS_SB_CRC_OFFSET, 224 as usize);
    expect_eq(XFS_AGF_MAGIC, 0x58414746 as u32);
    expect_eq(XFS_AGI_MAGIC, 0x58414749 as u32);

    // Standard iSCSI CRC32C("123456789") == 0xE3069283
    expect_eq(xfsCrc32c("123456789"), 0xE3069283 as u32);

    const sb_raw = buildSuperblock(
        XFS_SB_VERSION_5,
        RO_COMPAT_FINOBT | RO_COMPAT_RMAPBT | RO_COMPAT_REFLINK,
        INCOMPAT_FTYPE | INCOMPAT_BIGTIME,
    );
    expect_eq(isXfsSuperblock(sb_raw[0..]), true);

    if (sb := parseXfsSuperblock(sb_raw[0..])) {
        expect_eq(sb.version(), XFS_SB_VERSION_5);
        expect_eq(sb.isV5(), true);
        expect_eq(sb.hasReflink(), true);
        expect_eq(sb.hasFtype(), true);
        expect_eq(sb.blocksize, 4096 as u32);
        expect_eq(sb.dblocks, 2621440 as u64);
        expect_eq(sb.totalBytes(), (2621440 as u128) * (4096 as u128));
        expect_eq(sb.rootino, 128 as u64);
        expect_eq(sb.agblocks, 655360 as u32);
        expect_eq(sb.agcount, 4 as u32);
        expect_eq(sb.sectsize, 512 as u16);
        expect_eq(sb.inodesize, 512 as u16);
        expect_eq(sb.blocklog, 12 as u8);
        expect_eq(sb.sectlog, 9 as u8);
        expect_eq(sb.inodelog, 9 as u8);
        expect_eq(sb.agblklog, 20 as u8);
        expect_eq(sb.icount, 4096 as u64);
        expect_eq(sb.ifree, 1024 as u64);
        expect_eq(sb.fdblocks, 2000000 as u64);
        expect_eq(sb.uuid[0], 0x10 as u8);
        expect_eq(sb.uuid[15], 0x1F as u8);
        expect_eq(slicesEq(sb.labelSlice(), "cp-xfs-root"), true);
    } else {
        expect_eq(true, false);
    }
});

test("2. XFS v4 Superblock parsing (no v5 CRC, full 12-byte label without NUL)", () => {
    let sb_v4 = buildSuperblock(XFS_SB_VERSION_4, 0, 0);
    putBytes(sb_v4[0..], 108, "XFS_V4_LABEL"); // exact 12 bytes, no NUL

    if (sb := parseXfsSuperblock(sb_v4[0..264])) {
        expect_eq(sb.version(), XFS_SB_VERSION_4);
        expect_eq(sb.isV5(), false);
        expect_eq(sb.hasReflink(), false);
        expect_eq(sb.hasFtype(), false);
        expect_eq(sb.crc, 0 as u32);
        expect_eq(sb.labelSlice().len, 12 as usize);
        expect_eq(slicesEq(sb.labelSlice(), "XFS_V4_LABEL"), true);
    } else {
        expect_eq(true, false);
    }
});

test("3. 128-bit BMBT Extent Record encode/decode round-trip across 64-bit boundary", () => {
    // Test case A: unwritten=true, max-width values spanning bit 63..64 boundary
    const rec_a = XfsBmbtRec {
        is_unwritten: true,
        startoff: 0x002A55AA55AA55,   // 54-bit logical file block offset
        startblock: 0x000F123456789A, // 52-bit physical block (splits 9 bits into l0, 43 bits into l1)
        blockcount: 0x1A5A5A,         // 21-bit block count
    };

    let buf_arr: [16]u8 = std.mem.zeroes([16]u8);
    let out_slice: []u8 = buf_arr[0..];
    if (written := encodeXfsBmbtRec(rec_a, *out_slice)) {
        expect_eq(written, XFS_BMBT_REC_SIZE);
    } else {
        expect_eq(true, false);
    }

    if (dec_a := decodeXfsBmbtRec(buf_arr[0..])) {
        expect_eq(dec_a.is_unwritten, true);
        expect_eq(dec_a.startoff, rec_a.startoff);
        expect_eq(dec_a.startblock, rec_a.startblock);
        expect_eq(dec_a.blockcount, rec_a.blockcount);
    } else {
        expect_eq(true, false);
    }

    // Test case B: unwritten=false, boundary max bitmasks (all 1s)
    const rec_b = XfsBmbtRec {
        is_unwritten: false,
        startoff: 0x003FFFFFFFFFFFFF,
        startblock: 0x000FFFFFFFFFFFFF,
        blockcount: 0x1FFFFF,
    };
    if (written := encodeXfsBmbtRec(rec_b, *out_slice)) {
        expect_eq(written, 16 as usize);
    } else {
        expect_eq(true, false);
    }
    if (dec_b := decodeXfsBmbtRec(buf_arr[0..])) {
        expect_eq(dec_b.is_unwritten, false);
        expect_eq(dec_b.startoff, 0x003FFFFFFFFFFFFF as u64);
        expect_eq(dec_b.startblock, 0x000FFFFFFFFFFFFF as u64);
        expect_eq(dec_b.blockcount, 0x1FFFFF as u32);
    } else {
        expect_eq(true, false);
    }

    // Overflow & undersized buffer rejection
    const bad_rec = XfsBmbtRec {
        is_unwritten: false,
        startoff: 0x0040000000000000, // exceeds 54 bits
        startblock: 0,
        blockcount: 1,
    };
    if (_ := encodeXfsBmbtRec(bad_rec, *out_slice)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let short_arr: [15]u8 = std.mem.zeroes([15]u8);
    let short_slice: []u8 = short_arr[0..];
    if (_ := encodeXfsBmbtRec(rec_a, *short_slice)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});

test("4. XFS v2 and v3 Inode Core ('IN') parsing", () => {
    // 4a. v3 regular file with FMT_EXTENTS
    let ino_v3: [176]u8 = std.mem.zeroes([176]u8);
    putU16Be(ino_v3[0..], 0, XFS_INODE_MAGIC);        // "IN"
    putU16Be(ino_v3[0..], 2, XFS_S_IFREG | 0x01A4);       // 0100644
    ino_v3[4] = 3;                                    // version = 3
    ino_v3[5] = FMT_EXTENTS;                          // format = 2
    putU32Be(ino_v3[0..], 8, 1000);                   // uid
    putU32Be(ino_v3[0..], 12, 1000);                  // gid
    putU32Be(ino_v3[0..], 16, 1);                     // nlink
    putU64Be(ino_v3[0..], 56, 1048576);               // size (1 MiB)
    putU64Be(ino_v3[0..], 64, 256);                   // nblocks
    putU32Be(ino_v3[0..], 76, 4);                     // nextents

    if (ino := parseXfsInodeCore(ino_v3[0..])) {
        expect_eq(ino.isV3(), true);
        expect_eq(ino.isRegular(), true);
        expect_eq(ino.isDirectory(), false);
        expect_eq(ino.format, FMT_EXTENTS);
        expect_eq(ino.uid, 1000 as u32);
        expect_eq(ino.gid, 1000 as u32);
        expect_eq(ino.nlink, 1 as u32);
        expect_eq(ino.size, 1048576 as u64);
        expect_eq(ino.nblocks, 256 as u64);
        expect_eq(ino.nextents, 4 as u32);
    } else {
        expect_eq(true, false);
    }

    // 4b. v2 directory with FMT_LOCAL
    let ino_v2: [96]u8 = std.mem.zeroes([96]u8);
    putU16Be(ino_v2[0..], 0, XFS_INODE_MAGIC);
    putU16Be(ino_v2[0..], 2, XFS_S_IFDIR | 0x01ED);       // 0040755
    ino_v2[4] = 2;                                    // version = 2
    ino_v2[5] = FMT_LOCAL;
    putU32Be(ino_v2[0..], 16, 5);                     // nlink

    if (dir := parseXfsInodeCore(ino_v2[0..])) {
        expect_eq(dir.isV3(), false);
        expect_eq(dir.isRegular(), false);
        expect_eq(dir.isDirectory(), true);
        expect_eq(dir.format, FMT_LOCAL);
        expect_eq(dir.nlink, 5 as u32);
    } else {
        expect_eq(true, false);
    }

    expect_eq(FMT_DEV, 0 as u8);
    expect_eq(FMT_BTREE, 3 as u8);
    expect_eq(XFS_S_IFMT, 0xF000 as u16);
});

test("5. Malformed input rejection: bad magic, corrupt v5 CRC32C, mismatched blocklog, unsupported version, truncation", () => {
    // 5a. Wrong magic
    let bad_magic = buildSuperblock(XFS_SB_VERSION_5, 0, 0);
    bad_magic[0] = 'Z';
    expect_eq(isXfsSuperblock(bad_magic[0..]), false);
    if (_ := parseXfsSuperblock(bad_magic[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Bad v5 CRC32C (flip 1 bit in payload)
    let bad_crc = buildSuperblock(XFS_SB_VERSION_5, RO_COMPAT_REFLINK, INCOMPAT_FTYPE);
    bad_crc[108] ^= 0x01;
    expect_eq(isXfsSuperblock(bad_crc[0..]), true);
    if (_ := parseXfsSuperblock(bad_crc[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. blocksize != (1 << blocklog)
    let bad_log = buildSuperblock(XFS_SB_VERSION_4, 0, 0);
    bad_log[120] = 11; // 1<<11 == 2048 != 4096
    if (_ := parseXfsSuperblock(bad_log[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. Unsupported version (e.g. version 3)
    let bad_ver = buildSuperblock(3, 0, 0);
    if (_ := parseXfsSuperblock(bad_ver[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5e. Truncated superblock (v5 needs full 512B sector), truncated inode, truncated BMBT
    const valid_v5 = buildSuperblock(XFS_SB_VERSION_5, 0, 0);
    if (_ := parseXfsSuperblock(valid_v5[0..264])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
    expect_eq(isXfsSuperblock(valid_v5[0..263]), false);

    let short_ino: [95]u8 = std.mem.zeroes([95]u8);
    putU16Be(short_ino[0..], 0, XFS_INODE_MAGIC);
    if (_ := parseXfsInodeCore(short_ino[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let short_bmbt: [15]u8 = std.mem.zeroes([15]u8);
    if (_ := decodeXfsBmbtRec(short_bmbt[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
