// cap/test/testStaticPipeWriter.cp
// Production-grade deep tests for cap:io staticPipeWriter.cp
// (Subprocess static stdin pipe writer state machine, partial writes & ref lifecycle)

import { test, assert, assertEq } from "cap:test";
import {
    StaticPipeState,
    StaticPipeWriteStatus,
    StaticPipeWriterError,
    StaticPipeWriter,
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

test("StaticPipeWriter: empty source closes immediately on start()", () => {
    let w = StaticPipeWriter.init("");
    assertEq(w.state, StaticPipeState.Idle);
    assertEq(w.totalBytes(), 0);
    assertEq(w.progressPermille(), 1000);

    const step = w.start() catch {
        assert(false);
        return;
    };
    assert(step.should_close);
    assertEq(step.state, StaticPipeState.Closed);
    assertEq(w.state, StaticPipeState.Closed);
    assert(w.source_detached);
    assert(w.isTerminal());
    assertEq(w.ref_count, 2); // init(1) + start(1)
});

test("StaticPipeWriter: partial onWrite advancement, over-length clamping & auto-close", () => {
    let w = StaticPipeWriter.init("0123456789");
    const s0 = w.start() catch {
        assert(false);
        return;
    };
    assert(!s0.should_close);
    assertEq(s0.remaining_bytes, 10);
    assertEq(w.progressPermille(), 0);

    const s1 = w.onWrite(3, StaticPipeWriteStatus.Pending) catch {
        assert(false);
        return;
    };
    assertEq(s1.bytes_advanced, 3);
    assertEq(s1.remaining_bytes, 7);
    assert(!s1.should_close);
    assert(sliceEql(w.getBuffer(), "3456789"));
    assertEq(w.progressPermille(), 300);

    // Passing amount larger than remaining must safely clamp to 7
    const s2 = w.onWrite(999, StaticPipeWriteStatus.Drained) catch {
        assert(false);
        return;
    };
    assertEq(s2.bytes_advanced, 7);
    assertEq(s2.remaining_bytes, 0);
    assert(s2.should_close);
    assertEq(s2.state, StaticPipeState.Closed);
    assertEq(w.bytes_written, 10);
    assertEq(w.write_calls, 2);
    assertEq(w.progressPermille(), 1000);
    assert(w.source_detached);
});

test("StaticPipeWriter: EndOfFile terminates early and detaches source", () => {
    let w = StaticPipeWriter.init("abcdefghij");
    * = w.start() catch {
        assert(false);
        return;
    };

    const s = w.onWrite(4, StaticPipeWriteStatus.EndOfFile) catch {
        assert(false);
        return;
    };
    assertEq(s.bytes_advanced, 4);
    assert(s.should_close);
    assertEq(s.state, StaticPipeState.Closed);
    assertEq(w.bytes_written, 4);
    assertEq(w.remaining(), 0);
    assert(w.source_detached);
});

test("StaticPipeWriter: drainToBuf reassembles full payload across small chunks", () => {
    const payload = "Hello, StaticPipeWriter!";
    let w = StaticPipeWriter.init(payload);
    * = w.start() catch {
        assert(false);
        return;
    };

    let assembled: [64]u8 = [0; 64];
    let total: usize = 0;
    let chunk: [5]u8 = [0; 5];

    while (!w.isTerminal()) {
        const n = w.drainToBuf(chunk[0..]) catch {
            assert(false);
            return;
        };
        let i: usize = 0;
        while (i < n) {
            assembled[total + i] = chunk[i];
            i += 1;
        }
        total += n;
    }

    assertEq(total, payload.len);
    assert(sliceEql(assembled[0..total], payload));
    assertEq(w.state, StaticPipeState.Closed);
});

test("StaticPipeWriter: onError detaches source and preserves Errored across close()", () => {
    let w = StaticPipeWriter.init("payload");
    * = w.start() catch {
        assert(false);
        return;
    };

    w.onError(32); // EPIPE
    assertEq(w.state, StaticPipeState.Errored);
    assertEq(w.last_errno, 32);
    assert(w.source_detached);
    assertEq(w.remaining(), 0);
    assert(w.isTerminal());

    // Calling close() after onError must keep Errored state intact
    w.close();
    assertEq(w.state, StaticPipeState.Errored);
});

test("StaticPipeWriter: ref/deref lifecycle & invalid transition guards", () => {
    let w = StaticPipeWriter.init("abc");

    // onWrite before start -> NotWriting
    let saw_not_writing = false;
    * = w.onWrite(1, StaticPipeWriteStatus.Pending) catch e => {
        if (e == StaticPipeWriterError.NotWriting) saw_not_writing = true;
    };
    assert(saw_not_writing);

    * = w.start() catch {
        assert(false);
        return;
    };
    assertEq(w.ref_count, 2);

    // Second start -> AlreadyStarted
    let saw_already = false;
    * = w.start() catch e => {
        if (e == StaticPipeWriterError.AlreadyStarted) saw_already = true;
    };
    assert(saw_already);

    // Ref counting down to zero
    assert(!w.deref());
    assertEq(w.ref_count, 1);
    assert(w.deref());
    assertEq(w.ref_count, 0);
    assert(w.deref()); // saturates at 0 without underflow
    assertEq(w.ref_count, 0);
});
