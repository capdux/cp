// cap/test/test_phase_a_foundation_port.cp
// ============================================================================
// Phase A Foundation Port Integration Test Suite:
//   - cap:sys (Maybe<T>, SysError, openat/fstatat/mkdirAll/DirIterator/copy_file_range/sendfile/PosixSpawn)
//   - cap:io  (Pipe.createNonblocking, EAGAIN/BrokenPipe state machine, IoSource, MaxBuf, FileReader/FileWriter, BufferedReader)
//   - cap:string (strings.*, UTF-8/UTF-16/Latin-1/WTF-8, StringBuilder, MutableString, Unicode)
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    FD,
    File,
    Stat,
    FileKind,
    SysError,
    SyscallTag,
    Maybe,
    DirEntryKind,
    DirIterator,
    Dir,
    CopyFileState,
    PosixSpawn,
    SpawnOptions,
    O_RDONLY,
    O_RDWR,
    O_CREAT,
    O_TRUNC,
    O_CLOEXEC,
    RENAME_NOREPLACE,
    ENOENT,
    EEXIST,
    AT_FDCWD,
    openat,
    fstatat,
    lstatat,
    mkdirAll,
    rmdirRecursive,
    symlinkat,
    readlinkat,
    renameat2,
    readFile,
    writeFile,
    copyFileRange,
    sendfile,
    copyFileWithState,
    waitpid,
    getTempDir,
    makeTempDir,
    removeFile,
    exists,
} from "cap:sys";

import {
    IoStatus,
    FileType,
    PollState,
    IoSource,
    MaxBuf,
    MaxBufKind,
    Pipe,
    PipeReader,
    PipeWriter,
    FileReader,
    FileWriter,
    BufferReader,
    BufferedReader,
} from "cap:io";

import {
    strings,
    Strings,
    StringBuilder,
    MutableString,
    Unicode,
    CodepointIterator,
    CodepointCursor,
    Wtf8,
    Utf8,
    Utf16,
    EncodeIntoResult,
    copyLatin1IntoUTF8,
    elementLengthLatin1IntoUTF8,
    copyUTF16IntoUTF8,
    elementLengthUTF16IntoUTF8,
    convertUTF8toUTF16,
    convertUTF8toUTF16InBuffer,
    convertUTF16toUTF8,
    convertUTF16toUTF8InBuffer,
    utf8ByteSequenceLength,
    decodeUTF8,
    encodeUTF8,
    withoutUTF8BOM,
    escapeHtmlAlloc,
    escapeRegExpAlloc,
} from "cap:string";

