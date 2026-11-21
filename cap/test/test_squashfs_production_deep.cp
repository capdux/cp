// Source: cp/cap/test/test_squashfs_production_deep.cp (Production deep tests for cap:squashfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SQUASHFS_MAGIC,
    SQUASHFS_SUPER_SIZE,
    SQUASHFS_MAJOR,
    SQUASHFS_MINOR,
    SQUASHFS_METADATA_MAX_SIZE,
    SQUASHFS_META_UNCOMPRESSED_BIT,
    SQUASHFS_DATA_UNCOMPRESSED_BIT,
    SQUASHFS_INVALID_OFFSET,
    COMP_GZIP,
    COMP_LZMA,
    COMP_LZO,
    COMP_XZ,
    COMP_LZ4,
    COMP_ZSTD,
    FLAG_UNCOMPRESSED_INODES,
    FLAG_UNCOMPRESSED_DATA,
    FLAG_UNCOMPRESSED_FRAGMENTS,
    FLAG_NO_FRAGMENTS,
    FLAG_ALWAYS_FRAGMENTS,
    FLAG_DUPLICATES,
    FLAG_EXPORTABLE,
    FLAG_UNCOMPRESSED_XATTRS,
    FLAG_NO_XATTRS,
    FLAG_COMPRESSOR_OPTIONS,
    FLAG_UNCOMPRESSED_IDS,
    INODE_DIR,
    INODE_REG,
    INODE_SYMLINK,
    INODE_BLKDEV,
    INODE_CHRDEV,
    INODE_FIFO,
    INODE_SOCK,
    SquashfsSuperblock,
    isSquashfs,
    parseSquashfsSuperblock,
    SquashfsInodeRef,
    decodeInodeRef,
    encodeInodeRef,
    SquashfsMetaHeader,
    parseMetaBlock,
    SquashfsDataBlockSize,
    decodeDataBlockSize,
    SquashfsFragmentEntry,
    parseFragmentEntry,
} from "cap:squashfs";

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

fn buildValidSuperblock(buf: []u8, comp_id: u16, block_log: u16, flags: u16) {
    putU32Le(buf, 0, SQUASHFS_MAGIC);
    putU32Le(buf, 4, 1024); // inodes
    putU32Le(buf, 8, 1795300000); // mkfs_time
    putU32Le(buf, 12, (1 as u32) << (block_log as u5)); // block_size
    putU32Le(buf, 16, 12); // fragments
    putU16Le(buf, 20, comp_id);
    putU16Le(buf, 22, block_log);
    putU16Le(buf, 24, flags);
    putU16Le(buf, 26, 2); // no_ids
    putU16Le(buf, 28, SQUASHFS_MAJOR);
    putU16Le(buf, 30, SQUASHFS_MINOR);
    const root_ref = encodeInodeRef(0x000000001234, 0x00A0) ?? unreachable;
    putU64Le(buf, 32, root_ref);
    putU64Le(buf, 40, 1048576); // bytes_used
    putU64Le(buf, 48, 900000); // id_table_start
    putU64Le(buf, 56, 950000); // xattr_id_table_start
    putU64Le(buf, 64, 700000); // inode_table_start
    putU64Le(buf, 72, 750000); // directory_table_start
    putU64Le(buf, 80, 800000); // fragment_table_start
    putU64Le(buf, 88, 850000); // lookup_table_start
}

