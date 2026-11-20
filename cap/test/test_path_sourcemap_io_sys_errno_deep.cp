// Source: /data/dev/bun/src/paths/ & /data/dev/bun/src/sourcemap/ & /data/dev/bun/src/io/ & /data/dev/bun/src/sys/ & /data/dev/bun/src/errno/
// ============================================================================
// cap/test/test_path_sourcemap_io_sys_errno_deep.cp
// Comprehensive Deep Audit Test Suite for cap:path, cap:sourcemap, cap:io, cap:sys, cap:errno
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Path,
    PosixPath,
    WindowsPath,
    ParsedPath,
    toNamespacedPathPosix,
    toNamespacedPathWindows,
} from "cap:path";
import {
    VLQ,
    LineOffsetTable,
    DynamicLineOffsetTable,
    SourceMapping,
    MappingList,
    ParsedSourceMap,
    Chunk,
    SourceMapChunk,
    ParsedSourceMapRecord,
    SourceMapGenerator,
    SourceMapConsumer,
    utf8ToUtf16Column,
    utf16ToUtf8Column,
} from "cap:sourcemap";
import {
    SeekOrigin,
    BufferReader,
    BufferWriter,
    BufferedReader,
    BufferedWriter,
    LimitedReader,
    MultiReader,
    TeeReader,
    DiscardWriter,
    CountingWriter,
    ChunkedBuffer,
} from "cap:io";
import {
    FD,
    O_RDWR,
    O_CREAT,
    O_TRUNC,
    O_CLOEXEC,
    F_OK,
    R_OK,
    W_OK,
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
    open,
    close,
    read,
    write,
    pread,
    pwrite,
    lseek,
    ftruncate,
    stat,
    lstat,
    fstat,
    mkdir,
    rmdir,
    unlink,
    rename,
    symlink,
    readlink,
    chmod,
    access,
    Sys,
    dupFd,
    dup2,
    getcwd,
    chdir,
    getpid,
    getppid,
    getuid,
    getgid,
    uname,
    pageSize,
    cpuCount,
    monotonicNs,
    realtimeMs,
    copyFileRange,
    makeTempDir,
} from "cap:sys";
import { SystemErrno } from "cap:errno";

