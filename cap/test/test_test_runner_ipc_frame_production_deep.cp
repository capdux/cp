// Production Deep Tests for cap:test_runner Parallel IPC Frame Codec (lib/test_runner/ipc_frame.cp)
import { test, expect_eq } from "cap:test";
import {
    IPC_FRAME_HEADER_SIZE,
    IPC_FRAME_MAX_PAYLOAD,
    IPC_TRUNC_MARKER,
    IpcFrameError,
    IpcFrameKind,
    IpcDecodedFrame,
    IpcPayloadReader,
    IpcFrameWriter,
    decodeIpcFrame,
} from "cap:test_runner";

test("test_runner ipc_frame: run and file_done (9 x u32) encode/decode roundtrip", fn() {
    expect_eq(IPC_FRAME_HEADER_SIZE, @as(usize, 5));
    expect_eq(IPC_FRAME_MAX_PAYLOAD, @as(u32, 64 * 1024 * 1024));

    let buf: [128]u8 = [0; 128];
    let w = IpcFrameWriter.init(buf[0..]);

    // coordinator -> worker: run(file_idx=42, path="tests/auth.test.cp")
    * = w.begin(IpcFrameKind.run) catch unreachable;
    * = w.writeU32(42) catch unreachable;
    * = w.writeStr("tests/auth.test.cp") catch unreachable;
    const wire1 = w.finish() catch unreachable;

    const f1 = decodeIpcFrame(wire1) catch unreachable;
    expect_eq(f1.kind, IpcFrameKind.run);
    expect_eq(f1.total_bytes, wire1.len);
    let r1 = IpcPayloadReader.init(f1.payload);
    expect_eq(r1.readU32Checked() catch unreachable, @as(u32, 42));
    expect_eq(r1.readStrChecked() catch unreachable, "tests/auth.test.cp");
    expect_eq(r1.remaining(), @as(usize, 0));

    // worker -> coordinator: file_done (9 x u32)
    * = w.begin(IpcFrameKind.file_done) catch unreachable;
    let k: u32 = 1;
    while (k <= 9) {
        * = w.writeU32(k * 10) catch unreachable;
        k += 1;
    }
    const wire2 = w.finish() catch unreachable;
    const f2 = decodeIpcFrame(wire2) catch unreachable;
    expect_eq(f2.kind, IpcFrameKind.file_done);
    expect_eq(f2.payload.len, @as(usize, 36));

    let r2 = IpcPayloadReader.init(f2.payload);
    let m: u32 = 1;
    while (m <= 9) {
        expect_eq(r2.readU32Checked() catch unreachable, m * 10);
        m += 1;
    }
    expect_eq(r2.remaining(), @as(usize, 0));
});

test("test_runner ipc_frame: empty-payload control frames and all 9 IpcFrameKind values", fn() {
    let buf: [16]u8 = [0; 16];
    let w = IpcFrameWriter.init(buf[0..]);

    * = w.begin(IpcFrameKind.ready) catch unreachable;
    const wire_ready = w.finish() catch unreachable;
    expect_eq(wire_ready.len, @as(usize, 5));
    const d_ready = decodeIpcFrame(wire_ready) catch unreachable;
    expect_eq(d_ready.kind, IpcFrameKind.ready);
    expect_eq(d_ready.payload.len, @as(usize, 0));

    * = w.begin(IpcFrameKind.shutdown) catch unreachable;
    const wire_shut = w.finish() catch unreachable;
    const d_shut = decodeIpcFrame(wire_shut) catch unreachable;
    expect_eq(d_shut.kind, IpcFrameKind.shutdown);

    // All 0..8 map to valid kinds; 9+ returns null
    let i: u8 = 0;
    while (i <= 8) {
        expect_eq(IpcFrameKind.fromU8(i) != null, true);
        i += 1;
    }
    expect_eq(IpcFrameKind.fromU8(9) == null, true);
    expect_eq(IpcFrameKind.fromU8(255) == null, true);
});

test("test_runner ipc_frame: writeStrTruncating preserves valid framing with truncation marker", fn() {
    // Limit payload to 40 bytes -> 4 bytes str len prefix + 36 bytes room for string
    let buf: [64]u8 = [0; 64];
    let w = IpcFrameWriter.initWithLimit(buf[0..], 40);

    * = w.begin(IpcFrameKind.test_done) catch unreachable;
    const long_line = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const written = w.writeStrTruncating(long_line) catch unreachable;
    expect_eq(written, @as(usize, 40));

    const wire = w.finish() catch unreachable;
    const decoded = decodeIpcFrame(wire) catch unreachable;
    expect_eq(decoded.kind, IpcFrameKind.test_done);
    expect_eq(decoded.payload.len, @as(usize, 40));

    let r = IpcPayloadReader.init(decoded.payload);
    const s = r.readStrChecked() catch unreachable;
    expect_eq(s.len, @as(usize, 36));
    // Prefix starts with "0123456789ab" (36 - 24 = 12 bytes) followed by IPC_TRUNC_MARKER (24 bytes)
    expect_eq(IPC_TRUNC_MARKER.len, @as(usize, 24));
    expect_eq(s[0], @as(u8, '0'));
    expect_eq(s[11], @as(u8, 'b'));
    expect_eq(s[12..36], IPC_TRUNC_MARKER);
});

