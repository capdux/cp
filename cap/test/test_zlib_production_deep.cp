// Source: cp/cap/test/test_zlib_production_deep.cp (Deep Production Tests for cap:zlib & cap.zlib)

const std = @import("std");
import cap from "cap";
import {
    ZlibFormat,
    ZlibLevel,
    ZlibOptions,
    ZlibError,
    crc32,
    adler32,
    detectFormat,
    compress,
    decompress,
    gzipSync,
    gunzipSync,
    deflateSync,
    inflateSync,
    deflateRawSync,
    inflateRawSync,
} from "cap:zlib";
import { test, expect_eq } from "cap:test";

fn isExpectedErr(res: ([]u8) ! ZlibError, expected: ZlibError, alloc: std.mem.Allocator): bool {
    const buf = res catch |e| return e == expected;
    alloc.free(buf);
    return false;
}

test("zlib and raw deflate roundtrip and adler32 tamper detection") {
    const alloc = std.testing.allocator;
    const payload = "CP Zlib RFC 1950 & Raw Deflate RFC 1951 roundtrip payload! 1234567890 repeated repeated.";

    // Zlib default level roundtrip
    const z_comp = compress(alloc, payload, ZlibFormat.Zlib, ZlibOptions{}) ?;
    defer alloc.free(z_comp);
    const z_dec = decompress(alloc, z_comp, ZlibFormat.Zlib, ZlibOptions{}) ?;
    defer alloc.free(z_dec);
    expect_eq(std.mem.eql(u8, z_dec, payload), true);

    // Raw Deflate stored (level None) and sync wrappers
    const raw_none = deflateRawSync(alloc, payload, ZlibOptions{ .level = ZlibLevel.None }) ?;
    defer alloc.free(raw_none);
    const raw_dec = inflateRawSync(alloc, raw_none, ZlibOptions{}) ?;
    defer alloc.free(raw_dec);
    expect_eq(std.mem.eql(u8, raw_dec, payload), true);

    // deflateSync / inflateSync roundtrip
    const z_sync = deflateSync(alloc, payload, ZlibOptions{ .level = ZlibLevel.Best }) ?;
    defer alloc.free(z_sync);
    const z_sync_dec = inflateSync(alloc, z_sync, ZlibOptions{}) ?;
    defer alloc.free(z_sync_dec);
    expect_eq(std.mem.eql(u8, z_sync_dec, payload), true);

    // Tamper Adler-32 trailer byte -> must return ChecksumMismatch
    let tampered = alloc.dupe(u8, z_comp) ?;
    defer alloc.free(tampered);
    tampered[tampered.len - 1] ^= 0xff;
    expect_eq(isExpectedErr(inflateSync(alloc, tampered, ZlibOptions{}), ZlibError.ChecksumMismatch, alloc), true);
}

test("gzip rfc1952 metadata fhcrc multi-member and crc32 isize tamper") {
    const alloc = std.testing.allocator;
    const part1 = "First Gzip Member with FNAME/FCOMMENT/FHCRC! ";
    const part2 = "Second Concatenated Gzip Member!";

    const opts1 = ZlibOptions{
        .gzip_filename = "part1.txt",
        .gzip_comment = "first member",
        .gzip_mtime = 1700000000,
        .gzip_os = 3,
        .gzip_fhcrc = true,
    };
    const opts2 = ZlibOptions{
        .level = ZlibLevel.None,
        .gzip_filename = "part2.txt",
        .gzip_fhcrc = true,
    };

    const gz1 = gzipSync(alloc, part1, opts1) ?;
    defer alloc.free(gz1);
    const gz2 = gzipSync(alloc, part2, opts2) ?;
    defer alloc.free(gz2);

    // Concatenate two Gzip members
    let combined = std.ArrayList(u8).empty;
    defer combined.deinit(alloc);
    combined.appendSlice(alloc, gz1) ?;
    combined.appendSlice(alloc, gz2) ?;

    const merged = gunzipSync(alloc, combined.items, ZlibOptions{}) ?;
    defer alloc.free(merged);
    expect_eq(std.mem.eql(u8, merged, "First Gzip Member with FNAME/FCOMMENT/FHCRC! Second Concatenated Gzip Member!"), true);

    // Tamper CRC-32 in trailer (bytes len-8..len-4) -> ChecksumMismatch
    let bad_crc = alloc.dupe(u8, gz1) ?;
    defer alloc.free(bad_crc);
    bad_crc[bad_crc.len - 6] ^= 0x01;
    expect_eq(isExpectedErr(gunzipSync(alloc, bad_crc, ZlibOptions{}), ZlibError.ChecksumMismatch, alloc), true);

    // Tamper ISIZE in trailer (bytes len-4..len) -> ChecksumMismatch
    let bad_isize = alloc.dupe(u8, gz1) ?;
    defer alloc.free(bad_isize);
    bad_isize[bad_isize.len - 1] ^= 0x01;
    expect_eq(isExpectedErr(gunzipSync(alloc, bad_isize, ZlibOptions{}), ZlibError.ChecksumMismatch, alloc), true);

    // Tamper FHCRC (right after header + FNAME + FCOMMENT) -> ChecksumMismatch
    // Header(10) + "part1.txt\0"(10) + "first member\0"(13) = 33; FHCRC is at 33..35
    let bad_fhcrc = alloc.dupe(u8, gz1) ?;
    defer alloc.free(bad_fhcrc);
    bad_fhcrc[33] ^= 0x42;
    expect_eq(isExpectedErr(gunzipSync(alloc, bad_fhcrc, ZlibOptions{}), ZlibError.ChecksumMismatch, alloc), true);
}

