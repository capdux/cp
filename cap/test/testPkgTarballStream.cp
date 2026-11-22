import { test, assert, assertEq } from "cap:test";
import { gzip, gunzip } from "cap:pack";
import {
    TAR_BLOCK_SIZE,
    TarStreamError,
    TarEntryKind,
    TarStreamOptions,
    TarStreamEventKind,
    TarStreamDecoder,
    parseTarOctalOrBase256,
    verifyTarHeaderChecksum,
    parseTarHeaderBlock,
    sanitizeTarEntryPath,
    validateTarLinkTarget,
    encodeTarEntry,
    encodeTarPaxHeader,
    finalizeTarArchive,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("512B USTAR header checksum verification, octal and GNU base-256 size decoding", () => {
    // Octal field parsing
    const oct = parseTarOctalOrBase256("00000001000\x00") catch unreachable;
    assertEq(oct, @as(u64, 512));

    // GNU base-256 big-endian positive integer (0x80 | high bits)
    const b256: [12]u8 = [0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x04, 0x00];
    const sz = parseTarOctalOrBase256(b256[0..]) catch unreachable;
    assertEq(sz, @as(u64, 1024));

    // Encode a valid header and verify checksum + parsed metadata
    let buf: [2048]u8 = [0; 2048];
    let pos: usize = 0;
    encodeTarEntry("package/hello.txt", TarEntryKind.RegularFile, 0o644, 1700000000, "", "hello tar", buf[0..], &pos) catch unreachable;
    assert(verifyTarHeaderChecksum(buf[0..TAR_BLOCK_SIZE]));

    const hdr = parseTarHeaderBlock(buf[0..TAR_BLOCK_SIZE]) catch unreachable;
    assert(hdr.is_ustar);
    assert(hdr.kind == TarEntryKind.RegularFile);
    assertEq(hdr.mode, @as(u32, 0o644));
    assertEq(hdr.size, @as(u64, 9));
    assertEq(hdr.mtime, @as(u64, 1700000000));
    assert(strEql(hdr.raw_name, "package/hello.txt"));

    // Corrupting a header byte must fail checksum verification
    buf[5] = buf[5] ^ 0xFF;
    assert(!verifyTarHeaderChecksum(buf[0..TAR_BLOCK_SIZE]));
    let chk_err = false;
    * = parseTarHeaderBlock(buf[0..TAR_BLOCK_SIZE]) catch |e| {
        chk_err = (e == TarStreamError.InvalidHeaderChecksum);
    };
    assert(chk_err);
});

test("Incremental 17-byte chunk streaming decoder with strip_components=1 round-trip", () => {
    let archive: [8192]u8 = [0; 8192];
    let apos: usize = 0;
    encodeTarEntry("package/", TarEntryKind.Directory, 0o755, 1700000000, "", "", archive[0..], &apos) catch unreachable;
    encodeTarEntry("package/package.json", TarEntryKind.RegularFile, 0o644, 1700000001, "", "{\"name\":\"demo\"}\n", archive[0..], &apos) catch unreachable;
    encodeTarEntry("package/src/main.cp", TarEntryKind.RegularFile, 0o644, 1700000002, "", "pub fn main() {}\n", archive[0..], &apos) catch unreachable;
    finalizeTarArchive(archive[0..], &apos) catch unreachable;

    let pbuf: [512]u8 = [0; 512];
    let lbuf: [512]u8 = [0; 512];
    let mbuf: [512]u8 = [0; 512];
    let decoder = TarStreamDecoder.init(TarStreamOptions.defaults(), pbuf[0..], lbuf[0..], mbuf[0..]);

    let file1_buf: [128]u8 = [0; 128];
    let file1_len: usize = 0;
    let file2_buf: [128]u8 = [0; 128];
    let file2_len: usize = 0;
    let cur_file_idx: u32 = 0;
    let ended = false;

    // Feed archive in awkward 17-byte chunks to stress boundary reassembly
    let feed_pos: usize = 0;
    while (feed_pos < apos && !ended) {
        const end_pos = if (feed_pos + 17 < apos) feed_pos + 17 else apos;
        let chunk = archive[feed_pos..end_pos];
        let c_off: usize = 0;
        while (c_off < chunk.len) {
            const ev = decoder.step(chunk[c_off..]) catch unreachable;
            c_off += ev.consumed;
            if (ev.kind == TarStreamEventKind.EntryHeader) {
                cur_file_idx += 1;
                if (cur_file_idx == 1) {
                    assert(strEql(ev.entry.path, "package.json"));
                } else if (cur_file_idx == 2) {
                    assert(strEql(ev.entry.path, "src/main.cp"));
                }
            } else if (ev.kind == TarStreamEventKind.EntryDataChunk) {
                if (cur_file_idx == 1) {
                    let i: usize = 0;
                    while (i < ev.data.len) {
                        file1_buf[file1_len + i] = ev.data[i];
                        i += 1;
                    }
                    file1_len += ev.data.len;
                } else if (cur_file_idx == 2) {
                    let i: usize = 0;
                    while (i < ev.data.len) {
                        file2_buf[file2_len + i] = ev.data[i];
                        i += 1;
                    }
                    file2_len += ev.data.len;
                }
            } else if (ev.kind == TarStreamEventKind.ArchiveEnd) {
                ended = true;
                break;
            } else if (ev.kind == TarStreamEventKind.NeedMoreData) {
                break;
            }
        }
        feed_pos = end_pos;
    }

    decoder.finish() catch unreachable;
    assert(ended);
    assertEq(cur_file_idx, @as(u32, 2));
    assert(strEql(file1_buf[0..file1_len], "{\"name\":\"demo\"}\n"));
    assert(strEql(file2_buf[0..file2_len], "pub fn main() {}\n"));

    const st = decoder.stats();
    assertEq(st.files_extracted, @as(u32, 2));
    assertEq(st.skipped_entries, @as(u32, 1)); // stripped top-level `package/`
});

test("USTAR prefix+name long path splitting and POSIX PAX extended header overrides", () => {
    let archive: [8192]u8 = [0; 8192];
    let apos: usize = 0;

    // 1. Long path > 100 bytes automatically split into USTAR prefix + name
    const long_ustar_path =
        "package/very/deep/nested/directory/structure/for/testing/ustar/prefix/splitting/behavior/more/segments/component.cp";
    encodeTarEntry(long_ustar_path, TarEntryKind.RegularFile, 0o644, 1700000010, "", "ustar_ok", archive[0..], &apos) catch unreachable;

    // 2. PAX extended header overriding path & mtime
    encodeTarPaxHeader("path", "package/pax/overridden/unicode_safe_name.cp", archive[0..], &apos) catch unreachable;
    encodeTarPaxHeader("mtime", "1750000000.5", archive[0..], &apos) catch unreachable;
    encodeTarEntry("package/placeholder", TarEntryKind.RegularFile, 0o644, 1, "", "pax_ok", archive[0..], &apos) catch unreachable;

    finalizeTarArchive(archive[0..], &apos) catch unreachable;

    let pbuf: [512]u8 = [0; 512];
    let lbuf: [512]u8 = [0; 512];
    let mbuf: [512]u8 = [0; 512];
    let decoder = TarStreamDecoder.init(TarStreamOptions.defaults(), pbuf[0..], lbuf[0..], mbuf[0..]);

    let off: usize = 0;
    let seen: u32 = 0;
    while (off < apos) {
        const ev = decoder.step(archive[off..apos]) catch unreachable;
        off += ev.consumed;
        if (ev.kind == TarStreamEventKind.EntryHeader) {
            seen += 1;
            if (seen == 1) {
                assert(strEql(
                    ev.entry.path,
                    "very/deep/nested/directory/structure/for/testing/ustar/prefix/splitting/behavior/more/segments/component.cp",
                ));
            } else if (seen == 2) {
                assert(strEql(ev.entry.path, "pax/overridden/unicode_safe_name.cp"));
                assertEq(ev.entry.mtime, @as(u64, 1750000000));
            }
        } else if (ev.kind == TarStreamEventKind.ArchiveEnd) {
            break;
        }
    }
    assertEq(seen, @as(u32, 2));
    assertEq(decoder.stats().pax_headers_applied, @as(u32, 2));
});

test("GNU LongName (L) and LongLink (K) extended headers streaming support", () => {
    let archive: [8192]u8 = [0; 8192];
    let apos: usize = 0;

    encodeTarEntry("././@LongLink", TarEntryKind.GnuLongName, 0o644, 0, "", "package/gnu/long/dir/symlink_entry\x00", archive[0..], &apos) catch unreachable;
    encodeTarEntry("././@LongLink", TarEntryKind.GnuLongLink, 0o644, 0, "", "../../target.cp\x00", archive[0..], &apos) catch unreachable;
    encodeTarEntry("package/truncated", TarEntryKind.SymLink, 0o777, 0, "short", "", archive[0..], &apos) catch unreachable;
    finalizeTarArchive(archive[0..], &apos) catch unreachable;

    let pbuf: [512]u8 = [0; 512];
    let lbuf: [512]u8 = [0; 512];
    let mbuf: [512]u8 = [0; 512];
    let decoder = TarStreamDecoder.init(TarStreamOptions.defaults(), pbuf[0..], lbuf[0..], mbuf[0..]);

    let off: usize = 0;
    let found = false;
    while (off < apos) {
        const ev = decoder.step(archive[off..apos]) catch unreachable;
        off += ev.consumed;
        if (ev.kind == TarStreamEventKind.EntryHeader) {
            assert(ev.entry.kind == TarEntryKind.SymLink);
            assert(strEql(ev.entry.path, "gnu/long/dir/symlink_entry"));
            assert(strEql(ev.entry.link_target, "../../target.cp"));
            found = true;
        } else if (ev.kind == TarStreamEventKind.ArchiveEnd) {
            break;
        }
    }
    assert(found);
    assertEq(decoder.stats().gnu_long_headers_applied, @as(u32, 2));
});

test("Zip-Slip path traversal, absolute paths, and Windows drive prefix guards", () => {
    let out: [256]u8 = [0; 256];
    const ok_len = sanitizeTarEntryPath("./package//src/./app.cp", 1, out[0..]) catch unreachable;
    assert(strEql(out[0..ok_len], "src/app.cp"));

    const stripped_len = sanitizeTarEntryPath("package/", 1, out[0..]) catch unreachable;
    assertEq(stripped_len, @as(usize, 0));

    let e1 = false;
    * = sanitizeTarEntryPath("package/../../etc/passwd", 1, out[0..]) catch |e| {
        e1 = (e == TarStreamError.UnsafePathTraversal);
    };
    assert(e1);

    let e2 = false;
    * = sanitizeTarEntryPath("/etc/shadow", 0, out[0..]) catch |e| {
        e2 = (e == TarStreamError.UnsafePathTraversal);
    };
    assert(e2);

    let e3 = false;
    * = sanitizeTarEntryPath("C:/Windows/System32/cmd.exe", 0, out[0..]) catch |e| {
        e3 = (e == TarStreamError.UnsafePathTraversal);
    };
    assert(e3);
});

test("Symlink/hardlink root-escape validation and device node prohibition", () => {
    // Safe internal symlink: src/sub/link -> ../index.cp (stays inside root)
    assert(validateTarLinkTarget("src/sub/link", "../index.cp", true));
    // Escaping symlink: src/link -> ../../outside.cp (drops below root depth 0)
    assert(!validateTarLinkTarget("src/link", "../../outside.cp", true));
    // Absolute symlink rejected
    assert(!validateTarLinkTarget("bin/cli", "/usr/bin/env", true));
    // Hardlink with '..' rejected
    assert(!validateTarLinkTarget("a/b", "../c", false));
    assert(validateTarLinkTarget("a/b", "lib/c.cp", false));

    // Device node inside tarball rejected by default
    let archive: [2048]u8 = [0; 2048];
    let apos: usize = 0;
    encodeTarEntry("package/dev_null", TarEntryKind.CharDevice, 0o666, 0, "", "", archive[0..], &apos) catch unreachable;
    finalizeTarArchive(archive[0..], &apos) catch unreachable;

    let pbuf: [256]u8 = [0; 256];
    let lbuf: [256]u8 = [0; 256];
    let mbuf: [256]u8 = [0; 256];
    let decoder = TarStreamDecoder.init(TarStreamOptions.defaults(), pbuf[0..], lbuf[0..], mbuf[0..]);

    let dev_blocked = false;
    * = decoder.step(archive[0..apos]) catch |e| {
        dev_blocked = (e == TarStreamError.DeviceNodeForbidden);
    };
    assert(dev_blocked);
});

test("Resource exhaustion guards (max_files, max_file_size) and truncated stream UnexpectedEof", () => {
    let archive: [4096]u8 = [0; 4096];
    let apos: usize = 0;
    encodeTarEntry("package/big.bin", TarEntryKind.RegularFile, 0o644, 0, "", "0123456789abcdef", archive[0..], &apos) catch unreachable;
    finalizeTarArchive(archive[0..], &apos) catch unreachable;

    let pbuf: [256]u8 = [0; 256];
    let lbuf: [256]u8 = [0; 256];
    let mbuf: [256]u8 = [0; 256];

    // max_file_size = 8 must reject 16-byte entry
    let tight_opts = TarStreamOptions.defaults();
    tight_opts.max_file_size = 8;
    let decoder = TarStreamDecoder.init(tight_opts, pbuf[0..], lbuf[0..], mbuf[0..]);
    let sz_err = false;
    * = decoder.step(archive[0..apos]) catch |e| {
        sz_err = (e == TarStreamError.MaxFileSizeExceeded);
    };
    assert(sz_err);

    // Truncated stream mid-header triggers UnexpectedEof on finish()
    let dec2 = TarStreamDecoder.init(TarStreamOptions.defaults(), pbuf[0..], lbuf[0..], mbuf[0..]);
    * = dec2.step(archive[0..100]) catch unreachable;
    let eof_err = false;
    dec2.finish() catch |e| {
        eof_err = (e == TarStreamError.UnexpectedEof);
    };
    assert(eof_err);
});

test("End-to-end .tgz pipeline: encodeTarEntry -> gzip -> gunzip -> 31B streaming decode", () => {
    let raw_tar: [4096]u8 = [0; 4096];
    let tpos: usize = 0;
    encodeTarEntry("package/README.md", TarEntryKind.RegularFile, 0o644, 1700000100, "", "# Cap Pkg\nZero-alloc stream!\n", raw_tar[0..], &tpos) catch unreachable;
    finalizeTarArchive(raw_tar[0..], &tpos) catch unreachable;

    // Compress with cap:pack gzip and decompress with gunzip
    const compressed = gzip(raw_tar[0..tpos], 6);
    assert(compressed.len > 0 && compressed.len < tpos);
    const inflated_opt = gunzip(compressed);
    assert(inflated_opt != null);
    const inflated = inflated_opt.?;
    assertEq(inflated.len, tpos);

    let pbuf: [256]u8 = [0; 256];
    let lbuf: [256]u8 = [0; 256];
    let mbuf: [256]u8 = [0; 256];
    let decoder = TarStreamDecoder.init(TarStreamOptions.defaults(), pbuf[0..], lbuf[0..], mbuf[0..]);

    let out_content: [128]u8 = [0; 128];
    let out_len: usize = 0;
    let feed: usize = 0;
    while (feed < inflated.len && !decoder.isDone()) {
        const end = if (feed + 31 < inflated.len) feed + 31 else inflated.len;
        let sub = inflated[feed..end];
        let off: usize = 0;
        while (off < sub.len) {
            const ev = decoder.step(sub[off..]) catch unreachable;
            off += ev.consumed;
            if (ev.kind == TarStreamEventKind.EntryHeader) {
                assert(strEql(ev.entry.path, "README.md"));
            } else if (ev.kind == TarStreamEventKind.EntryDataChunk) {
                let i: usize = 0;
                while (i < ev.data.len) {
                    out_content[out_len + i] = ev.data[i];
                    i += 1;
                }
                out_len += ev.data.len;
            } else if (ev.kind == TarStreamEventKind.ArchiveEnd || ev.kind == TarStreamEventKind.NeedMoreData) {
                break;
            }
        }
        feed = end;
    }

    decoder.finish() catch unreachable;
    assert(strEql(out_content[0..out_len], "# Cap Pkg\nZero-alloc stream!\n"));
});
