// Deep production regression tests for cap:mysql localInfile.cp
// Covers 0xFB LOCAL INFILE Request frame & payload round-trip, empty terminator frame,
// client-side security policy (CLIENT_LOCAL_FILES opt-in, path traversal & absolute path guards),
// and malformed packet boundary protections.

import { test, expect, expect_eq } from "cap:test";
import {
    LOCAL_INFILE_HEADER,
    LOCAL_INFILE_MAX_FILENAME,
    LocalInfileError,
    LocalInfileRequest,
} from "cap:mysql";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn expectUsizeErr(res: LocalInfileError!usize, want: LocalInfileError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb 0;
    };
    expect(got_err);
}

fn expectBoolErr(res: LocalInfileError!bool, want: LocalInfileError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb false;
    };
    expect(got_err);
}

fn expectReqErr(res: LocalInfileError!LocalInfileRequest, want: LocalInfileError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb LocalInfileRequest{};
    };
    expect(got_err);
}

test("mysql localInfile: frame round-trip and 24-bit LE header verification", () => {
    expect(LOCAL_INFILE_HEADER == 0xFB);
    expect(LOCAL_INFILE_MAX_FILENAME == 4096);

    const req = LocalInfileRequest.init("data/batch_01.csv", 3);
    expect(req.encodedPayloadLen() == 18);
    expect(req.encodedFrameLen() == 22);
    expect(req.isSafeRelativeFilename());

    let buf: [64]u8 = [0; 64];
    const n = req.encodeFrame(buf[0..]) catch unreachable;
    expect(n == 22);
    // 3-byte LE payload length = 18 (0x12, 0x00, 0x00), seq_id = 3, header = 0xFB
    expect(buf[0] == 18 && buf[1] == 0 && buf[2] == 0 && buf[3] == 3);
    expect(buf[4] == 0xFB);

    const decoded = LocalInfileRequest.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.eql(req));
    expect(decoded.sequence_id == 3);
    expect(strEq(decoded.filename, "data/batch_01.csv"));
});

test("mysql localInfile: payload-only decode and empty terminator frame", () => {
    const req = LocalInfileRequest.init("import.tsv", 0);
    let pbuf: [32]u8 = [0; 32];
    const pn = req.encodePayload(pbuf[0..]) catch unreachable;
    expect(pn == 11);

    const decoded = LocalInfileRequest.decodePayload(pbuf[0..pn]) catch unreachable;
    expect(decoded.eql(req));

    // Empty terminator packet ends client data stream
    let tbuf: [8]u8 = [0xFF; 8];
    const tn = LocalInfileRequest.encodeEmptyTerminatorFrame(4, tbuf[0..]) catch unreachable;
    expect(tn == 4);
    expect(tbuf[0] == 0 && tbuf[1] == 0 && tbuf[2] == 0 && tbuf[3] == 4);

    let tiny: [3]u8 = [0; 3];
    expectUsizeErr(LocalInfileRequest.encodeEmptyTerminatorFrame(1, tiny[0..]), LocalInfileError.BufferTooSmall);
});

test("mysql localInfile: path traversal and absolute path security policy guards", () => {
    const safe = LocalInfileRequest.init("uploads/report.csv", 1);
    expect(safe.isSafeRelativeFilename());
    expect(!safe.hasPathTraversal());
    expect(!safe.isAbsolutePath());
    expect(safe.checkPolicy(true, false) catch unreachable);

    // Disabled CLIENT_LOCAL_FILES blocks even safe paths
    expectBoolErr(safe.checkPolicy(false, false), LocalInfileError.LocalFilesDisabled);

    // POSIX & Windows traversal rejected
    const trav1 = LocalInfileRequest.init("../etc/passwd", 1);
    expect(trav1.hasPathTraversal());
    expect(!trav1.isSafeRelativeFilename());
    expectBoolErr(trav1.checkPolicy(true, true), LocalInfileError.PathTraversalDenied);

    const trav2 = LocalInfileRequest.init("dir\\..\\secret.txt", 1);
    expect(trav2.hasPathTraversal());
    expectBoolErr(trav2.checkPolicy(true, true), LocalInfileError.PathTraversalDenied);

    // Absolute POSIX and Windows drive paths rejected unless allow_absolute == true
    const abs_unix = LocalInfileRequest.init("/etc/shadow", 1);
    expect(abs_unix.isAbsolutePath());
    expectBoolErr(abs_unix.checkPolicy(true, false), LocalInfileError.PathTraversalDenied);
    expect(abs_unix.checkPolicy(true, true) catch unreachable);

    const abs_win = LocalInfileRequest.init("C:\\Users\\admin\\data.csv", 1);
    expect(abs_win.isAbsolutePath());
    expectBoolErr(abs_win.checkPolicy(true, false), LocalInfileError.PathTraversalDenied);

    // Legitimate filenames containing double dots inside a name are allowed
    const dots_in_name = LocalInfileRequest.init("archive..2026.csv", 1);
    expect(!dots_in_name.hasPathTraversal());
    expect(dots_in_name.isSafeRelativeFilename());
});

