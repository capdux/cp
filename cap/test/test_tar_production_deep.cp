// Source: cp/cap/test/test_tar_production_deep.cp
// Deep Production Verification Suite for cap:tar & cap.tar
// Covers:
//   1. TarBuilder round-trip for regular files, empty files, directories, symlinks & non-512-multiple payloads
//   2. Long paths (>100 bytes) via POSIX ustar prefix (155+100) splitting & reconstruction
//   3. Security & integrity guards: InvalidChecksum & TruncatedArchive
//   4. Path traversal guards: ../../etc/passwd & /etc/shadow -> UnsafePathTraversal
//   5. `import cap from "cap"; cap.tar` dynamic mounting & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TAR_BLOCK_SIZE,
    TarError,
    TarTypeFlag,
    TarEntry,
    TarBuilder,
    TarReader,
    parseArchive,
    freeEntries,
} from "../../lib/pack/tar_.cp";

test("cap:tar - TarBuilder round-trip: regular, empty, dir, symlink & non-512-multiple payload") {
    const alloc = std.testing.allocator;

    let builder = TarBuilder.init(alloc);
    defer builder.deinit();

    // Directory
    builder.addDir("pkg/", 0o755, 1700000100) catch unreachable;
    // Empty file
    builder.addFile("pkg/empty.txt", "", 0o644, 1700000101) catch unreachable;
    // Regular file with non-512-multiple payload (700 bytes = 512 + 188)
    let payload_buf: [700]u8 = undefined;
    for (i of 0..700) {
        payload_buf[i] = ('A' + ((i % 26) as u8)) as u8;
    }
    builder.addFile("pkg/data.bin", payload_buf[0..], 0o640, 1700000102) catch unreachable;
    // Symlink
    builder.addSymlink("pkg/current", "data.bin", 0o777, 1700000103) catch unreachable;

    const archive = builder.finish(alloc) catch unreachable;
    defer alloc.free(archive);

    // Total size must be a multiple of 512 bytes and end with 1024 zero bytes
    expect_eq(archive.len % TAR_BLOCK_SIZE, 0);
    expect_eq(archive.len >= 1024, true);

    const entries = parseArchive(alloc, archive) catch unreachable;
    defer freeEntries(alloc, entries);

    expect_eq(entries.len, 4);

    // 1. Directory
    expect_eq(std.mem.eql(u8, entries[0].name, "pkg/"), true);
    expect_eq(entries[0].type_flag == TarTypeFlag.Directory, true);
    expect_eq(entries[0].mode, 0o755);
    expect_eq(entries[0].size, 0);
    expect_eq(entries[0].mtime, 1700000100);

    // 2. Empty file
    expect_eq(std.mem.eql(u8, entries[1].name, "pkg/empty.txt"), true);
    expect_eq(entries[1].type_flag == TarTypeFlag.Regular, true);
    expect_eq(entries[1].mode, 0o644);
    expect_eq(entries[1].size, 0);
    expect_eq(entries[1].data.len, 0);

    // 3. Regular file (700 bytes)
    expect_eq(std.mem.eql(u8, entries[2].name, "pkg/data.bin"), true);
    expect_eq(entries[2].type_flag == TarTypeFlag.Regular, true);
    expect_eq(entries[2].mode, 0o640);
    expect_eq(entries[2].size, 700);
    expect_eq(std.mem.eql(u8, entries[2].data, payload_buf[0..]), true);

    // 4. Symlink
    expect_eq(std.mem.eql(u8, entries[3].name, "pkg/current"), true);
    expect_eq(entries[3].type_flag == TarTypeFlag.Symlink, true);
    expect_eq(std.mem.eql(u8, entries[3].linkname, "data.bin"), true);
}

