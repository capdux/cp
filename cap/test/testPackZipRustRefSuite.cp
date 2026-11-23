import { test, expect_eq } from "cap:test";
import {
    ZIP_LOCAL_HEADER_SIG,
    ZIP_CENTRAL_DIR_SIG,
    ZIP_EOCD_SIG,
    ZIP64_EOCD_SIG,
    ZIP64_EOCD_LOCATOR_SIG,
    ZIP_LOCAL_HEADER_SIZE,
    ZIP_CENTRAL_DIR_SIZE,
    ZIP_EOCD_SIZE,
    ZIP64_EOCD_MIN_SIZE,
    ZIP64_EOCD_LOCATOR_SIZE,
    ZIP_METHOD_STORE,
    ZIP_METHOD_DEFLATE,
    ZIP64_EXTRA_HEADER_ID,
    ZIP_FLAG_ENCRYPTED,
    ZIP_FLAG_DATA_DESCRIPTOR,
    ZIP_FLAG_UTF8,
    ZipError,
    ZipEndian,
    copyStoredScalar,
    copyStoredSimd,
    findEocdOffsetScalar,
    findEocdOffsetSimd,
    isSafeZipEntryPathScalar,
    isSafeZipEntryPath,
    sanitizeZipEntryPath,
    parseZip64ExtraField,
    ZipLocalHeader,
    ZipCentralHeader,
    ZipEocd,
    ZipReader,
    ZipCdEntry,
    ZipArchive,
} from "../../lib/pack/zip.cp";
import { Crc32 } from "../../lib/pack/checksum.cp";

const std = @import("std");

fn tryOpenArchive(buf: []const u8): ZipArchive? {
    return ZipArchive.open(buf) catch return null;
}

fn tryExtractWithLimit(ar: ZipArchive, ent: ZipCdEntry, limit: usize): ([]u8)? {
    return ar.extractEntryWithLimit(ent, limit) catch return null;
}

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Real ZIP reference vectors (Python zipfile + zip-rs/zip2 MIT test data)
// ============================================================================

const PY_REAL_ZIP: [541]u8 = [0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x8a, 0x77, 0x5d, 0x31, 0x5f, 0x68, 0xa5, 0x21, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2e, 0x74, 0x78, 0x74, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x66, 0x72, 0x6f, 0x6d, 0x20, 0x50, 0x79, 0x74, 0x68, 0x6f, 0x6e, 0x20, 0x7a, 0x69, 0x70, 0x66, 0x69, 0x6c, 0x65, 0x20, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x64, 0x21, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x00, 0x08, 0x00, 0xf4, 0x8a, 0x77, 0x5d, 0x01, 0xf4, 0xe1, 0x69, 0x1e, 0x00, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x73, 0x75, 0x62, 0x2f, 0x64, 0x65, 0x66, 0x6c, 0x61, 0x74, 0x65, 0x64, 0x2e, 0x74, 0x78, 0x74, 0x73, 0x74, 0x72, 0x76, 0x71, 0x1c, 0xc1, 0xd8, 0x23, 0x35, 0x27, 0x27, 0x5f, 0xc1, 0x25, 0x35, 0x2d, 0x27, 0xb1, 0x24, 0x35, 0x45, 0x21, 0x3c, 0xbf, 0x28, 0x27, 0x45, 0x11, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x08, 0x08, 0x00, 0xf4, 0x8a, 0x77, 0x5d, 0x16, 0x93, 0x5f, 0x5b, 0x15, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x75, 0x6e, 0x69, 0x63, 0x6f, 0x64, 0x65, 0x2f, 0xe4, 0xb8, 0xad, 0xe6, 0x96, 0x87, 0x2e, 0x74, 0x78, 0x74, 0x0b, 0x0d, 0x71, 0xd3, 0xb5, 0x50, 0x78, 0xb6, 0xb5, 0xfb, 0xc5, 0xfa, 0xa9, 0x4f, 0xdb, 0x5a, 0x9f, 0xae, 0xdb, 0x09, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x8a, 0x77, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x65, 0x6d, 0x70, 0x74, 0x79, 0x5f, 0x64, 0x69, 0x72, 0x2f, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x03, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x8a, 0x77, 0x5d, 0x31, 0x5f, 0x68, 0xa5, 0x21, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2e, 0x74, 0x78, 0x74, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x03, 0x14, 0x00, 0x00, 0x00, 0x08, 0x00, 0xf4, 0x8a, 0x77, 0x5d, 0x01, 0xf4, 0xe1, 0x69, 0x1e, 0x00, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x48, 0x00, 0x00, 0x00, 0x73, 0x75, 0x62, 0x2f, 0x64, 0x65, 0x66, 0x6c, 0x61, 0x74, 0x65, 0x64, 0x2e, 0x74, 0x78, 0x74, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x03, 0x14, 0x00, 0x00, 0x08, 0x08, 0x00, 0xf4, 0x8a, 0x77, 0x5d, 0x16, 0x93, 0x5f, 0x5b, 0x15, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x94, 0x00, 0x00, 0x00, 0x75, 0x6e, 0x69, 0x63, 0x6f, 0x64, 0x65, 0x2f, 0xe4, 0xb8, 0xad, 0xe6, 0x96, 0x87, 0x2e, 0x74, 0x78, 0x74, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x03, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x8a, 0x77, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0xfd, 0x41, 0xd9, 0x00, 0x00, 0x00, 0x65, 0x6d, 0x70, 0x74, 0x79, 0x5f, 0x64, 0x69, 0x72, 0x2f, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0xed, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x19, 0x00, 0x43, 0x50, 0x2d, 0x5a, 0x69, 0x70, 0x2d, 0x54, 0x65, 0x73, 0x74, 0x2d, 0x43, 0x6f, 0x6d, 0x6d, 0x65, 0x6e, 0x74, 0x2d, 0x32, 0x30, 0x32, 0x36, 0x21];