fn testSysPhaseA(): !void {
    const gpa = std.heap.page_allocator;

    // 1. Maybe<T> & SysError
    const m_ok = Maybe<i32>.initResult(42);
    expect_eq(m_ok.isOk(), true);
    expect_eq(m_ok.isErr(), false);
    expect_eq(m_ok.errno(), 0);
    expect_eq(m_ok.unwrapOr(0), 42);
    expect_eq(try m_ok.unwrap(), 42);

    const se = SysError.fromCode(ENOENT, .openat).withPath("/no/such/file.cp").withFd(7);
    expect_eq(se.isNotFound(), true);
    expect_eq(se.errno, ENOENT);
    expect_eq(se.fd, 7);
    expect_eq(strings.eql(se.codeName(), "ENOENT"), true);

    const m_err = Maybe<i32>.initErr(se);
    expect_eq(m_err.isErr(), true);
    expect_eq(m_err.errno(), ENOENT);
    expect_eq(m_err.unwrapOr(99), 99);

    let fmt_buf: [256]u8 = undefined;
    const fmt_str = se.format(&fmt_buf);
    expect_eq(strings.contains(fmt_str, "ENOENT"), true);
    expect_eq(strings.contains(fmt_str, "openat"), true);

    // 2. mkdirAll, openat, fstatat, symlinkat, readlinkat, renameat2, DirIterator, rmdirRecursive
    let tmp_buf: [256]u8 = undefined;
    const base_dir = try makeTempDir("cp_phase_a_sys_", &tmp_buf);
    defer * = rmdirRecursive(base_dir) catch {};

    let nested_parts: [2][]const u8 = [base_dir, "/a/b/c"];
    const nested_dir = try strings.concat(gpa, nested_parts[0..2]);
    defer gpa.free(nested_dir);
    try mkdirAll(nested_dir, 0o755);
    expect_eq(exists(nested_dir), true);

    const dir_fd = try openat(FD.fromNative(AT_FDCWD), nested_dir, O_RDONLY | O_CLOEXEC, 0);
    defer dir_fd.close();

    const f1 = try openat(dir_fd, "alpha.txt", O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0o644);
    const payload = "Phase A Sys Foundation Test Payload Data 1234567890!";
    try f1.writeAll(payload);
    f1.close();

    const st_f1 = try fstatat(dir_fd, "alpha.txt");
    expect_eq(st_f1.isFile(), true);
    expect_eq(st_f1.kind() == FileKind.file, true);
    expect_eq(st_f1.size, payload.len as u64);

    try symlinkat("alpha.txt", dir_fd, "alpha_link");
    const st_link = try lstatat(dir_fd, "alpha_link");
    expect_eq(st_link.isSymlink(), true);
    expect_eq(st_link.kind() == FileKind.sym_link, true);

    let link_buf: [128]u8 = undefined;
    const link_target = try readlinkat(dir_fd, "alpha_link", &link_buf);
    expect_eq(strings.eql(link_target, "alpha.txt"), true);

    // copy_file_range / copyFileWithState
    let copy_parts: [2][]const u8 = [nested_dir, "/beta_copy.txt"];
    const copy_dst = try strings.concat(gpa, copy_parts[0..2]);
    defer gpa.free(copy_dst);
    const src_open = try openat(dir_fd, "alpha.txt", O_RDONLY | O_CLOEXEC, 0);
    defer src_open.close();
    const dst_open = try openat(FD.fromNative(AT_FDCWD), copy_dst, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0o644);
    defer dst_open.close();

    let cf_state = CopyFileState{};
    try copyFileWithState(src_open, dst_open, &cf_state);
    expect_eq(cf_state.has_seen_exdev, false);

    const read_back = try readFile(gpa, copy_dst);
    defer gpa.free(read_back);
    expect_eq(strings.eql(read_back, payload), true);

    // renameat2 NOREPLACE
    try renameat2(dir_fd, "beta_copy.txt", dir_fd, "gamma.txt", RENAME_NOREPLACE);

    // DirIterator (Linux getdents64)
    let it = try DirIterator.open(nested_dir);
    defer it.close();
    let seen_alpha = false;
    let seen_gamma = false;
    let seen_link = false;
    while (true) {
        const entry_opt = try it.next();
        if (entry_opt == null) break;
        const entry = entry_opt.?;
        if (strings.eql(entry.name, "alpha.txt")) {
            seen_alpha = true;
            expect_eq(entry.kind == DirEntryKind.file, true);
        } else if (strings.eql(entry.name, "gamma.txt")) {
            seen_gamma = true;
        } else if (strings.eql(entry.name, "alpha_link")) {
            seen_link = true;
            expect_eq(entry.kind == DirEntryKind.sym_link, true);
        }
    }
    expect_eq(seen_alpha, true);
    expect_eq(seen_gamma, true);
    expect_eq(seen_link, true);

    // 3. PosixSpawn with stdout pipe redirection, custom envp & cwd
    let out_pipe = try Pipe.create();
    let spawn_env: [2][]const u8 = ["CP_SPAWN_VAR=phase_a_ok", "PATH=/bin:/usr/bin"];
    let spawn_argv: [3][]const u8 = ["/bin/sh", "-c", "pwd && printf '%s' \"$CP_SPAWN_VAR\""];
    const spawn_opts = SpawnOptions{
        cwd: "/tmp",
        envp: spawn_env[0..2],
        stdout_fd: FD.fromNative(out_pipe.writer.getFd()),
        search_path: true,
    };
    const pid = try PosixSpawn.spawn(
        gpa,
        spawn_argv[0..3],
        spawn_opts,
    );
    out_pipe.writer.close();

    const child_out = try out_pipe.reader.readToEnd(gpa);
    defer gpa.free(child_out);
    out_pipe.reader.close();

    const wres = try waitpid(pid, 0);
    expect_eq(wres.exited(), true);
    expect_eq(wres.exitCode(), 0);
    expect_eq(strings.contains(child_out, "/tmp"), true);
    expect_eq(strings.contains(child_out, "phase_a_ok"), true);

    // Failed spawn returns Maybe error with ENOENT
    let bad_argv: [1][]const u8 = ["/nonexistent/binary_xyz_123"];
    const bad_spawn = PosixSpawn.spawnMaybe(gpa, bad_argv[0..1], SpawnOptions{ search_path: false });
    expect_eq(bad_spawn.isErr(), true);
    expect_eq(bad_spawn.errno(), ENOENT);
}