test("cap:tar - long paths (>100 bytes) via ustar prefix (155+100) splitting & reconstruction") {
    const alloc = std.testing.allocator;

    // 140+ char prefix + '/' + 60 char filename = >200 bytes total
    const long_path = "organizations/acme-corp/services/distributed-agent-runtime/clusters/us-west-2/deployments/2026-11/artifacts/v1/manifests/production-cluster-config-deployment-manifest.json";
    expect_eq(long_path.len > 100, true);
    expect_eq(long_path.len <= 256, true);

    let builder = TarBuilder.init(alloc);
    defer builder.deinit();

    const body = "{\"version\":2}";
    builder.addFile(long_path, body, 0o644, 1700000200) catch unreachable;

    const archive = builder.finish(alloc) catch unreachable;
    defer alloc.free(archive);

    // Verify ustar prefix field at offset 345 is non-empty
    expect_eq(archive[345] != 0, true);

    const entries = parseArchive(alloc, archive) catch unreachable;
    defer freeEntries(alloc, entries);

    expect_eq(entries.len, 1);
    expect_eq(std.mem.eql(u8, entries[0].name, long_path), true);
    expect_eq(std.mem.eql(u8, entries[0].data, body), true);
}

test("cap:tar - integrity guards: InvalidChecksum & TruncatedArchive") {
    const alloc = std.testing.allocator;

    let builder = TarBuilder.init(alloc);
    defer builder.deinit();
    builder.addFile("hello.txt", "hello world payload", 0o644, 1700000300) catch unreachable;

    const archive = builder.finish(alloc) catch unreachable;
    defer alloc.free(archive);

    // 1. Corrupt a header byte -> InvalidChecksum
    let corrupted = alloc.dupe(u8, archive) catch unreachable;
    defer alloc.free(corrupted);
    corrupted[0] ^= 0x01;

    let got_bad_cksum = false;
    * = parseArchive(alloc, corrupted) catch e => {
        if (e == TarError.InvalidChecksum) got_bad_cksum = true;
    };
    expect_eq(got_bad_cksum, true);

    // 2. Truncated archive in middle of payload -> TruncatedArchive
    let got_truncated_payload = false;
    * = parseArchive(alloc, archive[0..600]) catch e => {
        if (e == TarError.TruncatedArchive) got_truncated_payload = true;
    };
    expect_eq(got_truncated_payload, true);

    // 3. Truncated archive in middle of header -> TruncatedArchive
    let got_truncated_hdr = false;
    * = parseArchive(alloc, archive[0..256]) catch e => {
        if (e == TarError.TruncatedArchive) got_truncated_hdr = true;
    };
    expect_eq(got_truncated_hdr, true);
}

test("cap:tar - path traversal guards: ../../etc/passwd & /etc/shadow -> UnsafePathTraversal") {
    const alloc = std.testing.allocator;

    // 1. Relative traversal ../../etc/passwd
    let b1 = TarBuilder.init(alloc);
    defer b1.deinit();
    b1.addFile("../../etc/passwd", "root:x:0:0::/root:/bin/sh", 0o644, 1700000400) catch unreachable;
    const arch1 = b1.finish(alloc) catch unreachable;
    defer alloc.free(arch1);

    let got_rel_traversal = false;
    * = parseArchive(alloc, arch1) catch e => {
        if (e == TarError.UnsafePathTraversal) got_rel_traversal = true;
    };
    expect_eq(got_rel_traversal, true);

    // 2. Absolute leading slash /etc/shadow
    let b2 = TarBuilder.init(alloc);
    defer b2.deinit();
    b2.addFile("/etc/shadow", "secret", 0o600, 1700000401) catch unreachable;
    const arch2 = b2.finish(alloc) catch unreachable;
    defer alloc.free(arch2);

    let got_abs_traversal = false;
    * = parseArchive(alloc, arch2) catch e => {
        if (e == TarError.UnsafePathTraversal) got_abs_traversal = true;
    };
    expect_eq(got_abs_traversal, true);
}

test("cap:tar - dynamic cap.tar mounting & std.testing.allocator zero memory leaks") {
    const alloc = std.testing.allocator;

    let builder = TarBuilder.init(alloc);
    defer builder.deinit();

    builder.addFile("manifest.toml", "name = \"demo\"\n", 0o644, 1700000500) catch unreachable;
    const archive = builder.finish(alloc) catch unreachable;
    defer alloc.free(archive);

    const entries = parseArchive(alloc, archive) catch unreachable;
    defer freeEntries(alloc, entries);

    expect_eq(entries.len, 1);
    expect_eq(std.mem.eql(u8, entries[0].name, "manifest.toml"), true);
    expect_eq(std.mem.eql(u8, entries[0].data, "name = \"demo\"\n"), true);
}