const ZIP2_DATA_DESCRIPTOR: [198]u8 = [0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x08, 0x00, 0x08, 0x00, 0x05, 0x56, 0x97, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x1c, 0x00, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2e, 0x74, 0x78, 0x74, 0x55, 0x54, 0x09, 0x00, 0x03, 0x5a, 0xf4, 0x27, 0x66, 0x5d, 0xf4, 0x27, 0x66, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0x4f, 0x27, 0x1e, 0x15, 0x04, 0x14, 0x00, 0x00, 0x00, 0xf3, 0x48, 0xcd, 0xc9, 0xc9, 0x57, 0x08, 0xcf, 0x2f, 0xca, 0x49, 0xe1, 0x02, 0x00, 0x50, 0x4b, 0x07, 0x08, 0xe3, 0xe5, 0x95, 0xb0, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x50, 0x4b, 0x01, 0x02, 0x1e, 0x03, 0x14, 0x00, 0x08, 0x00, 0x08, 0x00, 0x05, 0x56, 0x97, 0x58, 0xe3, 0xe5, 0x95, 0xb0, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x09, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa4, 0x81, 0x00, 0x00, 0x00, 0x00, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2e, 0x74, 0x78, 0x74, 0x55, 0x54, 0x05, 0x00, 0x03, 0x5a, 0xf4, 0x27, 0x66, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0x4f, 0x27, 0x1e, 0x15, 0x04, 0x14, 0x00, 0x00, 0x00, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00];

const ZIP2_INVALID_OFFSET2: [117]u8 = [0x50, 0x4b, 0x03, 0x04, 0xb2, 0x27, 0xff, 0xff, 0xff, 0xff, 0x50, 0x4b, 0x06, 0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xd8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x06, 0x00, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x50, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x29, 0x50, 0x4b, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x05, 0x00, 0x04, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x50, 0x29, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x05, 0x00, 0x50, 0x4b, 0x05, 0x3d, 0x25];

const ZIP2_COMMENT_GARBAGE: [46]u8 = [0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x73, 0x68, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x6d, 0x6d, 0x65, 0x6e, 0x74, 0x20, 0x62, 0x6c, 0x61, 0x20, 0x62, 0x6c, 0x61, 0x20, 0x62, 0x6c, 0x61];

