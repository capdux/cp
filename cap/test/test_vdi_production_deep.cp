// Source: cp/cap/test/test_vdi_production_deep.cp (Production deep tests for cap:vdi)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VDI_SIGNATURE,
    VDI_MIN_FILE_HEADER_SIZE,
    VDI_V1_1_MIN_HEADER_SIZE,
    VDI_DEFAULT_SECTOR_SIZE,
    VDI_DEFAULT_BLOCK_SIZE,
    TYPE_DYNAMIC,
    TYPE_STATIC,
    TYPE_UNDO,
    TYPE_DIFF,
    VDI_BLOCK_FREE,
    VDI_BLOCK_ZERO,
    isVdi,
    parseVdiHeader,
    decodeVdiBlockEntry,
    lookupVdiBlock,
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

fn copyInto(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn buildVdi11Header(
    buf: []u8,
    image_type: u32,
    comment: []const u8,
    offset_blocks: u32,
    offset_data: u32,
    disk_size: u64,
    block_size: u32,
    block_extra: u32,
    blocks_in_hdd: u32,
    blocks_allocated: u32,
) {
    let i: usize = 0;
    while (i < VDI_MIN_FILE_HEADER_SIZE) {
        buf[i] = 0;
        i += 1;
    }
    copyInto(buf, 0, "<<< Oracle VM VirtualBox Disk Image >>>\n");
    writeU32Le(buf, 64, VDI_SIGNATURE);
    writeU16Le(buf, 68, 1); // version_minor = 1
    writeU16Le(buf, 70, 1); // version_major = 1
    writeU32Le(buf, 72, VDI_V1_1_MIN_HEADER_SIZE);
    writeU32Le(buf, 76, image_type);
    writeU32Le(buf, 80, 0); // image_flags
    copyInto(buf, 84, comment);
    writeU32Le(buf, 340, offset_blocks);
    writeU32Le(buf, 344, offset_data);
    writeU32Le(buf, 360, VDI_DEFAULT_SECTOR_SIZE);
    writeU64Le(buf, 368, disk_size);
    writeU32Le(buf, 376, block_size);
    writeU32Le(buf, 380, block_extra);
    writeU32Le(buf, 384, blocks_in_hdd);
    writeU32Le(buf, 388, blocks_allocated);

    // Populate uuid_create & uuid_modify with non-zero bytes
    let g: usize = 0;
    while (g < 16) {
        buf[392 + g] = (0x10 + g) as u8;
        buf[408 + g] = (0x20 + g) as u8;
        g += 1;
    }
}

test("1. VDI 1.1 Dynamic 512-byte header parsing, geometry, and NUL-terminated comment slice", () => {
    expect_eq(VDI_SIGNATURE, 0xBEDA107F as u32);
    expect_eq(VDI_MIN_FILE_HEADER_SIZE, 512);
    expect_eq(VDI_V1_1_MIN_HEADER_SIZE, 400 as u32);
    expect_eq(VDI_DEFAULT_SECTOR_SIZE, 512 as u32);
    expect_eq(VDI_DEFAULT_BLOCK_SIZE, 1048576 as u32);

    let buf: [512]u8 = [0; 512];
    buildVdi11Header(
        buf[0..],
        TYPE_DYNAMIC,
        "CP-VDI-Dynamic-Disk",
        1048576,          // offset_blocks = 1MB
        2097152,          // offset_data = 2MB
        10737418240,      // disk_size = 10 GiB
        1048576,          // block_size = 1MB
        0,                // block_extra_data = 0
        10240,            // blocks_in_hdd
        128,              // blocks_allocated
    );

    expect_eq(isVdi(buf[0..]), true);
    const hdr = parseVdiHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.version_major, 1 as u16);
    expect_eq(hdr.version_minor, 1 as u16);
    expect_eq(hdr.header_size, 400 as u32);
    expect_eq(hdr.isDynamic(), true);
    expect_eq(hdr.isStatic(), false);
    expect_eq(hdr.isDiff(), false);
    expect_eq(hdr.hasParent(), false);
    expect_eq(hdr.offset_blocks, 1048576 as u32);
    expect_eq(hdr.offset_data, 2097152 as u32);
    expect_eq(hdr.sector_size, 512 as u32);
    expect_eq(hdr.disk_size, 10737418240 as u64);
    expect_eq(hdr.block_size, 1048576 as u32);
    expect_eq(hdr.blocks_in_hdd, 10240 as u32);
    expect_eq(hdr.blocks_allocated, 128 as u32);
    expect_eq(hdr.blockMapBytes(), 40960 as u64);
    expect_eq(hdr.uuid_create[0], 0x10 as u8);
    expect_eq(hdr.uuid_modify[0], 0x20 as u8);
    expect_eq(std.mem.eql(u8, hdr.commentSlice(), "CP-VDI-Dynamic-Disk"), true);
});

