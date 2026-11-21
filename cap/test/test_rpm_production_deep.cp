// Source: cp/cap/test/test_rpm_production_deep.cp (Production deep tests for cap:rpm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RPM_LEAD_MAGIC,
    RPM_LEAD_SIZE,
    RPM_HEADER_MAGIC,
    RPM_HEADER_INTRO_SIZE,
    RPM_INDEX_ENTRY_SIZE,
    TAG_NAME,
    TAG_VERSION,
    TAG_RELEASE,
    TAG_SIZE,
    TAG_ARCH,
    TAG_PAYLOAD_COMPRESSOR,
    RPM_TYPE_INT32,
    RPM_TYPE_STRING,
    RpmLead,
    RpmHeaderSection,
    RpmTagEntry,
    RpmPackageInfo,
    isRpm,
    parseRpmLead,
    parseRpmHeaderSection,
    RpmTagIter,
    readRpmString,
    readRpmU32,
    parseRpmPackage,
} from "cap:rpm";

fn putU16BE(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn putU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeLead(dst: []u8, pkg_type: u16, archnum: u16, name: []const u8) {
    let i: usize = 0;
    while (i < 96) {
        dst[i] = 0;
        i += 1;
    }
    putU32BE(dst, 0, RPM_LEAD_MAGIC);
    dst[4] = 3; // major
    dst[5] = 0; // minor
    putU16BE(dst, 6, pkg_type);
    putU16BE(dst, 8, archnum);
    let k: usize = 0;
    while (k < name.len && k < 65) {
        dst[10 + k] = name[k];
        k += 1;
    }
    putU16BE(dst, 76, 1); // Linux
    putU16BE(dst, 78, 5); // Header-style signature
}

fn writeHeaderIntro(dst: []u8, off: usize, nindex: u32, hsize: u32) {
    dst[off] = 0x8E;
    dst[off + 1] = 0xAD;
    dst[off + 2] = 0xE8;
    dst[off + 3] = 0x01;
    putU32BE(dst, off + 4, 0);
    putU32BE(dst, off + 8, nindex);
    putU32BE(dst, off + 12, hsize);
}

fn writeIndexEntry(dst: []u8, off: usize, tag: u32, tag_type: u32, store_off: u32, count: u32) {
    putU32BE(dst, off, tag);
    putU32BE(dst, off + 4, tag_type);
    putU32BE(dst, off + 8, store_off);
    putU32BE(dst, off + 12, count);
}

fn writeCString(dst: []u8, off: usize, text: []const u8): usize {
    let i: usize = 0;
    while (i < text.len) {
        dst[off + i] = text[i];
        i += 1;
    }
    dst[off + text.len] = 0;
    return text.len + 1;
}

fn buildSampleRpm(dst: []u8, pkg_type: u16): usize {
    let i: usize = 0;
    while (i < dst.len) {
        dst[i] = 0;
        i += 1;
    }
    writeLead(dst, pkg_type, 1, "cp-runtime-1.0.0-1.x86_64");

    // Signature section at offset 96: 1 index entry + 5-byte store -> total = 37 bytes -> padded to 40 bytes (3B pad)
    const sig_off: usize = 96;
    writeHeaderIntro(dst, sig_off, 1, 5);
    writeIndexEntry(dst, sig_off + 16, 1000, RPM_TYPE_INT32, 0, 1);
    putU32BE(dst, sig_off + 32, 4096);
    dst[sig_off + 36] = 0; // 5th byte of store (forces 3-byte 8B-alignment pad)

    // Main Header starts at 96 + 40 = 136
    const hdr_off: usize = 136;
    // 6 tags: NAME, VERSION, RELEASE, ARCH, PAYLOAD_COMPRESSOR, SIZE
    const n_tags: u32 = 6;
    const entries_off: usize = hdr_off + 16;
    const store_off: usize = entries_off + (n_tags as usize) * 16;

    let sp: usize = 0;
    const off_size = sp as u32;
    putU32BE(dst, store_off + sp, 1048576); // 1 MiB installed size
    sp += 4;

    const off_name = sp as u32;
    sp += writeCString(dst, store_off + sp, "cp-runtime");

    const off_ver = sp as u32;
    sp += writeCString(dst, store_off + sp, "1.0.0");

    const off_rel = sp as u32;
    sp += writeCString(dst, store_off + sp, "1.el9");

    const off_arch = sp as u32;
    sp += writeCString(dst, store_off + sp, "x86_64");

    const off_comp = sp as u32;
    sp += writeCString(dst, store_off + sp, "zstd");

    writeHeaderIntro(dst, hdr_off, n_tags, sp as u32);
    writeIndexEntry(dst, entries_off + 0 * 16, TAG_SIZE, RPM_TYPE_INT32, off_size, 1);
    writeIndexEntry(dst, entries_off + 1 * 16, TAG_NAME, RPM_TYPE_STRING, off_name, 1);
    writeIndexEntry(dst, entries_off + 2 * 16, TAG_VERSION, RPM_TYPE_STRING, off_ver, 1);
    writeIndexEntry(dst, entries_off + 3 * 16, TAG_RELEASE, RPM_TYPE_STRING, off_rel, 1);
    writeIndexEntry(dst, entries_off + 4 * 16, TAG_ARCH, RPM_TYPE_STRING, off_arch, 1);
    writeIndexEntry(dst, entries_off + 5 * 16, TAG_PAYLOAD_COMPRESSOR, RPM_TYPE_STRING, off_comp, 1);

    const payload_off = store_off + sp;
    // Append 4-byte Zstd payload magic (0x28 B5 2F FD)
    dst[payload_off + 0] = 0x28;
    dst[payload_off + 1] = 0xB5;
    dst[payload_off + 2] = 0x2F;
    dst[payload_off + 3] = 0xFD;
    return payload_off + 4;
}

test("1. parseRpmPackage decodes Lead, 8-byte aligned Signature, Main Header tags, and payload_offset", () => {
    let buf: [512]u8 = undefined;
    const total = buildSampleRpm(buf[0..512], 0);
    const rpm = buf[0..total];

    expect_eq(isRpm(rpm), true);
    const pkg = parseRpmPackage(rpm) ?? unreachable;

    expect_eq(pkg.lead.major, 3);
    expect_eq(pkg.lead.minor, 0);
    expect_eq(pkg.lead.isBinary(), true);
    expect_eq(pkg.lead.isSource(), false);
    expect_eq(std.mem.eql(u8, pkg.lead.name, "cp-runtime-1.0.0-1.x86_64"), true);

    // Signature had total_bytes = 37 -> aligned next offset = 96 + 40 = 136
    expect_eq(pkg.signature.total_bytes, 37);
    expect_eq(pkg.signature.next_aligned_offset, 136);

    expect_eq(std.mem.eql(u8, pkg.name ?? unreachable, "cp-runtime"), true);
    expect_eq(std.mem.eql(u8, pkg.version ?? unreachable, "1.0.0"), true);
    expect_eq(std.mem.eql(u8, pkg.release ?? unreachable, "1.el9"), true);
    expect_eq(std.mem.eql(u8, pkg.arch ?? unreachable, "x86_64"), true);
    expect_eq(std.mem.eql(u8, pkg.payload_compressor ?? unreachable, "zstd"), true);
    expect_eq(pkg.installed_size ?? 0, 1048576);

    // Payload starts right at Zstd magic
    expect_eq(pkg.payload_offset, total - 4);
    expect_eq(rpm[pkg.payload_offset], 0x28);
    expect_eq(rpm[pkg.payload_offset + 1], 0xB5);
});

test("2. parseRpmLead distinguishes Source RPM (pkg_type == 1) from Binary RPM (pkg_type == 0)", () => {
    let buf: [512]u8 = undefined;
    const total = buildSampleRpm(buf[0..512], 1);
    const lead = parseRpmLead(buf[0..total]) ?? unreachable;

    expect_eq(lead.isSource(), true);
    expect_eq(lead.isBinary(), false);
    expect_eq(lead.signature_type, 5);
});

test("3. RpmTagIter, readRpmString, and readRpmU32 safely extract typed values and reject type mismatches", () => {
    let buf: [512]u8 = undefined;
    const total = buildSampleRpm(buf[0..512], 0);
    const rpm = buf[0..total];
    const pkg = parseRpmPackage(rpm) ?? unreachable;
    const store = rpm[pkg.header.store_offset .. pkg.header.end_offset];

    let it = RpmTagIter.init(rpm, pkg.header);
    const e0 = it.next() ?? unreachable;
    expect_eq(e0.tag, TAG_SIZE);
    expect_eq(readRpmU32(store, e0) ?? 0, 1048576);
    expect_eq(readRpmString(store, e0) == null, true); // wrong type

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.tag, TAG_NAME);
    expect_eq(std.mem.eql(u8, readRpmString(store, e1) ?? unreachable, "cp-runtime"), true);
    expect_eq(readRpmU32(store, e1) == null, true); // wrong type

    // Out-of-bounds store offset rejected safely
    const bad_entry = RpmTagEntry {
        tag: TAG_NAME,
        tag_type: RPM_TYPE_STRING,
        offset: 99999,
        count: 1,
    };
    expect_eq(readRpmString(store, bad_entry) == null, true);
});