fn testPathDeep(): !void {
    let buf: [2048]u8 = undefined;

    // 1. POSIX resolve, normalize, relative edge cases
    let empty_holder: [1][]const u8 = [""];
    expect_eq(std.mem.eql(u8, PosixPath.resolve(empty_holder[0..0], &buf), "/"), true);
    let multi_res: [4][]const u8 = [ "/var", "log", "/etc/nginx", "../ssl/./certs" ];
    expect_eq(std.mem.eql(u8, PosixPath.resolve(multi_res[0..4], &buf), "/etc/ssl/certs"), true);

    expect_eq(std.mem.eql(u8, PosixPath.normalize("///a//b/./c/../../d", &buf), "/a/d"), true);
    expect_eq(std.mem.eql(u8, PosixPath.normalize("/../../..", &buf), "/"), true);
    expect_eq(std.mem.eql(u8, PosixPath.normalize("", &buf), "."), true);

    expect_eq(std.mem.eql(u8, PosixPath.relative(".", "a/b", &buf), "a/b"), true);
    expect_eq(std.mem.eql(u8, PosixPath.relative("a/b", ".", &buf), "../.."), true);
    expect_eq(std.mem.eql(u8, PosixPath.relative("/", "/a/b/c", &buf), "a/b/c"), true);
    expect_eq(std.mem.eql(u8, PosixPath.relative("/a/b/c", "/", &buf), "../../.."), true);
    expect_eq(std.mem.eql(u8, PosixPath.relative("/a/b", "c/d", &buf), "../../c/d"), true);

    // 2. Windows drive letters, UNC paths, forward-slash UNC, rooted resolve
    expect_eq(WindowsPath.isAbsolute("C:\\foo"), true);
    expect_eq(WindowsPath.isAbsolute("c:/foo"), true);
    expect_eq(WindowsPath.isAbsolute("\\\\server\\share\\file"), true);
    expect_eq(WindowsPath.isAbsolute("//server/share/file"), true);
    expect_eq(WindowsPath.isAbsolute("C:relative"), false);

    expect_eq(std.mem.eql(u8, WindowsPath.volumeName("//server/share/a/b"), "//server/share"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.normalize("//server/share/a//b/../../c", &buf), "\\\\server\\share\\c"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.normalize("C:\\..\\..\\Windows\\System32", &buf), "C:\\Windows\\System32"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.normalize("C:a\\b\\..\\c", &buf), "C:a\\c"), true);

    // Rooted segment after drive path keeps drive letter (Node/Win32 parity)
    let win_res_rooted: [2][]const u8 = [ "D:\\projects\\cp", "\\assets\\icon.png" ];
    expect_eq(std.mem.eql(u8, WindowsPath.resolve(win_res_rooted[0..2], &buf), "D:\\assets\\icon.png"), true);

    let win_res_unc: [3][]const u8 = [ "C:\\local", "\\\\nas\\media\\movies", "..\\music" ];
    expect_eq(std.mem.eql(u8, WindowsPath.resolve(win_res_unc[0..3], &buf), "\\\\nas\\media\\music"), true);

    // Windows relative with UNC & case-insensitivity & cross-volume
    expect_eq(std.mem.eql(u8, WindowsPath.relative("C:\\Users\\Alice\\Docs", "c:\\users\\alice\\Music\\a.mp3", &buf), "..\\Music\\a.mp3"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.relative("\\\\srv\\sh\\a\\b", "\\\\SRV\\SH\\a\\c", &buf), "..\\c"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.relative("\\\\srv1\\sh1\\a", "\\\\srv2\\sh2\\b", &buf), "\\\\srv2\\sh2\\b"), true);

    // 3. toNamespacedPath (POSIX & Windows)
    expect_eq(std.mem.eql(u8, PosixPath.toNamespacedPath("/usr/local/bin", &buf), "/usr/local/bin"), true);
    expect_eq(std.mem.eql(u8, toNamespacedPathPosix("", &buf), ""), true);
    expect_eq(std.mem.eql(u8, WindowsPath.toNamespacedPath("C:\\Windows\\System32", &buf), "\\\\?\\C:\\Windows\\System32"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.toNamespacedPath("\\\\server\\share\\dir\\file.txt", &buf), "\\\\?\\UNC\\server\\share\\dir\\file.txt"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.toNamespacedPath("\\\\?\\C:\\Already\\Namespaced", &buf), "\\\\?\\C:\\Already\\Namespaced"), true);
    expect_eq(std.mem.eql(u8, toNamespacedPathWindows("", &buf), ""), true);

    // 4. PosixPath & WindowsPath parse/format (including dir+name+ext without base)
    let wp = WindowsPath.parse("C:\\dir\\sub\\App.Exe");
    expect_eq(std.mem.eql(u8, wp.root, "C:\\"), true);
    expect_eq(std.mem.eql(u8, wp.dir, "C:\\dir\\sub"), true);
    expect_eq(std.mem.eql(u8, wp.base, "App.Exe"), true);
    expect_eq(std.mem.eql(u8, wp.name, "App"), true);
    expect_eq(std.mem.eql(u8, wp.ext, ".Exe"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.format(wp, &buf), "C:\\dir\\sub\\App.Exe"), true);

    let no_base = ParsedPath{ dir: "/opt/cp/bin", name: "cap", ext: ".bin" };
    expect_eq(std.mem.eql(u8, PosixPath.format(no_base, &buf), "/opt/cp/bin/cap.bin"), true);

    let root_only = ParsedPath{ root: "/", base: "init" };
    expect_eq(std.mem.eql(u8, PosixPath.format(root_only, &buf), "/init"), true);
}