/// 构造单个最小合法 Stored ZIP 归档（可自定义文件名、载荷与最长 65535B 注释）
fn buildMinimalZip(name: []const u8, payload: []const u8, comment_len: usize): []u8 {
    const lfh_total = ZIP_LOCAL_HEADER_SIZE + name.len + payload.len;
    const cd_total = ZIP_CENTRAL_DIR_SIZE + name.len;
    const total = lfh_total + cd_total + ZIP_EOCD_SIZE + comment_len;
    let buf = std.heap.page_allocator.alloc(u8, total) catch unreachable;
    @memset(buf, 0);

    const crc = Crc32.checksum(payload);
    * = ZipLocalHeader.encodeEntry(buf[0..lfh_total], name, payload);

    const chdr = ZipCentralHeader{
        compression_method: ZIP_METHOD_STORE,
        crc32: crc,
        compressed_size: payload.len as u32,
        uncompressed_size: payload.len as u32,
        name_len: name.len as u16,
        local_header_offset: 0,
    };
    * = chdr.write(buf[lfh_total .. lfh_total + ZIP_CENTRAL_DIR_SIZE]);
    copyStoredSimd(buf[lfh_total + ZIP_CENTRAL_DIR_SIZE .. lfh_total + cd_total], name);

    const eocd_off = lfh_total + cd_total;
    const eocd = ZipEocd{
        entries_on_disk: 1,
        total_entries: 1,
        central_dir_size: cd_total as u32,
        central_dir_offset: lfh_total as u32,
        comment_len: comment_len as u16,
    };
    * = eocd.write(buf[eocd_off .. eocd_off + ZIP_EOCD_SIZE]);
    if (comment_len > 0) {
        let i: usize = 0;
        while (i < comment_len) : (i += 1) {
            buf[eocd_off + ZIP_EOCD_SIZE + i] = ((i % 26) + 65) as u8;
        }
    }
    return buf;
}

/// 构造带 ZIP64 Extra Field (0x0001) 与 ZIP64 EOCD/Locator 的标准归档
fn buildZip64Archive(name: []const u8, payload: []const u8): []u8 {
    // Extra Field: tag(2) + size(2) + uncomp(8) + comp(8) + lfh_off(8) = 28 bytes
    const extra_len: usize = 28;
    const lfh_total = ZIP_LOCAL_HEADER_SIZE + name.len + payload.len;
    const cd_offset = lfh_total;
    const cd_size = ZIP_CENTRAL_DIR_SIZE + name.len + extra_len;
    const z64_eocd_off = cd_offset + cd_size;
    const z64_loc_off = z64_eocd_off + ZIP64_EOCD_MIN_SIZE;
    const eocd_off = z64_loc_off + ZIP64_EOCD_LOCATOR_SIZE;
    const total = eocd_off + ZIP_EOCD_SIZE;

    let buf = std.heap.page_allocator.alloc(u8, total) catch unreachable;
    @memset(buf, 0);

    const crc = Crc32.checksum(payload);
    * = ZipLocalHeader.encodeEntry(buf[0..lfh_total], name, payload);

    // Central Directory Header with 0xFFFFFFFF sentinels
    const chdr = ZipCentralHeader{
        version_made_by: 45,
        version_needed: 45,
        compression_method: ZIP_METHOD_STORE,
        crc32: crc,
        compressed_size: 0xFFFFFFFF,
        uncompressed_size: 0xFFFFFFFF,
        name_len: name.len as u16,
        extra_len: extra_len as u16,
        local_header_offset: 0xFFFFFFFF,
    };
    * = chdr.write(buf[cd_offset .. cd_offset + ZIP_CENTRAL_DIR_SIZE]);
    const name_pos = cd_offset + ZIP_CENTRAL_DIR_SIZE;
    copyStoredSimd(buf[name_pos .. name_pos + name.len], name);

    const ext_pos = name_pos + name.len;
    ZipEndian.writeU16LE(buf, ext_pos, ZIP64_EXTRA_HEADER_ID);
    ZipEndian.writeU16LE(buf, ext_pos + 2, 24);
    ZipEndian.writeU64LE(buf, ext_pos + 4, payload.len as u64);
    ZipEndian.writeU64LE(buf, ext_pos + 12, payload.len as u64);
    ZipEndian.writeU64LE(buf, ext_pos + 20, 0);

    // ZIP64 EOCD (56 bytes)
    ZipEndian.writeU32LE(buf, z64_eocd_off, ZIP64_EOCD_SIG);
    ZipEndian.writeU64LE(buf, z64_eocd_off + 4, 44);
    ZipEndian.writeU16LE(buf, z64_eocd_off + 12, 45);
    ZipEndian.writeU16LE(buf, z64_eocd_off + 14, 45);
    ZipEndian.writeU32LE(buf, z64_eocd_off + 16, 0);
    ZipEndian.writeU32LE(buf, z64_eocd_off + 20, 0);
    ZipEndian.writeU64LE(buf, z64_eocd_off + 24, 1);
    ZipEndian.writeU64LE(buf, z64_eocd_off + 32, 1);
    ZipEndian.writeU64LE(buf, z64_eocd_off + 40, cd_size as u64);
    ZipEndian.writeU64LE(buf, z64_eocd_off + 48, cd_offset as u64);

    // ZIP64 EOCD Locator (20 bytes)
    ZipEndian.writeU32LE(buf, z64_loc_off, ZIP64_EOCD_LOCATOR_SIG);
    ZipEndian.writeU32LE(buf, z64_loc_off + 4, 0);
    ZipEndian.writeU64LE(buf, z64_loc_off + 8, z64_eocd_off as u64);
    ZipEndian.writeU32LE(buf, z64_loc_off + 16, 1);

    // Standard EOCD with 0xFFFF / 0xFFFFFFFF markers
    const eocd = ZipEocd{
        disk_num: 0xFFFF,
        central_dir_disk: 0xFFFF,
        entries_on_disk: 0xFFFF,
        total_entries: 0xFFFF,
        central_dir_size: 0xFFFFFFFF,
        central_dir_offset: 0xFFFFFFFF,
        comment_len: 0,
    };
    * = eocd.write(buf[eocd_off .. eocd_off + ZIP_EOCD_SIZE]);
    return buf;
}

