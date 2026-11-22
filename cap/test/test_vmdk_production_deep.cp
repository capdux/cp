// Source: cp/cap/test/test_vmdk_production_deep.cp (Production deep tests for cap:vmdk)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VMDK_MAGIC,
    VMDK_SECTOR_SIZE,
    VMDK_HEADER_SIZE,
    VMDK_DEFAULT_GTES_PER_GT,
    FLAG_VALID_NEWLINE_DETECTOR,
    FLAG_REDUNDANT_GRAIN_TABLE,
    FLAG_ZEROED_GRAIN_GTE,
    FLAG_COMPRESSED_GRAINS,
    FLAG_MARKERS_IN_STREAM,
    COMPRESS_NONE,
    COMPRESS_DEFLATE,
    CID_NONE,
    VmdkSparseHeader,
    isVmdkSparse,
    parseVmdkSparseHeader,
    VmdkExtent,
    parseVmdkExtentLine,
    VmdkExtentIter,
    VmdkDescriptor,
    parseVmdkDescriptor,
} from "cap:binfmt";

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

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn eqStr(a: []const u8, b: []const u8): bool {
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

fn initSparseHeader(buf: []u8, version: u32, flags: u32, grain_size: u64, compress: u16) {
    putU32Le(buf, 0, VMDK_MAGIC);
    putU32Le(buf, 4, version);
    putU32Le(buf, 8, flags);
    putU64Le(buf, 12, 20971520); // 10 GiB in 512B sectors
    putU64Le(buf, 20, grain_size);
    putU64Le(buf, 28, 1); // descriptor_offset = sector 1 (byte 512)
    putU64Le(buf, 36, 2); // descriptor_size = 2 sectors (1024B)
    putU32Le(buf, 44, VMDK_DEFAULT_GTES_PER_GT);
    putU64Le(buf, 48, 4); // rgd_offset
    putU64Le(buf, 56, 128); // gd_offset
    putU64Le(buf, 64, 256); // overhead_sectors
    buf[72] = 0; // clean shutdown
    buf[73] = 0x0A; // '\n'
    buf[74] = 0x20; // ' '
    buf[75] = 0x0D; // '\r'
    buf[76] = 0x0A; // '\n'
    putU16Le(buf, 77, compress);
}

test("1. VMDK SparseExtentHeader 512B standard monolithicSparse parsing & byte helpers", () => {
    expect_eq(VMDK_MAGIC, 0x564D444B as u32);
    expect_eq(VMDK_SECTOR_SIZE, 512 as usize);
    expect_eq(VMDK_HEADER_SIZE, 512 as usize);
    expect_eq(VMDK_DEFAULT_GTES_PER_GT, 512 as u32);
    expect_eq(CID_NONE, 0xFFFFFFFF as u32);

    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const flags = FLAG_VALID_NEWLINE_DETECTOR | FLAG_REDUNDANT_GRAIN_TABLE | FLAG_ZEROED_GRAIN_GTE;
    initSparseHeader(buf[0..], 1, flags, 128, COMPRESS_NONE);
    buf[72] = 1; // unclean shutdown

    expect_eq(isVmdkSparse(buf[0..]), true);
    const hdr = parseVmdkSparseHeader(buf[0..]) ?? unreachable;

    expect_eq(hdr.version, 1 as u32);
    expect_eq(hdr.flags, flags);
    expect_eq(hdr.capacity_sectors, 20971520 as u64);
    expect_eq(hdr.capacityBytes(), 10737418240 as u64);
    expect_eq(hdr.grain_size_sectors, 128 as u64);
    expect_eq(hdr.grainSizeBytes(), 65536 as u64);
    expect_eq(hdr.descriptor_offset, 1 as u64);
    expect_eq(hdr.descriptor_size, 2 as u64);
    expect_eq(hdr.descriptorByteOffset(), 512 as u64);
    expect_eq(hdr.descriptorByteSize(), 1024 as u64);
    expect_eq(hdr.num_gtes_per_gt, 512 as u32);
    expect_eq(hdr.rgd_offset, 4 as u64);
    expect_eq(hdr.gd_offset, 128 as u64);
    expect_eq(hdr.overhead_sectors, 256 as u64);
    expect_eq(hdr.unclean_shutdown, true);
    expect_eq(hdr.compress_algorithm, COMPRESS_NONE);
    expect_eq(hdr.isCompressed(), false);
    expect_eq(hdr.isStreamOptimized(), false);
    expect_eq(hdr.hasRedundantGrainTable(), true);
    expect_eq(hdr.hasValidNewlineDetector(), true);
    expect_eq(hdr.hasZeroedGrainGte(), true);
});

test("2. VMDK streamOptimized SparseExtentHeader with Deflate compression & embedded descriptor extraction", () => {
    let img: [1536]u8 = std.mem.zeroes([1536]u8); // 1 header sector + 2 descriptor sectors
    const flags = FLAG_VALID_NEWLINE_DETECTOR | FLAG_COMPRESSED_GRAINS | FLAG_MARKERS_IN_STREAM;
    initSparseHeader(img[0..512], 3, flags, 128, COMPRESS_DEFLATE);

    const embedded_text =
        "# Disk DescriptorFile\nversion=1\nCID=1234abcd\nparentCID=ffffffff\ncreateType=\"streamOptimized\"\n\n# Extent description\nRDONLY 20971520 SPARSE \"stream.vmdk\"\n\nddb.adapterType = \"lsilogic\"\n";
    copyBytes(img[0..], 512, embedded_text);

    const hdr = parseVmdkSparseHeader(img[0..]) ?? unreachable;
    expect_eq(hdr.version, 3 as u32);
    expect_eq(hdr.isCompressed(), true);
    expect_eq(hdr.isStreamOptimized(), true);
    expect_eq(hdr.hasRedundantGrainTable(), false);
    expect_eq(hdr.compress_algorithm, COMPRESS_DEFLATE);

    const raw_desc = hdr.embeddedDescriptor(img[0..]) ?? unreachable;
    expect_eq(raw_desc.len, embedded_text.len);

    const desc = parseVmdkDescriptor(raw_desc) ?? unreachable;
    expect_eq(desc.version, 1 as u32);
    expect_eq(desc.cid, 0x1234ABCD as u32);
    expect_eq(desc.parent_cid, CID_NONE);
    expect_eq(desc.hasParent(), false);
    expect_eq(eqStr(desc.create_type, "streamOptimized"), true);
    const adapter = desc.findDdb("adapterType") ?? unreachable;
    expect_eq(eqStr(adapter, "lsilogic"), true);
});

test("3. VMDK text Descriptor full parsing (CID, parentCID, parentFileNameHint, DDB geometry & metadata)", () => {
    const text =
        "# Disk DescriptorFile\r\n" ++
        "version=1\r\n" ++
        "encoding=\"UTF-8\"\r\n" ++
        "CID=fffffffe\r\n" ++
        "parentCID=abcdef01\r\n" ++
        "isNativeSnapshot=\"no\"\r\n" ++
        "createType=\"twoGbMaxExtentSparse\"\r\n" ++
        "parentFileNameHint=\"base-disk.vmdk\"\r\n" ++
        "\r\n" ++
        "# Extent description\r\n" ++
        "RW 4192256 SPARSE \"child-s001.vmdk\"\r\n" ++
        "\r\n" ++
        "# The Disk Data Base\r\n" ++
        "#DDB\r\n" ++
        "ddb.adapterType = \"ide\"\r\n" ++
        "ddb.geometry.cylinders = \"1305\"\r\n" ++
        "ddb.geometry.heads = \"255\"\r\n" ++
        "ddb.geometry.sectors = \"63\"\r\n" ++
        "ddb.virtualHWVersion = \"14\"\r\n";

    const desc = parseVmdkDescriptor(text) ?? unreachable;
    expect_eq(desc.version, 1 as u32);
    expect_eq(desc.cid, 0xFFFFFFFE as u32);
    expect_eq(desc.parent_cid, 0xABCDEF01 as u32);
    expect_eq(desc.hasParent(), true);
    expect_eq(eqStr(desc.create_type, "twoGbMaxExtentSparse"), true);
    expect_eq(eqStr(desc.parent_filename_hint, "base-disk.vmdk"), true);

    const adapter = desc.findDdb("adapterType") ?? unreachable;
    expect_eq(eqStr(adapter, "ide"), true);
    // Also supports explicit "ddb." prefix
    const adapter_prefixed = desc.findDdb("ddb.adapterType") ?? unreachable;
    expect_eq(eqStr(adapter_prefixed, "ide"), true);

    const cyl = desc.findDdb("geometry.cylinders") ?? unreachable;
    expect_eq(eqStr(cyl, "1305"), true);
    const heads = desc.findDdb("geometry.heads") ?? unreachable;
    expect_eq(eqStr(heads, "255"), true);
    const secs = desc.findDdb("geometry.sectors") ?? unreachable;
    expect_eq(eqStr(secs, "63"), true);
    const hwv = desc.findDdb("virtualHWVersion") ?? unreachable;
    expect_eq(eqStr(hwv, "14"), true);

    expect_eq(desc.findDdb("nonExistentKey") == null, true);
    expect_eq(desc.findDdb("") == null, true);
});

test("4. VMDK multi-extent zero-copy iteration (SPARSE, FLAT with offset, quoted space filename, ZERO)", () => {
    const text =
        "# Disk DescriptorFile\n" ++
        "version=1\n" ++
        "CID=00001234\n" ++
        "parentCID=ffffffff\n" ++
        "createType=\"twoGbMaxExtentFlat\"\n" ++
        "\n" ++
        "# Extent description\n" ++
        "RW 20971520 SPARSE \"disk-s001.vmdk\"\n" ++
        "RW 4096 FLAT \"disk-flat.vmdk\" 0\n" ++
        "RW 8192 FLAT \"my vm disk.vmdk\" 2048\n" ++
        "RDONLY 1024 ZERO\n" ++
        "NOACCESS 512 VMFS \"partition-flat.vmdk\" 63\n" ++
        "\n" ++
        "ddb.adapterType = \"lsilogic\"\n";

    const desc = parseVmdkDescriptor(text) ?? unreachable;
    let it = desc.extents();

    const e0 = it.next() ?? unreachable;
    expect_eq(eqStr(e0.access, "RW"), true);
    expect_eq(e0.sectors, 20971520 as u64);
    expect_eq(eqStr(e0.extent_type, "SPARSE"), true);
    expect_eq(eqStr(e0.filename, "disk-s001.vmdk"), true);
    expect_eq(e0.has_offset, false);
    expect_eq(e0.offset, 0 as u64);

    const e1 = it.next() ?? unreachable;
    expect_eq(eqStr(e1.access, "RW"), true);
    expect_eq(e1.sectors, 4096 as u64);
    expect_eq(eqStr(e1.extent_type, "FLAT"), true);
    expect_eq(eqStr(e1.filename, "disk-flat.vmdk"), true);
    expect_eq(e1.has_offset, true);
    expect_eq(e1.offset, 0 as u64);

    const e2 = it.next() ?? unreachable;
    expect_eq(eqStr(e2.access, "RW"), true);
    expect_eq(e2.sectors, 8192 as u64);
    expect_eq(eqStr(e2.extent_type, "FLAT"), true);
    expect_eq(eqStr(e2.filename, "my vm disk.vmdk"), true);
    expect_eq(e2.has_offset, true);
    expect_eq(e2.offset, 2048 as u64);

    const e3 = it.next() ?? unreachable;
    expect_eq(eqStr(e3.access, "RDONLY"), true);
    expect_eq(e3.sectors, 1024 as u64);
    expect_eq(eqStr(e3.extent_type, "ZERO"), true);
    expect_eq(e3.filename.len, 0 as usize);
    expect_eq(e3.has_offset, false);

    const e4 = it.next() ?? unreachable;
    expect_eq(eqStr(e4.access, "NOACCESS"), true);
    expect_eq(e4.sectors, 512 as u64);
    expect_eq(eqStr(e4.extent_type, "VMFS"), true);
    expect_eq(eqStr(e4.filename, "partition-flat.vmdk"), true);
    expect_eq(e4.has_offset, true);
    expect_eq(e4.offset, 63 as u64);

    expect_eq(it.next() == null, true);
});

test("5. Malformed SparseExtentHeader rejection (bad magic, truncated, bad version/grain/GTE/newline/compress)", () => {
    // 1. Truncated buffer
    let short_buf: [256]u8 = std.mem.zeroes([256]u8);
    putU32Le(short_buf[0..], 0, VMDK_MAGIC);
    expect_eq(isVmdkSparse(short_buf[0..]), true);
    expect_eq(parseVmdkSparseHeader(short_buf[0..]) == null, true);

    // 2. Bad magic
    let bad_magic: [512]u8 = std.mem.zeroes([512]u8);
    initSparseHeader(bad_magic[0..], 1, FLAG_VALID_NEWLINE_DETECTOR, 128, COMPRESS_NONE);
    bad_magic[0] = 0x00;
    expect_eq(isVmdkSparse(bad_magic[0..]), false);
    expect_eq(parseVmdkSparseHeader(bad_magic[0..]) == null, true);

    // 3. Invalid version (0 and 4)
    let bad_ver: [512]u8 = std.mem.zeroes([512]u8);
    initSparseHeader(bad_ver[0..], 0, FLAG_VALID_NEWLINE_DETECTOR, 128, COMPRESS_NONE);
    expect_eq(parseVmdkSparseHeader(bad_ver[0..]) == null, true);
    putU32Le(bad_ver[0..], 4, 4);
    expect_eq(parseVmdkSparseHeader(bad_ver[0..]) == null, true);

    // 4. Invalid grain_size_sectors (< 8 or non-power-of-2)
    let bad_grain: [512]u8 = std.mem.zeroes([512]u8);
    initSparseHeader(bad_grain[0..], 1, FLAG_VALID_NEWLINE_DETECTOR, 4, COMPRESS_NONE);
    expect_eq(parseVmdkSparseHeader(bad_grain[0..]) == null, true);
    putU64Le(bad_grain[0..], 20, 100); // non-power-of-2
    expect_eq(parseVmdkSparseHeader(bad_grain[0..]) == null, true);

    // 5. Invalid num_gtes_per_gt (!= 512)
    let bad_gte: [512]u8 = std.mem.zeroes([512]u8);
    initSparseHeader(bad_gte[0..], 1, FLAG_VALID_NEWLINE_DETECTOR, 128, COMPRESS_NONE);
    putU32Le(bad_gte[0..], 44, 256);
    expect_eq(parseVmdkSparseHeader(bad_gte[0..]) == null, true);

    // 6. Corrupted newline detector when FLAG_VALID_NEWLINE_DETECTOR is set
    let bad_nl: [512]u8 = std.mem.zeroes([512]u8);
    initSparseHeader(bad_nl[0..], 1, FLAG_VALID_NEWLINE_DETECTOR, 128, COMPRESS_NONE);
    bad_nl[75] = 0x0A; // corrupted '\r' -> '\n'
    expect_eq(parseVmdkSparseHeader(bad_nl[0..]) == null, true);

    // Without FLAG_VALID_NEWLINE_DETECTOR, arbitrary bytes at 73..76 are accepted
    putU32Le(bad_nl[0..], 8, 0);
    expect_eq(parseVmdkSparseHeader(bad_nl[0..]) != null, true);

    // 7. Unknown compress_algorithm
    let bad_comp: [512]u8 = std.mem.zeroes([512]u8);
    initSparseHeader(bad_comp[0..], 3, FLAG_VALID_NEWLINE_DETECTOR, 128, 99);
    expect_eq(parseVmdkSparseHeader(bad_comp[0..]) == null, true);
});

test("6. Malformed Descriptor & Extent line boundary guards", () => {
    // Empty or NUL-only descriptor
    expect_eq(parseVmdkDescriptor("") == null, true);
    expect_eq(parseVmdkDescriptor("\x00\x00\x00") == null, true);

    // Missing required keys (createType / CID / version)
    expect_eq(parseVmdkDescriptor("version=1\nCID=ffffffff\n") == null, true);
    expect_eq(parseVmdkDescriptor("version=1\ncreateType=\"monolithicSparse\"\n") == null, true);
    expect_eq(parseVmdkDescriptor("CID=ffffffff\ncreateType=\"monolithicSparse\"\n") == null, true);

    // Invalid hex CID
    expect_eq(parseVmdkDescriptor("version=1\nCID=zzzzzzzz\ncreateType=\"monolithicSparse\"\n") == null, true);

    // Malformed extent lines
    expect_eq(parseVmdkExtentLine("") == null, true);
    expect_eq(parseVmdkExtentLine("# RW 100 SPARSE \"a.vmdk\"") == null, true);
    expect_eq(parseVmdkExtentLine("BADMODE 100 SPARSE \"a.vmdk\"") == null, true);
    expect_eq(parseVmdkExtentLine("RW 0 SPARSE \"a.vmdk\"") == null, true);
    expect_eq(parseVmdkExtentLine("RW 100 SPARSE") == null, true); // missing filename for non-ZERO
    expect_eq(parseVmdkExtentLine("RW 100 SPARSE \"unterminated.vmdk") == null, true);
    expect_eq(parseVmdkExtentLine("RW 100 FLAT \"a.vmdk\" notanumber") == null, true);
    expect_eq(parseVmdkExtentLine("RW 100 FLAT \"a.vmdk\" 0 extra_garbage") == null, true);
});