fn testSourceMapDeep(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Base64 VLQ signed boundaries (i32 min/max & i64 large ints)
    let v_max = VLQ.encode(2147483647);
    let d_max = VLQ.decode(v_max.slice(), 0);
    expect_eq(d_max.valid, true);
    expect_eq(d_max.value, 2147483647);

    let v_min = VLQ.encode(-2147483648);
    let d_min = VLQ.decode(v_min.slice(), 0);
    expect_eq(d_min.valid, true);
    expect_eq(d_min.value, -2147483648);

    let v64_pos = VLQ.encode64(9007199254740991);
    let d64_pos = VLQ.decode64(v64_pos.slice(), 0);
    expect_eq(d64_pos.valid, true);
    expect_eq(d64_pos.value, 9007199254740991 as i64);

    let v64_neg = VLQ.encode64(-9007199254740991);
    let d64_neg = VLQ.decode64(v64_neg.slice(), 0);
    expect_eq(d64_neg.valid, true);
    expect_eq(d64_neg.value, -9007199254740991 as i64);

    let dec_vals: [4]i32 = [0; 4];
    expect_eq(VLQ.decodeValues("ACDgB", &dec_vals), 4 as usize);
    expect_eq(dec_vals[0], 0);
    expect_eq(dec_vals[1], 1);
    expect_eq(dec_vals[2], -1);
    expect_eq(dec_vals[3], 16);

    // 2. MappingList out-of-order append -> sort -> binary search find
    let mlist = MappingList.init();
    defer mlist.deinit(alloc);
    try mlist.appendParts(alloc, 2, 10, 20, 4, 1, 2);
    try mlist.appendParts(alloc, 0, 0, 0, 0, 0, -1);
    try mlist.appendParts(alloc, 1, 8, 10, 5, 0, 1);
    try mlist.appendParts(alloc, 1, 2, 10, 0, 0, 0);
    expect_eq(mlist.is_sorted, false);
    mlist.sort();
    expect_eq(mlist.is_sorted, true);

    let f_exact = mlist.find(1, 5);
    expect_eq(f_exact != null, true);
    expect_eq(f_exact.?.generated_column, 2);
    expect_eq(f_exact.?.name_index, 0);

    let f_online = mlist.findOnLine(0, 5);
    expect_eq(f_online != null, true);
    expect_eq(f_online.?.generated_line, 0);
    expect_eq(mlist.findOnLine(3, 0) == null, true);

    let vlq_out = try mlist.toMappingsString(alloc);
    defer alloc.free(vlq_out);
    let reparsed = try MappingList.parseMappings(alloc, vlq_out);
    defer reparsed.deinit(alloc);
    expect_eq(reparsed.len(), 4 as usize);

    // 3. LineOffsetTable + UTF-8/UTF-16 column conversion
    const sample_utf8 = "let s = \"中🚀\";\nnext_line\n";
    const lt = LineOffsetTable<16>.build(sample_utf8);
    expect_eq(lt.lineCount(), 3 as usize);
    expect_eq(std.mem.eql(u8, lt.lineText(sample_utf8, 0), "let s = \"中🚀\";"), true);
    expect_eq(std.mem.eql(u8, lt.lineText(sample_utf8, 1), "next_line"), true);
    // "let s = \"中🚀\";" -> '中' is 3 bytes (1 UTF-16), '🚀' is 4 bytes (2 UTF-16)
    expect_eq(utf8ToUtf16Column("let s = \"中🚀\";", 16), 12 as u32);
    expect_eq(utf16ToUtf8Column("let s = \"中🚀\";", 12), 16 as u32);

    // 4. SourceMap v3 JSON with escaped strings, null sourcesContent, sourceRoot
    const raw_json =
        "{\n" ++
        "  \"version\": 3,\n" ++
        "  \"file\": \"out.js\",\n" ++
        "  \"sourceRoot\": \"https://example.com/src/\",\n" ++
        "  \"sources\": [\"a.cp\", \"b.cp\"],\n" ++
        "  \"sourcesContent\": [null, \"const msg = \\\"hi\\n\\\";\"],\n" ++
        "  \"names\": [\"msg\"],\n" ++
        "  \"mappings\": \"AAAA;ACACA\"\n" ++
        "}\n";
    let rec = try ParsedSourceMapRecord.parse(alloc, raw_json);
    defer rec.deinit(alloc);
    expect_eq(rec.version, 3);
    expect_eq(std.mem.eql(u8, rec.source_root, "https://example.com/src/"), true);
    expect_eq(rec.sources.len, 2 as usize);
    expect_eq(rec.sources_content.len, 2 as usize);
    expect_eq(rec.sources_content[0].len, 0 as usize);
    expect_eq(std.mem.eql(u8, rec.sources_content[1], "const msg = \"hi\n\";"), true);

    let rejson = try rec.toJSON(alloc, true);
    defer alloc.free(rejson);
    let consumer = try SourceMapConsumer.init(alloc, rejson);
    defer consumer.deinit();
    let sc_b = consumer.sourceContentFor("b.cp");
    expect_eq(sc_b != null, true);
    expect_eq(std.mem.eql(u8, sc_b.?, "const msg = \"hi\n\";"), true);

    // 5. SourceMapChunk multi-chunk concatenation & merge
    let ch1 = SourceMapChunk.init();
    defer ch1.deinit(alloc);
    const s_a = try ch1.addSource(alloc, "mod_a.cp", "fn fa() {}");
    const n_a = try ch1.addName(alloc, "fa");
    try ch1.addMapping(alloc, SourceMapping{ generated_line: 0, generated_column: 0, source_index: s_a, original_line: 1, original_column: 0, name_index: n_a });

    let ch2 = SourceMapChunk.init();
    defer ch2.deinit(alloc);
    const s_b = try ch2.addSource(alloc, "mod_b.cp", "fn fb() {}");
    const n_b = try ch2.addName(alloc, "fb");
    try ch2.addMapping(alloc, SourceMapping{ generated_line: 0, generated_column: 4, source_index: s_b, original_line: 5, original_column: 2, name_index: n_b });

    let chunks_arr: [2]Chunk = [ ch1, ch2 ];
    let offsets_arr: [2]i32 = [ 0, 12 ];
    let merged = try Chunk.merge(alloc, chunks_arr[0..2], offsets_arr[0..2]);
    defer merged.deinit(alloc);
    expect_eq(merged.sources.items.len, 2 as usize);
    expect_eq(merged.names.items.len, 2 as usize);
    let m_merged = merged.find(12, 6);
    expect_eq(m_merged != null, true);
    expect_eq(m_merged.?.source_index, 1);
    expect_eq(m_merged.?.original_line, 5);
    expect_eq(m_merged.?.original_column, 2);
    expect_eq(m_merged.?.name_index, 1);
}

