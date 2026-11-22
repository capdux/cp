// cap/test/testDirnameBuiltin.cp
// Production-grade deep tests for cap:shell dirnameBuiltin.cp
// (POSIX/Bun shell dirname builtin state machine, POSIX dirname evaluation & async I/O completion)

import { test, assert, assertEq } from "cap:test";
import {
    DIRNAME_USAGE,
    DirnameOutputKind,
    DirnameState,
    DirnameError,
    DirnameResult,
    evalPosixDirname,
    DirnameBuiltin,
    runDirnameBuiltin,
} from "cap:shell";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("evalPosixDirname: POSIX edge cases (empty, root, trailing & duplicate slashes)", () => {
    assert(sliceEql(evalPosixDirname(""), "."));
    assert(sliceEql(evalPosixDirname("/"), "/"));
    assert(sliceEql(evalPosixDirname("///"), "/"));
    assert(sliceEql(evalPosixDirname("foo"), "."));
    assert(sliceEql(evalPosixDirname("foo///"), "."));
    assert(sliceEql(evalPosixDirname("/foo"), "/"));
    assert(sliceEql(evalPosixDirname("/foo/"), "/"));
    assert(sliceEql(evalPosixDirname("/usr/local/bin/"), "/usr/local"));
    assert(sliceEql(evalPosixDirname("a//b"), "a"));
    assert(sliceEql(evalPosixDirname("///a///b///"), "///a"));
});

test("DirnameBuiltin: synchronous multi-path batch execution", () => {
    const args: [3][]const u8 = ["/usr/bin/env", "a/b/c/", "plain"];
    let out: [128]u8 = [0; 128];
    const res = runDirnameBuiltin(args[0..], out[0..]) catch {
        assert(false);
        return;
    };
    assertEq(res.output_kind, DirnameOutputKind.Stdout);
    assertEq(res.exit_code, 0);
    assertEq(res.paths_processed, 3);
    assert(!res.needs_async_io);
    assert(sliceEql(out[0..res.bytes_written], "/usr/bin\na/b\n.\n"));
});

test("DirnameBuiltin: zero arguments emits usage to stderr with exit code 1", () => {
    const empty: [0][]const u8 = [];
    let out: [64]u8 = [0; 64];
    let b = DirnameBuiltin.init();
    const res = b.start(empty[0..], out[0..]) catch {
        assert(false);
        return;
    };
    assertEq(res.output_kind, DirnameOutputKind.Stderr);
    assertEq(res.exit_code, 1);
    assertEq(res.paths_processed, 0);
    assertEq(b.state, DirnameState.Err);
    assert(b.isTerminal());
    assert(sliceEql(out[0..res.bytes_written], DIRNAME_USAGE));
});

test("DirnameBuiltin: async I/O writer completion (stdout success, stderr & io_failed)", () => {
    const args: [1][]const u8 = ["/opt/cp/bin"];
    let out: [64]u8 = [0; 64];

    // 1. WaitingStdout -> Done (exit 0)
    let b1 = DirnameBuiltin.init();
    const r1 = b1.startAsync(args[0..], out[0..]) catch {
        assert(false);
        return;
    };
    assert(r1.needs_async_io);
    assertEq(b1.state, DirnameState.WaitingStdout);
    assert(!b1.isTerminal());
    const code1 = b1.onIOWriterChunk(false) catch {
        assert(false);
        return;
    };
    assertEq(code1, 0);
    assertEq(b1.state, DirnameState.Done);
    assert(b1.isTerminal());

    // 2. WaitingStdout + io_failed -> Err (exit 1)
    let b2 = DirnameBuiltin.init();
    * = b2.startAsync(args[0..], out[0..]) catch {
        assert(false);
        return;
    };
    const code2 = b2.onIOWriterChunk(true) catch {
        assert(false);
        return;
    };
    assertEq(code2, 1);
    assertEq(b2.state, DirnameState.Err);

    // 3. WaitingStderr (zero args) -> Err (exit 1)
    const empty: [0][]const u8 = [];
    let b3 = DirnameBuiltin.init();
    * = b3.startAsync(empty[0..], out[0..]) catch {
        assert(false);
        return;
    };
    assertEq(b3.state, DirnameState.WaitingStderr);
    const code3 = b3.onIOWriterChunk(false) catch {
        assert(false);
        return;
    };
    assertEq(code3, 1);
    assertEq(b3.state, DirnameState.Err);
});

test("DirnameBuiltin: BufferTooSmall & InvalidState guards", () => {
    const args: [1][]const u8 = ["/very/long/directory/path/file"];
    let tiny: [4]u8 = [0; 4];
    let got_small = false;
    * = runDirnameBuiltin(args[0..], tiny[0..]) catch |e| {
        assertEq(e, DirnameError.BufferTooSmall);
        got_small = true;
        DirnameResult{}
    };
    assert(got_small);

    // Calling onIOWriterChunk in Idle returns InvalidState
    let b = DirnameBuiltin.init();
    let got_idle_err = false;
    * = b.onIOWriterChunk(false) catch |e| {
        assertEq(e, DirnameError.InvalidState);
        got_idle_err = true;
        0
    };
    assert(got_idle_err);

    // Calling start twice returns InvalidState
    let out: [64]u8 = [0; 64];
    * = b.start(args[0..], out[0..]) catch {
        assert(false);
        return;
    };
    let got_reenter = false;
    * = b.start(args[0..], out[0..]) catch |e| {
        assertEq(e, DirnameError.InvalidState);
        got_reenter = true;
        DirnameResult{}
    };
    assert(got_reenter);
});