fn runTestRealVectors(): void! {
    const ar = ZipArchive.open(PY_REAL_ZIP[0..]) catch unreachable;
    expect_eq(ar.entryCount(), 4);
    expect_true(bytesEqual(ar.archiveComment(), "CP-Zip-Test-Comment-2026!"))?;
    expect_eq(ar.validateAllEntries() catch unreachable, 4);

    // 1a. Stored entry
    const e1 = (ar.findEntry("hello.txt") catch unreachable).?;
    const p1 = ar.extractEntry(e1) catch unreachable;
    defer std.heap.page_allocator.free(p1);
    expect_true(bytesEqual(p1, "Hello from Python zipfile Stored!"))?;

    // 1b. Deflated entry
    const e2 = (ar.findEntry("sub/deflated.txt") catch unreachable).?;
    const p2 = ar.extractEntry(e2) catch unreachable;
    defer std.heap.page_allocator.free(p2);
    expect_eq(p2.len, 277);
    expect_true(bytesEqual(p2[0..4], "ABCD"))?;
    expect_true(bytesEqual(p2[256..277], "Hello Deflated World!"))?;

    // 1c. UTF-8 entry (bit 11)
    const e3 = (ar.findEntry("unicode/\xe4\xb8\xad\xe6\x96\x87.txt") catch unreachable).?;
    expect_true(e3.is_utf8)?;
    const p3 = ar.extractEntry(e3) catch unreachable;
    defer std.heap.page_allocator.free(p3);
    expect_true(bytesEqual(p3, "UTF-8 \xe6\xb5\x8b\xe8\xaf\x95\xe5\x86\x85\xe5\xae\xb9"))?;

    // 1d. Directory entry
    const e4 = (ar.findEntry("empty_dir/") catch unreachable).?;
    expect_true(e4.isDirectory())?;

    // 1e. zip-rs/zip2 data_descriptor.zip (bit 3)
    const dd_ar = ZipArchive.open(ZIP2_DATA_DESCRIPTOR[0..]) catch unreachable;
    expect_eq(dd_ar.entryCount(), 1);
    const dd_e = (dd_ar.findEntry("hello.txt") catch unreachable).?;
    expect_true(dd_e.has_data_descriptor)?;
    const dd_p = dd_ar.extractEntry(dd_e) catch unreachable;
    defer std.heap.page_allocator.free(dd_p);
    expect_true(bytesEqual(dd_p, "Hello World\n"))?;

    // 1f. zip-rs/zip2 comment_garbage.zip & invalid_offset2.zip must be rejected cleanly
    let garbage_rejected = false;
    * = ZipArchive.open(ZIP2_COMMENT_GARBAGE[0..]) catch {
        garbage_rejected = true;
    };
    expect_true(garbage_rejected)?;

    let inv_rejected = false;
    const inv_ar_opt = tryOpenArchive(ZIP2_INVALID_OFFSET2[0..]);
    if (inv_ar_opt == null) {
        inv_rejected = true;
    } else {
        * = inv_ar_opt.?.validateAllEntries() catch {
            inv_rejected = true;
        };
    }
    expect_true(inv_rejected)?;

    // 1g. Maximum 65535-byte archive comment
    const max_zip = buildMinimalZip("max_comment.txt", "65535B-comment-ok", 65535);
    defer std.heap.page_allocator.free(max_zip);
    const max_ar = ZipArchive.open(max_zip) catch unreachable;
    expect_eq(max_ar.archiveComment().len, 65535);
    const me = (max_ar.findEntry("max_comment.txt") catch unreachable).?;
    const mp = max_ar.extractEntry(me) catch unreachable;
    defer std.heap.page_allocator.free(mp);
    expect_true(bytesEqual(mp, "65535B-comment-ok"))?;

    // 1h. Backward-compatible ZipReader & ZipEocd.findInBuffer
    const eocd_compat = ZipEocd.findInBuffer(max_zip).?;
    expect_eq(eocd_compat.comment_len, 65535);
    let zr = ZipReader.init(max_zip);
    const zv = zr.next().?;
    expect_true(zv.isStored())?;
    expect_true(zv.verifyCrc())?;
}