test("2. VDI 1.1 Diff & Static headers, parent linkage UUID detection, and full 256-byte comment", () => {
    let diff_buf: [512]u8 = [0; 512];
    buildVdi11Header(
        diff_buf[0..],
        TYPE_DIFF,
        "snapshot-child",
        512,
        4096,
        4194304,
        1048576,
        64,
        4,
        2,
    );
    // Set non-zero parent linkage UUID & parent modify UUID
    diff_buf[424 + 15] = 0x77;
    diff_buf[440] = 0x88;

    const diff_hdr = parseVdiHeader(diff_buf[0..]) ?? unreachable;
    expect_eq(diff_hdr.isDiff(), true);
    expect_eq(diff_hdr.isDynamic(), false);
    expect_eq(diff_hdr.hasParent(), true);
    expect_eq(diff_hdr.uuid_linkage[15], 0x77 as u8);
    expect_eq(diff_hdr.uuid_parent_modify[0], 0x88 as u8);
    expect_eq(std.mem.eql(u8, diff_hdr.commentSlice(), "snapshot-child"), true);

    // Static header with all 256 comment bytes non-zero (no NUL terminator)
    let static_buf: [512]u8 = [0; 512];
    buildVdi11Header(
        static_buf[0..],
        TYPE_STATIC,
        "",
        512,
        4096,
        4194304,
        1048576,
        0,
        4,
        4,
    );
    let i: usize = 0;
    while (i < 256) {
        static_buf[84 + i] = 0x41; // 'A'
        i += 1;
    }
    const static_hdr = parseVdiHeader(static_buf[0..]) ?? unreachable;
    expect_eq(static_hdr.isStatic(), true);
    expect_eq(static_hdr.hasParent(), false);
    expect_eq(static_hdr.commentSlice().len, 256);
});

test("3. Block Allocation Map lookup: allocated blocks with extra data, VDI_BLOCK_FREE, and VDI_BLOCK_ZERO", () => {
    let hdr_buf: [512]u8 = [0; 512];
    buildVdi11Header(
        hdr_buf[0..],
        TYPE_DYNAMIC,
        "bam-test",
        4096,     // offset_blocks
        8192,     // offset_data
        6291456,  // 6 MB
        1048576,  // block_size = 1 MB
        512,      // block_extra_data = 512 B
        6,        // blocks_in_hdd = 6
        3,        // blocks_allocated = 3
    );
    const hdr = parseVdiHeader(hdr_buf[0..]) ?? unreachable;

    // 6 entries * 4 bytes = 24 bytes
    let bam: [24]u8 = [0; 24];
    writeU32Le(bam[0..], 0, 0);               // logical 0 -> phys 0
    writeU32Le(bam[0..], 4, VDI_BLOCK_FREE);  // logical 1 -> free
    writeU32Le(bam[0..], 8, VDI_BLOCK_ZERO);  // logical 2 -> zero
    writeU32Le(bam[0..], 12, 2);              // logical 3 -> phys 2
    writeU32Le(bam[0..], 16, 1);              // logical 4 -> phys 1
    writeU32Le(bam[0..], 20, VDI_BLOCK_FREE); // logical 5 -> free

    const m0 = lookupVdiBlock(hdr, bam[0..], 0) ?? unreachable;
    expect_eq(m0.is_allocated, true);
    expect_eq(m0.is_zero, false);
    expect_eq(m0.phys_block, 0 as u32);
    expect_eq(m0.file_offset, 8192 as u64);

    const m1 = lookupVdiBlock(hdr, bam[0..], 1) ?? unreachable;
    expect_eq(m1.is_allocated, false);
    expect_eq(m1.is_zero, false);
    expect_eq(m1.file_offset, 0 as u64);

    const m2 = lookupVdiBlock(hdr, bam[0..], 2) ?? unreachable;
    expect_eq(m2.is_allocated, false);
    expect_eq(m2.is_zero, true);
    expect_eq(m2.file_offset, 0 as u64);

    // logical 3 -> phys 2 => 8192 + 2 * (1048576 + 512) = 8192 + 2098176 = 2106368
    const m3 = lookupVdiBlock(hdr, bam[0..], 3) ?? unreachable;
    expect_eq(m3.is_allocated, true);
    expect_eq(m3.phys_block, 2 as u32);
    expect_eq(m3.file_offset, 2106368 as u64);

    // Direct decodeVdiBlockEntry for large physical block (> 4 GiB offset)
    const m_large = decodeVdiBlockEntry(hdr, 5000);
    expect_eq(m_large.is_allocated, true);
    expect_eq(m_large.file_offset, (8192 as u64) + (5000 as u64) * (1049088 as u64));
});