test("1. 96B SquashFS v4.0 Superblock parsing (zstd / xz / gzip) & flag helpers", () => {
    expect_eq(SQUASHFS_SUPER_SIZE, 96 as usize);
    expect_eq(SQUASHFS_METADATA_MAX_SIZE, 8192 as usize);
    expect_eq(COMP_LZMA, 2 as u16);
    expect_eq(COMP_LZO, 3 as u16);
    expect_eq(COMP_LZ4, 5 as u16);
    expect_eq(FLAG_UNCOMPRESSED_FRAGMENTS, 0x0008 as u16);
    expect_eq(FLAG_ALWAYS_FRAGMENTS, 0x0020 as u16);
    expect_eq(FLAG_DUPLICATES, 0x0040 as u16);
    expect_eq(FLAG_UNCOMPRESSED_XATTRS, 0x0100 as u16);
    expect_eq(FLAG_COMPRESSOR_OPTIONS, 0x0400 as u16);
    expect_eq(FLAG_UNCOMPRESSED_IDS, 0x0800 as u16);
    expect_eq(INODE_DIR, 1 as u16);
    expect_eq(INODE_REG, 2 as u16);
    expect_eq(INODE_SYMLINK, 3 as u16);
    expect_eq(INODE_BLKDEV, 4 as u16);
    expect_eq(INODE_CHRDEV, 5 as u16);
    expect_eq(INODE_FIFO, 6 as u16);
    expect_eq(INODE_SOCK, 7 as u16);

    let sb_buf: [96]u8 = std.mem.zeroes([96]u8);
    buildValidSuperblock(sb_buf[0..], COMP_ZSTD, 17, FLAG_DUPLICATES | FLAG_EXPORTABLE);
    expect_eq(isSquashfs(sb_buf[0..]), true);

    const sb = parseSquashfsSuperblock(sb_buf[0..]) ?? unreachable;
    expect_eq(sb.inodes, 1024 as u32);
    expect_eq(sb.block_size, 131072 as u32); // 1 << 17 = 128 KiB
    expect_eq(sb.block_log, 17 as u16);
    expect_eq(sb.major, 4 as u16);
    expect_eq(sb.minor, 0 as u16);
    expect_eq(std.mem.eql(u8, sb.compressionName(), "zstd"), true);
    expect_eq(sb.hasXattrs(), true);
    expect_eq(sb.hasFragments(), true);
    expect_eq(sb.hasExportTable(), true);
    expect_eq(sb.inodesUncompressed(), false);
    expect_eq(sb.dataUncompressed(), false);

    // Check xz & gzip names + NO_XATTRS / NO_FRAGMENTS / UNCOMPRESSED flags
    buildValidSuperblock(
        sb_buf[0..],
        COMP_XZ,
        18,
        FLAG_NO_XATTRS | FLAG_NO_FRAGMENTS | FLAG_UNCOMPRESSED_INODES | FLAG_UNCOMPRESSED_DATA,
    );
    const sb_xz = parseSquashfsSuperblock(sb_buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, sb_xz.compressionName(), "xz"), true);
    expect_eq(sb_xz.block_size, 262144 as u32);
    expect_eq(sb_xz.hasXattrs(), false);
    expect_eq(sb_xz.hasFragments(), false);
    expect_eq(sb_xz.hasExportTable(), false);
    expect_eq(sb_xz.inodesUncompressed(), true);
    expect_eq(sb_xz.dataUncompressed(), true);

    buildValidSuperblock(sb_buf[0..], COMP_GZIP, 12, 0);
    const sb_gz = parseSquashfsSuperblock(sb_buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, sb_gz.compressionName(), "gzip"), true);
    expect_eq(sb_gz.block_size, 4096 as u32);
});

test("2. SquashfsInodeRef 48-bit block_offset & 13-bit inner_offset round-trip", () => {
    const inode_ref = encodeInodeRef(0x000012345678, 8191) ?? unreachable;
    const decoded = decodeInodeRef(inode_ref);
    expect_eq(decoded.block_offset, 0x000012345678 as u64);
    expect_eq(decoded.inner_offset, 8191 as u16);

    // Out-of-range inner_offset (>= 8192) or block_offset (> 48-bit) rejected
    expect_eq(encodeInodeRef(0, 8192) == null, true);
    expect_eq(encodeInodeRef(0x0001000000000000, 0) == null, true);
});

test("3. 2-byte Metadata Block compressed & uncompressed header parsing", () => {
    // Compressed metadata block: size=5, uncompressed bit = 0 -> 0x0005
    let mb1: [10]u8 = std.mem.zeroes([10]u8);
    putU16Le(mb1[0..], 0, 5);
    mb1[2] = 'H';
    mb1[3] = 'E';
    mb1[4] = 'L';
    mb1[5] = 'L';
    mb1[6] = 'O';
    const mh1 = parseMetaBlock(mb1[0..]) ?? unreachable;
    expect_eq(mh1.is_compressed, true);
    expect_eq(mh1.size, 5 as u16);
    expect_eq(mh1.consumed, 7 as usize);
    expect_eq(std.mem.eql(u8, mh1.payload, "HELLO"), true);

    // Uncompressed metadata block: size=4 | 0x8000 -> 0x8004
    let mb2: [6]u8 = std.mem.zeroes([6]u8);
    putU16Le(mb2[0..], 0, SQUASHFS_META_UNCOMPRESSED_BIT | 4);
    mb2[2] = 'R';
    mb2[3] = 'A';
    mb2[4] = 'W';
    mb2[5] = '!';
    const mh2 = parseMetaBlock(mb2[0..]) ?? unreachable;
    expect_eq(mh2.is_compressed, false);
    expect_eq(mh2.size, 4 as u16);
    expect_eq(mh2.consumed, 6 as usize);
    expect_eq(std.mem.eql(u8, mh2.payload, "RAW!"), true);
});

