import { test, expect } from "cap:test";
import {
    SHELL_IO_MAX_HANDLES,
    SHELL_IO_INVALID_HANDLE,
    ShellInKind,
    ShellOutKind,
    SubprocStdioKind,
    ShellInChannel,
    ShellOutChannel,
    ShellIoTriplet,
    ShellIoHandleTable,
} from "cap:shell";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("copyTriplet and releaseTriplet track reference counts and close cleanly at zero", () => {
    let tbl = ShellIoHandleTable.init();
    const h_in = tbl.openHandle(0, false, 64) catch unreachable;
    const h_out = tbl.openHandle(1, false, 128) catch unreachable;
    const h_err = tbl.openHandle(2, false, 128) catch unreachable;
    expect(tbl.active_handles == 3);

    const io = ShellIoTriplet{
        stdin: ShellInChannel{ kind: .Fd, handle: h_in },
        stdout: ShellOutChannel{ kind: .Fd, handle: h_out },
        stderr: ShellOutChannel{ kind: .Fd, handle: h_err },
    };

    // Copy for child command -> ref_count becomes 2
    const child_io = tbl.copyTriplet(io) catch unreachable;
    expect((tbl.getHandle(h_in) catch unreachable).ref_count == 2);
    expect((tbl.getHandle(h_out) catch unreachable).ref_count == 2);
    expect((tbl.getHandle(h_err) catch unreachable).ref_count == 2);

    // Release child copy -> ref_count returns to 1, still open
    const rel1 = tbl.releaseTriplet(child_io) catch unreachable;
    expect(rel1 == 3);
    expect(tbl.active_handles == 3);
    expect(!(tbl.getHandle(h_out) catch unreachable).closed);

    // Release parent -> ref_count hits 0, all handles closed
    const rel2 = tbl.releaseTriplet(io) catch unreachable;
    expect(rel2 == 3);
    expect(tbl.active_handles == 0);
    expect((tbl.getHandle(h_in) catch unreachable).closed);
    expect((tbl.getHandle(h_out) catch unreachable).closed);
    expect((tbl.getHandle(h_err) catch unreachable).closed);
});

test("toSubprocStdio routes Fd, Capture, moved_to_loop Inherit, Pipe, and Ignore accurately", () => {
    let tbl = ShellIoHandleTable.init();
    const h_in = tbl.openHandle(10, false, 64) catch unreachable;
    const h_out_cap = tbl.openHandle(11, false, 128) catch unreachable;
    const h_err_loop = tbl.openHandle(12, true, 128) catch unreachable; // moved_to_loop -> Inherit

    const io = ShellIoTriplet{
        stdin: ShellInChannel{ kind: .Fd, handle: h_in },
        stdout: ShellOutChannel{ kind: .Fd, handle: h_out_cap, has_capture: true, capture_buf_id: 42 },
        stderr: ShellOutChannel{ kind: .Fd, handle: h_err_loop },
    };

    const plan = tbl.toSubprocStdio(io) catch unreachable;
    expect(plan.stdin.kind == .Fd);
    expect(plan.stdin.os_fd == 10);
    expect(plan.stdout.kind == .Capture);
    expect(plan.stdout.capture_buf_id == 42);
    expect(plan.stdout.writer_handle == h_out_cap);
    expect(plan.stderr.kind == .Inherit);
    expect(plan.stderr.writer_handle == h_err_loop);

    // Output fd routing automatically duplicated writer references
    expect((tbl.getHandle(h_out_cap) catch unreachable).ref_count == 2);
    expect((tbl.getHandle(h_err_loop) catch unreachable).ref_count == 2);
});

test("ShellIoTriplet.ignored and quietPipe presets route as expected", () => {
    let tbl = ShellIoHandleTable.init();

    const ign_plan = tbl.toSubprocStdio(ShellIoTriplet.ignored()) catch unreachable;
    expect(ign_plan.stdin.kind == .Ignore);
    expect(ign_plan.stdout.kind == .Ignore);
    expect(ign_plan.stderr.kind == .Ignore);

    const quiet_plan = tbl.toSubprocStdio(ShellIoTriplet.quietPipe()) catch unreachable;
    expect(quiet_plan.stdin.kind == .Ignore);
    expect(quiet_plan.stdout.kind == .Pipe);
    expect(quiet_plan.stderr.kind == .Pipe);
});

test("formatTriplet formats fd, pipe, and ignore channels cleanly", () => {
    let tbl = ShellIoHandleTable.init();
    const h_in = tbl.openHandle(0, false, 64) catch unreachable;
    const h_out = tbl.openHandle(1, false, 64) catch unreachable;

    const io = ShellIoTriplet{
        stdin: ShellInChannel{ kind: .Fd, handle: h_in },
        stdout: ShellOutChannel{ kind: .Fd, handle: h_out },
        stderr: ShellOutChannel{ kind: .Pipe },
    };

    let buf: [128]u8 = [0; 128];
    const n = tbl.formatTriplet(io, buf[0..]) catch unreachable;
    expect(strEq(buf[0..n], "stdin: fd: 0\nstdout: fd: 1\nstderr: pipe"));

    const n2 = tbl.formatTriplet(ShellIoTriplet.ignored(), buf[0..]) catch unreachable;
    expect(strEq(buf[0..n2], "stdin: ignore\nstdout: ignore\nstderr: ignore"));
});

test("estimateMemoryCost accounts for handle memory and capture buffer sizes", () => {
    let tbl = ShellIoHandleTable.init();
    const h_out = tbl.openHandle(1, false, 200) catch unreachable;
    const io = ShellIoTriplet{
        stdin: ShellInChannel{ kind: .Ignore },
        stdout: ShellOutChannel{ kind: .Fd, handle: h_out, has_capture: true, capture_bytes_estimate: 1024 },
        stderr: ShellOutChannel{ kind: .Pipe },
    };

    const cost = tbl.estimateMemoryCost(io);
    expect(cost == 64 + 200 + 1024);
});

test("Boundary guards: double-close AlreadyClosed, InvalidHandle, HandleTableFull, and BufferTooSmall", () => {
    let tbl = ShellIoHandleTable.init();
    const h = tbl.openHandle(3, false, 64) catch unreachable;
    * = tbl.derefHandle(h) catch unreachable;

    // Double close rejected
    let saw_closed = false;
    * = tbl.derefHandle(h) catch {
        saw_closed = true;
    };
    expect(saw_closed);

    // Ref on closed handle rejected
    let saw_ref_closed = false;
    * = tbl.refHandle(h) catch {
        saw_ref_closed = true;
    };
    expect(saw_ref_closed);

    // Invalid handle rejected
    let saw_invalid = false;
    * = tbl.getHandle(SHELL_IO_INVALID_HANDLE) catch {
        saw_invalid = true;
    };
    expect(saw_invalid);

    // Table capacity guard (1 slot already occupied by h)
    for (i of 1..SHELL_IO_MAX_HANDLES) {
        * = i;
        * = tbl.openHandle(10, false, 64) catch unreachable;
    }
    let saw_full = false;
    * = tbl.openHandle(10, false, 64) catch {
        saw_full = true;
    };
    expect(saw_full);

    // Tiny output buffer rejected
    let tiny: [4]u8 = [0; 4];
    let saw_small = false;
    * = tbl.formatTriplet(ShellIoTriplet.quietPipe(), tiny[0..]) catch {
        saw_small = true;
    };
    expect(saw_small);
});