fn runTestZip64(): void! {
    const z64_buf = buildZip64Archive("big/data.bin", "ZIP64-Payload-Check-12345");
    defer std.heap.page_allocator.free(z64_buf);

    const ar = ZipArchive.open(z64_buf) catch unreachable;
    expect_true(ar.is_zip64)?;
    expect_eq(ar.entryCount(), 1);
    const e = (ar.findEntry("big/data.bin") catch unreachable).?;
    expect_true(e.is_zip64)?;
    expect_eq(e.uncompressed_size, 25);
    expect_eq(e.compressed_size, 25);
    expect_eq(e.local_header_offset, 0);

    const plain = ar.extractEntry(e) catch unreachable;
    defer std.heap.page_allocator.free(plain);
    expect_true(bytesEqual(plain, "ZIP64-Payload-Check-12345"))?;

    // Truncated ZIP64 extra field must return InvalidZip64Extra
    const bad_extra: [8]u8 = [0x01, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00];
    let got_z64_err = false;
    * = parseZip64ExtraField(bad_extra[0..], true, false, false, false) catch {
        got_z64_err = true;
    };
    expect_true(got_z64_err)?;
}

fn runTestSecurityBoundaries(): void! {
    // 3a. ZipSlip all variants rejected
    const unsafe_paths: [13][]const u8 = [
        "",
        "..",
        "../etc/passwd",
        "a/../../b",
        "a/..",
        "/etc/passwd",
        "/",
        "\\windows\\win.ini",
        "dir\\file.txt",
        "C:boot.ini",
        "C:/Windows/System32",
        "stream:ads",
        "bad\x00name.txt",
    ];
    let i: usize = 0;
    while (i < unsafe_paths.len) : (i += 1) {
        expect_eq(isSafeZipEntryPath(unsafe_paths[i]), false);
        expect_eq(sanitizeZipEntryPath(unsafe_paths[i]) == null, true);
    }

    const safe_paths: [6][]const u8 = [
        "hello.txt",
        ".gitignore",
        "dir/.hidden",
        "a..b/file.txt",
        "sub/dir/archive.tar.gz",
        "unicode/\xe4\xb8\xad\xe6\x96\x87.txt",
    ];
    i = 0;
    while (i < safe_paths.len) : (i += 1) {
        expect_eq(isSafeZipEntryPath(safe_paths[i]), true);
        expect_eq(sanitizeZipEntryPath(safe_paths[i]) != null, true);
    }

    // 3b. ZipArchive rejects ZipSlip entry on validateAllEntries & extractEntry
    const evil_zip = buildMinimalZip("../escape.txt", "pwn", 0);
    defer std.heap.page_allocator.free(evil_zip);
    const evil_ar = ZipArchive.open(evil_zip) catch unreachable;
    let caught_unsafe_val = false;
    * = evil_ar.validateAllEntries() catch |e| {
        if (e == ZipError.UnsafePath) caught_unsafe_val = true;
    };
    expect_true(caught_unsafe_val)?;

    const evil_entry = (evil_ar.findEntry("../escape.txt") catch unreachable).?;
    let caught_unsafe_ext = false;
    * = evil_ar.extractEntry(evil_entry) catch |e| {
        if (e == ZipError.UnsafePath) caught_unsafe_ext = true;
    };
    expect_true(caught_unsafe_ext)?;

    // 3c. Single-bit CRC32 tamper rejected
    let crc_zip = buildMinimalZip("data.bin", "0123456789ABCDEF", 0);
    defer std.heap.page_allocator.free(crc_zip);
    // Flip 1 bit in stored payload (starts at 30 + 8 = 38)
    crc_zip[38] ^= 0x01;
    const crc_ar = ZipArchive.open(crc_zip) catch unreachable;
    const crc_e = (crc_ar.findEntry("data.bin") catch unreachable).?;
    let caught_crc = false;
    * = crc_ar.extractEntry(crc_e) catch |e| {
        if (e == ZipError.CrcMismatch) caught_crc = true;
    };
    expect_true(caught_crc)?;

    // 3d. CD vs LFH HeaderMismatch (tamper LFH compression_method)
    let mm_zip = buildMinimalZip("data.bin", "0123456789ABCDEF", 0);
    defer std.heap.page_allocator.free(mm_zip);
    mm_zip[8] = 8; // LFH method=Deflate while CD method=Store
    const mm_ar = ZipArchive.open(mm_zip) catch unreachable;
    const mm_e = (mm_ar.findEntry("data.bin") catch unreachable).?;
    let caught_mm = false;
    * = mm_ar.extractEntry(mm_e) catch |e| {
        if (e == ZipError.HeaderMismatch) caught_mm = true;
    };
    expect_true(caught_mm)?;

    // 3e. OutputTooLarge on extractEntryWithLimit (Stored & Deflate)
    const lim_zip = buildMinimalZip("big.txt", "0123456789ABCDEF", 0);
    defer std.heap.page_allocator.free(lim_zip);
    const lim_ar = ZipArchive.open(lim_zip) catch unreachable;
    const lim_e = (lim_ar.findEntry("big.txt") catch unreachable).?;
    let caught_limit = false;
    * = lim_ar.extractEntryWithLimit(lim_e, 8) catch |e| {
        if (e == ZipError.OutputTooLarge) caught_limit = true;
    };
    expect_true(caught_limit)?;

    const py_ar = ZipArchive.open(PY_REAL_ZIP[0..]) catch unreachable;
    const def_e = (py_ar.findEntry("sub/deflated.txt") catch unreachable).?;
    let caught_def_limit = false;
    * = py_ar.extractEntryWithLimit(def_e, 64) catch |e| {
        if (e == ZipError.OutputTooLarge) caught_def_limit = true;
    };
    expect_true(caught_def_limit)?;

    // 3f. Multi-disk & forged cd_offset+cd_size overflow rejected
    let md_zip = buildMinimalZip("a.txt", "abc", 0);
    defer std.heap.page_allocator.free(md_zip);
    const eocd_pos = findEocdOffsetSimd(md_zip).?;
    ZipEndian.writeU16LE(md_zip, eocd_pos + 4, 1); // disk_num = 1
    let caught_md = false;
    * = ZipArchive.open(md_zip) catch |e| {
        if (e == ZipError.UnsupportedMultiDisk) caught_md = true;
    };
    expect_true(caught_md)?;

    ZipEndian.writeU16LE(md_zip, eocd_pos + 4, 0);
    ZipEndian.writeU32LE(md_zip, eocd_pos + 12, 0xFFFFFFFF); // forged cd_size (no zip64)
    let caught_cd_ovf = false;
    * = ZipArchive.open(md_zip) catch {
        caught_cd_ovf = true;
    };
    expect_true(caught_cd_ovf)?;
}

