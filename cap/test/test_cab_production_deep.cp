// Source: cp/cap/test/test_cab_production_deep.cp (Production deep tests for cap:cab)

import { test, expect_eq } from "cap:test";
import {
    CAB_MAGIC,
    CAB_HEADER_SIZE,
    CAB_FOLDER_SIZE,
    CAB_FLAG_PREV,
    CAB_FLAG_NEXT,
    CAB_FLAG_RESERVE,
    CAB_ATTR_RDONLY,
    CAB_ATTR_HIDDEN,
    CAB_ATTR_NAME_IS_UTF,
    CabCompression,
    decodeCabCompression,
    CabHeader,
    CabFolder,
    CabFile,
    isCab,
    parseCabHeader,
    CabFolderIter,
    CabFileIter,
    findCabFile,
} from "cap:cab";

fn zeroBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
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

fn writeBytes(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

fn writeCString(dst: []u8, off: usize, src: []const u8): usize {
    const end = writeBytes(dst, off, src);
    dst[end] = 0;
    return end + 1;
}

fn writeCabHeaderBase(
    dst: []u8,
    cb_cabinet: u32,
    coff_files: u32,
    c_folders: u16,
    c_files: u16,
    flags: u16,
    set_id: u16,
    i_cabinet: u16,
) {
    * = writeBytes(dst, 0, CAB_MAGIC);
    writeU32LE(dst, 4, 0); // reserved1
    writeU32LE(dst, 8, cb_cabinet);
    writeU32LE(dst, 12, 0); // reserved2
    writeU32LE(dst, 16, coff_files);
    writeU32LE(dst, 20, 0); // reserved3
    dst[24] = 3; // version_minor
    dst[25] = 1; // version_major
    writeU16LE(dst, 26, c_folders);
    writeU16LE(dst, 28, c_files);
    writeU16LE(dst, 30, flags);
    writeU16LE(dst, 32, set_id);
    writeU16LE(dst, 34, i_cabinet);
}

fn writeFolder(
    dst: []u8,
    off: usize,
    coff_cab_start: u32,
    c_cf_data: u16,
    type_compress: u16,
    reserve_len: usize,
): usize {
    writeU32LE(dst, off, coff_cab_start);
    writeU16LE(dst, off + 4, c_cf_data);
    writeU16LE(dst, off + 6, type_compress);
    return off + CAB_FOLDER_SIZE + reserve_len;
}

fn writeFile(
    dst: []u8,
    off: usize,
    cb_file: u32,
    uoff_folder_start: u32,
    i_folder: u16,
    date: u16,
    time: u16,
    attribs: u16,
    name: []const u8,
): usize {
    writeU32LE(dst, off, cb_file);
    writeU32LE(dst, off + 4, uoff_folder_start);
    writeU16LE(dst, off + 8, i_folder);
    writeU16LE(dst, off + 10, date);
    writeU16LE(dst, off + 12, time);
    writeU16LE(dst, off + 14, attribs);
    return writeCString(dst, off + 16, name);
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

test("1. parses standard single-volume MSZIP and LZX CAB header, folders, and multiple files", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    zeroBytes(buf);

    // Folders start at 36: 2 folders * 8B = 16B -> files start at 52
    const coff_files: usize = 52;
    let p: usize = CAB_HEADER_SIZE;
    p = writeFolder(buf, p, 160, 2, 0x0001, 0); // MSZIP
    p = writeFolder(buf, p, 220, 4, 0x1503, 0); // LZX (window 21 in high bits)
    expect_eq(p, coff_files);

    p = writeFile(buf, p, 1024, 0, 0, 0x5421, 0x6100, 0x0020, "README.TXT");
    p = writeFile(buf, p, 4096, 1024, 0, 0x5421, 0x6101, 0x0021, "bin\\app.exe");
    p = writeFile(buf, p, 512, 0, 1, 0x5422, 0x7000, 0x0082, "docs/guide.md");

    writeCabHeaderBase(buf, p as u32, coff_files as u32, 2, 3, 0, 0x1234, 0);
    const cab = buf[0..p];

    expect_eq(CAB_HEADER_SIZE, 36);
    expect_eq(CAB_FOLDER_SIZE, 8);
    expect_eq(isCab(cab), true);

    if (hdr := parseCabHeader(cab)) {
        expect_eq(hdr.cb_cabinet, p as u32);
        expect_eq(hdr.coff_files, coff_files as u32);
        expect_eq(hdr.version_major, 1);
        expect_eq(hdr.version_minor, 3);
        expect_eq(hdr.c_folders, 2);
        expect_eq(hdr.c_files, 3);
        expect_eq(hdr.flags, 0);
        expect_eq(hdr.set_id, 0x1234);
        expect_eq(hdr.i_cabinet, 0);
        expect_eq(hdr.folders_offset, 36);

        let fit = CabFolderIter.init(cab, hdr);
        if (f0 := fit.next()) {
            expect_eq(f0.coff_cab_start, 160);
            expect_eq(f0.c_cf_data, 2);
            expect_eq(f0.type_compress, 0x0001);
            expect_eq(f0.compression == CabCompression.mszip, true);
        } else {
            expect_eq(true, false);
        }
        if (f1 := fit.next()) {
            expect_eq(f1.coff_cab_start, 220);
            expect_eq(f1.c_cf_data, 4);
            expect_eq(f1.type_compress, 0x1503);
            expect_eq(f1.compression == CabCompression.lzx, true);
        } else {
            expect_eq(true, false);
        }
        expect_eq(fit.next() == null, true);
        expect_eq(fit.had_error, false);

        let files = CabFileIter.init(cab, hdr);
        let count: usize = 0;
        while (file := files.next()) {
            if (count == 0) {
                expect_eq(file.cb_file, 1024);
                expect_eq(file.uoff_folder_start, 0);
                expect_eq(file.i_folder, 0);
                expect_eq(sliceEq(file.name, "README.TXT"), true);
            } else if (count == 1) {
                expect_eq(file.cb_file, 4096);
                expect_eq(file.uoff_folder_start, 1024);
                expect_eq(sliceEq(file.name, "bin\\app.exe"), true);
            } else if (count == 2) {
                expect_eq(file.cb_file, 512);
                expect_eq(file.i_folder, 1);
                expect_eq(sliceEq(file.name, "docs/guide.md"), true);
            }
            count += 1;
        }
        expect_eq(count, 3);
        expect_eq(files.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("2. handles CAB_FLAG_RESERVE + CAB_FLAG_PREV + CAB_FLAG_NEXT offset calculation and cb_cf_folder stride", () => {
    let raw: [320]u8 = undefined;
    let buf = raw[0..320];
    zeroBytes(buf);

    let p: usize = CAB_HEADER_SIZE;
    // RESERVE block: cb_cf_header=6, cb_cf_folder=4, cb_cf_data=2
    writeU16LE(buf, p, 6);
    buf[p + 2] = 4;
    buf[p + 3] = 2;
    p += 4;
    // 6 bytes of ab_reserve
    p = writeBytes(buf, p, "RESV01");

    // PREV cabinet + disk strings
    p = writeCString(buf, p, "disk1.cab");
    p = writeCString(buf, p, "Disk 1");

    // NEXT cabinet + disk strings
    p = writeCString(buf, p, "disk3.cab");
    p = writeCString(buf, p, "Disk 3");

    const expected_folders_offset = p;

    // Two folders, each 8 + 4 (cb_cf_folder) = 12 bytes
    p = writeFolder(buf, p, 300, 1, 0x0000, 4); // NONE
    p = writeFolder(buf, p, 500, 3, 0x0A02, 4); // QUANTUM

    const coff_files = p;
    p = writeFile(buf, p, 777, 0, 1, 0x5000, 0x1000, 0x0000, "part2.dat");

    const flags = CAB_FLAG_RESERVE | CAB_FLAG_PREV | CAB_FLAG_NEXT;
    writeCabHeaderBase(buf, p as u32, coff_files as u32, 2, 1, flags, 0xBEEF, 1);
    const cab = buf[0..p];

    if (hdr := parseCabHeader(cab)) {
        expect_eq(hdr.cb_cf_header, 6);
        expect_eq(hdr.cb_cf_folder, 4);
        expect_eq(hdr.cb_cf_data, 2);
        expect_eq(hdr.folders_offset, expected_folders_offset);
        expect_eq(hdr.i_cabinet, 1);

        let fit = CabFolderIter.init(cab, hdr);
        if (f0 := fit.next()) {
            expect_eq(f0.coff_cab_start, 300);
            expect_eq(f0.compression == CabCompression.none, true);
        } else {
            expect_eq(true, false);
        }
        if (f1 := fit.next()) {
            expect_eq(f1.coff_cab_start, 500);
            expect_eq(f1.c_cf_data, 3);
            expect_eq(f1.compression == CabCompression.quantum, true);
        } else {
            expect_eq(true, false);
        }
        expect_eq(fit.next() == null, true);
        expect_eq(fit.had_error, false);

        if (file := findCabFile(cab, "part2.dat")) {
            expect_eq(file.cb_file, 777);
            expect_eq(file.i_folder, 1);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. findCabFile and CabFile attribute helpers (isUtf8Name, isReadOnly, isHidden)", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    zeroBytes(buf);

    let p: usize = CAB_HEADER_SIZE;
    p = writeFolder(buf, p, 200, 1, 1, 0);
    const coff_files = p;

    p = writeFile(buf, p, 100, 0, 0, 0, 0, CAB_ATTR_RDONLY, "readonly.ini");
    p = writeFile(buf, p, 200, 100, 0, 0, 0, CAB_ATTR_HIDDEN | CAB_ATTR_NAME_IS_UTF, "hidden_utf8.txt");
    p = writeFile(buf, p, 300, 300, 0, 0, 0, 0, "plain.bin");

    writeCabHeaderBase(buf, p as u32, coff_files as u32, 1, 3, 0, 1, 0);
    const cab = buf[0..p];

    if (ro := findCabFile(cab, "readonly.ini")) {
        expect_eq(ro.isReadOnly(), true);
        expect_eq(ro.isHidden(), false);
        expect_eq(ro.isUtf8Name(), false);
        expect_eq(ro.cb_file, 100);
    } else {
        expect_eq(true, false);
    }

    if (hu := findCabFile(cab, "hidden_utf8.txt")) {
        expect_eq(hu.isReadOnly(), false);
        expect_eq(hu.isHidden(), true);
        expect_eq(hu.isUtf8Name(), true);
        expect_eq(hu.cb_file, 200);
    } else {
        expect_eq(true, false);
    }

    if (pl := findCabFile(cab, "plain.bin")) {
        expect_eq(pl.isReadOnly(), false);
        expect_eq(pl.isHidden(), false);
        expect_eq(pl.isUtf8Name(), false);
    } else {
        expect_eq(true, false);
    }

    expect_eq(findCabFile(cab, "nonexistent.txt") == null, true);
});

test("4. CabCompression enum mapping covers all low-nibble methods and unknown values", () => {
    expect_eq(decodeCabCompression(0x0000) == CabCompression.none, true);
    expect_eq(decodeCabCompression(0x0001) == CabCompression.mszip, true);
    expect_eq(decodeCabCompression(0x1302) == CabCompression.quantum, true);
    expect_eq(decodeCabCompression(0x1503) == CabCompression.lzx, true);
    expect_eq(decodeCabCompression(0x0004) == CabCompression.unknown, true);
    expect_eq(decodeCabCompression(0xFFFF) == CabCompression.unknown, true);
});

test("5. rejects truncated header, bad MSCF magic, nonzero reserved1, bad version, and truncated extensions", () => {
    // 5a. Truncated (< 36 bytes)
    let tiny: [16]u8 = undefined;
    zeroBytes(tiny[0..16]);
    expect_eq(isCab(tiny[0..16]), false);
    expect_eq(parseCabHeader(tiny[0..16]) == null, true);

    // 5b. Valid 36B baseline
    let raw: [64]u8 = undefined;
    let buf = raw[0..64];
    zeroBytes(buf);
    writeCabHeaderBase(buf, 36, 36, 0, 0, 0, 0, 0);
    expect_eq(isCab(buf[0..36]), true);
    expect_eq(parseCabHeader(buf[0..36]) != null, true);

    // Bad magic
    buf[3] = 'X';
    expect_eq(isCab(buf[0..36]), false);
    expect_eq(parseCabHeader(buf[0..36]) == null, true);
    buf[3] = 'F';

    // Nonzero reserved1
    buf[4] = 1;
    expect_eq(isCab(buf[0..36]), false);
    buf[4] = 0;

    // Wrong version (major != 1 or minor != 3)
    buf[24] = 4;
    expect_eq(isCab(buf[0..36]), false);
    buf[24] = 3;
    buf[25] = 2;
    expect_eq(isCab(buf[0..36]), false);
    buf[25] = 1;

    // Truncated RESERVE header (flag set, but only 36 bytes total)
    writeU16LE(buf, 30, CAB_FLAG_RESERVE);
    expect_eq(parseCabHeader(buf[0..36]) == null, true);

    // RESERVE present with cb_cf_header=100 exceeding remaining buffer
    writeU16LE(buf, 36, 100);
    expect_eq(parseCabHeader(buf[0..44]) == null, true);

    // PREV flag with unterminated string
    writeU16LE(buf, 30, CAB_FLAG_PREV);
    buf[36] = 'a';
    buf[37] = 'b';
    buf[38] = 'c';
    expect_eq(parseCabHeader(buf[0..39]) == null, true);
});

test("6. defensive bounds for out-of-bounds coff_files, unterminated CFFILE name, and truncated folder/file", () => {
    let raw: [128]u8 = undefined;
    let buf = raw[0..128];
    zeroBytes(buf);

    // 6a. Out-of-bounds coff_files
    writeCabHeaderBase(buf, 64, 9999, 0, 1, 0, 0, 0);
    expect_eq(parseCabHeader(buf[0..64]) == null, true);

    // 6b. Unterminated CFFILE filename
    zeroBytes(buf);
    let p: usize = CAB_HEADER_SIZE;
    p = writeFolder(buf, p, 120, 1, 1, 0);
    const coff_files = p;
    writeU32LE(buf, p, 10);
    buf[p + 16] = 'A';
    buf[p + 17] = 'B';
    buf[p + 18] = 'C';
    const end_no_nul = p + 19; // deliberately no '\0' terminator
    writeCabHeaderBase(buf, end_no_nul as u32, coff_files as u32, 1, 1, 0, 0, 0);

    if (hdr := parseCabHeader(buf[0..end_no_nul])) {
        let it = CabFileIter.init(buf[0..end_no_nul], hdr);
        expect_eq(it.next() == null, true);
        expect_eq(it.had_error, true);
    } else {
        expect_eq(true, false);
    }

    // 6c. Truncated CFFOLDER (c_folders = 2, but buffer only has space for 1)
    zeroBytes(buf);
    writeCabHeaderBase(buf, 44, 44, 2, 0, 0, 0, 0);
    if (hdr2 := parseCabHeader(buf[0..44])) {
        let fit = CabFolderIter.init(buf[0..44], hdr2);
        expect_eq(fit.next() != null, true);
        expect_eq(fit.next() == null, true);
        expect_eq(fit.had_error, true);
    } else {
        expect_eq(true, false);
    }
});