test("test_runner ipc_frame: stream decoding of back-to-back concatenated frames", fn() {
    let f1_buf: [32]u8 = [0; 32];
    let f2_buf: [32]u8 = [0; 32];
    let w1 = IpcFrameWriter.init(f1_buf[0..]);
    let w2 = IpcFrameWriter.init(f2_buf[0..]);

    * = w1.begin(IpcFrameKind.file_start) catch unreachable;
    * = w1.writeU32(3) catch unreachable;
    const b1 = w1.finish() catch unreachable;

    * = w2.begin(IpcFrameKind.junit_file) catch unreachable;
    * = w2.writeStr("out.xml") catch unreachable;
    const b2 = w2.finish() catch unreachable;

    // Concatenate both frames into a single stream buffer
    let stream: [64]u8 = [0; 64];
    let idx: usize = 0;
    let i: usize = 0;
    while (i < b1.len) {
        stream[idx] = b1[i];
        idx += 1;
        i += 1;
    }
    let j: usize = 0;
    while (j < b2.len) {
        stream[idx] = b2[j];
        idx += 1;
        j += 1;
    }

    const d1 = decodeIpcFrame(stream[0..idx]) catch unreachable;
    expect_eq(d1.kind, IpcFrameKind.file_start);
    let r1 = IpcPayloadReader.init(d1.payload);
    expect_eq(r1.readU32(), @as(u32, 3));

    const d2 = decodeIpcFrame(stream[d1.total_bytes..idx]) catch unreachable;
    expect_eq(d2.kind, IpcFrameKind.junit_file);
    let r2 = IpcPayloadReader.init(d2.payload);
    expect_eq(r2.readStr(), "out.xml");
    expect_eq(d1.total_bytes + d2.total_bytes, idx);
});

test("test_runner ipc_frame: IpcPayloadReader lenient vs checked truncation behavior", fn() {
    // Lenient reads on truncated payload return 0 / ""
    const short_p: [2]u8 = [0x01, 0x02];
    let r_lenient = IpcPayloadReader.init(short_p[0..]);
    expect_eq(r_lenient.readU32(), @as(u32, 0));

    const bad_str_p: [5]u8 = [0x10, 0x00, 0x00, 0x00, 'a']; // declares len=16, only 1 byte
    let r_str_lenient = IpcPayloadReader.init(bad_str_p[0..]);
    expect_eq(r_str_lenient.readStr(), "");

    // Checked reads return TruncatedFrame and preserve cursor atomically
    let r_checked = IpcPayloadReader.init(bad_str_p[0..]);
    let got_trunc = false;
    * = r_checked.readStrChecked() catch |e| {
        got_trunc = (e == IpcFrameError.TruncatedFrame);
        ""
    };
    expect_eq(got_trunc, true);
    expect_eq(r_checked.remaining(), @as(usize, 5));
});

test("test_runner ipc_frame: PayloadTooLarge, InvalidKind, TruncatedFrame and FrameNotStarted guards", fn() {
    // 1. Header declares > 64 MiB -> PayloadTooLarge
    const huge_hdr: [5]u8 = [0x01, 0x00, 0x00, 0x04, 0x00]; // 0x04000001 = 64 MiB + 1
    let got_huge = false;
    * = decodeIpcFrame(huge_hdr[0..]) catch |e| {
        got_huge = (e == IpcFrameError.PayloadTooLarge);
        IpcDecodedFrame{}
    };
    expect_eq(got_huge, true);

    // 2. Invalid kind = 99 -> InvalidKind
    const bad_kind: [5]u8 = [0x00, 0x00, 0x00, 0x00, 99];
    let got_kind = false;
    * = decodeIpcFrame(bad_kind[0..]) catch |e| {
        got_kind = (e == IpcFrameError.InvalidKind);
        IpcDecodedFrame{}
    };
    expect_eq(got_kind, true);

    // 3. Truncated header (< 5 bytes) -> TruncatedFrame
    const tiny_hdr: [3]u8 = [0, 0, 0];
    let got_tiny = false;
    * = decodeIpcFrame(tiny_hdr[0..]) catch |e| {
        got_tiny = (e == IpcFrameError.TruncatedFrame);
        IpcDecodedFrame{}
    };
    expect_eq(got_tiny, true);

    // 4. Writer writeU32 before begin -> FrameNotStarted
    let buf: [16]u8 = [0; 16];
    let w = IpcFrameWriter.init(buf[0..]);
    let not_started = false;
    * = w.writeU32(1) catch |e| {
        not_started = (e == IpcFrameError.FrameNotStarted);
        @as(usize, 0)
    };
    expect_eq(not_started, true);

    // 5. Writer buffer smaller than 5 bytes -> BufferTooSmall
    let small: [4]u8 = [0; 4];
    let ws = IpcFrameWriter.init(small[0..]);
    let buf_small = false;
    * = ws.begin(IpcFrameKind.ready) catch |e| {
        buf_small = (e == IpcFrameError.BufferTooSmall);
        @as(usize, 0)
    };
    expect_eq(buf_small, true);
});
