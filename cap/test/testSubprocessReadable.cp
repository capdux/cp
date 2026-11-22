// cap/test/testSubprocessReadable.cp
// Production-grade deep tests for cap:io subprocessReadable.cp
// (Subprocess stdout/stderr 7-variant Readable state machine, pipe-to-buffer transition & memory cost)

import { test, assert, assertEq } from "cap:test";
import {
    SUBPROCESS_READABLE_BUF_CAP,
    SUBPROCESS_PIPE_OVERHEAD_BYTES,
    SubprocessReadableKind,
    SubprocessReadableError,
    SubprocessReadable,
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

test("SubprocessReadable: all 7 variants init, memoryCost & hasPendingActivity", () => {
    const inh = SubprocessReadable.initInherit();
    assertEq(inh.kind, SubprocessReadableKind.Inherit);
    assertEq(inh.memoryCost(), 0);
    assert(!inh.hasPendingActivity());
    assertEq(inh.slice().len, 0);

    const ign = SubprocessReadable.initIgnore();
    assertEq(ign.kind, SubprocessReadableKind.Ignore);
    assertEq(ign.memoryCost(), 0);
    assert(!ign.hasPendingActivity());

    const fd_r = SubprocessReadable.initFd(3);
    assertEq(fd_r.kind, SubprocessReadableKind.Fd);
    assertEq(fd_r.fd, 3);
    assertEq(fd_r.memoryCost(), 0);

    const memfd_r = SubprocessReadable.initMemfd(9);
    assertEq(memfd_r.kind, SubprocessReadableKind.Memfd);
    assertEq(memfd_r.fd, 9);
    assertEq(memfd_r.memoryCost(), 0);

    const pipe_r = SubprocessReadable.initPipe(11, 0);
    assertEq(pipe_r.kind, SubprocessReadableKind.Pipe);
    assertEq(pipe_r.fd, 11);
    assertEq(pipe_r.memoryCost(), SUBPROCESS_PIPE_OVERHEAD_BYTES);
    assert(pipe_r.hasPendingActivity());
    assert(pipe_r.ref_active);

    const buf_r = SubprocessReadable.initBuffer("hello") catch {
        assert(false);
        return;
    };
    assertEq(buf_r.kind, SubprocessReadableKind.Buffer);
    assertEq(buf_r.memoryCost(), 5);
    assert(!buf_r.hasPendingActivity());
    assert(sliceEql(buf_r.slice(), "hello"));
});

test("SubprocessReadable: pipe chunk accumulation & finishPipeToBuffer transition", () => {
    let r = SubprocessReadable.initPipe(4, 1024);
    const n1 = r.appendPipeChunk("line1\n") catch {
        assert(false);
        return;
    };
    assertEq(n1, 6);
    assertEq(r.memoryCost(), SUBPROCESS_PIPE_OVERHEAD_BYTES + 6);

    const n2 = r.appendPipeChunk("line2\n") catch {
        assert(false);
        return;
    };
    assertEq(n2, 12);
    assert(sliceEql(r.slice(), "line1\nline2\n"));

    const final_data = r.finishPipeToBuffer() catch {
        assert(false);
        return;
    };
    assert(sliceEql(final_data, "line1\nline2\n"));
    assertEq(r.kind, SubprocessReadableKind.Buffer);
    assertEq(r.fd, -1);
    assert(!r.hasPendingActivity());
    assert(!r.ref_active);
    assertEq(r.memoryCost(), 12);
    assert(sliceEql(r.slice(), "line1\nline2\n"));
});

test("SubprocessReadable: ref/unref affect only Pipe variant", () => {
    let p = SubprocessReadable.initPipe(5, 0);
    assert(p.ref_active);
    p.unref();
    assert(!p.ref_active);
    p.ref();
    assert(p.ref_active);

    let inh = SubprocessReadable.initInherit();
    inh.ref();
    assert(!inh.ref_active);
    inh.unref();
    assert(!inh.ref_active);
});

test("SubprocessReadable: max_bytes cap enforces BufferOverflow without corrupting prior data", () => {
    let r = SubprocessReadable.initPipe(6, 8);
    * = r.appendPipeChunk("12345") catch {
        assert(false);
        return;
    };

    let saw_overflow = false;
    * = r.appendPipeChunk("6789") catch e => {
        if (e == SubprocessReadableError.BufferOverflow) saw_overflow = true;
    };
    assert(saw_overflow);
    assert(r.truncated);
    assert(sliceEql(r.slice(), "12345"));

    // Remaining 3 bytes still fit cleanly
    const n = r.appendPipeChunk("678") catch {
        assert(false);
        return;
    };
    assertEq(n, 8);
    assert(sliceEql(r.slice(), "12345678"));
    assertEq(SUBPROCESS_READABLE_BUF_CAP, 4096);
});

test("SubprocessReadable: close() clears state and guards against post-close mutations", () => {
    let r = SubprocessReadable.initPipe(8, 256);
    * = r.appendPipeChunk("abc") catch {
        assert(false);
        return;
    };

    r.close();
    assert(r.isClosed());
    assertEq(r.kind, SubprocessReadableKind.Closed);
    assertEq(r.memoryCost(), 0);
    assert(!r.hasPendingActivity());
    assert(!r.ref_active);
    assertEq(r.slice().len, 0);

    let saw_closed1 = false;
    * = r.appendPipeChunk("x") catch e => {
        if (e == SubprocessReadableError.AlreadyClosed) saw_closed1 = true;
    };
    assert(saw_closed1);

    let saw_closed2 = false;
    * = r.finishPipeToBuffer() catch e => {
        if (e == SubprocessReadableError.AlreadyClosed) saw_closed2 = true;
    };
    assert(saw_closed2);
});

test("SubprocessReadable: non-pipe variants reject pipe-only operations with NotPipe", () => {
    let inh = SubprocessReadable.initInherit();
    let saw_not_pipe1 = false;
    * = inh.appendPipeChunk("x") catch e => {
        if (e == SubprocessReadableError.NotPipe) saw_not_pipe1 = true;
    };
    assert(saw_not_pipe1);

    let buf_r = SubprocessReadable.initBuffer("done") catch {
        assert(false);
        return;
    };
    let saw_not_pipe2 = false;
    * = buf_r.finishPipeToBuffer() catch e => {
        if (e == SubprocessReadableError.NotPipe) saw_not_pipe2 = true;
    };
    assert(saw_not_pipe2);
});
