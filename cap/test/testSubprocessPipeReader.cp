// cap/test/testSubprocessPipeReader.cp
// Production-grade deep tests for cap:io subprocessPipeReader.cp
// (Subprocess buffered PipeReader state machine: Pending/Done/Errored, ref-counting, MaxBuf, detach & consumption)

import { test, assert, assertEq } from "cap:test";
import {
    SUBPROCESS_PIPE_READER_BUF_CAP,
    SUBPROCESS_PIPE_READER_OVERHEAD_BYTES,
    SubprocessPipeStreamKind,
    SubprocessPipeReaderState,
    SubprocessPipeReaderError,
    SubprocessPipeReader,
} from "cap:io";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("SubprocessPipeReader: init, start, ref-counting & memoryCost", () => {
    let pr = SubprocessPipeReader.init(SubprocessPipeStreamKind.Stdout, 7, 1024) catch {
        assert(false);
        return;
    };
    assertEq(pr.stream_kind, SubprocessPipeStreamKind.Stdout);
    assertEq(pr.state, SubprocessPipeReaderState.Pending);
    assertEq(pr.fd, 7);
    assert(pr.attached);
    assertEq(pr.ref_count, 1);
    assert(pr.hasPendingActivity());
    assert(!pr.isTerminal());
    assertEq(pr.memoryCost(), SUBPROCESS_PIPE_READER_OVERHEAD_BYTES);

    const started = pr.start() catch {
        assert(false);
        return;
    };
    assert(started);
    assert(pr.started);
    assert(pr.watching);
    assert(pr.loop_ref);
    assertEq(pr.ref_count, 2);

    // Double start rejected
    let dup_start = false;
    * = pr.start() catch |e| {
        assertEq(e, SubprocessPipeReaderError.AlreadyStarted);
        dup_start = true;
        false
    };
    assert(dup_start);

    // updateRef toggles loop_ref independently
    pr.updateRef(false);
    assert(!pr.loop_ref);
    pr.updateRef(true);
    assert(pr.loop_ref);
});

test("SubprocessPipeReader: chunked read, onReaderDone & single-shot takeBuffer", () => {
    let pr = SubprocessPipeReader.init(SubprocessPipeStreamKind.Stderr, 8, 0) catch {
        assert(false);
        return;
    };
    * = pr.start() catch {
        assert(false);
        return;
    };

    const l1 = pr.onReadChunk("warn: ") catch {
        assert(false);
        return;
    };
    assertEq(l1, 6);
    const l2 = pr.onReadChunk("disk low\n") catch {
        assert(false);
        return;
    };
    assertEq(l2, 15);
    assert(sliceEql(pr.peekSlice(), "warn: disk low\n"));
    assertEq(pr.memoryCost(), SUBPROCESS_PIPE_READER_OVERHEAD_BYTES + 15);

    // takeBuffer before Done returns NotDone
    let tmp: [32]u8 = [0; 32];
    let got_not_done = false;
    * = pr.takeBuffer(tmp[0..]) catch |e| {
        assertEq(e, SubprocessPipeReaderError.NotDone);
        got_not_done = true;
        0
    };
    assert(got_not_done);

    // EOF transitions to Done, detaches from subprocess and decrements ref_count
    pr.onReaderDone();
    assertEq(pr.state, SubprocessPipeReaderState.Done);
    assert(pr.isTerminal());
    assert(!pr.attached);
    assert(!pr.watching);
    assert(!pr.loop_ref);
    assert(!pr.hasPendingActivity());
    assertEq(pr.ref_count, 1);

    const n = pr.takeBuffer(tmp[0..]) catch {
        assert(false);
        return;
    };
    assertEq(n, 15);
    assert(sliceEql(tmp[0..15], "warn: disk low\n"));
    assert(pr.consumed);
    assertEq(pr.memoryCost(), 0);
    assertEq(pr.peekSlice().len, 0);

    // Second consumption rejected with AlreadyConsumed
    let got_consumed = false;
    * = pr.takeBuffer(tmp[0..]) catch |e| {
        assertEq(e, SubprocessPipeReaderError.AlreadyConsumed);
        got_consumed = true;
        0
    };
    assert(got_consumed);
});