fn testIoPhaseA(): !void {
    const gpa = std.heap.page_allocator;

    // 1. Pipe.createNonblocking() + EAGAIN state machine + IoSource + MaxBuf
    let nb_pipe = try Pipe.createNonblocking();
    expect_eq(nb_pipe.reader.isNonblocking(), true);
    expect_eq(nb_pipe.writer.isNonblocking(), true);

    let src_info = IoSource.fromFd(FD.fromNative(nb_pipe.reader.getFd()));
    expect_eq(src_info.getFileType() == FileType.NonblockingPipe, true);
    expect_eq(src_info.isNonblocking(), true);

    // Empty non-blocking pipe -> WouldBlock (EAGAIN)
    let rbuf: [64]u8 = undefined;
    const empty_res = nb_pipe.reader.attemptRead(rbuf[0..rbuf.len]);
    expect_eq(empty_res.status == IoStatus.WouldBlock, true);

    // Write & poll & read with MaxBuf
    let mb = new MaxBuf(16);
    nb_pipe.reader.attachMaxBuf(mb);

    const wr_res = nb_pipe.writer.attemptWrite("hello-pipe");
    expect_eq(wr_res.status == IoStatus.Ok, true);
    expect_eq(wr_res.bytes, 10);

    const pr = nb_pipe.reader.pollReadable(500);
    expect_eq(pr.readable, true);

    const rd_res = nb_pipe.reader.attemptRead(rbuf[0..rbuf.len]);
    expect_eq(rd_res.status == IoStatus.Ok, true);
    expect_eq(rd_res.bytes, 10);
    expect_eq(strings.eql(rbuf[0..10], "hello-pipe"), true);
    expect_eq(mb.consumed(), 10);

    // Exceed MaxBuf (10 + 10 = 20 > 16)
    * = nb_pipe.writer.attemptWrite("0123456789");
    const over_res = nb_pipe.reader.attemptRead(rbuf[0..rbuf.len]);
    expect_eq(over_res.status == IoStatus.BufferFull, true);
    nb_pipe.reader.detachMaxBuf();

    // Ignore SIGPIPE temporarily to verify BrokenPipe status safely
    let old_act: std.os.linux.Sigaction = undefined;
    let ign_act = std.os.linux.Sigaction{
        .handler = .{ .handler = std.os.linux.SIG.IGN },
        .mask = std.os.linux.sigemptyset(),
        .flags = 0,
    };
    * = std.os.linux.sigaction(std.os.linux.SIG.PIPE, &ign_act, &old_act);
    nb_pipe.reader.close();
    const bp_res = nb_pipe.writer.attemptWrite("after-close");
    * = std.os.linux.sigaction(std.os.linux.SIG.PIPE, &old_act, null);
    expect_eq(bp_res.status == IoStatus.BrokenPipe, true);
    expect_eq(nb_pipe.writer.isBrokenPipe(), true);
    nb_pipe.writer.close();

    // 2. FileReader / FileWriter
    let tmp_buf: [256]u8 = undefined;
    const dir = try makeTempDir("cp_phase_a_io_", &tmp_buf);
    defer * = rmdirRecursive(dir) catch {};
    let fpath_parts: [2][]const u8 = [dir, "/stream.dat"];
    const fpath = try strings.concat(gpa, fpath_parts[0..2]);
    defer gpa.free(fpath);

    let dummy_nb: bool = false;
    let fw = try FileWriter.openForWriting(fpath, 0o644, true, &dummy_nb);
    expect_eq(dummy_nb, false);
    try fw.writeAll("ABCDE-12345-FGHIJ");
    try fw.sync();
    fw.close();

    let fr = try FileReader.open(fpath);
    defer fr.close();
    expect_eq(try fr.size(), 17);
    let pbuf: [5]u8 = undefined;
    const pn = try fr.pread(pbuf[0..5], 6);
    expect_eq(pn, 5);
    expect_eq(strings.eql(pbuf[0..5], "12345"), true);
    const full_data = try fr.readToEnd(gpa);
    defer gpa.free(full_data);
    expect_eq(strings.eql(full_data, "ABCDE-12345-FGHIJ"), true);

    // 3. BufferedReader enhancements: readUntilDelimiter, readLine, peek, discard, unreadSlice, shift
    let br_src = new BufferReader("line1\r\nline2\npart1:part2");
    let br = new BufferedReader<BufferReader>(br_src);
    let line_buf: [64]u8 = undefined;

    const l1 = (try br.readLine(line_buf[0..64])).?;
    expect_eq(strings.eql(l1, "line1"), true);
    const l2 = (try br.readLine(line_buf[0..64])).?;
    expect_eq(strings.eql(l2, "line2"), true);

    const pk = try br.peek(5);
    expect_eq(strings.eql(pk, "part1"), true);
    expect_eq(br.buffered() >= 5, true);
    br.shift();
    expect_eq(strings.startsWith(br.unreadSlice(), "part1"), true);

    const dlen = try br.readUntilDelimiter(':', line_buf[0..64]);
    expect_eq(strings.eql(line_buf[0..dlen], "part1:"), true);
    const skipped = try br.discard(3);
    expect_eq(skipped, 3);
    expect_eq(strings.eql(try br.peek(2), "t2"), true);
}

