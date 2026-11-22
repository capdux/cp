// cap/test/testSubprocessWritable.cp
// Production-grade deep tests for cap:io subprocessWritable.cp
// (Subprocess stdin 6-variant Writable state machine, pipe/buffer flushing, ownership transfer & memory cost)

import { test, assert, assertEq } from "cap:test";
import {
    SUBPROCESS_WRITABLE_BUF_CAP,
    SUBPROCESS_WRITABLE_PIPE_OVERHEAD_BYTES,
    SUBPROCESS_WRITABLE_BUFFER_OVERHEAD_BYTES,
    SubprocessWritableKind,
    SubprocessWritableError,
    SubprocessWritable,
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

test("SubprocessWritable: all 6 variants init, memoryCost & hasPendingActivity", () => {
    const inh = SubprocessWritable.initInherit();
    assertEq(inh.kind, SubprocessWritableKind.Inherit);
    assertEq(inh.memoryCost(), 0);
    assert(!inh.hasPendingActivity());
    assert(!inh.isIgnored());
    assertEq(inh.pendingBytes(), 0);
    assertEq(inh.pendingSlice().len, 0);

    const ign = SubprocessWritable.initIgnore();
    assertEq(ign.kind, SubprocessWritableKind.Ignore);
    assertEq(ign.memoryCost(), 0);
    assert(!ign.hasPendingActivity());
    assert(ign.isIgnored());

    const fd_w = SubprocessWritable.initFd(4) catch {
        assert(false);
        return;
    };
    assertEq(fd_w.kind, SubprocessWritableKind.Fd);
    assertEq(fd_w.fd, 4);
    assertEq(fd_w.memoryCost(), 0);
    assert(!fd_w.hasPendingActivity());

    const memfd_w = SubprocessWritable.initMemfd(11) catch {
        assert(false);
        return;
    };
    assertEq(memfd_w.kind, SubprocessWritableKind.Memfd);
    assertEq(memfd_w.fd, 11);
    assertEq(memfd_w.memoryCost(), 0);
    assert(!memfd_w.hasPendingActivity());

    const pipe_w = SubprocessWritable.initPipe(6) catch {
        assert(false);
        return;
    };
    assertEq(pipe_w.kind, SubprocessWritableKind.Pipe);
    assertEq(pipe_w.fd, 6);
    assert(pipe_w.ref_active);
    assert(pipe_w.subprocess_ref_held);
    assertEq(pipe_w.memoryCost(), SUBPROCESS_WRITABLE_PIPE_OVERHEAD_BYTES);
    assert(!pipe_w.hasPendingActivity());

    const buf_w = SubprocessWritable.initBuffer("stdin payload") catch {
        assert(false);
        return;
    };
    assertEq(buf_w.kind, SubprocessWritableKind.Buffer);
    assert(buf_w.ref_active);
    assert(buf_w.hasPendingActivity());
    assertEq(buf_w.pendingBytes(), 13);
    assertEq(buf_w.memoryCost(), SUBPROCESS_WRITABLE_BUFFER_OVERHEAD_BYTES + 13);
    assert(sliceEql(buf_w.pendingSlice(), "stdin payload"));
});

test("SubprocessWritable: Pipe incremental write, compaction, graceful close & drain", () => {
    let w = SubprocessWritable.initPipe(12) catch {
        assert(false);
        return;
    };

    const n1 = w.writePipe("hello ") catch {
        assert(false);
        return;
    };
    assertEq(n1, 6);
    const n2 = w.writePipe("world!") catch {
        assert(false);
        return;
    };
    assertEq(n2, 6);
    assertEq(w.pendingBytes(), 12);
    assertEq(w.memoryCost(), SUBPROCESS_WRITABLE_PIPE_OVERHEAD_BYTES + 12);

    // Partial flush (5 bytes)
    let out1: [5]u8 = [0; 5];
    const f1 = w.flushTo(out1[0..]) catch {
        assert(false);
        return;
    };
    assertEq(f1, 5);
    assert(sliceEql(out1[0..5], "hello"));
    assertEq(w.pendingBytes(), 7);
    assertEq(w.flushed_bytes, 5);
    assert(sliceEql(w.pendingSlice(), " world!"));

    // Another write triggers prefix compaction
    const n3 = w.writePipe("!!") catch {
        assert(false);
        return;
    };
    assertEq(n3, 2);
    assertEq(w.buf_pos, 0);
    assertEq(w.pendingBytes(), 9);
    assert(sliceEql(w.pendingSlice(), " world!!!"));

    // Graceful close with pending bytes: stays Pipe until drained, rejects further writes
    w.close();
    assertEq(w.kind, SubprocessWritableKind.Pipe);
    assert(w.ended);

    let got_ended_err = false;
    * = w.writePipe("x") catch |e| {
        assertEq(e, SubprocessWritableError.AlreadyEnded);
        got_ended_err = true;
        0
    };
    assert(got_ended_err);

    // Final flush drains remaining bytes and auto-transitions to Ignore
    let out2: [16]u8 = [0; 16];
    const f2 = w.flushTo(out2[0..]) catch {
        assert(false);
        return;
    };
    assertEq(f2, 9);
    assert(sliceEql(out2[0..9], " world!!!"));
    assertEq(w.flushed_bytes, 14);
    assert(w.isIgnored());
    assert(!w.ref_active);
    assert(!w.subprocess_ref_held);
    assertEq(w.memoryCost(), 0);
});

test("SubprocessWritable: Buffer static streaming flush & auto-onClose", () => {
    let w = SubprocessWritable.initBuffer("abcdefghij") catch {
        assert(false);
        return;
    };
    assert(w.hasPendingActivity());

    let c1: [4]u8 = [0; 4];
    const r1 = w.flushTo(c1[0..]) catch {
        assert(false);
        return;
    };
    assertEq(r1, 4);
    assert(sliceEql(c1[0..4], "abcd"));
    assertEq(w.pendingBytes(), 6);
    assertEq(w.kind, SubprocessWritableKind.Buffer);
    assert(w.hasPendingActivity());

    let c2: [10]u8 = [0; 10];
    const r2 = w.flushTo(c2[0..]) catch {
        assert(false);
        return;
    };
    assertEq(r2, 6);
    assert(sliceEql(c2[0..6], "efghij"));
    assertEq(w.flushed_bytes, 10);
    assert(w.isIgnored());
    assert(!w.hasPendingActivity());
    assert(!w.ref_active);
    assertEq(w.memoryCost(), 0);
});

test("SubprocessWritable: ref/unref & takePipeHandle ownership transfer", () => {
    let w_live = SubprocessWritable.initPipe(21) catch {
        assert(false);
        return;
    };
    w_live.unref();
    assert(!w_live.ref_active);
    w_live.ref();
    assert(w_live.ref_active);

    // Non-pipe/buffer ref/unref are safe no-ops
    let inh = SubprocessWritable.initInherit();
    inh.ref();
    assert(!inh.ref_active);
    inh.unref();
    assert(!inh.ref_active);

    // Transfer ownership while subprocess is still alive: keeps subprocess_ref_held
    const fd1 = w_live.takePipeHandle(false) catch {
        assert(false);
        return;
    };
    assertEq(fd1, 21);
    assert(w_live.isIgnored());
    assert(w_live.detached_to_caller);
    assert(w_live.subprocess_ref_held);

    // Transfer ownership after subprocess exited: balances and drops subprocess_ref_held
    let w_exited = SubprocessWritable.initPipe(22) catch {
        assert(false);
        return;
    };
    const fd2 = w_exited.takePipeHandle(true) catch {
        assert(false);
        return;
    };
    assertEq(fd2, 22);
    assert(w_exited.isIgnored());
    assert(w_exited.detached_to_caller);
    assert(!w_exited.subprocess_ref_held);
    assert(!w_exited.ref_active);
});

test("SubprocessWritable: close vs finalize semantics across variants", () => {
    // Empty pipe closes immediately
    let p = SubprocessWritable.initPipe(30) catch {
        assert(false);
        return;
    };
    p.close();
    assert(p.isIgnored());
    assert(!p.subprocess_ref_held);

    // Fd: close transitions to Ignore, finalize leaves Fd untouched
    let f1 = SubprocessWritable.initFd(31) catch {
        assert(false);
        return;
    };
    f1.close();
    assert(f1.isIgnored());

    let f2 = SubprocessWritable.initFd(32) catch {
        assert(false);
        return;
    };
    f2.finalize();
    assertEq(f2.kind, SubprocessWritableKind.Fd);
    assertEq(f2.fd, 32);

    // Memfd & Buffer: finalize transitions to Ignore
    let m = SubprocessWritable.initMemfd(33) catch {
        assert(false);
        return;
    };
    m.finalize();
    assert(m.isIgnored());

    let b = SubprocessWritable.initBuffer("tmp") catch {
        assert(false);
        return;
    };
    b.finalize();
    assert(b.isIgnored());
    assert(!b.ref_active);

    // Inherit: both close and finalize preserve Inherit
    let inh = SubprocessWritable.initInherit();
    inh.close();
    inh.finalize();
    assertEq(inh.kind, SubprocessWritableKind.Inherit);
});

test("SubprocessWritable: boundary & error guards (InvalidFd, Overflow, NotPipe, NotWritable)", () => {
    // InvalidFd checks
    let bad_fd = false;
    * = SubprocessWritable.initFd(-1) catch |e| {
        assertEq(e, SubprocessWritableError.InvalidFd);
        bad_fd = true;
        SubprocessWritable.initIgnore()
    };
    assert(bad_fd);

    let bad_memfd = false;
    * = SubprocessWritable.initMemfd(-5) catch |e| {
        assertEq(e, SubprocessWritableError.InvalidFd);
        bad_memfd = true;
        SubprocessWritable.initIgnore()
    };
    assert(bad_memfd);

    let bad_pipe = false;
    * = SubprocessWritable.initPipe(-2) catch |e| {
        assertEq(e, SubprocessWritableError.InvalidFd);
        bad_pipe = true;
        SubprocessWritable.initIgnore()
    };
    assert(bad_pipe);

    // BufferOverflow on Pipe write
    let big: [SUBPROCESS_WRITABLE_BUF_CAP]u8 = [0x41; SUBPROCESS_WRITABLE_BUF_CAP];
    let pw = SubprocessWritable.initPipe(40) catch {
        assert(false);
        return;
    };
    const wfull = pw.writePipe(big[0..]) catch {
        assert(false);
        return;
    };
    assertEq(wfull, SUBPROCESS_WRITABLE_BUF_CAP);

    let got_overflow = false;
    * = pw.writePipe("!") catch |e| {
        assertEq(e, SubprocessWritableError.BufferOverflow);
        got_overflow = true;
        0
    };
    assert(got_overflow);

    // NotPipe / NotWritable guards on Ignore & Buffer
    let ign = SubprocessWritable.initIgnore();
    let got_not_pipe = false;
    * = ign.writePipe("x") catch |e| {
        assertEq(e, SubprocessWritableError.NotPipe);
        got_not_pipe = true;
        0
    };
    assert(got_not_pipe);

    let out: [8]u8 = [0; 8];
    let got_not_writable = false;
    * = ign.flushTo(out[0..]) catch |e| {
        assertEq(e, SubprocessWritableError.NotWritable);
        got_not_writable = true;
        0
    };
    assert(got_not_writable);

    let got_take_err = false;
    * = ign.takePipeHandle(false) catch |e| {
        assertEq(e, SubprocessWritableError.NotPipe);
        got_take_err = true;
        -1
    };
    assert(got_take_err);
});
