// Source: cp/cap/test/test_edid_production_deep.cp (Production deep tests for cap:edid)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EDID_BLOCK_SIZE,
    EDID_HEADER_MAGIC,
    DESC_SERIAL,
    DESC_UNSPECIFIED_TEXT,
    DESC_RANGE_LIMITS,
    DESC_MONITOR_NAME,
    DESC_DUMMY,
    CEA_EXT_TAG,
    edidChecksum,
    verifyEdidChecksum,
    decodePnpId,
    encodePnpId,
    EdidDetailedTiming,
    EdidDescriptor,
    EdidStandardTiming,
    EdidInfo,
    isEdid,
    parseEdid,
} from "cap:sys";

fn finalizeEdid(block: []u8) {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < 127) {
        sum += block[i] as u32;
        i += 1;
    }
    block[127] = ((256 - (sum & 0xFF)) & 0xFF) as u8;
}

fn writeTextDescriptor(block: []u8, slot: usize, tag: u8, text: []const u8) {
    const off = 54 + slot * 18;
    block[off] = 0;
    block[off + 1] = 0;
    block[off + 2] = 0;
    block[off + 3] = tag;
    block[off + 4] = 0;
    let i: usize = 0;
    while (i < 13) {
        if (i < text.len) {
            block[off + 5 + i] = text[i];
        } else if (i == text.len) {
            block[off + 5 + i] = 0x0A;
        } else {
            block[off + 5 + i] = 0x20;
        }
        i += 1;
    }
}

fn buildSampleEdid14(block: []u8) {
    let i: usize = 0;
    while (i < 8) {
        block[i] = EDID_HEADER_MAGIC[i];
        i += 1;
    }
    // Manufacturer "DEL"
    const pnp = encodePnpId(['D', 'E', 'L']) ?? unreachable;
    block[8] = ((pnp >> 8) & 0xFF) as u8;
    block[9] = (pnp & 0xFF) as u8;
    // Product code = 0xA0B1 (LE)
    block[10] = 0xB1;
    block[11] = 0xA0;
    // Serial number = 0x12345678 (LE)
    block[12] = 0x78;
    block[13] = 0x56;
    block[14] = 0x34;
    block[15] = 0x12;
    // Week 22, Year 2026 (1990 + 36)
    block[16] = 22;
    block[17] = 36;
    // EDID 1.4
    block[18] = 1;
    block[19] = 4;
    // Digital (0x80) + 10-bit depth (3 << 4 = 0x30) + DisplayPort (0x05)
    block[20] = 0xB5;
    // 60 cm x 34 cm
    block[21] = 60;
    block[22] = 34;
    // Gamma 2.20 -> 120
    block[23] = 120;
    // Feature flags
    block[24] = 0xEA;

    // Standard Timings [38..54]: initialize unused as 0x01, 0x01
    i = 38;
    while (i < 54) {
        block[i] = 0x01;
        i += 1;
    }
    // Slot 0: 1920x1080 @ 60Hz (16:9 -> aspect 3 = 0xC0): (1920/8)-31 = 209 = 0xD1
    block[38] = 0xD1;
    block[39] = 0xC0;
    // Slot 1: 1280x1024 @ 75Hz (5:4 -> aspect 2 = 0x80): (1280/8)-31 = 129 = 0x81, 75-60=15
    block[40] = 0x81;
    block[41] = 0x80 | 15;
    // Slot 2: 1680x1050 @ 60Hz (16:10 -> aspect 0 = 0x00): (1680/8)-31 = 179 = 0xB3
    block[42] = 0xB3;
    block[43] = 0x00;
    // Slot 3: 1024x768 @ 60Hz (4:3 -> aspect 1 = 0x40): (1024/8)-31 = 97 = 0x61
    block[44] = 0x61;
    block[45] = 0x40;

    // Descriptor 0 [54..72]: 1920x1080 @ 60Hz CEA DTD
    // Pixel clock = 148.50 MHz -> 14850 (0x3A02 LE)
    // H active=1920 (0x780), H blank=280 (0x118) -> H total = 2200
    // V active=1080 (0x438), V blank=45 (0x02D) -> V total = 1125
    // Refresh = 148,500,000 / (2200 * 1125) = 60.000 Hz exact!
    block[54] = 0x02;
    block[55] = 0x3A;
    block[56] = 0x80; // h_active low
    block[57] = 0x18; // h_blank low
    block[58] = 0x71; // h_active hi (7) | h_blank hi (1)
    block[59] = 0x38; // v_active low
    block[60] = 0x2D; // v_blank low
    block[61] = 0x40; // v_active hi (4) | v_blank hi (0)
    block[62] = 0x58; // h_sync_offset = 88
    block[63] = 0x2C; // h_sync_pulse = 44
    block[64] = 0x45; // v_sync_offset = 4, v_sync_pulse = 5
    block[65] = 0x00; // sync high bits
    block[66] = 0x58; // h_size_mm = 600 (0x258)
    block[67] = 0x54; // v_size_mm = 340 (0x154)
    block[68] = 0x21; // h_size hi (2) | v_size hi (1)
    block[71] = 0x1E; // non-interlaced

    // Descriptor 1: Monitor Name
    writeTextDescriptor(block, 1, DESC_MONITOR_NAME, "DELL U2723QE");
    // Descriptor 2: Serial Text
    writeTextDescriptor(block, 2, DESC_SERIAL, "CP998877");
    // Descriptor 3: Dummy
    block[108 + 3] = DESC_DUMMY;

    // 1 CEA extension block
    block[126] = 1;
    finalizeEdid(block);
}