fn runTestSimdParity(): void! {
    // 4a. findEocdOffsetSimd vs findEocdOffsetScalar
    const sizes: [6]usize = [0, 7, 16, 33, 255, 1024];
    let si: usize = 0;
    while (si < sizes.len) : (si += 1) {
        let z = buildMinimalZip("simd/test.txt", "16-lane-SIMD-parity-check-payload", sizes[si]);
        defer std.heap.page_allocator.free(z);
        // Inject decoy 'P' (0x50) bytes in payload & comment
        z[35] = 0x50;
        const s_off = findEocdOffsetScalar(z);
        const v_off = findEocdOffsetSimd(z);
        expect_eq(s_off != null, true);
        expect_eq(s_off.?, v_off.?);
    }

    // 4b. copyStoredSimd vs copyStoredScalar across lengths 0..257
    let src: [257]u8 = [0; 257];
    let dst_scalar: [257]u8 = [0; 257];
    let dst_simd: [257]u8 = [0; 257];
    let k: usize = 0;
    while (k < 257) : (k += 1) {
        src[k] = ((k * 37 + 13) & 0xFF) as u8;
    }
    const copy_lens: [9]usize = [0, 1, 15, 16, 17, 31, 32, 64, 257];
    let ci: usize = 0;
    while (ci < copy_lens.len) : (ci += 1) {
        const n = copy_lens[ci];
        @memset(dst_scalar[0..n], 0);
        @memset(dst_simd[0..n], 0);
        copyStoredScalar(dst_scalar[0..n], src[0..n]);
        copyStoredSimd(dst_simd[0..n], src[0..n]);
        expect_true(bytesEqual(dst_scalar[0..n], dst_simd[0..n]))?;
    }

    // 4c. isSafeZipEntryPath (cap:simd) vs isSafeZipEntryPathScalar
    const paths: [10][]const u8 = [
        "a",
        "very/long/safe/relative/path/without/special/chars/file_name_0123456789.bin",
        "dir/sub/..hidden_ok",
        "dir/../evil",
        "long/path/before/backslash\\file",
        "long/path/before/colon:ads",
        "long/path/before/nul\x00byte",
        "/leading/slash/path/longer_than_16_bytes",
        "a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/..",
        "a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/ok.txt",
    ];
    let pi: usize = 0;
    while (pi < paths.len) : (pi += 1) {
        expect_eq(isSafeZipEntryPath(paths[pi]), isSafeZipEntryPathScalar(paths[pi]));
    }
}