fn testIoDeep(): !void {
    const alloc = std.heap.page_allocator;

    // 1. ChunkedBuffer dynamic multi-chunk expansion + Reader/Writer/Seeker
    let cb = ChunkedBuffer.withChunkSize(alloc, 32);
    defer cb.deinit();
    let payload: [100]u8 = [0x41; 100]; // 100 'A's (> 32 byte chunk_size)
    expect_eq(try cb.write(&payload), 100 as usize);
    try cb.writeByte('\n');
    expect_eq(cb.len(), 101 as usize);
    expect_eq(cb.chunkCount() >= 4, true);

    expect_eq(try cb.seek(0, SeekOrigin.Start), 0 as u64);
    expect_eq(std.mem.eql(u8, cb.peek(4), "AAAA"), true);
    expect_eq(cb.remaining(), 101 as usize);

    // 2. Enhanced BufferedReader: peek, peekByte, discard, readUntil, readLine, readFull
    let line_src = new BufferReader("alpha\r\nbeta\ngamma");
    let br = new BufferedReader<BufferReader>(line_src);
    expect_eq(try br.peekByte(), 'a' as u8);
    expect_eq(std.mem.eql(u8, try br.peek(5), "alpha"), true);

    let lbuf: [64]u8 = undefined;
    const l1 = try br.readLine(lbuf[0..64]);
    expect_eq(l1 != null, true);
    expect_eq(std.mem.eql(u8, l1.?, "alpha"), true);

    const u2_n = try br.readUntil('\n', lbuf[0..64]);
    expect_eq(std.mem.eql(u8, lbuf[0..u2_n], "beta\n"), true);

    expect_eq(try br.discard(2), 2 as usize); // skip "ga"
    let rem: [3]u8 = undefined;
    expect_eq(try br.readFull(rem[0..3]), 3 as usize);
    expect_eq(std.mem.eql(u8, rem[0..3], "mma"), true);

    // UnexpectedEof when reading past EOF with readFull
    let eof_hit = false;
    let extra: [4]u8 = undefined;
    * = br.readFull(extra[0..4]) catch {
        eof_hit = true;
    };
    expect_eq(eof_hit, true);

    // 3. BufferedWriter: custom threshold + large-block zero-copy passthrough + CountingWriter + DiscardWriter
    let sink = new ChunkedBuffer(alloc);
    defer sink.deinit();
    let cw = new CountingWriter<ChunkedBuffer>(sink);
    let bw = new BufferedWriter<ChunkedBuffer>(sink);
    bw.setThreshold(8);
    * = try bw.write("1234");
    expect_eq(bw.buffered(), 4 as usize);
    expect_eq(sink.len(), 0 as usize);
    * = try bw.write("5678"); // hits threshold 8 -> auto flush!
    expect_eq(bw.buffered(), 0 as usize);
    expect_eq(sink.len(), 8 as usize);
    // Large write >= threshold flushes buffered then bypasses cache directly
    * = try bw.write("AB");
    expect_eq(bw.buffered(), 2 as usize);
    * = try bw.write("0123456789"); // >= 8 -> flushes "AB" then direct writes 10B
    expect_eq(bw.buffered(), 0 as usize);
    expect_eq(sink.len(), 20 as usize);
    expect_eq(std.mem.eql(u8, sink.bytes(), "12345678AB0123456789"), true);

    expect_eq(try cw.write("!"), 1 as usize);
    expect_eq(cw.count(), 1 as usize);
    cw.resetCount();
    expect_eq(cw.count(), 0 as usize);

    let dw = new DiscardWriter();
    expect_eq(try dw.write("discarded bytes"), 15 as usize);
    expect_eq(dw.bytesDiscarded(), 15 as usize);

    // 4. MultiReader, LimitedReader, TeeReader
    let r1 = new BufferReader("hello ");
    let r2 = new BufferReader("world!!!");
    let mr = MultiReader<BufferReader>.initPair(r1, r2);
    let mr_out: [32]u8 = undefined;
    let mr_total: usize = 0;
    while (true) {
        let got = try mr.read(mr_out[mr_total..32]);
        if (got == 0) break;
        mr_total += got;
    }
    expect_eq(mr_total, 14 as usize);
    expect_eq(std.mem.eql(u8, mr_out[0..14], "hello world!!!"), true);

    let lim_src = new BufferReader("hello world!!!");
    let lim = new LimitedReader<BufferReader>(lim_src, 11); // "hello world"
    let lim_out: [32]u8 = undefined;
    let lim_n = try lim.read(lim_out[0..32]);
    expect_eq(lim_n, 11 as usize);
    expect_eq(std.mem.eql(u8, lim_out[0..11], "hello world"), true);
    expect_eq(lim.isExhausted(), true);

    let tee_src = new BufferReader("tee-payload");
    let tee_buf = new BufferWriter();
    let tee = new TeeReader<BufferReader, BufferWriter>(tee_src, tee_buf);
    let out_buf: [32]u8 = undefined;
    let total: usize = 0;
    while (true) {
        let got = try tee.read(out_buf[total..32]);
        if (got == 0) break;
        total += got;
    }
    expect_eq(total, 11 as usize);
    expect_eq(std.mem.eql(u8, out_buf[0..11], "tee-payload"), true);
    expect_eq(std.mem.eql(u8, tee_buf.bytes(), "tee-payload"), true);
    expect_eq(tee.bytesTransferred(), 11 as usize);
}

