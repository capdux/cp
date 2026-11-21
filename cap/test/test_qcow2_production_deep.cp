// Source: cp/cap/test/test_qcow2_production_deep.cp (Production deep tests for cap:qcow2)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    QCOW2_MAGIC,
    QCOW2_V2_HEADER_SIZE,
    QCOW2_V3_MIN_HEADER_SIZE,
    QCOW2_MIN_CLUSTER_BITS,
    QCOW2_MAX_CLUSTER_BITS,
    CRYPT_NONE,
    CRYPT_AES,
    CRYPT_LUKS,
    INCOMPAT_DIRTY,
    INCOMPAT_CORRUPT,
    INCOMPAT_EXTERNAL_DATA,
    INCOMPAT_COMPRESSION_TYPE,
    INCOMPAT_EXTENDED_L2,
    EXT_END,
    EXT_BACKING_FORMAT,
    EXT_FEATURE_NAME_TABLE,
    EXT_BITMAPS,
    EXT_FULL_DISK_ENCRYPTION,
    Qcow2Header,
    isQcow2,
    parseQcow2Header,
    Qcow2Ext,
    Qcow2ExtIter,
    iterQcow2Exts,
    Qcow2L1Entry,
    decodeL1Entry,
    Qcow2L2Entry,
    decodeL2Entry,
} from "cap:qcow2";

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putU64Be(buf: []u8, off: usize, v: u64) {
    putU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    putU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn initQcow2Base(buf: []u8, version: u32, cluster_bits: u32) {
    putU32Be(buf, 0, QCOW2_MAGIC);
    putU32Be(buf, 4, version);
    putU64Be(buf, 8, 0); // backing_file_offset
    putU32Be(buf, 16, 0); // backing_file_size
    putU32Be(buf, 20, cluster_bits);
    putU64Be(buf, 24, 10737418240); // 10 GiB
    putU32Be(buf, 32, CRYPT_NONE);
    putU32Be(buf, 36, 16); // l1_size
    putU64Be(buf, 40, 0x30000); // l1_table_offset
    putU64Be(buf, 48, 0x10000); // refcount_table_offset
    putU32Be(buf, 56, 1); // refcount_table_clusters
    putU32Be(buf, 60, 0); // nb_snapshots
    putU64Be(buf, 64, 0); // snapshots_offset
    if (version == 3) {
        putU64Be(buf, 72, INCOMPAT_DIRTY | INCOMPAT_EXTENDED_L2);
        putU64Be(buf, 80, 0);
        putU64Be(buf, 88, 0);
        putU32Be(buf, 96, 4); // refcount_order = 4 -> 16-bit
        putU32Be(buf, 100, 104); // header_length
    }
}

test("1. QCOW2 v2 (72B) & v3 (104B) header parsing, clusterSize, refcountBits & feature flags", () => {
    expect_eq(QCOW2_V2_HEADER_SIZE, 72 as usize);
    expect_eq(QCOW2_V3_MIN_HEADER_SIZE, 104 as usize);
    expect_eq(QCOW2_MIN_CLUSTER_BITS, 9 as u32);
    expect_eq(QCOW2_MAX_CLUSTER_BITS, 21 as u32);
    expect_eq(CRYPT_AES, 1 as u32);
    expect_eq(CRYPT_LUKS, 2 as u32);
    expect_eq(INCOMPAT_EXTERNAL_DATA, 0x04 as u64);
    expect_eq(INCOMPAT_COMPRESSION_TYPE, 0x08 as u64);
    expect_eq(EXT_BITMAPS, 0x23852875 as u32);
    expect_eq(EXT_FULL_DISK_ENCRYPTION, 0x0537BE77 as u32);

    // v2 header (72 bytes)
    let v2: [72]u8 = std.mem.zeroes([72]u8);
    initQcow2Base(v2[0..], 2, 16); // 64 KiB clusters
    expect_eq(isQcow2(v2[0..]), true);

    const h2 = parseQcow2Header(v2[0..]) ?? unreachable;
    expect_eq(h2.version, 2 as u32);
    expect_eq(h2.clusterSize(), 65536 as u64);
    expect_eq(h2.refcountBits(), 16 as u32);
    expect_eq(h2.header_length, 72 as u32);
    expect_eq(h2.isDirty(), false);
    expect_eq(h2.isCorrupt(), false);
    expect_eq(h2.hasExtendedL2(), false);
    expect_eq(h2.hasBackingFile(), false);
    expect_eq(h2.backingFile(v2[0..]) == null, true);

    // v3 header (104 bytes)
    let v3: [104]u8 = std.mem.zeroes([104]u8);
    initQcow2Base(v3[0..], 3, 17); // 128 KiB clusters
    putU32Be(v3[0..], 96, 5); // refcount_order = 5 -> 32-bit
    const h3 = parseQcow2Header(v3[0..]) ?? unreachable;
    expect_eq(h3.version, 3 as u32);
    expect_eq(h3.clusterSize(), 131072 as u64);
    expect_eq(h3.refcountBits(), 32 as u32);
    expect_eq(h3.size, 10737418240 as u64);
    expect_eq(h3.isDirty(), true);
    expect_eq(h3.isCorrupt(), false);
    expect_eq(h3.hasExtendedL2(), true);
});

test("2. Backing File zero-copy extraction & out-of-bounds protection", () => {
    let img: [160]u8 = std.mem.zeroes([160]u8);
    initQcow2Base(img[0..], 3, 16);
    const backing = "/var/lib/libvirt/base.qcow2";
    putU64Be(img[0..], 8, 120);
    putU32Be(img[0..], 16, backing.len as u32);
    copyBytes(img[0..], 120, backing);

    const hdr = parseQcow2Header(img[0..]) ?? unreachable;
    expect_eq(hdr.hasBackingFile(), true);
    expect_eq(std.mem.eql(u8, hdr.backingFile(img[0..]) ?? unreachable, backing), true);

    // Truncated image slice where backing file extends past EOF returns null
    expect_eq(hdr.backingFile(img[0..130]) == null, true);
});

test("3. 8-byte aligned Header Extensions iteration (BackingFormat + FeatureNameTable + End)", () => {
    let img: [192]u8 = std.mem.zeroes([192]u8);
    initQcow2Base(img[0..], 3, 16);

    // Offset 104: EXT_BACKING_FORMAT, len=5 ("qcow2"), padded to 8B -> total 16B [104..120]
    putU32Be(img[0..], 104, EXT_BACKING_FORMAT);
    putU32Be(img[0..], 108, 5);
    copyBytes(img[0..], 112, "qcow2");

    // Offset 120: EXT_FEATURE_NAME_TABLE, len=8 -> total 16B [120..136]
    putU32Be(img[0..], 120, EXT_FEATURE_NAME_TABLE);
    putU32Be(img[0..], 124, 8);
    copyBytes(img[0..], 128, "FEATDATA");

    // Offset 136: EXT_END (type=0, len=0) [136..144]
    putU32Be(img[0..], 136, EXT_END);
    putU32Be(img[0..], 140, 0);

    const hdr = parseQcow2Header(img[0..]) ?? unreachable;
    let it = iterQcow2Exts(hdr, img[0..]);

    const e0 = it.next() ?? unreachable;
    expect_eq(e0.ext_type, EXT_BACKING_FORMAT);
    expect_eq(std.mem.eql(u8, e0.data, "qcow2"), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.ext_type, EXT_FEATURE_NAME_TABLE);
    expect_eq(std.mem.eql(u8, e1.data, "FEATDATA"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.sawEnd(), true);
    expect_eq(it.isMalformed(), false);
});

test("4. L1 and L2 cluster table entry decoding (Copied / Compressed / Zero / Host Offset)", () => {
    // L1 entry: copied (bit 63) + L2 table at 0x00000000_00040000
    const l1 = decodeL1Entry(0x80000000_00040000);
    expect_eq(l1.isCopied(), true);
    expect_eq(l1.l2Offset(), 0x00040000 as u64);

    // L2 standard copied cluster at 0x00000000_00100000
    const l2_std = decodeL2Entry(0x80000000_00100000);
    expect_eq(l2_std.isCopied(), true);
    expect_eq(l2_std.isCompressed(), false);
    expect_eq(l2_std.isZeroCluster(), false);
    expect_eq(l2_std.hostOffset(), 0x00100000 as u64);

    // L2 zero cluster (bit 0 set, non-compressed)
    const l2_zero = decodeL2Entry(0x80000000_00200001);
    expect_eq(l2_zero.isCopied(), true);
    expect_eq(l2_zero.isCompressed(), false);
    expect_eq(l2_zero.isZeroCluster(), true);
    expect_eq(l2_zero.hostOffset(), 0x00200000 as u64);

    // L2 compressed cluster (bit 62 set)
    const l2_comp = decodeL2Entry(0x40000000_00300001);
    expect_eq(l2_comp.isCopied(), false);
    expect_eq(l2_comp.isCompressed(), true);
    expect_eq(l2_comp.isZeroCluster(), false);
    expect_eq(l2_comp.hostOffset(), 0 as u64);
});

test("5. Malformed magic, bad version, cluster_bits bounds, crypt_method & truncation rejection", () => {
    let buf: [104]u8 = std.mem.zeroes([104]u8);
    initQcow2Base(buf[0..], 3, 16);

    // Bad magic
    buf[0] = 0x00;
    expect_eq(isQcow2(buf[0..]), false);
    expect_eq(parseQcow2Header(buf[0..]) == null, true);

    // Bad version (1 and 4)
    initQcow2Base(buf[0..], 3, 16);
    putU32Be(buf[0..], 4, 1);
    expect_eq(parseQcow2Header(buf[0..]) == null, true);
    putU32Be(buf[0..], 4, 4);
    expect_eq(parseQcow2Header(buf[0..]) == null, true);

    // Out-of-bounds cluster_bits (8 and 22)
    initQcow2Base(buf[0..], 3, 8);
    expect_eq(parseQcow2Header(buf[0..]) == null, true);
    initQcow2Base(buf[0..], 3, 22);
    expect_eq(parseQcow2Header(buf[0..]) == null, true);

    // Bad crypt_method (3)
    initQcow2Base(buf[0..], 3, 16);
    putU32Be(buf[0..], 32, 3);
    expect_eq(parseQcow2Header(buf[0..]) == null, true);

    // v3 header_length < 104 or truncated buffer (< 104)
    initQcow2Base(buf[0..], 3, 16);
    putU32Be(buf[0..], 100, 96);
    expect_eq(parseQcow2Header(buf[0..]) == null, true);
    initQcow2Base(buf[0..], 3, 16);
    expect_eq(parseQcow2Header(buf[0..100]) == null, true);
});