fn runTestMalformedFuzz(): void! {
    let seed: u64 = 0x1E3779B97F4A7C15;
    let buf: [256]u8 = [0; 256];
    let case_idx: usize = 0;
    while (case_idx < 1300) : (case_idx += 1) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        const len: usize = ((seed % 255) as usize) + 1;

        let j: usize = 0;
        while (j < len) : (j += 1) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            buf[j] = (seed & 0xFF) as u8;
        }

        // Mutate some cases with real ZIP signatures to exercise deep parser branches
        if ((case_idx % 3) == 0 && len >= 30) {
            ZipEndian.writeU32LE(buf[0..], 0, ZIP_LOCAL_HEADER_SIG);
        }
        if ((case_idx % 5) == 0 && len >= 22) {
            ZipEndian.writeU32LE(buf[0..], len - 22, ZIP_EOCD_SIG);
            ZipEndian.writeU16LE(buf[0..], len - 2, 0);
        }
        if ((case_idx % 7) == 0 && len >= 46) {
            ZipEndian.writeU32LE(buf[0..], 0, ZIP_CENTRAL_DIR_SIG);
        }

        const slice = buf[0..len];
        const s_off = findEocdOffsetScalar(slice);
        const v_off = findEocdOffsetSimd(slice);
        expect_eq(s_off == null, v_off == null);
        if (s_off != null) {
            expect_eq(s_off.?, v_off.?);
        }

        const ar_opt = tryOpenArchive(slice);
        if (ar_opt != null) {
            const ar = ar_opt.?;
            * = ar.validateAllEntries() catch {};
            let it = ar.iterEntries();
            let steps: usize = 0;
            while (it.hasNext() && steps < 8) : (steps += 1) {
                const ent = it.next() catch break;
                const out_opt = tryExtractWithLimit(ar, ent, 4096);
                if (out_opt != null) {
                    std.heap.page_allocator.free(out_opt.?);
                }
            }
        }
    }
}

fn main() {
    test("1. Real Python zipfile + zip-rs/zip2 vectors (Stored/Deflate/UTF-8/DataDescriptor/65535B comment)", () => {
        runTestRealVectors()?;
    });
    test("2. ZIP64 Extra Field (0x0001) & ZIP64 EOCD/Locator parsing", () => {
        runTestZip64()?;
    });
    test("3. Security boundaries: ZipSlip, EOCD/CD overflow, HeaderMismatch, CRC32 & OutputLimit", () => {
        runTestSecurityBoundaries()?;
    });
    test("4. 16-lane SIMD vs Scalar 0-diff parity (EOCD scan, copyStored, isSafeZipEntryPath)", () => {
        runTestSimdParity()?;
    });
    test("5. 1,300 deterministic PRNG malformed ZIP fuzz cases (0 panic / 0 OOB)", () => {
        runTestMalformedFuzz()?;
    });
}