test("4. parseRpmHeaderSection rejects bad magic, non-zero reserved bytes, and overflowing nindex/hsize", () => {
    let bad: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) {
        bad[i] = 0;
        i += 1;
    }
    let s = bad[0..32];

    // Wrong header magic
    expect_eq(parseRpmHeaderSection(s, 0) == null, true);

    // Valid magic but non-zero reserved field
    writeHeaderIntro(s, 0, 0, 0);
    s[7] = 1;
    expect_eq(parseRpmHeaderSection(s, 0) == null, true);

    // Huge nindex causing potential multiplication overflow
    writeHeaderIntro(s, 0, 0x7FFFFFFF, 0);
    expect_eq(parseRpmHeaderSection(s, 0) == null, true);

    // hsize exceeds remaining buffer
    writeHeaderIntro(s, 0, 0, 500);
    expect_eq(parseRpmHeaderSection(s, 0) == null, true);
});

test("5. isRpm and parseRpmPackage reject short buffers and non-RPM binaries", () => {
    const short_buf: [16]u8 = [0xED, 0xAB, 0xEE, 0xDB, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(isRpm(short_buf[0..16]), false);
    expect_eq(parseRpmLead(short_buf[0..16]) == null, true);

    let fake: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 128) {
        fake[i] = 0;
        i += 1;
    }
    expect_eq(isRpm(fake[0..128]), false);
    expect_eq(parseRpmPackage(fake[0..128]) == null, true);
});
