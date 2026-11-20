// Source: cp/cap/test/test_zip_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:zip & cap.zip

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ZipError,
    CompressionMethod,
    ZipEntry,
    ZipArchive,
    ZipBuilder,
    readArchive,
    readArchiveWithLimits,
} from "cap:zip";

fn emptyArchive(): ZipArchive {
    let empty: [0]ZipEntry = [];
    return ZipArchive{ entries: &empty };
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    let builder = ZipBuilder.init(alloc);
    defer builder.deinit();

    builder.addStored("readme.txt", "Hello PKZIP Stored!")?;
    builder.addDeflated("nested/dir/data.txt", "Deflated payload inside nested directory: 1234567890 1234567890!")?;
    builder.addStored("empty/zero.bin", "")?;
    builder.addDeflated("empty/zero_deflated.bin", "")?;

    const zip_bytes = builder.finish(alloc)?;
    defer alloc.free(zip_bytes);

    // Verify PKZIP magic signatures exist
    expect_eq(zip_bytes[0], 0x50 as u8);
    expect_eq(zip_bytes[1], 0x4b as u8);
    expect_eq(zip_bytes[2], 0x03 as u8);
    expect_eq(zip_bytes[3], 0x04 as u8);

    let archive = readArchive(alloc, zip_bytes)?;
    defer archive.deinit(alloc);

    expect_eq(archive.entries.len, 4 as usize);

    const e1 = archive.find("readme.txt") orelse unreachable;
    expect_eq(e1.compression == CompressionMethod.Stored, true);
    expect_eq(std.mem.eql(u8, e1.data, "Hello PKZIP Stored!"), true);
    expect_eq(e1.uncompressed_size, 19 as u32);
    expect_eq(e1.compressed_size, 19 as u32);

    const e2 = archive.find("nested/dir/data.txt") orelse unreachable;
    expect_eq(e2.compression == CompressionMethod.Deflated, true);
    expect_eq(std.mem.eql(u8, e2.data, "Deflated payload inside nested directory: 1234567890 1234567890!"), true);

    const e3 = archive.find("empty/zero.bin") orelse unreachable;
    expect_eq(e3.data.len, 0 as usize);
    expect_eq(e3.uncompressed_size, 0 as u32);

    const e4 = archive.find("empty/zero_deflated.bin") orelse unreachable;
    expect_eq(e4.data.len, 0 as usize);
    expect_eq(e4.uncompressed_size, 0 as u32);

    expect_eq(archive.find("missing/file.txt") == null, true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    const pattern = "PKZIP_DEFLATE_REPETITION_BENCHMARK_LINE_0123456789_ABCDEF_\n";
    const repeat_count: usize = 120;
    const total_len = pattern.len * repeat_count;

    let raw_buf = alloc.alloc(u8, total_len) catch unreachable;
    defer alloc.free(raw_buf);

    let r: usize = 0;
    while (r < repeat_count) : (r += 1) {
        const start = r * pattern.len;
        @memcpy(raw_buf[start..(start + pattern.len)], pattern);
    }

    let b_stored = ZipBuilder.init(alloc);
    defer b_stored.deinit();
    b_stored.addStored("large.txt", raw_buf)?;
    const stored_zip = b_stored.finish(alloc)?;
    defer alloc.free(stored_zip);

    let b_deflated = ZipBuilder.init(alloc);
    defer b_deflated.deinit();
    b_deflated.addDeflated("large.txt", raw_buf)?;
    const deflated_zip = b_deflated.finish(alloc)?;
    defer alloc.free(deflated_zip);

    // Deflated archive must be significantly smaller than Stored (< 25% of stored size)
    expect_eq(deflated_zip.len * 4 < stored_zip.len, true);

    let archive = readArchive(alloc, deflated_zip)?;
    defer archive.deinit(alloc);
    const entry = archive.find("large.txt") orelse unreachable;
    expect_eq(entry.compressed_size * 4 < entry.uncompressed_size, true);
    expect_eq(std.mem.eql(u8, entry.data, raw_buf), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    let builder = ZipBuilder.init(alloc);
    defer builder.deinit();
    builder.addStored("check.txt", "IntegrityCheckPayload1234567890")?;
    const zip_bytes = builder.finish(alloc)?;
    defer alloc.free(zip_bytes);

    // 1. CRC-32 bit-flip tampering in stored payload (LFH is 30 + 9 = 39 bytes)
    let tampered = alloc.dupe(u8, zip_bytes) catch unreachable;
    defer alloc.free(tampered);
    tampered[40] ^= 0x01;

    let got_crc_err = false;
    * = readArchive(alloc, tampered) catch e => errCrc: {
        got_crc_err = (e == ZipError.ChecksumMismatch);
        break :errCrc emptyArchive();
    };
    expect_eq(got_crc_err, true);

    // 2. Truncated archive guard
    let got_trunc_err = false;
    * = readArchive(alloc, zip_bytes[0..(zip_bytes.len - 10)]) catch e => errTrunc: {
        got_trunc_err = (e == ZipError.TruncatedArchive);
        break :errTrunc emptyArchive();
    };
    expect_eq(got_trunc_err, true);

    // 3. Invalid magic / non-zip bytes guard
    const garbage = "NOT_A_VALID_PKZIP_ARCHIVE_AT_ALL_0000";
    let got_invalid_err = false;
    * = readArchive(alloc, garbage) catch e => errInv: {
        got_invalid_err = (e == ZipError.InvalidArchive);
        break :errInv emptyArchive();
    };
    expect_eq(got_invalid_err, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. Path traversal: "../../etc/passwd"
    let b_trav1 = ZipBuilder.init(alloc);
    defer b_trav1.deinit();
    b_trav1.addStored("../../etc/passwd", "root:x:0:0:")?;
    const z_trav1 = b_trav1.finish(alloc)?;
    defer alloc.free(z_trav1);

    let got_trav1 = false;
    * = readArchive(alloc, z_trav1) catch e => errT1: {
        got_trav1 = (e == ZipError.UnsafePathTraversal);
        break :errT1 emptyArchive();
    };
    expect_eq(got_trav1, true);

    // 2. Absolute path: "/abs/path"
    let b_trav2 = ZipBuilder.init(alloc);
    defer b_trav2.deinit();
    b_trav2.addStored("/abs/path", "secret")?;
    const z_trav2 = b_trav2.finish(alloc)?;
    defer alloc.free(z_trav2);

    let got_trav2 = false;
    * = readArchive(alloc, z_trav2) catch e => errT2: {
        got_trav2 = (e == ZipError.UnsafePathTraversal);
        break :errT2 emptyArchive();
    };
    expect_eq(got_trav2, true);

    // 3. Backslash path: "dir\\evil.txt"
    let b_trav3 = ZipBuilder.init(alloc);
    defer b_trav3.deinit();
    b_trav3.addStored("dir\\evil.txt", "secret")?;
    const z_trav3 = b_trav3.finish(alloc)?;
    defer alloc.free(z_trav3);

    let got_trav3 = false;
    * = readArchive(alloc, z_trav3) catch e => errT3: {
        got_trav3 = (e == ZipError.UnsafePathTraversal);
        break :errT3 emptyArchive();
    };
    expect_eq(got_trav3, true);

    // 4. TooManyEntries guard
    let b_many = ZipBuilder.init(alloc);
    defer b_many.deinit();
    b_many.addStored("a.txt", "1")?;
    b_many.addStored("b.txt", "2")?;
    b_many.addStored("c.txt", "3")?;
    const z_many = b_many.finish(alloc)?;
    defer alloc.free(z_many);

    let got_too_many = false;
    * = readArchiveWithLimits(alloc, z_many, 2, 1024) catch e => errMany: {
        got_too_many = (e == ZipError.TooManyEntries);
        break :errMany emptyArchive();
    };
    expect_eq(got_too_many, true);

    // 5. EntryTooLarge anti-Zip-Bomb guard
    let b_bomb = ZipBuilder.init(alloc);
    defer b_bomb.deinit();
    const bomb_payload = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    b_bomb.addDeflated("bomb.txt", bomb_payload)?;
    const z_bomb = b_bomb.finish(alloc)?;
    defer alloc.free(z_bomb);

    let got_too_large = false;
    * = readArchiveWithLimits(alloc, z_bomb, 16, 32) catch e => errLarge: {
        got_too_large = (e == ZipError.EntryTooLarge);
        break :errLarge emptyArchive();
    };
    expect_eq(got_too_large, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    let builder = cap.zip.ZipBuilder.init(alloc);
    defer builder.deinit();

    builder.addStored("a.txt", "stored via cap.zip")?;
    builder.addDeflated("b/c.txt", "deflated via cap.zip -- repeated repeated repeated!")?;

    const zip_bytes = builder.finish(alloc)?;
    defer alloc.free(zip_bytes);

    let archive = cap.zip.readArchiveWithLimits(alloc, zip_bytes, 16, 4096)?;
    defer archive.deinit(alloc);

    expect_eq(archive.entries.len, 2 as usize);
    const ea = archive.find("a.txt") orelse unreachable;
    expect_eq(ea.compression == cap.zip.CompressionMethod.Stored, true);
    expect_eq(std.mem.eql(u8, ea.data, "stored via cap.zip"), true);

    const eb = archive.find("b/c.txt") orelse unreachable;
    expect_eq(eb.compression == cap.zip.CompressionMethod.Deflated, true);
    expect_eq(std.mem.eql(u8, eb.data, "deflated via cap.zip -- repeated repeated repeated!"), true);
}

test("1. ZipBuilder mixed Stored + Deflated + empty files + nested dirs and readArchive roundtrip", () => {
    runTest1() catch unreachable;
});

test("2. High-repetition text Deflated archive size significantly smaller than Stored", () => {
    runTest2() catch unreachable;
});

test("3. CRC-32 bit-flip ChecksumMismatch, TruncatedArchive, and InvalidArchive guards", () => {
    runTest3() catch unreachable;
});

test("4. Path traversal (.., leading /, \\) UnsafePathTraversal and TooManyEntries / EntryTooLarge anti-bomb guards", () => {
    runTest4() catch unreachable;
});

test("5. import cap from \"cap\"; cap.zip dynamic mount and std.testing.allocator zero leak", () => {
    runTest5() catch unreachable;
});