test("mysql localInfile: encode guards reject empty filename, embedded NUL, and small buffers", () => {
    let buf: [64]u8 = [0; 64];

    const empty = LocalInfileRequest.init("", 1);
    expectUsizeErr(empty.encodeFrame(buf[0..]), LocalInfileError.EmptyFilename);
    expectUsizeErr(empty.encodePayload(buf[0..]), LocalInfileError.EmptyFilename);

    let nul_name: [5]u8 = ['a', '.', 'c', 0, 'v'];
    const with_nul = LocalInfileRequest.init(nul_name[0..], 1);
    expect(!with_nul.isSafeRelativeFilename());
    expectUsizeErr(with_nul.encodeFrame(buf[0..]), LocalInfileError.EmbeddedNul);

    const valid = LocalInfileRequest.init("table.csv", 1);
    let tiny: [5]u8 = [0; 5];
    expectUsizeErr(valid.encodeFrame(tiny[0..]), LocalInfileError.BufferTooSmall);
    expectUsizeErr(valid.encodePayload(tiny[0..]), LocalInfileError.BufferTooSmall);
});

test("mysql localInfile: decodeFrame & decodePayload malformed input boundaries", () => {
    // Empty payload -> Truncated
    let empty_slice: [0]u8 = [];
    expectReqErr(LocalInfileRequest.decodePayload(empty_slice[0..]), LocalInfileError.Truncated);

    // Wrong header byte (0x00 OK packet instead of 0xFB)
    let wrong_hdr: [5]u8 = [0x00, 'a', '.', 't', 'x'];
    expectReqErr(LocalInfileRequest.decodePayload(wrong_hdr[0..]), LocalInfileError.InvalidHeader);

    // Header 0xFB with zero-length filename -> EmptyFilename
    let only_hdr: [1]u8 = [0xFB];
    expectReqErr(LocalInfileRequest.decodePayload(only_hdr[0..]), LocalInfileError.EmptyFilename);

    // Truncated frame (< 4 bytes header)
    let short_hdr: [3]u8 = [2, 0, 0];
    expectReqErr(LocalInfileRequest.decodeFrame(short_hdr[0..]), LocalInfileError.Truncated);

    // Truncated frame body (declared payload_len = 5, only 3 present)
    let short_body: [7]u8 = [5, 0, 0, 1, 0xFB, 'a', 'b'];
    expectReqErr(LocalInfileRequest.decodeFrame(short_body[0..]), LocalInfileError.Truncated);

    // Trailing bytes after declared frame length
    let trailing: [8]u8 = [3, 0, 0, 1, 0xFB, 'a', 'b', 0x99];
    expectReqErr(LocalInfileRequest.decodeFrame(trailing[0..]), LocalInfileError.TrailingBytes);
});

test("mysql localInfile: filename length limit enforcement (> 4096 bytes)", () => {
    let huge: [4097]u8 = ['a'; 4097];
    const too_long = LocalInfileRequest.init(huge[0..], 2);
    expect(!too_long.isSafeRelativeFilename());
    expectBoolErr(too_long.validate(), LocalInfileError.FilenameTooLong);
    expectBoolErr(too_long.checkPolicy(true, true), LocalInfileError.FilenameTooLong);

    // Exact boundary 4096 bytes succeeds validation
    const at_limit = LocalInfileRequest.init(huge[0..4096], 2);
    expect(at_limit.validate() catch unreachable);
});