test("1. 3-letter PNP ID encoding & decoding round-trip (DEL, SAM, APP)", () => {
    expect_eq(EDID_BLOCK_SIZE, 128 as usize);
    expect_eq(DESC_UNSPECIFIED_TEXT, 0xFE as u8);
    expect_eq(DESC_RANGE_LIMITS, 0xFD as u8);
    expect_eq(CEA_EXT_TAG, 0x02 as u8);

    const del_be = encodePnpId(['D', 'E', 'L']) ?? unreachable;
    expect_eq(del_be, 0x10AC as u16);
    const del_id = decodePnpId(del_be);
    expect_eq(std.mem.eql(u8, del_id[0..], "DEL"), true);

    const sam_be = encodePnpId(['S', 'A', 'M']) ?? unreachable;
    const sam_id = decodePnpId(sam_be);
    expect_eq(std.mem.eql(u8, sam_id[0..], "SAM"), true);

    const app_be = encodePnpId(['A', 'P', 'P']) ?? unreachable;
    const app_id = decodePnpId(app_be);
    expect_eq(std.mem.eql(u8, app_id[0..], "APP"), true);

    // Lowercase or non-alpha must be rejected
    expect_eq(encodePnpId(['d', 'e', 'l']) == null, true);
    expect_eq(encodePnpId(['A', '1', 'Z']) == null, true);
});

test("2. Full 128B EDID 1.4 parsing & 1920x1080@60Hz Preferred Timing calculation", () => {
    let edid: [128]u8 = std.mem.zeroes([128]u8);
    buildSampleEdid14(edid[0..]);

    expect_eq(edidChecksum(edid[0..]), 0 as u8);
    expect_eq(verifyEdidChecksum(edid[0..]), true);
    expect_eq(isEdid(edid[0..]), true);

    const info = parseEdid(edid[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, info.manufacturer[0..], "DEL"), true);
    expect_eq(info.product_code, 0xA0B1 as u16);
    expect_eq(info.serial_number, 0x12345678 as u32);
    expect_eq(info.mfg_week, 22 as u8);
    expect_eq(info.mfg_year, 2026 as u16);
    expect_eq(info.version, 1 as u8);
    expect_eq(info.revision, 4 as u8);
    expect_eq(info.is_digital, true);
    expect_eq(info.color_bit_depth, 10 as u8);
    expect_eq(info.width_cm, 60 as u8);
    expect_eq(info.height_cm, 34 as u8);
    expect_eq(info.extension_count, 1 as u8);
    expect_eq(info.gamma() > 2.19 && info.gamma() < 2.21, true);

    const pt = info.preferredTiming() ?? unreachable;
    expect_eq(pt.pixel_clock_khz, 148500 as u32);
    expect_eq(pt.h_active, 1920 as u16);
    expect_eq(pt.h_blank, 280 as u16);
    expect_eq(pt.v_active, 1080 as u16);
    expect_eq(pt.v_blank, 45 as u16);
    expect_eq(pt.h_sync_offset, 88 as u16);
    expect_eq(pt.h_sync_pulse, 44 as u16);
    expect_eq(pt.v_sync_offset, 4 as u16);
    expect_eq(pt.v_sync_pulse, 5 as u16);
    expect_eq(pt.h_size_mm, 600 as u16);
    expect_eq(pt.v_size_mm, 340 as u16);
    expect_eq(pt.interlaced, false);
    const hz = pt.refreshRateHz();
    expect_eq(hz > 59.99 && hz < 60.01, true);
});