test("autodetect format for gzip and zlib") {
    const alloc = std.testing.allocator;
    const msg = "AutoDetect format verification payload for Gzip and Zlib!";

    const gz = compress(alloc, msg, ZlibFormat.Gzip, ZlibOptions{}) ?;
    defer alloc.free(gz);
    const zl = compress(alloc, msg, ZlibFormat.Zlib, ZlibOptions{}) ?;
    defer alloc.free(zl);

    const fmt_gz = detectFormat(gz) ?;
    const fmt_zl = detectFormat(zl) ?;
    expect_eq(fmt_gz == ZlibFormat.Gzip, true);
    expect_eq(fmt_zl == ZlibFormat.Zlib, true);

    const dec_gz = decompress(alloc, gz, ZlibFormat.AutoDetect, ZlibOptions{}) ?;
    defer alloc.free(dec_gz);
    const dec_zl = decompress(alloc, zl, ZlibFormat.AutoDetect, ZlibOptions{}) ?;
    defer alloc.free(dec_zl);

    expect_eq(std.mem.eql(u8, dec_gz, msg), true);
    expect_eq(std.mem.eql(u8, dec_zl, msg), true);
}

test("anti-bomb max_output_bytes and malformed header rejection") {
    const alloc = std.testing.allocator;

    // 4096-byte compressible buffer
    let big: [4096]u8 = ['A'; 4096];
    const gz = gzipSync(alloc, big[0..], ZlibOptions{}) ?;
    defer alloc.free(gz);
    const zl = deflateSync(alloc, big[0..], ZlibOptions{}) ?;
    defer alloc.free(zl);

    const tight_opts = ZlibOptions{ .max_output_bytes = 512 };
    expect_eq(isExpectedErr(gunzipSync(alloc, gz, tight_opts), ZlibError.OutputTooLarge, alloc), true);
    expect_eq(isExpectedErr(inflateSync(alloc, zl, tight_opts), ZlibError.OutputTooLarge, alloc), true);

    // Malformed Gzip CM byte (0x09 instead of 0x08)
    const bad_gz_hdr: [10]u8 = [ 0x1f, 0x8b, 0x09, 0x00, 0, 0, 0, 0, 0, 0xff ];
    expect_eq(isExpectedErr(gunzipSync(alloc, bad_gz_hdr[0..], ZlibOptions{}), ZlibError.InvalidHeader, alloc), true);

    // Malformed Zlib FCHECK
    const bad_zl_hdr: [8]u8 = [ 0x78, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01 ];
    expect_eq(isExpectedErr(inflateSync(alloc, bad_zl_hdr[0..], ZlibOptions{}), ZlibError.InvalidHeader, alloc), true);
}

test("cap.zlib dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;
    const text = "cap.zlib dynamic mount & checksum test";

    const c32_a = crc32(text);
    const c32_b = cap.zlib.crc32(text);
    expect_eq(c32_a, c32_b);

    const a32_a = adler32(text);
    const a32_b = cap.zlib.adler32(text);
    expect_eq(a32_a, a32_b);

    const gz = cap.zlib.gzipSync(alloc, text, ZlibOptions{}) ?;
    defer alloc.free(gz);
    const out = cap.zlib.gunzipSync(alloc, gz, ZlibOptions{}) ?;
    defer alloc.free(out);
    expect_eq(std.mem.eql(u8, out, text), true);
}