test("4. Data Block size (sparse/compressed/uncompressed) & 16B Fragment Entry parsing", () => {
    // Sparse block (size == 0)
    const d_sparse = decodeDataBlockSize(0);
    expect_eq(d_sparse.is_sparse, true);
    expect_eq(d_sparse.is_compressed, false);
    expect_eq(d_sparse.size, 0 as u32);

    // Compressed block (4096 bytes)
    const d_comp = decodeDataBlockSize(4096);
    expect_eq(d_comp.is_sparse, false);
    expect_eq(d_comp.is_compressed, true);
    expect_eq(d_comp.size, 4096 as u32);

    // Uncompressed block (8192 bytes | 0x01000000)
    const d_raw = decodeDataBlockSize(SQUASHFS_DATA_UNCOMPRESSED_BIT | 8192);
    expect_eq(d_raw.is_sparse, false);
    expect_eq(d_raw.is_compressed, false);
    expect_eq(d_raw.size, 8192 as u32);

    // 16B Fragment Entry
    let frag: [16]u8 = std.mem.zeroes([16]u8);
    putU64Le(frag[0..], 0, 0x00000000_00080000);
    putU32Le(frag[0..], 8, SQUASHFS_DATA_UNCOMPRESSED_BIT | 12345);
    const fe = parseFragmentEntry(frag[0..]) ?? unreachable;
    expect_eq(fe.start_block, 0x00080000 as u64);
    expect_eq(fe.is_compressed, false);
    expect_eq(fe.size, 12345 as u32);
    expect_eq(parseFragmentEntry(frag[0..15]) == null, true);
});

test("5. Malformed magic, block_size mismatch, bad compression_id & metadata > 8192 rejection", () => {
    let sb_buf: [96]u8 = std.mem.zeroes([96]u8);
    buildValidSuperblock(sb_buf[0..], COMP_ZSTD, 17, 0);

    // Short superblock
    expect_eq(parseSquashfsSuperblock(sb_buf[0..95]) == null, true);

    // Bad magic
    sb_buf[0] = 'X';
    expect_eq(isSquashfs(sb_buf[0..]), false);
    expect_eq(parseSquashfsSuperblock(sb_buf[0..]) == null, true);
    buildValidSuperblock(sb_buf[0..], COMP_ZSTD, 17, 0);

    // block_size != (1 << block_log)
    putU32Le(sb_buf[0..], 12, 65536); // mismatches block_log=17
    expect_eq(parseSquashfsSuperblock(sb_buf[0..]) == null, true);

    // Invalid compression_id = 7
    buildValidSuperblock(sb_buf[0..], COMP_ZSTD, 17, 0);
    putU16Le(sb_buf[0..], 20, 7);
    expect_eq(parseSquashfsSuperblock(sb_buf[0..]) == null, true);

    // Wrong version (major = 3)
    buildValidSuperblock(sb_buf[0..], COMP_ZSTD, 17, 0);
    putU16Le(sb_buf[0..], 28, 3);
    expect_eq(parseSquashfsSuperblock(sb_buf[0..]) == null, true);

    // Metadata block with size > 8192, size == 0, or truncated payload
    let bad_meta: [16]u8 = std.mem.zeroes([16]u8);
    putU16Le(bad_meta[0..], 0, 8193);
    expect_eq(parseMetaBlock(bad_meta[0..]) == null, true);
    putU16Le(bad_meta[0..], 0, 0);
    expect_eq(parseMetaBlock(bad_meta[0..]) == null, true);
    putU16Le(bad_meta[0..], 0, 100);
    expect_eq(parseMetaBlock(bad_meta[0..]) == null, true);
});
