// Source: cp/cap/test/test_sstable_production_deep.cp (Production deep tests for cap:sstable)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SST_FOOTER_SIZE,
    SST_LEVELDB_MAGIC,
    SST_ROCKSDB_MAGIC,
    SST_BLOCK_TRAILER_SIZE,
    SstCompression,
    BlockHandle,
    SstFooter,
    SstBlockView,
    SstBlockHeader,
    SstRawEntry,
    SstBlockIter,
    maskCrc,
    unmaskCrc,
    decodeVarint64,
    encodeVarint64,
    decodeBlockHandle,
    encodeBlockHandle,
    parseSstFooter,
    isSst,
    extractSstBlock,
    parseSstBlockInfo,
} from "cap:db";

fn putU32LE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU64LE(buf: *[]u8, off: usize, v: u64) {
    putU32LE(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32LE(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn copyBytes(dst: *[]u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

test("1. Varint64 and BlockHandle round-trip across boundary values", () => {
    let buf: [32]u8 = [0; 32];
    let b = buf[0..32];

    const h_in = BlockHandle {
        offset: 0x123456789ABCDEF0,
        size: 16384,
    };
    const written = encodeBlockHandle(*b, h_in) ?? unreachable;
    const decoded = decodeBlockHandle(b[0..written], 0) ?? unreachable;
    expect_eq(decoded.next_pos, written);
    expect_eq(decoded.handle.offset, h_in.offset);
    expect_eq(decoded.handle.size, h_in.size);

    // Single-byte 0 and max u64 varint round-trip
    const n0 = encodeVarint64(*b, 0) ?? unreachable;
    expect_eq(n0, 1);
    const r0 = decodeVarint64(b[0..1], 0) ?? unreachable;
    expect_eq(r0.value, 0);

    const nmax = encodeVarint64(*b, 0xFFFFFFFFFFFFFFFF) ?? unreachable;
    expect_eq(nmax, 10);
    const rmax = decodeVarint64(b[0..10], 0) ?? unreachable;
    expect_eq(rmax.value, 0xFFFFFFFFFFFFFFFF);
});

test("2. parseSstFooter and isSst decode both LevelDB and RocksDB 48B footers", () => {
    // Minimal synthetic SSTable: 10B meta block + 5B trailer + 12B index block + 5B trailer + 48B footer = 80B
    let file: [80]u8 = [0; 80];
    let f = file[0..80];

    const meta_h = BlockHandle { offset: 0, size: 10 };
    const idx_h = BlockHandle { offset: 15, size: 12 };

    let foot_handles = f[32..72];
    const n1 = encodeBlockHandle(*foot_handles, meta_h) ?? unreachable;
    let tail_handles = foot_handles[n1..40];
    * = encodeBlockHandle(*tail_handles, idx_h) ?? unreachable;

    // LevelDB magic
    putU64LE(*f, 72, SST_LEVELDB_MAGIC);
    expect_eq(isSst(f), true);
    const ldb = parseSstFooter(f) ?? unreachable;
    expect_eq(ldb.is_rocksdb, false);
    expect_eq(ldb.magic, SST_LEVELDB_MAGIC);
    expect_eq(ldb.metaindex_handle.offset, 0);
    expect_eq(ldb.metaindex_handle.size, 10);
    expect_eq(ldb.index_handle.offset, 15);
    expect_eq(ldb.index_handle.size, 12);

    // RocksDB magic
    putU64LE(*f, 72, SST_ROCKSDB_MAGIC);
    expect_eq(isSst(f), true);
    const rdb = parseSstFooter(f) ?? unreachable;
    expect_eq(rdb.is_rocksdb, true);
    expect_eq(rdb.magic, SST_ROCKSDB_MAGIC);
});

test("3. extractSstBlock decodes payload, compression type, and CRC32C mask/unmask round-trip", () => {
    const raw_crc: u32 = 0xDEADBEEF;
    const masked = maskCrc(raw_crc);
    expect_eq(unmaskCrc(masked), raw_crc);
    expect_eq(unmaskCrc(maskCrc(0)), 0);
    expect_eq(unmaskCrc(maskCrc(0xFFFFFFFF)), 0xFFFFFFFF);

    let file: [32]u8 = [0; 32];
    let f = file[0..32];
    copyBytes(*f, 4, "HELLO_BLOCK"); // 11 bytes at offset 4..15
    f[15] = 7; // zstd
    putU32LE(*f, 16, masked);

    const blk = extractSstBlock(f, BlockHandle { offset: 4, size: 11 }) ?? unreachable;
    expect_eq(std.mem.eql(u8, blk.payload, "HELLO_BLOCK"), true);
    expect_eq(blk.compression == SstCompression.zstd, true);
    expect_eq(blk.masked_crc32c, masked);
    expect_eq(blk.crc32c(), raw_crc);
});

test("4. parseSstBlockInfo and SstBlockIter decode prefix-compressed Data Block with multiple restarts", () => {
    // Build a block with 3 entries and 2 restart points:
    // Entry 0 (offset 0, restart 0): shared=0, non_shared=5 ("apple"), val="red" (3B)
    // Entry 1 (shared prefix):       shared=3, non_shared=3 ("ric"),   val="tan" (3B)
    // Entry 2 (restart 1):           shared=0, non_shared=6 ("banana"),val="yel" (3B)
    let blk: [128]u8 = [0; 128];
    let b = blk[0..128];
    let pos: usize = 0;

    // Entry 0
    const r0_off: u32 = pos as u32;
    b[pos] = 0; // shared
    b[pos + 1] = 5; // non_shared
    b[pos + 2] = 3; // val_len
    pos += 3;
    copyBytes(*b, pos, "apple");
    pos += 5;
    copyBytes(*b, pos, "red");
    pos += 3;

    // Entry 1
    b[pos] = 3; // shared ("app")
    b[pos + 1] = 3; // non_shared ("ric")
    b[pos + 2] = 3; // val_len
    pos += 3;
    copyBytes(*b, pos, "ric");
    pos += 3;
    copyBytes(*b, pos, "tan");
    pos += 3;

    // Entry 2 (restart point 1)
    const r1_off: u32 = pos as u32;
    b[pos] = 0;
    b[pos + 1] = 6;
    b[pos + 2] = 3;
    pos += 3;
    copyBytes(*b, pos, "banana");
    pos += 6;
    copyBytes(*b, pos, "yel");
    pos += 3;

    // Restart array: [r0_off, r1_off] + num_restarts = 2
    const restarts_start = pos;
    putU32LE(*b, pos, r0_off);
    pos += 4;
    putU32LE(*b, pos, r1_off);
    pos += 4;
    putU32LE(*b, pos, 2);
    pos += 4;

    const info = parseSstBlockInfo(b[0..pos]) ?? unreachable;
    expect_eq(info.num_restarts, 2);
    expect_eq(info.restarts_offset, restarts_start);
    expect_eq(info.restartOffset(0) ?? 999, r0_off);
    expect_eq(info.restartOffset(1) ?? 999, r1_off);
    expect_eq(info.restartOffset(2) == null, true);

    let it = info.entries();
    const e0 = it.next() ?? unreachable;
    expect_eq(e0.shared_bytes, 0);
    expect_eq(e0.non_shared_bytes, 5);
    expect_eq(std.mem.eql(u8, e0.key_delta, "apple"), true);
    expect_eq(std.mem.eql(u8, e0.value, "red"), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.shared_bytes, 3);
    expect_eq(e1.non_shared_bytes, 3);
    expect_eq(std.mem.eql(u8, e1.key_delta, "ric"), true);
    expect_eq(std.mem.eql(u8, e1.value, "tan"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.shared_bytes, 0);
    expect_eq(e2.non_shared_bytes, 6);
    expect_eq(std.mem.eql(u8, e2.key_delta, "banana"), true);
    expect_eq(std.mem.eql(u8, e2.value, "yel"), true);

    expect_eq(it.next() == null, true);
});

test("5. malformed footers, bad magic, out-of-bounds handles, and corrupted num_restarts fail safely", () => {
    const short_buf: [16]u8 = [0; 16];
    expect_eq(isSst(short_buf[0..16]), false);
    expect_eq(parseSstFooter(short_buf[0..16]) == null, true);

    // Wrong magic
    let bad_foot: [48]u8 = [0; 48];
    let bf = bad_foot[0..48];
    putU64LE(*bf, 40, 0x1122334455667788);
    expect_eq(parseSstFooter(bf) == null, true);

    // Valid magic but out-of-bounds handle (offset + size + 5 > 48)
    putU64LE(*bf, 40, SST_LEVELDB_MAGIC);
    let hbuf = bf[0..40];
    const n = encodeBlockHandle(*hbuf, BlockHandle { offset: 0, size: 100 }) ?? unreachable;
    let htail = hbuf[n..40];
    * = encodeBlockHandle(*htail, BlockHandle { offset: 0, size: 0 }) ?? unreachable;
    expect_eq(parseSstFooter(bf) == null, true);

    // extractSstBlock out-of-bounds
    expect_eq(extractSstBlock(bf, BlockHandle { offset: 44, size: 10 }) == null, true);

    // Block info with num_restarts == 0 or num_restarts exceeding block size
    let bad_blk: [12]u8 = [0; 12];
    let bb = bad_blk[0..12];
    putU32LE(*bb, 8, 0);
    expect_eq(parseSstBlockInfo(bb) == null, true);
    putU32LE(*bb, 8, 99);
    expect_eq(parseSstBlockInfo(bb) == null, true);

    // Truncated varint (all bytes have continuation bit 0x80)
    const bad_varint: [3]u8 = [0x80, 0x80, 0x80];
    expect_eq(decodeVarint64(bad_varint[0..3], 0) == null, true);
});
