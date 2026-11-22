// Source: cp/cap/test/test_udf_production_deep.cp (Production deep tests for cap:udf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    UDF_SECTOR_SIZE,
    UDF_VRS_OFFSET,
    UDF_AVDP_SECTOR,
    UDF_TAG_SIZE,
    VRS_BEA01,
    VRS_NSR02,
    VRS_NSR03,
    VRS_TEA01,
    TAG_PVD,
    TAG_AVDP,
    TAG_VDP,
    TAG_IUVD,
    TAG_PD,
    TAG_LVD,
    TAG_USD,
    TAG_TD,
    TAG_LVID,
    TAG_FSD,
    TAG_FID,
    TAG_FE,
    TAG_EFE,
    udfTagChecksum,
    udfCrc16,
    UdfExtentAd,
    UdfTag,
    parseUdfTag,
    UdfVrsEntry,
    parseUdfVrsEntry,
    UdfAvdp,
    parseUdfAvdp,
    decodeOstaCs0Ascii,
    UdfPvd,
    parseUdfPvd,
} from "cap:devops";

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

fn sealUdfTag(buf: []u8, tag_id: u16, desc_ver: u16, serial: u16, crc_len: u16, location: u32) {
    putU16Le(buf, 0, tag_id);
    putU16Le(buf, 2, desc_ver);
    buf[4] = 0; // placeholder
    buf[5] = 0; // reserved
    putU16Le(buf, 6, serial);
    const body_crc = udfCrc16(buf[16..(16 + (crc_len as usize))]);
    putU16Le(buf, 8, body_crc);
    putU16Le(buf, 10, crc_len);
    putU32Le(buf, 12, location);
    buf[4] = udfTagChecksum(buf[0..16]);
}

test("1. ECMA-167 CRC-16 (0x1021 XMODEM) & 16B Descriptor Tag Checksum", () => {
    expect_eq(UDF_SECTOR_SIZE, 2048 as usize);
    expect_eq(UDF_VRS_OFFSET, 32768 as usize);
    expect_eq(UDF_AVDP_SECTOR, 256 as u32);
    expect_eq(UDF_TAG_SIZE, 16 as usize);
    expect_eq(TAG_VDP, 3 as u16);
    expect_eq(TAG_IUVD, 4 as u16);
    expect_eq(TAG_PD, 5 as u16);
    expect_eq(TAG_LVD, 6 as u16);
    expect_eq(TAG_USD, 7 as u16);
    expect_eq(TAG_TD, 8 as u16);
    expect_eq(TAG_LVID, 9 as u16);
    expect_eq(TAG_FSD, 256 as u16);
    expect_eq(TAG_FID, 257 as u16);
    expect_eq(TAG_FE, 261 as u16);
    expect_eq(TAG_EFE, 266 as u16);

    // Standard CRC-16/XMODEM (ECMA-167) check vector: "123456789" -> 0x31C3
    expect_eq(udfCrc16("123456789"), 0x31C3 as u16);
    expect_eq(udfCrc16(""), 0x0000 as u16);

    let sec: [64]u8 = std.mem.zeroes([64]u8);
    copyBytes(sec[0..], 16, "123456789");
    sealUdfTag(sec[0..], TAG_TD, 3, 1, 9, 500);

    const tag = parseUdfTag(sec[0..]) ?? unreachable;
    expect_eq(tag.tag_id, TAG_TD);
    expect_eq(tag.desc_version, 3 as u16);
    expect_eq(tag.serial_num, 1 as u16);
    expect_eq(tag.desc_crc, 0x31C3 as u16);
    expect_eq(tag.desc_crc_len, 9 as u16);
    expect_eq(tag.tag_location, 500 as u32);
});

