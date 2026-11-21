// Source: cp/cap/test/test_iso_production_deep.cp (Production deep tests for cap:iso)

import { test, expect_eq } from "cap:test";
import {
    ISO_SECTOR_SIZE,
    ISO_SYSTEM_AREA_SECTORS,
    ISO_PVD_OFFSET,
    ISO_MAGIC,
    VD_TYPE_BOOT,
    VD_TYPE_PRIMARY,
    VD_TYPE_SUPPLEMENTARY,
    VD_TYPE_TERMINATOR,
    ISO_FLAG_HIDDEN,
    ISO_FLAG_DIRECTORY,
    IsoPrimaryVolume,
    IsoDirEntry,
    IsoDirIter,
    isIso9660,
    parseIsoPvd,
    findIsoEntry,
    readIsoFileData,
} from "cap:iso";

fn zeroBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn writePaddedSpaces(dst: []u8, off: usize, width: usize, text: []const u8) {
    let i: usize = 0;
    while (i < width) {
        dst[off + i] = if (i < text.len) text[i] else ' ';
        i += 1;
    }
}

fn writeBothU16(dst: []u8, off: usize, val: u16) {
    dst[off + 0] = (val & 0xFF) as u8;
    dst[off + 1] = ((val >> 8) & 0xFF) as u8;
    dst[off + 2] = ((val >> 8) & 0xFF) as u8;
    dst[off + 3] = (val & 0xFF) as u8;
}

fn writeBothU32(dst: []u8, off: usize, val: u32) {
    dst[off + 0] = (val & 0xFF) as u8;
    dst[off + 1] = ((val >> 8) & 0xFF) as u8;
    dst[off + 2] = ((val >> 16) & 0xFF) as u8;
    dst[off + 3] = ((val >> 24) & 0xFF) as u8;
    dst[off + 4] = ((val >> 24) & 0xFF) as u8;
    dst[off + 5] = ((val >> 16) & 0xFF) as u8;
    dst[off + 6] = ((val >> 8) & 0xFF) as u8;
    dst[off + 7] = (val & 0xFF) as u8;
}

fn writeVdHeader(img: []u8, sector_idx: usize, vd_type: u8) {
    const base = sector_idx * ISO_SECTOR_SIZE;
    img[base] = vd_type;
    copyBytes(img, base + 1, ISO_MAGIC);
    img[base + 6] = 1;
}

fn writeStandardPvd(
    img: []u8,
    sector_idx: usize,
    vol_blocks: u32,
    block_size: u16,
    path_table_size: u32,
    root_lba: u32,
    root_len: u32,
) {
    const base = sector_idx * ISO_SECTOR_SIZE;
    writeVdHeader(img, sector_idx, VD_TYPE_PRIMARY);
    writePaddedSpaces(img, base + 8, 32, "LINUX");
    writePaddedSpaces(img, base + 40, 32, "CP_ISO_IMAGE");
    writeBothU32(img, base + 80, vol_blocks);
    writeBothU16(img, base + 120, 1);
    writeBothU16(img, base + 124, 1);
    writeBothU16(img, base + 128, block_size);
    writeBothU32(img, base + 132, path_table_size);

    // Root Directory Record at offset 156 (length 34)
    img[base + 156] = 34;
    img[base + 157] = 0;
    writeBothU32(img, base + 158, root_lba);
    writeBothU32(img, base + 166, root_len);
    img[base + 181] = ISO_FLAG_DIRECTORY;
    img[base + 188] = 1;
    img[base + 189] = 0;

    writePaddedSpaces(img, base + 318, 128, "CP-LANG FOUNDATION");
    writePaddedSpaces(img, base + 574, 128, "CAP-ISO-BUILDER 1.0");
}