test("3. Monitor Name (0xFC), Serial (0xFF), and descriptor slot accessors", () => {
    let edid: [128]u8 = std.mem.zeroes([128]u8);
    buildSampleEdid14(edid[0..]);
    const info = parseEdid(edid[0..]) ?? unreachable;

    expect_eq(std.mem.eql(u8, info.monitorName() ?? unreachable, "DELL U2723QE"), true);
    expect_eq(std.mem.eql(u8, info.serialText() ?? unreachable, "CP998877"), true);

    const d3 = info.descriptorAt(3) ?? unreachable;
    expect_eq(d3.is_timing, false);
    expect_eq(d3.tag, DESC_DUMMY);
    expect_eq(info.descriptorAt(4) == null, true);
});

test("4. Standard Timing decoding across all 4 aspect ratios (16:9, 5:4, 16:10, 4:3)", () => {
    let edid: [128]u8 = std.mem.zeroes([128]u8);
    buildSampleEdid14(edid[0..]);
    const info = parseEdid(edid[0..]) ?? unreachable;

    const st0 = info.standardTimingAt(0) ?? unreachable;
    expect_eq(st0.width, 1920 as u16);
    expect_eq(st0.height, 1080 as u16);
    expect_eq(st0.refresh_hz, 60 as u8);

    const st1 = info.standardTimingAt(1) ?? unreachable;
    expect_eq(st1.width, 1280 as u16);
    expect_eq(st1.height, 1024 as u16);
    expect_eq(st1.refresh_hz, 75 as u8);

    const st2 = info.standardTimingAt(2) ?? unreachable;
    expect_eq(st2.width, 1680 as u16);
    expect_eq(st2.height, 1050 as u16);
    expect_eq(st2.refresh_hz, 60 as u8);

    const st3 = info.standardTimingAt(3) ?? unreachable;
    expect_eq(st3.width, 1024 as u16);
    expect_eq(st3.height, 768 as u16);
    expect_eq(st3.refresh_hz, 60 as u8);

    // Unused slot 4 (0x01, 0x01) and out-of-range slot 8
    expect_eq(info.standardTimingAt(4) == null, true);
    expect_eq(info.standardTimingAt(8) == null, true);
});

test("5. Malformed magic, bad checksum, wrong version, and short block rejection", () => {
    let edid: [128]u8 = std.mem.zeroes([128]u8);
    buildSampleEdid14(edid[0..]);

    // Short block (< 128 bytes)
    expect_eq(verifyEdidChecksum(edid[0..127]), false);
    expect_eq(parseEdid(edid[0..127]) == null, true);

    // Bad checksum
    edid[127] = edid[127] ^ 0x01;
    expect_eq(verifyEdidChecksum(edid[0..]), false);
    expect_eq(parseEdid(edid[0..]) == null, true);
    edid[127] = edid[127] ^ 0x01;

    // Bad header magic
    edid[0] = 0xAA;
    finalizeEdid(edid[0..]);
    expect_eq(isEdid(edid[0..]), false);
    expect_eq(parseEdid(edid[0..]) == null, true);

    // Bad version (version = 2)
    edid[0] = 0x00;
    edid[18] = 2;
    finalizeEdid(edid[0..]);
    expect_eq(parseEdid(edid[0..]) == null, true);
});
