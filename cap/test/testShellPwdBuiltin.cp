// cap/test/testShellPwdBuiltin.cp
// Production-grade deep tests for cap:shell pwdBuiltin.cp
// (POSIX/Bun shell pwd builtin state machine, -L/-P flag parser & zero-alloc formatter)

import { test, assert, assertEq } from "cap:test";
import {
    PwdOutputKind,
    PwdState,
    PwdMode,
    PwdError,
    PwdBuiltin,
    parsePwdArgs,
    runPwdBuiltin,
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

test("parsePwdArgs: -L, -P, combined flags, -- terminator & error guards", () => {
    const empty: [0][]const u8 = [];
    const m0 = parsePwdArgs(empty[0..]) catch {
        assert(false);
        return;
    };
    assertEq(m0, PwdMode.Logical);

    const p_args: [1][]const u8 = ["-P"];
    const m1 = parsePwdArgs(p_args[0..]) catch {
        assert(false);
        return;
    };
    assertEq(m1, PwdMode.Physical);

    const combined: [2][]const u8 = ["-LP", "-PL"];
    const m2 = parsePwdArgs(combined[0..]) catch {
        assert(false);
        return;
    };
    assertEq(m2, PwdMode.Logical);

    const with_dashdash: [2][]const u8 = ["-P", "--"];
    const m3 = parsePwdArgs(with_dashdash[0..]) catch {
        assert(false);
        return;
    };
    assertEq(m3, PwdMode.Physical);

    // Positional argument after -- is rejected as TooManyArguments
    const after_dashdash: [2][]const u8 = ["--", "-L"];
    let saw_too_many = false;
    * = parsePwdArgs(after_dashdash[0..]) catch e => {
        if (e == PwdError.TooManyArguments) saw_too_many = true;
    };
    assert(saw_too_many);

    // Unknown short flag is rejected as InvalidOption
    const bad_opt: [1][]const u8 = ["-x"];
    let saw_invalid = false;
    * = parsePwdArgs(bad_opt[0..]) catch e => {
        if (e == PwdError.InvalidOption) saw_invalid = true;
    };
    assert(saw_invalid);
});

test("PwdBuiltin.start: logical vs physical selection & empty-path fallback", () => {
    let out: [128]u8 = [0; 128];

    // Default logical mode
    let b1 = PwdBuiltin.init();
    const empty: [0][]const u8 = [];
    const r1 = b1.start(empty[0..], "/home/dev/link", "/data/real", out[0..]) catch {
        assert(false);
        return;
    };
    assertEq(r1.exit_code, 0);
    assertEq(r1.output_kind, PwdOutputKind.Stdout);
    assertEq(r1.mode, PwdMode.Logical);
    assert(sliceEql(out[0..r1.bytes_written], "/home/dev/link\n"));
    assertEq(b1.state, PwdState.Done);
    assert(b1.isTerminal());

    // Physical mode
    let b2 = PwdBuiltin.init();
    const p_args: [1][]const u8 = ["-P"];
    const r2 = b2.start(p_args[0..], "/home/dev/link", "/data/real", out[0..]) catch {
        assert(false);
        return;
    };
    assertEq(r2.exit_code, 0);
    assertEq(r2.mode, PwdMode.Physical);
    assert(sliceEql(out[0..r2.bytes_written], "/data/real\n"));

    // Empty logical falls back to physical; empty physical falls back to logical
    let b3 = PwdBuiltin.init();
    const r3 = b3.start(empty[0..], "", "/data/real", out[0..]) catch {
        assert(false);
        return;
    };
    assert(sliceEql(out[0..r3.bytes_written], "/data/real\n"));

    let b4 = PwdBuiltin.init();
    const r4 = b4.start(p_args[0..], "/home/only", "", out[0..]) catch {
        assert(false);
        return;
    };
    assert(sliceEql(out[0..r4.bytes_written], "/home/only\n"));
});

test("PwdBuiltin.start: stderr diagnostics and exit code 1 on extra args or invalid flag", () => {
    let out: [128]u8 = [0; 128];

    let b1 = PwdBuiltin.init();
    const extra: [1][]const u8 = ["extra_arg"];
    const r1 = b1.start(extra[0..], "/tmp", "/tmp", out[0..]) catch {
        assert(false);
        return;
    };
    assertEq(r1.exit_code, 1);
    assertEq(r1.output_kind, PwdOutputKind.Stderr);
    assertEq(b1.state, PwdState.Err);
    assert(b1.isTerminal());
    assert(sliceEql(out[0..r1.bytes_written], "pwd: too many arguments\n"));

    let b2 = PwdBuiltin.init();
    const bad: [1][]const u8 = ["-Z"];
    const r2 = b2.start(bad[0..], "/tmp", "/tmp", out[0..]) catch {
        assert(false);
        return;
    };
    assertEq(r2.exit_code, 1);
    assertEq(r2.output_kind, PwdOutputKind.Stderr);
    assertEq(b2.state, PwdState.Err);
    assert(sliceEql(out[0..r2.bytes_written], "pwd: invalid option\n"));
});

test("PwdBuiltin.start: BufferTooSmall guard on tight destination buffer", () => {
    let b = PwdBuiltin.init();
    const empty: [0][]const u8 = [];
    // "/tmp" + "\n" requires 5 bytes; 4 bytes must fail with BufferTooSmall
    let tiny: [4]u8 = [0; 4];
    let saw_small = false;
    * = b.start(empty[0..], "/tmp", "/tmp", tiny[0..]) catch e => {
        if (e == PwdError.BufferTooSmall) saw_small = true;
    };
    assert(saw_small);
});

test("PwdBuiltin async state machine: startAsync & onIoComplete transitions", () => {
    // WaitingStdout + io_ok=true -> Done (exit 0)
    let b1 = PwdBuiltin.init();
    assert(!b1.isTerminal());
    const st1 = b1.startAsync(PwdOutputKind.Stdout) catch {
        assert(false);
        return;
    };
    assertEq(st1, PwdState.WaitingStdout);
    const code1 = b1.onIoComplete(true) catch {
        assert(false);
        return;
    };
    assertEq(code1, 0);
    assertEq(b1.state, PwdState.Done);

    // WaitingStdout + io_ok=false -> Err (exit 1)
    let b2 = PwdBuiltin.init();
    * = b2.startAsync(PwdOutputKind.Stdout) catch {
        assert(false);
        return;
    };
    const code2 = b2.onIoComplete(false) catch {
        assert(false);
        return;
    };
    assertEq(code2, 1);
    assertEq(b2.state, PwdState.Err);

    // WaitingStderr + io_ok=true -> Err (exit 1)
    let b3 = PwdBuiltin.init();
    * = b3.startAsync(PwdOutputKind.Stderr) catch {
        assert(false);
        return;
    };
    const code3 = b3.onIoComplete(true) catch {
        assert(false);
        return;
    };
    assertEq(code3, 1);
    assertEq(b3.state, PwdState.Err);

    // Invalid state transition guards
    let b4 = PwdBuiltin.init();
    let saw_invalid = false;
    * = b4.onIoComplete(true) catch e => {
        if (e == PwdError.InvalidState) saw_invalid = true;
    };
    assert(saw_invalid);
});

test("runPwdBuiltin: convenience single-cwd wrapper", () => {
    const empty: [0][]const u8 = [];
    let out: [64]u8 = [0; 64];
    const res = runPwdBuiltin(empty[0..], "/data/dev/cp", out[0..]) catch {
        assert(false);
        return;
    };
    assertEq(res.exit_code, 0);
    assert(sliceEql(out[0..res.bytes_written], "/data/dev/cp\n"));
});