fn writeDirRecord(
    img: []u8,
    off: usize,
    extent_lba: u32,
    data_len: u32,
    flags: u8,
    name: []const u8,
): usize {
    let rec_len: usize = (33 as usize) + name.len;
    if ((rec_len & 1) == 1) {
        rec_len += 1;
    }
    img[off] = rec_len as u8;
    img[off + 1] = 0;
    writeBothU32(img, off + 2, extent_lba);
    writeBothU32(img, off + 10, data_len);
    img[off + 25] = flags;
    writeBothU16(img, off + 28, 1);
    img[off + 32] = name.len as u8;
    copyBytes(img, off + 33, name);
    return rec_len;
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

test("1. parses PVD after 16 reserved sectors and leading Boot VD with trailing-space trim", () => {
    // 20 sectors = 40960 bytes
    let raw: [40960]u8 = undefined;
    const img = raw[0..40960];
    zeroBytes(img);

    // Sector 16: Boot Record VD, Sector 17: Primary VD, Sector 18: Terminator
    writeVdHeader(img, 16, VD_TYPE_BOOT);
    writeStandardPvd(img, 17, 20, 2048, 42, 19, 2048);
    writeVdHeader(img, 18, VD_TYPE_TERMINATOR);

    expect_eq(ISO_SECTOR_SIZE, 2048);
    expect_eq(ISO_SYSTEM_AREA_SECTORS, 16);
    expect_eq(ISO_PVD_OFFSET, 32768);
    expect_eq(isIso9660(img), true);

    if (pvd := parseIsoPvd(img)) {
        expect_eq(sliceEq(pvd.system_id, "LINUX"), true);
        expect_eq(sliceEq(pvd.volume_id, "CP_ISO_IMAGE"), true);
        expect_eq(pvd.volume_space_size, 20);
        expect_eq(pvd.logical_block_size, 2048);
        expect_eq(pvd.path_table_size, 42);
        expect_eq(pvd.root_extent_lba, 19);
        expect_eq(pvd.root_data_len, 2048);
        expect_eq(sliceEq(pvd.publisher_id, "CP-LANG FOUNDATION"), true);
        expect_eq(sliceEq(pvd.application_id, "CAP-ISO-BUILDER 1.0"), true);
    } else {
        expect_eq(true, false);
    }
});

test("2. validates both-endian consistency and rejects mismatched PVD numeric fields", () => {
    let raw: [36864]u8 = undefined; // 18 sectors
    let img = raw[0..36864];
    zeroBytes(img);

    writeStandardPvd(img, 16, 18, 2048, 10, 17, 2048);
    writeVdHeader(img, 17, VD_TYPE_TERMINATOR);

    // Baseline valid
    expect_eq(parseIsoPvd(img) != null, true);

    // Corrupt BE half of volume_space_size (offset 84..88)
    img[ISO_PVD_OFFSET + 87] ^= 0x01;
    expect_eq(parseIsoPvd(img) == null, true);
    img[ISO_PVD_OFFSET + 87] ^= 0x01;

    // Corrupt BE half of logical_block_size (offset 130..132)
    img[ISO_PVD_OFFSET + 131] ^= 0x01;
    expect_eq(parseIsoPvd(img) == null, true);
    img[ISO_PVD_OFFSET + 131] ^= 0x01;

    // Corrupt BE half of path_table_size (offset 136..140)
    img[ISO_PVD_OFFSET + 139] ^= 0x01;
    expect_eq(parseIsoPvd(img) == null, true);
    img[ISO_PVD_OFFSET + 139] ^= 0x01;

    // Corrupt BE half of root_extent_lba (offset 162..166)
    img[ISO_PVD_OFFSET + 165] ^= 0x01;
    expect_eq(parseIsoPvd(img) == null, true);
    img[ISO_PVD_OFFSET + 165] ^= 0x01;

    // Zero logical_block_size must also be rejected
    writeBothU16(img, ISO_PVD_OFFSET + 128, 0);
    expect_eq(parseIsoPvd(img) == null, true);
});

test("3. iterates root and subdirectory entries with dot/dotdot/hidden/dir flags and baseName stripping", () => {
    // 22 sectors = 45056 bytes
    // Sector 16: PVD, Sector 17: Terminator, Sector 18: Root dir, Sector 19: DOCS dir, Sector 20: file
    let raw: [45056]u8 = undefined;
    const img = raw[0..45056];
    zeroBytes(img);

    writeStandardPvd(img, 16, 22, 2048, 10, 18, 2048);
    writeVdHeader(img, 17, VD_TYPE_TERMINATOR);

    const dot_name: [1]u8 = [0x00];
    const dotdot_name: [1]u8 = [0x01];

    // Root directory at sector 18
    let pos: usize = (18 as usize) * ISO_SECTOR_SIZE;
    pos += writeDirRecord(img, pos, 18, 2048, ISO_FLAG_DIRECTORY, dot_name[0..1]);
    pos += writeDirRecord(img, pos, 18, 2048, ISO_FLAG_DIRECTORY, dotdot_name[0..1]);
    pos += writeDirRecord(img, pos, 20, 13, 0, "README.TXT;1");
    pos += writeDirRecord(img, pos, 20, 5, ISO_FLAG_HIDDEN, "SECRET.DAT;2");
    pos += writeDirRecord(img, pos, 19, 2048, ISO_FLAG_DIRECTORY, "DOCS");

    // Subdirectory DOCS at sector 19
    let sub_pos: usize = (19 as usize) * ISO_SECTOR_SIZE;
    sub_pos += writeDirRecord(img, sub_pos, 19, 2048, ISO_FLAG_DIRECTORY, dot_name[0..1]);
    sub_pos += writeDirRecord(img, sub_pos, 18, 2048, ISO_FLAG_DIRECTORY, dotdot_name[0..1]);
    sub_pos += writeDirRecord(img, sub_pos, 20, 13, 0, "GUIDE.MD;1");

    if (pvd := parseIsoPvd(img)) {
        let it = IsoDirIter.fromRoot(img, pvd);

        // 1) "."
        if (e0 := it.next()) {
            expect_eq(e0.isDot(), true);
            expect_eq(e0.isDotDot(), false);
            expect_eq(e0.isDir(), true);
        } else {
            expect_eq(true, false);
        }

        // 2) ".."
        if (e1 := it.next()) {
            expect_eq(e1.isDotDot(), true);
            expect_eq(e1.isDir(), true);
        } else {
            expect_eq(true, false);
        }

        // 3) "README.TXT;1"
        if (e2 := it.next()) {
            expect_eq(e2.isDir(), false);
            expect_eq(e2.isHidden(), false);
            expect_eq(sliceEq(e2.name, "README.TXT;1"), true);
            expect_eq(sliceEq(e2.baseName(), "README.TXT"), true);
            expect_eq(e2.extent_lba, 20);
            expect_eq(e2.data_len, 13);
        } else {
            expect_eq(true, false);
        }

        // 4) "SECRET.DAT;2" (hidden)
        if (e3 := it.next()) {
            expect_eq(e3.isHidden(), true);
            expect_eq(sliceEq(e3.baseName(), "SECRET.DAT"), true);
        } else {
            expect_eq(true, false);
        }

        // 5) "DOCS" (subdirectory)
        if (e4 := it.next()) {
            expect_eq(e4.isDir(), true);
            expect_eq(sliceEq(e4.baseName(), "DOCS"), true);

            let sub_it = IsoDirIter.fromEntry(img, pvd, e4);
            let sub_count: usize = 0;
            let found_guide = false;
            while (se := sub_it.next()) {
                sub_count += 1;
                if (sliceEq(se.baseName(), "GUIDE.MD")) {
                    found_guide = true;
                }
            }
            expect_eq(sub_count, 3);
            expect_eq(found_guide, true);
            expect_eq(sub_it.had_error, false);
        } else {
            expect_eq(true, false);
        }

        expect_eq(it.next() == null, true);
        expect_eq(it.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("4. skips zero-padded sector tail (record_len == 0) across multi-sector directory extent", () => {
    // 22 sectors = 45056 bytes
    // Sector 16: PVD, Sector 17: Terminator, Sectors 18..19 (4096 bytes): Root dir, Sector 20: payload
    let raw: [45056]u8 = undefined;
    const img = raw[0..45056];
    zeroBytes(img);

    writeStandardPvd(img, 16, 22, 2048, 10, 18, 4096);
    writeVdHeader(img, 17, VD_TYPE_TERMINATOR);

    // First sector of root dir (sector 18): write ONE entry, leaving rest of sector 18 zero-padded
    const sec18 = 18 * ISO_SECTOR_SIZE;
    * = writeDirRecord(img, sec18, 20, 4, 0, "FIRST.BIN;1");

    // Second sector of root dir (sector 19): write SECOND entry at sector boundary
    const sec19 = 19 * ISO_SECTOR_SIZE;
    * = writeDirRecord(img, sec19, 20, 9, 0, "SECOND.BIN;1");

    if (pvd := parseIsoPvd(img)) {
        let it = IsoDirIter.fromRoot(img, pvd);
        let names_ok: usize = 0;

        if (e1 := it.next()) {
            if (sliceEq(e1.baseName(), "FIRST.BIN")) names_ok += 1;
        }
        if (e2 := it.next()) {
            if (sliceEq(e2.baseName(), "SECOND.BIN")) names_ok += 1;
        }
        expect_eq(names_ok, 2);
        expect_eq(it.next() == null, true);
        expect_eq(it.had_error, false);

        // findIsoEntry should also find SECOND.BIN across the sector gap
        if (found := findIsoEntry(img, pvd, "SECOND.BIN")) {
            expect_eq(found.data_len, 9);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. findIsoEntry and readIsoFileData perform zero-copy file reads and block directory reads", () => {
    // 21 sectors = 43008 bytes
    let raw: [43008]u8 = undefined;
    const img = raw[0..43008];
    zeroBytes(img);

    writeStandardPvd(img, 16, 21, 2048, 10, 18, 2048);
    writeVdHeader(img, 17, VD_TYPE_TERMINATOR);

    const payload = "Hello, ISO 9660 from CP!";
    copyBytes(img, 19 * ISO_SECTOR_SIZE, payload);

    let pos: usize = (18 as usize) * ISO_SECTOR_SIZE;
    pos += writeDirRecord(img, pos, 19, payload.len as u32, 0, "HELLO.TXT;1");
    pos += writeDirRecord(img, pos, 20, 2048, ISO_FLAG_DIRECTORY, "SUBDIR");

    if (pvd := parseIsoPvd(img)) {
        // Lookup by baseName (without ";1")
        if (entry := findIsoEntry(img, pvd, "HELLO.TXT")) {
            if (content := readIsoFileData(img, pvd, entry)) {
                expect_eq(sliceEq(content, payload), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // Lookup by exact name (with ";1")
        expect_eq(findIsoEntry(img, pvd, "HELLO.TXT;1") != null, true);
        expect_eq(findIsoEntry(img, pvd, "MISSING.TXT") == null, true);

        // Directory entry must not be read as raw file data
        if (dir_entry := findIsoEntry(img, pvd, "SUBDIR")) {
            expect_eq(readIsoFileData(img, pvd, dir_entry) == null, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("6. defensive bounds: truncated image, bad magic, terminator-only, out-of-bounds LBA, malformed records", () => {
    // 6a. Truncated image (< 32768 + 2048)
    let small: [128]u8 = undefined;
    zeroBytes(small[0..128]);
    expect_eq(isIso9660(small[0..128]), false);
    expect_eq(parseIsoPvd(small[0..128]) == null, true);

    // 6b. Wrong magic / version / terminator-only
    let raw: [40960]u8 = undefined; // 20 sectors
    let img = raw[0..40960];
    zeroBytes(img);
    writeStandardPvd(img, 16, 20, 2048, 10, 18, 2048);
    writeVdHeader(img, 17, VD_TYPE_TERMINATOR);

    // Corrupt "CD001" magic
    img[ISO_PVD_OFFSET + 3] = 'X';
    expect_eq(isIso9660(img), false);
    expect_eq(parseIsoPvd(img) == null, true);
    img[ISO_PVD_OFFSET + 3] = '0';

    // Corrupt version byte
    img[ISO_PVD_OFFSET + 6] = 2;
    expect_eq(isIso9660(img), false);
    expect_eq(parseIsoPvd(img) == null, true);
    img[ISO_PVD_OFFSET + 6] = 1;

    // Terminator immediately at sector 16 -> valid ISO header, no PVD
    img[ISO_PVD_OFFSET] = VD_TYPE_TERMINATOR;
    expect_eq(isIso9660(img), true);
    expect_eq(parseIsoPvd(img) == null, true);
    img[ISO_PVD_OFFSET] = VD_TYPE_PRIMARY;

    // 6c. Out-of-bounds LBA & overflow-safe readIsoFileData
    if (pvd := parseIsoPvd(img)) {
        const oob_entry = IsoDirEntry {
            extent_lba: 99999,
            data_len: 100,
            flags: 0,
            name: "OOB.BIN;1",
        };
        expect_eq(readIsoFileData(img, pvd, oob_entry) == null, true);

        const huge_lba_entry = IsoDirEntry {
            extent_lba: 0xFFFFFFFF,
            data_len: 0xFFFFFFFF,
            flags: 0,
            name: "HUGE.BIN;1",
        };
        expect_eq(readIsoFileData(img, pvd, huge_lba_entry) == null, true);

        // 6d. Malformed directory record (record_len = 10 < 33)
        img[18 * ISO_SECTOR_SIZE] = 10;
        let bad_it = IsoDirIter.fromRoot(img, pvd);
        expect_eq(bad_it.next() == null, true);
        expect_eq(bad_it.had_error, true);

        // 6e. Out-of-bounds root extent LBA in IsoDirIter.init
        let oob_it = IsoDirIter.init(img, 500, 2048, 2048);
        expect_eq(oob_it.next() == null, true);
        expect_eq(oob_it.had_error, true);
    } else {
        expect_eq(true, false);
    }
});
