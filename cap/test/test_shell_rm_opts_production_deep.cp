// cap/test/test_shell_rm_opts_production_deep.cp
// Production-deep regression tests for cap:shell rm_opts module
// Source: /data/dev/bun/src/shell/builtin/rm.cp & /data/dev/bun/docs/runtime/shell.mdx

import { test, expect } from "cap:test";
import {
    SHELL_RM_MAX_TARGETS,
    ShellRmError,
    RmPromptBehaviour,
    RmPathKind,
    RmJoinStyle,
    RmTargetDecision,
    RmPlan,
    isRmRootPath,
    isRmDotOrDotDotTarget,
} from "cap:shell";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("shell rm_opts: combined short flags, long flags & -- sentinel") {
    const args1: [4]([]const u8) = ["-rfv", "--", "-dash-file.txt", "normal.txt"];
    const p1 = RmPlan.parse(args1[0..]) catch {
        expect(false);
        return;
    };
    expect(p1.opts.force);
    expect(p1.opts.recursive);
    expect(p1.opts.remove_empty_dirs);
    expect(p1.opts.verbose);
    expect(p1.opts.preserve_root);
    expect(p1.opts.prompt_behaviour == RmPromptBehaviour.Never);
    expect(p1.target_count == 2);
    if (t0 := p1.targetAt(0)) { expect(strEq(t0, "-dash-file.txt")); } else { expect(false); }
    if (t1 := p1.targetAt(1)) { expect(strEq(t1, "normal.txt")); } else { expect(false); }
    expect(p1.targetAt(2) == null);

    // Long flags & last-flag-wins between -i and -f
    const args2: [5]([]const u8) = ["--interactive=always", "-f", "--dir", "--no-preserve-root", "empty_dir"];
    const p2 = RmPlan.parse(args2[0..]) catch {
        expect(false);
        return;
    };
    expect(p2.opts.force);
    expect(p2.opts.prompt_behaviour == RmPromptBehaviour.Never);
    expect(p2.opts.remove_empty_dirs);
    expect(!p2.opts.recursive);
    expect(!p2.opts.preserve_root);

    // -i after -f re-enables interactive prompt
    const args3: [3]([]const u8) = ["-f", "--interactive=once", "a.txt"];
    const p3 = RmPlan.parse(args3[0..]) catch {
        expect(false);
        return;
    };
    expect(p3.opts.prompt_behaviour == RmPromptBehaviour.Once);
}

test("shell rm_opts: root path & dot/dotdot deletion safety guards") {
    expect(isRmRootPath("/workspace/app", "/"));
    expect(isRmRootPath("/workspace/app", "///"));
    expect(isRmRootPath("/workspace/app", "../.."));
    expect(isRmRootPath("/workspace/app", "../../.."));
    expect(isRmRootPath("/workspace/app", "/a/b/../.."));
    expect(isRmRootPath("C:\\workspace\\app", "C:\\"));
    expect(isRmRootPath("C:\\workspace\\app", "..\\.."));
    expect(!isRmRootPath("/workspace/app", "dist"));
    expect(!isRmRootPath("/workspace/app", "../other"));
    expect(!isRmRootPath("relative/dir", "../.."));

    expect(isRmDotOrDotDotTarget("."));
    expect(isRmDotOrDotDotTarget(".."));
    expect(isRmDotOrDotDotTarget("./"));
    expect(isRmDotOrDotDotTarget("foo/bar/.."));
    expect(isRmDotOrDotDotTarget("foo/bar/.///"));
    expect(!isRmDotOrDotDotTarget(".gitignore"));
    expect(!isRmDotOrDotDotTarget("..hidden"));
    expect(!isRmDotOrDotDotTarget("/"));

    const args: [2]([]const u8) = ["-rf", "/"];
    const plan = RmPlan.parse(args[0..]) catch {
        expect(false);
        return;
    };

    let root_blocked = false;
    * = plan.validateTarget("/workspace/app", "../..", RmPathKind.NonEmptyDir) catch |e| {
        expect(e == ShellRmError.RefusingToRemoveRoot);
        root_blocked = true;
    };
    expect(root_blocked);

    let dot_blocked = false;
    * = plan.validateTarget("/workspace/app", "sub/..", RmPathKind.NonEmptyDir) catch |e| {
        expect(e == ShellRmError.RefusingToRemoveDotOrDotDot);
        dot_blocked = true;
    };
    expect(dot_blocked);

    // Explicit `--no-preserve-root` allows root check bypass (still blocks `.`/`..`)
    const no_pr_args: [3]([]const u8) = ["-rf", "--no-preserve-root", "/"];
    const no_pr_plan = RmPlan.parse(no_pr_args[0..]) catch {
        expect(false);
        return;
    };
    const decision = no_pr_plan.validateTarget("/workspace", "/", RmPathKind.NonEmptyDir) catch {
        expect(false);
        return;
    };
    expect(decision == RmTargetDecision.Remove);
}