fn testSysDeep(): !void {
    // 1. Process & hardware introspection
    expect_eq(getpid() > 0, true);
    expect_eq(getppid() >= 0, true);
    * = getuid();
    * = getgid();
    expect_eq(pageSize() >= 4096, true);
    expect_eq(cpuCount() >= 1, true);
    let t1 = monotonicNs();
    let t2 = monotonicNs();
    expect_eq(t2 >= t1, true);
    expect_eq(realtimeMs() > 1700000000000, true);

    let uinfo = try uname();
    expect_eq(uinfo.sysnameStr().len > 0, true);
    expect_eq(uinfo.machineStr().len > 0, true);

    // 2. Filesystem & FD primitives in isolated temp dir
    let dir_buf: [512]u8 = undefined;
    const tmp_root = try makeTempDir("cap_sys_deep", &dir_buf);

    let saved_cwd_buf: [512]u8 = undefined;
    const saved_cwd = try getcwd(&saved_cwd_buf);
    expect_eq(saved_cwd.len > 0, true);

    let sub_buf: [512]u8 = undefined;
    const sub_dir = try std.fmt.bufPrint(&sub_buf, "{s}/sub", .{tmp_root});
    try mkdir(sub_dir, 0o755);

    try chdir(sub_dir);
    let cur_cwd_buf: [512]u8 = undefined;
    const cur_cwd = try getcwd(&cur_cwd_buf);
    expect_eq(std.mem.endsWith(u8, cur_cwd, "/sub"), true);
    try chdir(saved_cwd);

    // open, write, ftruncate, lseek, pread, pwrite, dup, dup2, close
    let f1_buf: [512]u8 = undefined;
    const f1_path = try std.fmt.bufPrint(&f1_buf, "{s}/file1.txt", .{tmp_root});
    let f2_buf: [512]u8 = undefined;
    const f2_path = try std.fmt.bufPrint(&f2_buf, "{s}/file2.txt", .{tmp_root});
    let lnk_buf: [512]u8 = undefined;
    const lnk_path = try std.fmt.bufPrint(&lnk_buf, "{s}/link1", .{tmp_root});

    const fd1 = try open(f1_path, O_CREAT | O_RDWR | O_TRUNC | O_CLOEXEC, 0o644);
    expect_eq(try write(fd1, "0123456789ABCDEF"), 16 as usize);
    try ftruncate(fd1, 10);
    let st_fd1 = try fstat(fd1);
    expect_eq(st_fd1.size, 10 as u64);
    expect_eq(try pwrite(fd1, "XY", 4), 2 as usize);

    let dup_fd = try Sys.dup(fd1);
    let dup_fd2 = try dupFd(fd1);
    close(dup_fd2);
    expect_eq(try lseek(dup_fd, 0, SEEK_SET), 0 as u64);
    let rbuf: [16]u8 = undefined;
    expect_eq(try read(dup_fd, rbuf[0..10]), 10 as usize);
    expect_eq(std.mem.eql(u8, rbuf[0..10], "0123XY6789"), true);

    let dup2_target = FD.fromNative(dup_fd.raw + 5);
    let d2 = try dup2(fd1, dup2_target);
    let pread_buf: [4]u8 = undefined;
    expect_eq(try pread(d2, &pread_buf, 4), 4 as usize);
    expect_eq(std.mem.eql(u8, &pread_buf, "XY67"), true);

    // copyFileRange from fd1 to fd2
    const fd2 = try open(f2_path, O_CREAT | O_RDWR | O_TRUNC | O_CLOEXEC, 0o644);
    let src_off: u64 = 0;
    let dst_off: u64 = 0;
    expect_eq(try copyFileRange(fd1, &src_off, fd2, &dst_off, 10), 10 as usize);
    expect_eq(try pread(fd2, rbuf[0..10], 0), 10 as usize);
    expect_eq(std.mem.eql(u8, rbuf[0..10], "0123XY6789"), true);

    close(fd2);
    close(d2);
    close(dup_fd);
    close(fd1);

    // access, chmod, symlink, lstat, readlink, rename, unlink, rmdir
    try access(f1_path, F_OK | R_OK | W_OK);
    try chmod(f1_path, 0o600);
    let st_f1 = try stat(f1_path);
    expect_eq(st_f1.permissions(), 0o600 as u32);

    try symlink(f1_path, lnk_path);
    const lst = try lstat(lnk_path);
    expect_eq(lst.isSymLink(), true);
    let rl_buf: [512]u8 = undefined;
    const target_read = try readlink(lnk_path, &rl_buf);
    expect_eq(std.mem.eql(u8, target_read, f1_path), true);

    let f3_buf: [512]u8 = undefined;
    const f3_path = try std.fmt.bufPrint(&f3_buf, "{s}/file3.txt", .{tmp_root});
    try rename(f2_path, f3_path);
    try access(f3_path, F_OK);

    try unlink(lnk_path);
    try unlink(f3_path);
    try unlink(f1_path);
    try rmdir(sub_dir);
    try rmdir(tmp_root);
}

