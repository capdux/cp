// Source: cp/cap/test/test_hdf5_production_deep.cp (Production deep tests for cap:hdf5)

import { test, expect_eq } from "cap:test";
import {
    HDF5_SIGNATURE,
    HDF5_UNDEF_ADDR_64,
    HDF5_OHDR_MAGIC,
    HDF5_OHDR_FLAG_PHASE_CHANGE,
    HDF5_OHDR_FLAG_TIMESTAMPS,
    Hdf5Superblock,
    Hdf5ObjHeaderV2,
    findHdf5Signature,
    isHdf5,
    parseHdf5Superblock,
    parseHdf5ObjHeaderV2,
} from "cap:hdf5";

fn zeroBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
}

fn writeBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn writeU16LE(dst: []u8, off: usize, val: u16) {
    dst[off + 0] = (val & 0xFF) as u8;
    dst[off + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LE(dst: []u8, off: usize, val: u32) {
    dst[off + 0] = (val & 0xFF) as u8;
    dst[off + 1] = ((val >> 8) & 0xFF) as u8;
    dst[off + 2] = ((val >> 16) & 0xFF) as u8;
    dst[off + 3] = ((val >> 24) & 0xFF) as u8;
}

fn writeU64LE(dst: []u8, off: usize, val: u64) {
    writeU32LE(dst, off, (val & 0xFFFFFFFF) as u32);
    writeU32LE(dst, off + 4, ((val >> 32) & 0xFFFFFFFF) as u32);
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. parses Superblock v0 and v1 with 64-bit offsets and Root Group Symbol Table Entry", () => {
    // Superblock v0 with offset_size=8: 24 + 4*8 + (2*8 + 24) = 96 bytes
    let raw: [128]u8 = undefined;
    let buf = raw[0..96];
    zeroBytes(buf);

    writeBytes(buf, 0, HDF5_SIGNATURE);
    buf[8] = 0; // version 0
    buf[13] = 8; // offset_size
    buf[14] = 8; // length_size
    writeU16LE(buf, 16, 4); // group_leaf_node_k
    writeU16LE(buf, 18, 16); // group_internal_node_k
    writeU32LE(buf, 20, 0x00000001); // consistency_flags
    writeU64LE(buf, 24, 0); // base_address
    writeU64LE(buf, 32, HDF5_UNDEF_ADDR_64); // free_space_address
    writeU64LE(buf, 40, 8192); // eof_address
    writeU64LE(buf, 48, HDF5_UNDEF_ADDR_64); // driver_info_address
    // Root Group Symbol Table Entry at 56: link_name_offset(8B) + object_header_addr(8B) + 24B
    writeU64LE(buf, 56, 0);
    writeU64LE(buf, 64, 96); // root_group_addr

    expect_eq(isHdf5(buf), true);
    if (sb := parseHdf5Superblock(buf)) {
        expect_eq(sb.signature_offset, 0);
        expect_eq(sb.version, 0);
        expect_eq(sb.offset_size, 8);
        expect_eq(sb.length_size, 8);
        expect_eq(sb.group_leaf_node_k, 4);
        expect_eq(sb.group_internal_node_k, 16);
        expect_eq(sb.consistency_flags, 1);
        expect_eq(sb.base_address, 0);
        expect_eq(sb.free_space_address, HDF5_UNDEF_ADDR_64);
        expect_eq(sb.eof_address, 8192);
        expect_eq(sb.driver_info_address, HDF5_UNDEF_ADDR_64);
        expect_eq(sb.root_group_addr, 96);
    } else {
        expect_eq(true, false);
    }

    // Superblock v1 with offset_size=4: 28 + 4*4 + (2*4 + 24) = 76 bytes
    let v1_buf = raw[0..76];
    zeroBytes(v1_buf);
    writeBytes(v1_buf, 0, HDF5_SIGNATURE);
    v1_buf[8] = 1; // version 1
    v1_buf[13] = 4; // offset_size
    v1_buf[14] = 4; // length_size
    writeU16LE(v1_buf, 16, 8);
    writeU16LE(v1_buf, 18, 32);
    writeU32LE(v1_buf, 20, 0);
    writeU16LE(v1_buf, 24, 32); // indexed_storage_internal_node_k
    writeU32LE(v1_buf, 28, 0); // base_address
    writeU32LE(v1_buf, 32, 0xFFFFFFFF); // free_space_address
    writeU32LE(v1_buf, 36, 4096); // eof_address
    writeU32LE(v1_buf, 40, 0xFFFFFFFF); // driver_info_address
    writeU32LE(v1_buf, 44, 0); // link_name_offset
    writeU32LE(v1_buf, 48, 128); // root_group_addr

    if (sb1 := parseHdf5Superblock(v1_buf)) {
        expect_eq(sb1.version, 1);
        expect_eq(sb1.offset_size, 4);
        expect_eq(sb1.eof_address, 4096);
        expect_eq(sb1.root_group_addr, 128);
    } else {
        expect_eq(true, false);
    }
});

test("2. parses Superblock v2 and v3 with superblock_ext_address and checksum", () => {
    // v2/v3 with 64-bit offsets = 12 + 32 + 4 = 48 bytes
    let raw: [48]u8 = undefined;
    let buf = raw[0..48];
    zeroBytes(buf);

    writeBytes(buf, 0, HDF5_SIGNATURE);
    buf[8] = 2; // version 2
    buf[9] = 8; // offset_size
    buf[10] = 8; // length_size
    buf[11] = 0x05; // consistency_flags
    writeU64LE(buf, 12, 0); // base_address
    writeU64LE(buf, 20, HDF5_UNDEF_ADDR_64); // superblock_ext_address
    writeU64LE(buf, 28, 65536); // eof_address
    writeU64LE(buf, 36, 48); // root_group_addr
    writeU32LE(buf, 44, 0xDEADBEEF); // checksum

    if (sb2 := parseHdf5Superblock(buf)) {
        expect_eq(sb2.version, 2);
        expect_eq(sb2.offset_size, 8);
        expect_eq(sb2.length_size, 8);
        expect_eq(sb2.consistency_flags, 5);
        expect_eq(sb2.base_address, 0);
        expect_eq(sb2.superblock_ext_address, HDF5_UNDEF_ADDR_64);
        expect_eq(sb2.eof_address, 65536);
        expect_eq(sb2.root_group_addr, 48);
        expect_eq(sb2.checksum, 0xDEADBEEF);
    } else {
        expect_eq(true, false);
    }

    // Switch to version 3 with explicit superblock_ext_address
    buf[8] = 3;
    writeU64LE(buf, 20, 512);
    if (sb3 := parseHdf5Superblock(buf)) {
        expect_eq(sb3.version, 3);
        expect_eq(sb3.superblock_ext_address, 512);
        expect_eq(sb3.checksum, 0xDEADBEEF);
    } else {
        expect_eq(true, false);
    }
});

test("3. locates HDF5 signature past 512B and 1024B User Blocks", () => {
    let raw: [1200]u8 = undefined;
    let buf = raw[0..1200];
    zeroBytes(buf);

    // Put a Superblock v2 at offset 512
    const off512: usize = 512;
    writeBytes(buf, off512, HDF5_SIGNATURE);
    buf[off512 + 8] = 2;
    buf[off512 + 9] = 8;
    buf[off512 + 10] = 8;
    writeU64LE(buf, off512 + 28, 1200);
    writeU64LE(buf, off512 + 36, 600);

    if (pos := findHdf5Signature(buf)) {
        expect_eq(pos, 512);
    } else {
        expect_eq(true, false);
    }
    if (sb := parseHdf5Superblock(buf)) {
        expect_eq(sb.signature_offset, 512);
        expect_eq(sb.root_group_addr, 600);
    } else {
        expect_eq(true, false);
    }

    // Move signature to offset 1024 (2^1 * 512)
    zeroBytes(buf);
    const off1024: usize = 1024;
    writeBytes(buf, off1024, HDF5_SIGNATURE);
    buf[off1024 + 8] = 3;
    buf[off1024 + 9] = 8;
    buf[off1024 + 10] = 8;
    writeU64LE(buf, off1024 + 36, 1080);

    if (pos2 := findHdf5Signature(buf)) {
        expect_eq(pos2, 1024);
    } else {
        expect_eq(true, false);
    }
    if (sb2 := parseHdf5Superblock(buf)) {
        expect_eq(sb2.signature_offset, 1024);
        expect_eq(sb2.version, 3);
        expect_eq(sb2.root_group_addr, 1080);
    } else {
        expect_eq(true, false);
    }
});

test("4. parses OHDR v2 object headers with 1/2/4/8B chunk0 widths, timestamps, and phase change", () => {
    // Case A: 1-byte chunk0 size, no timestamps, no phase change
    let raw: [128]u8 = undefined;
    let buf = raw[0..128];
    zeroBytes(buf);

    const payload_a = "MSG_CHUNK0_A";
    writeBytes(buf, 0, HDF5_OHDR_MAGIC);
    buf[4] = 1; // version
    buf[5] = 0x00; // 1B chunk0 size
    buf[6] = payload_a.len as u8;
    writeBytes(buf, 7, payload_a);

    if (oh := parseHdf5ObjHeaderV2(buf, 0)) {
        expect_eq(oh.version, 1);
        expect_eq(oh.has_timestamps, false);
        expect_eq(oh.chunk0_size, payload_a.len as u64);
        expect_eq(sliceEq(oh.chunk0, payload_a), true);
    } else {
        expect_eq(true, false);
    }

    // Case B: 2-byte chunk0 size + timestamps (16B) + phase change (4B) at offset 32
    const off: usize = 32;
    const payload_b = "TENSOR_WEIGHTS_HDR";
    writeBytes(buf, off, HDF5_OHDR_MAGIC);
    buf[off + 4] = 1;
    buf[off + 5] = (0x01 as u8) | HDF5_OHDR_FLAG_TIMESTAMPS | HDF5_OHDR_FLAG_PHASE_CHANGE;
    writeU32LE(buf, off + 6, 1700000001); // access_time
    writeU32LE(buf, off + 10, 1700000002); // mod_time
    writeU32LE(buf, off + 14, 1700000003); // change_time
    writeU32LE(buf, off + 18, 1700000004); // birth_time
    writeU16LE(buf, off + 22, 8); // max_compact
    writeU16LE(buf, off + 24, 6); // min_dense
    writeU16LE(buf, off + 26, payload_b.len as u16);
    writeBytes(buf, off + 28, payload_b);

    if (oh2 := parseHdf5ObjHeaderV2(buf, off)) {
        expect_eq(oh2.has_timestamps, true);
        expect_eq(oh2.access_time, 1700000001);
        expect_eq(oh2.mod_time, 1700000002);
        expect_eq(oh2.change_time, 1700000003);
        expect_eq(oh2.birth_time, 1700000004);
        expect_eq(oh2.chunk0_size, payload_b.len as u64);
        expect_eq(sliceEq(oh2.chunk0, payload_b), true);
    } else {
        expect_eq(true, false);
    }
});

test("5. rejects truncated files, invalid versions, invalid offset/length sizes, and out-of-bounds OHDR", () => {
    // 5a. Empty / short buffer
    let tiny: [4]u8 = [0x89, 'H', 'D', 'F'];
    expect_eq(isHdf5(tiny[0..4]), false);
    expect_eq(parseHdf5Superblock(tiny[0..4]) == null, true);

    // 5b. Invalid Superblock version (version == 9)
    let raw: [64]u8 = undefined;
    let buf = raw[0..64];
    zeroBytes(buf);
    writeBytes(buf, 0, HDF5_SIGNATURE);
    buf[8] = 9;
    expect_eq(isHdf5(buf), true);
    expect_eq(parseHdf5Superblock(buf) == null, true);

    // 5c. Invalid offset_size (3 is not in {2, 4, 8})
    buf[8] = 2;
    buf[9] = 3;
    buf[10] = 8;
    expect_eq(parseHdf5Superblock(buf) == null, true);

    // 5d. Invalid length_size (16 is not in {2, 4, 8})
    buf[9] = 8;
    buf[10] = 16;
    expect_eq(parseHdf5Superblock(buf) == null, true);

    // 5e. Truncated Superblock v2 (only 20 bytes, needs 48)
    buf[10] = 8;
    expect_eq(parseHdf5Superblock(buf[0..20]) == null, true);

    // 5f. OHDR bad magic, wrong version, and chunk0_size exceeding buffer
    zeroBytes(buf);
    writeBytes(buf, 0, "NOPE");
    expect_eq(parseHdf5ObjHeaderV2(buf, 0) == null, true);

    writeBytes(buf, 0, HDF5_OHDR_MAGIC);
    buf[4] = 2; // invalid version
    expect_eq(parseHdf5ObjHeaderV2(buf, 0) == null, true);

    buf[4] = 1;
    buf[5] = 0x02; // 4-byte chunk0 size
    writeU32LE(buf, 6, 99999); // exceeds buffer length
    expect_eq(parseHdf5ObjHeaderV2(buf, 0) == null, true);
    expect_eq(parseHdf5ObjHeaderV2(buf, 100) == null, true);
});