test("shell rm_opts: directory removal rules (-d vs -r vs plain)") {
    // Plain `rm` rejects both empty and non-empty directories
    const plain_args: [1]([]const u8) = ["dir"];
    const plain = RmPlan.parse(plain_args[0..]) catch { expect(false); return; };

    let err_empty = false;
    * = plain.validateTarget("/repo", "dir", RmPathKind.EmptyDir) catch |e| {
        expect(e == ShellRmError.CannotRemoveDirectoryWithoutFlag);
        err_empty = true;
    };
    expect(err_empty);

    let err_nonempty = false;
    * = plain.validateTarget("/repo", "dir", RmPathKind.NonEmptyDir) catch |e| {
        expect(e == ShellRmError.CannotRemoveDirectoryWithoutFlag);
        err_nonempty = true;
    };
    expect(err_nonempty);

    // `rm -d` allows empty dir, rejects non-empty dir
    const d_args: [2]([]const u8) = ["-d", "dir"];
    const d_plan = RmPlan.parse(d_args[0..]) catch { expect(false); return; };
    const d_ok = d_plan.validateTarget("/repo", "dir", RmPathKind.EmptyDir) catch { expect(false); return; };
    expect(d_ok == RmTargetDecision.Remove);

    let d_rejects_nonempty = false;
    * = d_plan.validateTarget("/repo", "dir", RmPathKind.NonEmptyDir) catch |e| {
        expect(e == ShellRmError.CannotRemoveDirectoryWithoutFlag);
        d_rejects_nonempty = true;
    };
    expect(d_rejects_nonempty);

    // `rm -R` allows both empty and non-empty directories
    const r_args: [2]([]const u8) = ["-R", "dir"];
    const r_plan = RmPlan.parse(r_args[0..]) catch { expect(false); return; };
    const r_ok = r_plan.validateTarget("/repo", "dir", RmPathKind.NonEmptyDir) catch { expect(false); return; };
    expect(r_ok == RmTargetDecision.Remove);
}

test("shell rm_opts: force missing-skip, empty operand & interactive prompt decision") {
    // `rm -f` with no operands succeeds silently per POSIX
    const f_only: [1]([]const u8) = ["-f"];
    const fp = RmPlan.parse(f_only[0..]) catch { expect(false); return; };
    expect(fp.target_count == 0);
    const skip = fp.validateTarget("/repo", "missing.txt", RmPathKind.Missing) catch { expect(false); return; };
    expect(skip == RmTargetDecision.SkipMissing);

    // `rm` without `-f` and without operands fails with MissingOperand
    const empty_args: [0]([]const u8) = [];
    let got_missing_op = false;
    * = RmPlan.parse(empty_args[0..]) catch |e| {
        expect(e == ShellRmError.MissingOperand);
        got_missing_op = true;
    };
    expect(got_missing_op);

    // Interactive prompt decision
    const i_args: [2]([]const u8) = ["-i", "important.txt"];
    const ip = RmPlan.parse(i_args[0..]) catch { expect(false); return; };
    const prompt = ip.validateTarget("/repo", "important.txt", RmPathKind.File) catch { expect(false); return; };
    expect(prompt == RmTargetDecision.PromptBeforeRemove);
}

test("shell rm_opts: malformed flags & target capacity overflow defense") {
    const bad_short: [2]([]const u8) = ["-rz", "a"];
    let got_bad_short = false;
    * = RmPlan.parse(bad_short[0..]) catch |e| {
        expect(e == ShellRmError.IllegalOption);
        got_bad_short = true;
    };
    expect(got_bad_short);

    const bad_long: [2]([]const u8) = ["--unknown-flag", "a"];
    let got_bad_long = false;
    * = RmPlan.parse(bad_long[0..]) catch |e| {
        expect(e == ShellRmError.IllegalOption);
        got_bad_long = true;
    };
    expect(got_bad_long);

    // Exceeding SHELL_RM_MAX_TARGETS (64) returns TooManyTargets
    expect(SHELL_RM_MAX_TARGETS == 64);
    let many: [65]([]const u8) = ["f"; 65];
    let got_overflow = false;
    * = RmPlan.parse(many[0..]) catch |e| {
        expect(e == ShellRmError.TooManyTargets);
        got_overflow = true;
    };
    expect(got_overflow);
}

test("shell rm_opts: verbose output formatting & join style detection") {
    const args: [2]([]const u8) = ["-rv", "build"];
    const plan = RmPlan.parse(args[0..]) catch { expect(false); return; };

    let buf: [64]u8 = [0; 64];
    const n1 = plan.formatVerboseLine("app.log", false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "removed 'app.log'\n"));

    const n2 = plan.formatVerboseLine("build", true, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "removed directory 'build'\n"));

    // BufferTooSmall protection
    let tiny: [6]u8 = [0; 6];
    let got_small = false;
    * = plan.formatVerboseLine("build", true, tiny[0..]) catch |e| {
        expect(e == ShellRmError.BufferTooSmall);
        got_small = true;
    };
    expect(got_small);

    // RmJoinStyle detection
    expect(RmJoinStyle.fromPath("a/b/c") == RmJoinStyle.Posix);
    expect(RmJoinStyle.fromPath("C:\\a\\b") == RmJoinStyle.Windows);
    expect(RmJoinStyle.fromPath("plain.txt") == RmJoinStyle.Posix);
}
