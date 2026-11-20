// Source: cp/cap/test/test_libarchive_production_deep.cp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ArchiveFormat,
    ArchiveError,
    ArchiveEntry,
    ArchiveLimits,
    detectArchiveFormat,
    validateArchivePath,
    freeEntries,
    encodeAr,
    decodeAr,
    encodeCpioNewc,
    decodeCpioNewc,
    encodeUstarTar,
    decodeUstarTar,
} from "cap:libarchive";
import cap from "cap";

test("libarchive magic detection and truncated/malformed header rejection") {
    expect_eq(detectArchiveFormat("!<arch>\nrest"), ArchiveFormat.Ar);
    expect_eq(detectArchiveFormat("070701rest"), ArchiveFormat.CpioNewc);
    expect_eq(detectArchiveFormat("070702rest"), ArchiveFormat.CpioCrc);
    expect_eq(detectArchiveFormat("PK\x03\x04rest"), ArchiveFormat.Zip);
    expect_eq(detectArchiveFormat("\x1f\x8brest"), ArchiveFormat.Gzip);
    expect_eq(detectArchiveFormat("\x28\xb5\x2f\xfdrest"), ArchiveFormat.Zstd);
    expect_eq(detectArchiveFormat("unknown"), ArchiveFormat.Unknown);

    let tar_hdr: [512]u8 = undefined;
    @memset(&tar_hdr, 0);
    @memcpy(tar_hdr[257..262], "ustar");
    expect_eq(detectArchiveFormat(&tar_hdr), ArchiveFormat.Tar);

    const alloc = std.testing.allocator;
    const limits = ArchiveLimits{};

    // Truncated & invalid magic checks
    let ar_trunc_ok = false;
    * = decodeAr(alloc, "!<ar", limits) catch |e| {
        if (e == ArchiveError.TruncatedHeader) {
            ar_trunc_ok = true;
        }
    };
    expect_eq(ar_trunc_ok, true);

    let ar_magic_ok = false;
    * = decodeAr(alloc, "NOT_AR_ARCHIVE!!", limits) catch |e| {
        if (e == ArchiveError.InvalidMagic) {
            ar_magic_ok = true;
        }
    };
    expect_eq(ar_magic_ok, true);

    let cpio_magic_ok = false;
    * = decodeCpioNewc(alloc, "999999" ++ ("0" ** 104), limits) catch |e| {
        if (e == ArchiveError.InvalidMagic) {
            cpio_magic_ok = true;
        }
    };
    expect_eq(cpio_magic_ok, true);
}

test("libarchive unix ar encode/decode roundtrip with odd/even padding and GNU longnames") {
    const alloc = std.testing.allocator;
    const entries: [3]ArchiveEntry = [
        ArchiveEntry{ .name = "odd.txt", .data = "abc", .mode = 0o100644, .uid = 1000, .gid = 1000, .mtime = 1700000000 },
        ArchiveEntry{ .name = "even.bin", .data = "1234", .mode = 0o100755, .uid = 0, .gid = 0, .mtime = 1700000001 },
        ArchiveEntry{ .name = "very_long_member_filename_exceeding_15_bytes.o", .data = "payload_long", .mode = 0o100644 },
    ];

    const blob = try encodeAr(alloc, &entries);
    defer alloc.free(blob);

    expect_eq(detectArchiveFormat(blob), ArchiveFormat.Ar);

    const decoded = try decodeAr(alloc, blob, ArchiveLimits{});
    defer freeEntries(alloc, decoded);

    expect_eq(decoded.len, 3);
    expect_eq(std.mem.eql(u8, decoded[0].name, "odd.txt"), true);
    expect_eq(std.mem.eql(u8, decoded[0].data, "abc"), true);
    expect_eq(decoded[0].uid, 1000);
    expect_eq(std.mem.eql(u8, decoded[1].name, "even.bin"), true);
    expect_eq(std.mem.eql(u8, decoded[1].data, "1234"), true);
    expect_eq(std.mem.eql(u8, decoded[2].name, "very_long_member_filename_exceeding_15_bytes.o"), true);
    expect_eq(std.mem.eql(u8, decoded[2].data, "payload_long"), true);
}