test("SubprocessPipeReader: takeStreamSnapshot from Pending, Done & Errored", () => {
    // 1. From Pending: detaches, copies buffered bytes, transitions to Done & consumed
    let p1 = SubprocessPipeReader.init(SubprocessPipeStreamKind.Stdout, 10, 0) catch {
        assert(false);
        return;
    };
    * = p1.start() catch {
        assert(false);
        return;
    };
    * = p1.onReadChunk("streamed") catch {
        assert(false);
        return;
    };
    let out1: [16]u8 = [0; 16];
    const n1 = p1.takeStreamSnapshot(out1[0..]) catch {
        assert(false);
        return;
    };
    assertEq(n1, 8);
    assert(sliceEql(out1[0..8], "streamed"));
    assert(!p1.attached);
    assertEq(p1.state, SubprocessPipeReaderState.Done);
    assert(p1.consumed);

    // 2. From Errored: returns 0 (cancelled empty stream) and marks consumed
    let p2 = SubprocessPipeReader.init(SubprocessPipeStreamKind.Stderr, 11, 0) catch {
        assert(false);
        return;
    };
    * = p2.onReadChunk("partial") catch {
        assert(false);
        return;
    };
    p2.onReaderError(5); // EIO
    assertEq(p2.state, SubprocessPipeReaderState.Errored);
    assertEq(p2.err_code, 5);
    assertEq(p2.memoryCost(), 0);
    assertEq(p2.peekSlice().len, 0);

    let out2: [16]u8 = [0; 16];
    const n2 = p2.takeStreamSnapshot(out2[0..]) catch {
        assert(false);
        return;
    };
    assertEq(n2, 0);
    assert(p2.consumed);
});

test("SubprocessPipeReader: close & idempotent detach/deref saturation", () => {
    let pr = SubprocessPipeReader.init(SubprocessPipeStreamKind.Stdout, 12, 0) catch {
        assert(false);
        return;
    };
    * = pr.onReadChunk("abc") catch {
        assert(false);
        return;
    };
    pr.close();
    assertEq(pr.state, SubprocessPipeReaderState.Done);
    assert(!pr.attached);
    assertEq(pr.ref_count, 0);

    // Second close & detach are idempotent no-ops; deref saturates at 0
    pr.close();
    pr.detach();
    assertEq(pr.deref(), 0);
    assert(sliceEql(pr.peekSlice(), "abc"));
});

test("SubprocessPipeReader: MaxBufExceeded, BufferOverflow, InvalidFd & NotPending guards", () => {
    // InvalidFd
    let got_bad_fd = false;
    * = SubprocessPipeReader.init(SubprocessPipeStreamKind.Stdout, -1, 0) catch |e| {
        assertEq(e, SubprocessPipeReaderError.InvalidFd);
        got_bad_fd = true;
        SubprocessPipeReader{}
    };
    assert(got_bad_fd);

    // MaxBufExceeded
    let capped = SubprocessPipeReader.init(SubprocessPipeStreamKind.Stdout, 15, 8) catch {
        assert(false);
        return;
    };
    * = capped.onReadChunk("12345") catch {
        assert(false);
        return;
    };
    let got_maxbuf = false;
    * = capped.onReadChunk("6789") catch |e| {
        assertEq(e, SubprocessPipeReaderError.MaxBufExceeded);
        got_maxbuf = true;
        0
    };
    assert(got_maxbuf);

    // BufferOverflow on full inline cap
    let full = SubprocessPipeReader.init(SubprocessPipeStreamKind.Stdout, 16, 0) catch {
        assert(false);
        return;
    };
    let big: [SUBPROCESS_PIPE_READER_BUF_CAP]u8 = [0x5A; SUBPROCESS_PIPE_READER_BUF_CAP];
    * = full.onReadChunk(big[0..]) catch {
        assert(false);
        return;
    };
    let got_overflow = false;
    * = full.onReadChunk("!") catch |e| {
        assertEq(e, SubprocessPipeReaderError.BufferOverflow);
        got_overflow = true;
        0
    };
    assert(got_overflow);

    // NotPending after Done
    capped.onReaderDone();
    let got_not_pending = false;
    * = capped.onReadChunk("x") catch |e| {
        assertEq(e, SubprocessPipeReaderError.NotPending);
        got_not_pending = true;
        0
    };
    assert(got_not_pending);

    // BufferOverflow on undersized output buffer in takeBuffer
    let tiny: [2]u8 = [0; 2];
    let got_out_small = false;
    * = capped.takeBuffer(tiny[0..]) catch |e| {
        assertEq(e, SubprocessPipeReaderError.BufferOverflow);
        got_out_small = true;
        0
    };
    assert(got_out_small);
});