test("4. Out-of-bounds logical block numbers and truncated block map rejection", () => {
    let hdr_buf: [512]u8 = [0; 512];
    buildVdi11Header(
        hdr_buf[0..],
        TYPE_UNDO,
        "bounds-test",
        4096,
        8192,
        4194304,
        1048576,
        0,
        4,
        1,
    );
    const hdr = parseVdiHeader(hdr_buf[0..]) ?? unreachable;

    let bam: [12]u8 = [0; 12]; // Only 3 entries (12 bytes), but header declares 4 blocks
    writeU32Le(bam[0..], 0, 0);
    writeU32Le(bam[0..], 4, VDI_BLOCK_ZERO);
    writeU32Le(bam[0..], 8, VDI_BLOCK_FREE);

    // Valid indices within truncated map
    expect_eq(lookupVdiBlock(hdr, bam[0..], 0) != null, true);
    expect_eq(lookupVdiBlock(hdr, bam[0..], 2) != null, true);

    // Index 3 is < blocks_in_hdd (4), but exceeds bam.len (12 bytes) -> rejected
    expect_eq(lookupVdiBlock(hdr, bam[0..], 3) == null, true);

    // Index 4 and 999 >= blocks_in_hdd -> rejected
    expect_eq(lookupVdiBlock(hdr, bam[0..], 4) == null, true);
    expect_eq(lookupVdiBlock(hdr, bam[0..], 999) == null, true);
});

test("5. Malformed VDI header rejection: bad magic, non-1.1 version, small header_size, bad image_type, zero sector/block size", () => {
    // Too short buffer
    let short_buf: [64]u8 = [0; 64];
    expect_eq(isVdi(short_buf[0..]), false);
    expect_eq(parseVdiHeader(short_buf[0..]) == null, true);

    let buf: [512]u8 = [0; 512];

    // Bad signature
    buildVdi11Header(buf[0..], TYPE_DYNAMIC, "", 512, 4096, 1048576, 1048576, 0, 1, 0);
    writeU32Le(buf[0..], 64, 0xDEADBEEF);
    expect_eq(isVdi(buf[0..]), false);
    expect_eq(parseVdiHeader(buf[0..]) == null, true);

    // Non-1.1 versions (0.9, 1.0, 1.2, 2.1)
    buildVdi11Header(buf[0..], TYPE_DYNAMIC, "", 512, 4096, 1048576, 1048576, 0, 1, 0);
    writeU16Le(buf[0..], 68, 2); // 1.2
    expect_eq(parseVdiHeader(buf[0..]) == null, true);
    writeU16Le(buf[0..], 68, 1);
    writeU16Le(buf[0..], 70, 2); // 2.1
    expect_eq(parseVdiHeader(buf[0..]) == null, true);

    // header_size < 400
    buildVdi11Header(buf[0..], TYPE_DYNAMIC, "", 512, 4096, 1048576, 1048576, 0, 1, 0);
    writeU32Le(buf[0..], 72, 399);
    expect_eq(parseVdiHeader(buf[0..]) == null, true);

    // Invalid image_type (0 and 5)
    buildVdi11Header(buf[0..], TYPE_DYNAMIC, "", 512, 4096, 1048576, 1048576, 0, 1, 0);
    writeU32Le(buf[0..], 76, 0);
    expect_eq(parseVdiHeader(buf[0..]) == null, true);
    writeU32Le(buf[0..], 76, 5);
    expect_eq(parseVdiHeader(buf[0..]) == null, true);

    // sector_size == 0
    buildVdi11Header(buf[0..], TYPE_DYNAMIC, "", 512, 4096, 1048576, 1048576, 0, 1, 0);
    writeU32Le(buf[0..], 360, 0);
    expect_eq(parseVdiHeader(buf[0..]) == null, true);

    // block_size == 0
    buildVdi11Header(buf[0..], TYPE_DYNAMIC, "", 512, 4096, 1048576, 1048576, 0, 1, 0);
    writeU32Le(buf[0..], 376, 0);
    expect_eq(parseVdiHeader(buf[0..]) == null, true);
});