fn testStringPhaseA(): !void {
    const gpa = std.heap.page_allocator;

    // 1. High-frequency Bun strings.*
    expect_eq(strings.eql("bun", "bun"), true);
    expect_eq(strings.eqlLong("hello", "hello", true), true);
    expect_eq(strings.eqlComptime("cp-lang", "cp-lang"), true);
    expect_eq(strings.eqlCaseInsensitiveASCII("Content-Type", "content-type", true), true);
    expect_eq(strings.eqlCaseInsensitiveASCIIICheckLength("Host", "HOST"), true);
    expect_eq(strings.hasPrefix("https://cp.dev", "https://"), true);
    expect_eq(strings.hasPrefixComptime("https://cp.dev", "https://"), true);
    expect_eq(strings.hasSuffix("main.cp", ".cp"), true);
    expect_eq(strings.hasSuffixComptime("main.cp", ".cp"), true);
    expect_eq(strings.startsWithCaseInsensitiveASCII("Authorization: Bearer", "authorization:"), true);
    expect_eq(strings.endsWithCaseInsensitiveASCII("INDEX.CP", ".cp"), true);

    expect_eq(strings.indexOfChar("a/b/c", '/').?, 1);
    expect_eq(strings.indexOfCharPos("a/b/c", '/', 2).?, 3);
    expect_eq(strings.lastIndexOfChar("a/b/c", '/').?, 3);
    expect_eq(strings.indexOfAny("hello=world;foo", ";=").?, 5);
    expect_eq(strings.indexOfNotChar("   abc", ' ').?, 3);
    expect_eq(strings.containsChar("hello", 'e'), true);
    expect_eq(strings.contains("hello world", "world"), true);
    expect_eq(strings.containsComptime("hello world", "llo"), true);
    expect_eq(strings.countChar("a,b,c,d", ','), 3);

    expect_eq(strings.eql(strings.trimLeft("  hi "), "hi "), true);
    expect_eq(strings.eql(strings.trimRight("  hi "), "  hi"), true);
    expect_eq(strings.eql(strings.trimWhitespace(" \t hi \n"), "hi"), true);
    expect_eq(strings.eql(strings.trimPrefix("lib/sys", "lib/"), "sys"), true);
    expect_eq(strings.eql(strings.trimSuffix("file.cp", ".cp"), "file"), true);

    let sc_it = strings.splitScalar("one:two:three", ':');
    expect_eq(strings.eql(sc_it.next().?, "one"), true);
    expect_eq(strings.eql(sc_it.next().?, "two"), true);
    expect_eq(strings.eql(sc_it.next().?, "three"), true);
    expect_eq(sc_it.next() == null, true);

    let join_parts: [3][]const u8 = ["lib", "sys", "index.cp"];
    const joined = try strings.join(gpa, "/", join_parts[0..3]);
    defer gpa.free(joined);
    expect_eq(strings.eql(joined, "lib/sys/index.cp"), true);

    const rep = try strings.repeat(gpa, "ab", 3);
    defer gpa.free(rep);
    expect_eq(strings.eql(rep, "ababab"), true);

    const fmt_s = try strings.formatZigString(gpa, "x={d},s={s}", .{ 7, "ok" });
    defer gpa.free(fmt_s);
    expect_eq(strings.eql(fmt_s, "x=7,s=ok"), true);

    const html_esc = try escapeHtmlAlloc(gpa, "<a href=\"x\">'&'</a>");
    defer gpa.free(html_esc);
    expect_eq(strings.eql(html_esc, "&lt;a href=&quot;x&quot;&gt;&#x27;&amp;&#x27;&lt;/a&gt;"), true);

    const re_esc = try escapeRegExpAlloc(gpa, "a.b+(c)?");
    defer gpa.free(re_esc);
    expect_eq(strings.eql(re_esc, "a\\.b\\+\\(c\\)\\?"), true);

    expect_eq(strings.isAllASCII("pure-ascii-123"), true);
    expect_eq(strings.isAllASCII("中文"), false);

    // 2. Latin-1 / UTF-16 / UTF-8 / WTF-8 codecs
    const latin1: [4]u8 = ['H', 'i', 0xE9, '!']; // "Hié!"
    expect_eq(elementLengthLatin1IntoUTF8(&latin1), 5);
    let utf8_out: [16]u8 = undefined;
    const l1_res: EncodeIntoResult = copyLatin1IntoUTF8(&utf8_out, &latin1);
    expect_eq(l1_res.read, 4);
    expect_eq(l1_res.written, 5);
    expect_eq(Utf8.isValid(utf8_out[0..l1_res.written]), true);

    // UTF-16LE with surrogate pair U+1F680 (🚀 = D83D DE80)
    const u16_in: [3]u16 = ['A', 0xD83D, 0xDE80];
    expect_eq(elementLengthUTF16IntoUTF8(&u16_in), 5);
    const u16_res = copyUTF16IntoUTF8(&utf8_out, &u16_in);
    expect_eq(u16_res.read, 3);
    expect_eq(u16_res.written, 5);
    expect_eq(strings.eql(utf8_out[0..u16_res.written], "A🚀"), true);

    const back_u16 = try convertUTF8toUTF16(gpa, "A🚀");
    defer gpa.free(back_u16);
    expect_eq(back_u16.len, 3);
    expect_eq(back_u16[1], 0xD83D);
    expect_eq(back_u16[2], 0xDE80);

    const back_u8 = try convertUTF16toUTF8(gpa, back_u16);
    defer gpa.free(back_u8);
    expect_eq(strings.eql(back_u8, "A🚀"), true);

    expect_eq(utf8ByteSequenceLength(0xF0), 4);
    expect_eq(decodeUTF8("🚀", 0).codepoint, 0x1F680);
    let enc_one: [4]u8 = undefined;
    expect_eq(encodeUTF8(0x1F680, &enc_one), 4);

    // WTF-8 lone high surrogate round-trip
    let wtf_buf: [8]u8 = undefined;
    const lone_n = Wtf8.encodeRune(0xD800, &wtf_buf);
    expect_eq(lone_n, 3);
    expect_eq(Wtf8.isValid(wtf_buf[0..lone_n]), true);
    expect_eq(Wtf8.decodeRune(wtf_buf[0..lone_n], 0).codepoint, 0xD800);

    expect_eq(strings.eql(withoutUTF8BOM("\xEF\xBB\xBFhello"), "hello"), true);

    // 3. StringBuilder (Two-pass + appendFmt + append16 + appendZ)
    let sb = StringBuilder.initWithAllocator(gpa);
    defer sb.release();
    sb.count("prefix:");
    sb.count16(&u16_in);
    sb.countZ(":end");
    try sb.allocateIn(gpa);
    * = sb.append("prefix:");
    * = sb.append16(&u16_in);
    const ztail = sb.appendZ(":end");
    expect_eq(strings.eql(ztail, ":end"), true);
    expect_eq(strings.eql(sb.slice()[0 .. sb.length() - 1], "prefix:A🚀:end"), true);

    // 4. MutableString
    let ms = try MutableString.init2048(gpa);
    defer ms.deinit();
    try ms.append("count=");
    try ms.appendInt(42);
    try ms.appendChar(' ');
    try ms.appendSignedInt(-7);
    try ms.appendCharNTimes('!', 2);
    expect_eq(ms.eql("count=42 -7!!"), true);
    expect_eq(ms.containsChar('-'), true);
    expect_eq(ms.indexOfChar('=').?, 5);
    let ms_clone = try ms.clone();
    defer ms_clone.deinit();
    expect_eq(ms_clone.eql("count=42 -7!!"), true);

    // 5. Unicode predicates & CodepointIterator
    expect_eq(Unicode.isWhitespace(0x3000), true); // Ideographic space
    expect_eq(Unicode.isLineTerminator(0x2028), true);
    expect_eq(Unicode.isIdentifierStart('α'), true);
    expect_eq(Unicode.isIdentifierContinue('9'), true);
    expect_eq(Unicode.toLower('Ω'), 'ω');
    expect_eq(Unicode.toUpper('ω'), 'Ω');
    expect_eq(Unicode.caseFold(0x03C2), 0x03C3);
    expect_eq(Unicode.charWidth('中'), 2);
    expect_eq(Unicode.displayWidth("A中🚀"), 5);

    let cp_it = CodepointIterator.init("Aα中🚀");
    expect_eq(cp_it.next().?, 'A');
    expect_eq(cp_it.next().?, 0x03B1);
    expect_eq(strings.eql(cp_it.peek(1), "中"), true);
    expect_eq(cp_it.next().?, 0x4E2D);
    expect_eq(cp_it.next().?, 0x1F680);
    expect_eq(cp_it.next() == null, true);
}

fn main() {
    test("Phase A Port 1: cap:sys — Maybe, SysError, *at syscalls, DirIterator, copy_file_range & PosixSpawn", () => {
        try testSysPhaseA();
    });
    test("Phase A Port 2: cap:io — Nonblocking Pipe EAGAIN/BrokenPipe, IoSource, MaxBuf, FileReader/Writer & BufferedReader", () => {
        try testIoPhaseA();
    });
    test("Phase A Port 3: cap:string — Bun strings.*, Latin-1/UTF-16/WTF-8, StringBuilder, MutableString & Unicode", () => {
        try testStringPhaseA();
    });
}