fn testErrnoDeep(): !void {
    // Verify extended 134-entry errno set & semantic predicates
    const e_nomsg = SystemErrno.fromCode(-42);
    expect_eq(e_nomsg == SystemErrno.ENOMSG, true);
    expect_eq(std.mem.eql(u8, e_nomsg.name, "ENOMSG"), true);
    expect_eq(std.mem.eql(u8, SystemErrno.name(e_nomsg), "ENOMSG"), true);
    expect_eq(std.mem.eql(u8, e_nomsg.description(), "No message of desired type"), true);

    const e_ilseq = SystemErrno.fromCode(84);
    expect_eq(e_ilseq == SystemErrno.EILSEQ, true);
    expect_eq(std.mem.eql(u8, e_ilseq.codeName(), "EILSEQ"), true);

    const e_hw = SystemErrno.fromCode(133);
    expect_eq(e_hw == SystemErrno.EHWPOISON, true);
    expect_eq(std.mem.eql(u8, e_hw.description(), "Memory page has hardware error"), true);

    // isWouldBlock
    expect_eq(SystemErrno.EAGAIN.isWouldBlock(), true);
    expect_eq(SystemErrno.EWOULDBLOCK.isWouldBlock(), true);
    expect_eq(SystemErrno.ENOENT.isWouldBlock(), false);

    // isConnectionError
    expect_eq(SystemErrno.ECONNRESET.isConnectionError(), true);
    expect_eq(SystemErrno.ECONNREFUSED.isConnectionError(), true);
    expect_eq(SystemErrno.EPIPE.isConnectionError(), true);
    expect_eq(SystemErrno.ENETUNREACH.isConnectionError(), true);
    expect_eq(SystemErrno.EEXIST.isConnectionError(), false);

    // isTimeout
    expect_eq(SystemErrno.ETIMEDOUT.isTimeout(), true);
    expect_eq(SystemErrno.ETIME.isTimeout(), true);
    expect_eq(SystemErrno.EAGAIN.isTimeout(), false);

    // isAlreadyExists
    expect_eq(SystemErrno.EEXIST.isAlreadyExists(), true);
    expect_eq(SystemErrno.ENOENT.isAlreadyExists(), false);
}

fn main() {
    test("1. cap:path Deep — resolve, relative, normalize, parse/format, UNC & toNamespacedPath", () => {
        try testPathDeep();
    });
    test("2. cap:sourcemap Deep — VLQ i32/i64 boundaries, MappingList, UTF-16 LineOffsetTable, JSON & Chunk merge", () => {
        try testSourceMapDeep();
    });
    test("3. cap:io Deep — BufferedReader/Writer, ChunkedBuffer, Limited/Multi/Tee/Counting/Discard", () => {
        try testIoDeep();
    });
    test("4. cap:sys Deep — Zig 0.17 std.c/std.os.linux FS, FD, symlink, copyFileRange & process primitives", () => {
        try testSysDeep();
    });
    test("5. cap:errno Deep — 134 POSIX/Linux Errnos & semantic predicates", () => {
        try testErrnoDeep();
    });
}