test("2. VRS (BEA01, NSR02, NSR03, TEA01) sector identification", () => {
    let sec: [2048]u8 = std.mem.zeroes([2048]u8);

    // BEA01
    sec[0] = 0;
    copyBytes(sec[0..], 1, VRS_BEA01);
    sec[6] = 1;
    const e1 = parseUdfVrsEntry(sec[0..]) ?? unreachable;
    expect_eq(e1.isBea01(), true);
    expect_eq(e1.isNsr02(), false);
    expect_eq(e1.isNsr03(), false);
    expect_eq(e1.isTea01(), false);

    // NSR02
    copyBytes(sec[0..], 1, VRS_NSR02);
    const e2 = parseUdfVrsEntry(sec[0..]) ?? unreachable;
    expect_eq(e2.isNsr02(), true);

    // NSR03 (UDF 2.x)
    copyBytes(sec[0..], 1, VRS_NSR03);
    const e3 = parseUdfVrsEntry(sec[0..]) ?? unreachable;
    expect_eq(e3.isNsr03(), true);

    // TEA01
    copyBytes(sec[0..], 1, VRS_TEA01);
    const e4 = parseUdfVrsEntry(sec[0..]) ?? unreachable;
    expect_eq(e4.isTea01(), true);
});

test("3. AVDP (Tag 2) Main & Reserve VDS Extent parsing", () => {
    let sec: [512]u8 = std.mem.zeroes([512]u8);
    // Main VDS: 32768 bytes at sector 32
    putU32Le(sec[0..], 16, 32768);
    putU32Le(sec[0..], 20, 32);
    // Reserve VDS: 32768 bytes at sector 48
    putU32Le(sec[0..], 24, 32768);
    putU32Le(sec[0..], 28, 48);
    sealUdfTag(sec[0..], TAG_AVDP, 2, 0, 496, UDF_AVDP_SECTOR);

    const avdp = parseUdfAvdp(sec[0..]) ?? unreachable;
    expect_eq(avdp.tag.tag_id, TAG_AVDP);
    expect_eq(avdp.tag.tag_location, 256 as u32);
    expect_eq(avdp.main_vds.length, 32768 as u32);
    expect_eq(avdp.main_vds.location, 32 as u32);
    expect_eq(avdp.reserve_vds.length, 32768 as u32);
    expect_eq(avdp.reserve_vds.location, 48 as u32);
});

test("4. PVD (Tag 1) parsing & OSTA CS0 8-bit + 16-bit UTF-16BE dstring decoding", () => {
    let sec: [512]u8 = std.mem.zeroes([512]u8);
    putU32Le(sec[0..], 16, 1); // vds_seq_num
    putU32Le(sec[0..], 20, 0); // pvd_num

    // VolumeIdentifier (dstring[32] at offset 24..56): CS0 8-bit "UDF_DISK_01"
    sec[24] = 8; // comp_id = 8
    copyBytes(sec[0..], 25, "UDF_DISK_01");
    sec[55] = 12; // 1 (comp_id) + 11 chars

    putU16Le(sec[0..], 56, 1); // vol_seq_num
    putU16Le(sec[0..], 58, 1); // max_vol_seq_num
    putU16Le(sec[0..], 60, 2); // interchange_level

    // VolumeSetIdentifier (dstring[128] at offset 72..200): CS0 16-bit UTF-16BE "CP26" + non-ASCII 0x4E2D
    sec[72] = 16; // comp_id = 16 (UTF-16BE)
    sec[73] = 0x00;
    sec[74] = 0x43; // 'C'
    sec[75] = 0x00;
    sec[76] = 0x50; // 'P'
    sec[77] = 0x00;
    sec[78] = 0x32; // '2'
    sec[79] = 0x00;
    sec[80] = 0x36; // '6'
    sec[81] = 0x4E;
    sec[82] = 0x2D; // non-ASCII -> '?'
    sec[199] = 11; // 1 (comp_id) + 10 bytes (5 UTF-16BE chars)

    sealUdfTag(sec[0..], TAG_PVD, 2, 1, 496, 32);

    const pvd = parseUdfPvd(sec[0..]) ?? unreachable;
    expect_eq(pvd.tag.tag_id, TAG_PVD);
    expect_eq(pvd.vds_seq_num, 1 as u32);
    expect_eq(pvd.pvd_num, 0 as u32);
    expect_eq(pvd.vol_seq_num, 1 as u16);
    expect_eq(pvd.max_vol_seq_num, 1 as u16);
    expect_eq(pvd.interchange_level, 2 as u16);

    let vol_buf: [32]u8 = [0; 32];
    const vol_len = pvd.decodeVolIdAscii(vol_buf[0..]);
    expect_eq(vol_len, 11 as usize);
    expect_eq(eqStr(vol_buf[0..vol_len], "UDF_DISK_01"), true);

    let set_buf: [32]u8 = [0; 32];
    const set_len = decodeOstaCs0Ascii(pvd.vol_set_id_raw[0..], set_buf[0..]) ?? unreachable;
    expect_eq(set_len, 5 as usize);
    expect_eq(eqStr(set_buf[0..set_len], "CP26?"), true);
});