test("libarchive svr4 cpio newc encode/decode roundtrip with dirs, modes, 4-byte align, TRAILER") {
    const alloc = std.testing.allocator;
    const entries: [3]ArchiveEntry = [
        ArchiveEntry{ .name = "etc", .data = "", .mode = 0o755, .kind = .Directory },
        ArchiveEntry{ .name = "etc/conf", .data = "k=v\n", .mode = 0o644, .uid = 10, .gid = 20, .mtime = 12345 },
        ArchiveEntry{ .name = "bin/app", .data = "XYZ", .mode = 0o755, .kind = .File },
    ];

    const blob = try encodeCpioNewc(alloc, &entries);
    defer alloc.free(blob);

    expect_eq(detectArchiveFormat(blob), ArchiveFormat.CpioNewc);
    expect_eq(std.mem.indexOf(u8, blob, "TRAILER!!!") != null, true);

    const decoded = try decodeCpioNewc(alloc, blob, ArchiveLimits{});
    defer freeEntries(alloc, decoded);

    expect_eq(decoded.len, 3);
    expect_eq(std.mem.eql(u8, decoded[0].name, "etc"), true);
    expect_eq(decoded[0].kind == .Directory, true);
    expect_eq(std.mem.eql(u8, decoded[1].name, "etc/conf"), true);
    expect_eq(std.mem.eql(u8, decoded[1].data, "k=v\n"), true);
    expect_eq(decoded[1].uid, 10);
    expect_eq(decoded[1].gid, 20);
    expect_eq(std.mem.eql(u8, decoded[2].name, "bin/app"), true);
    expect_eq(std.mem.eql(u8, decoded[2].data, "XYZ"), true);
}

test("libarchive path traversal, null-byte, bomb limits, and ustar tar guard") {
    const alloc = std.testing.allocator;

    let trav_err = false;
    validateArchivePath("../etc/passwd", false) catch |e| {
        if (e == ArchiveError.PathTraversal) trav_err = true;
    };
    expect_eq(trav_err, true);

    let abs_err = false;
    validateArchivePath("/root/.ssh", false) catch |e| {
        if (e == ArchiveError.PathTraversal) abs_err = true;
    };
    expect_eq(abs_err, true);

    let nul_err = false;
    validateArchivePath("safe\x00evil", false) catch |e| {
        if (e == ArchiveError.PathTraversal) nul_err = true;
    };
    expect_eq(nul_err, true);

    // Bomb limits: max_entries & max_total_bytes on USTAR Tar
    const entries: [2]ArchiveEntry = [
        ArchiveEntry{ .name = "a.txt", .data = "123456" },
        ArchiveEntry{ .name = "b.txt", .data = "abcdef" },
    ];
    const tar_blob = try encodeUstarTar(alloc, &entries);
    defer alloc.free(tar_blob);
    expect_eq(detectArchiveFormat(tar_blob), ArchiveFormat.Tar);

    let too_many = false;
    * = decodeUstarTar(alloc, tar_blob, ArchiveLimits{ .max_entries = 1 }) catch |e| {
        if (e == ArchiveError.TooManyEntries) too_many = true;
    };
    expect_eq(too_many, true);

    let too_big = false;
    * = decodeUstarTar(alloc, tar_blob, ArchiveLimits{ .max_total_bytes = 8 }) catch |e| {
        if (e == ArchiveError.SizeLimitExceeded) too_big = true;
    };
    expect_eq(too_big, true);
}

test("libarchive cap.libarchive dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;
    const la = cap.libarchive;

    const entries: [1]la.ArchiveEntry = [
        la.ArchiveEntry{ .name = "mount.txt", .data = "cap.libarchive works!" },
    ];
    const tar_blob = try la.encodeUstarTar(alloc, &entries);
    defer alloc.free(tar_blob);

    const decoded = try la.decodeUstarTar(alloc, tar_blob, la.ArchiveLimits{});
    defer la.freeEntries(alloc, decoded);

    expect_eq(decoded.len, 1);
    expect_eq(std.mem.eql(u8, decoded[0].name, "mount.txt"), true);
    expect_eq(std.mem.eql(u8, decoded[0].data, "cap.libarchive works!"), true);
}