test("5. Malformed Tag checksum, CRC16 mismatch, wrong Tag ID, VRS & CS0 boundary rejection", () => {
    // 1. Truncated tag
    let tiny: [12]u8 = std.mem.zeroes([12]u8);
    expect_eq(parseUdfTag(tiny[0..]) == null, true);

    // 2. Bad tag checksum
    let sec: [512]u8 = std.mem.zeroes([512]u8);
    sealUdfTag(sec[0..], TAG_AVDP, 2, 0, 496, 256);
    sec[4] = sec[4] ^ 0x01;
    expect_eq(parseUdfTag(sec[0..]) == null, true);
    expect_eq(parseUdfAvdp(sec[0..]) == null, true);

    // 3. Bad body CRC16 (tag checksum still valid)
    sealUdfTag(sec[0..], TAG_AVDP, 2, 0, 496, 256);
    sec[20] = 0xFF; // mutate body without updating desc_crc
    expect_eq(parseUdfTag(sec[0..]) == null, true);
    expect_eq(parseUdfAvdp(sec[0..]) == null, true);

    // 4. Wrong Tag ID (PVD passed to parseUdfAvdp and vice versa)
    sealUdfTag(sec[0..], TAG_PVD, 2, 0, 496, 32);
    expect_eq(parseUdfAvdp(sec[0..]) == null, true);
    sealUdfTag(sec[0..], TAG_AVDP, 2, 0, 496, 256);
    expect_eq(parseUdfPvd(sec[0..]) == null, true);

    // 5. Truncated AVDP / PVD
    expect_eq(parseUdfAvdp(sec[0..24]) == null, true);
    expect_eq(parseUdfPvd(sec[0..100]) == null, true);

    // 6. Malformed VRS (wrong struct_type, wrong version, truncated)
    let vrs: [16]u8 = std.mem.zeroes([16]u8);
    vrs[0] = 1; // struct_type != 0
    vrs[6] = 1;
    expect_eq(parseUdfVrsEntry(vrs[0..]) == null, true);
    vrs[0] = 0;
    vrs[6] = 2; // version != 1
    expect_eq(parseUdfVrsEntry(vrs[0..]) == null, true);
    expect_eq(parseUdfVrsEntry(vrs[0..5]) == null, true);

    // 7. Malformed OSTA CS0 dstrings
    let out: [16]u8 = [0; 16];
    expect_eq(decodeOstaCs0Ascii("", out[0..]) == null, true);

    let bad_cs0: [8]u8 = [0; 8];
    bad_cs0[0] = 99; // invalid comp_id
    bad_cs0[7] = 4;
    expect_eq(decodeOstaCs0Ascii(bad_cs0[0..], out[0..]) == null, true);

    bad_cs0[0] = 16; // UTF-16BE with odd payload byte count (4 - 1 = 3)
    bad_cs0[7] = 4;
    expect_eq(decodeOstaCs0Ascii(bad_cs0[0..], out[0..]) == null, true);

    bad_cs0[0] = 8;
    bad_cs0[7] = 8; // length >= dstring.len (overlaps length byte)
    expect_eq(decodeOstaCs0Ascii(bad_cs0[0..], out[0..]) == null, true);
});
