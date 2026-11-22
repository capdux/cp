import { test, expect } from "cap:test";
import {
    MvOpts,
    MvOverwritePolicy,
    MvPlan,
} from "cap:shell";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

test("shell mv_opts: basic single-source rename and default Force policy", () => {
    const args: [2][]const u8 = ["old.txt", "new.txt"];
    const plan = MvPlan.parse(args[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(plan.opts.forceOverwrite()).toBe(true);
    expect(plan.opts.interactiveMode()).toBe(false);
    expect(plan.opts.noOverwrite()).toBe(false);
    expect(plan.opts.verbose).toBe(false);
    expect(plan.source_count).toBe(1);
    expect(plan.requiresDirectoryTarget()).toBe(false);
    expect(sliceEq(plan.sources[0], "old.txt")).toBe(true);
    expect(sliceEq(plan.target, "new.txt")).toBe(true);

    let out: [128]u8 = [0; 128];
    const n = plan.resolveDestPath(0, false, out[0..]) catch 0;
    expect(sliceEq(out[0..n], "new.txt")).toBe(true);
});

test("shell mv_opts: combined short flags and last-flag-wins overwrite policy (-f/-i/-n)", () => {
    // -fvn -> NoClobber + verbose
    const args1: [3][]const u8 = ["-fvn", "a.cp", "dir/"];
    const p1 = MvPlan.parse(args1[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(p1.opts.overwrite_policy == MvOverwritePolicy.NoClobber).toBe(true);
    expect(p1.opts.noOverwrite()).toBe(true);
    expect(p1.opts.verbose).toBe(true);

    // -n -i -h -> Interactive + no_dereference
    const args2: [5][]const u8 = ["-n", "-i", "-h", "src", "dst"];
    const p2 = MvPlan.parse(args2[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(p2.opts.interactiveMode()).toBe(true);
    expect(p2.opts.no_dereference).toBe(true);

    // -i -f -> Force overrides Interactive
    const args3: [4][]const u8 = ["-i", "-f", "src", "dst"];
    const p3 = MvPlan.parse(args3[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(p3.opts.forceOverwrite()).toBe(true);
});

test("shell mv_opts: multi-source directory resolution and trailing slash normalization", () => {
    const args: [4][]const u8 = ["-v", "src/a.cp", "src/b.cp/", "/out/dir/"];
    const plan = MvPlan.parse(args[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(plan.source_count).toBe(2);
    expect(plan.requiresDirectoryTarget()).toBe(true);

    let out: [128]u8 = [0; 128];
    const n0 = plan.resolveDestPath(0, true, out[0..]) catch 0;
    expect(sliceEq(out[0..n0], "/out/dir/a.cp")).toBe(true);

    const n1 = plan.resolveDestPath(1, true, out[0..]) catch 0;
    expect(sliceEq(out[0..n1], "/out/dir/b.cp")).toBe(true);

    // Multi-source with non-directory target rejected
    let not_dir_err = false;
    * = plan.resolveDestPath(0, false, out[0..]) catch {
        not_dir_err = true;
    };
    expect(not_dir_err).toBe(true);
});

test("shell mv_opts: -- option terminator allows dash-prefixed filenames", () => {
    const args: [4][]const u8 = ["-v", "--", "-weird.txt", "dest/"];
    const plan = MvPlan.parse(args[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(plan.opts.verbose).toBe(true);
    expect(plan.source_count).toBe(1);
    expect(sliceEq(plan.sources[0], "-weird.txt")).toBe(true);

    let out: [128]u8 = [0; 128];
    const n = plan.resolveDestPath(0, true, out[0..]) catch 0;
    expect(sliceEq(out[0..n], "dest/-weird.txt")).toBe(true);
});

test("shell mv_opts: verbose line formatting and MvOpts direct API", () => {
    let out: [128]u8 = [0; 128];
    const n = MvPlan.formatVerboseLine("src/app.cp", "dist/app.cp", out[0..]) catch 0;
    expect(sliceEq(out[0..n], "'src/app.cp' -> 'dist/app.cp'\n")).toBe(true);

    let opts = MvOpts{};
    expect(opts.applyShortFlag('v')).toBe(true);
    expect(opts.applyShortFlag('z')).toBe(false);
    expect(opts.verbose).toBe(true);
});

test("shell mv_opts: usage, illegal option, and BufferTooSmall error guards", () => {
    // Empty args -> ShowUsage
    const empty: [0][]const u8 = [];
    let empty_err = false;
    * = MvPlan.parse(empty[0..]) catch {
        empty_err = true;
    };
    expect(empty_err).toBe(true);

    // Only 1 operand -> ShowUsage
    const one: [2][]const u8 = ["-f", "only-src"];
    let one_err = false;
    * = MvPlan.parse(one[0..]) catch {
        one_err = true;
    };
    expect(one_err).toBe(true);

    // Illegal option -z -> IllegalOption
    const bad_opt: [3][]const u8 = ["-fz", "a", "b"];
    let opt_err = false;
    * = MvPlan.parse(bad_opt[0..]) catch {
        opt_err = true;
    };
    expect(opt_err).toBe(true);

    // BufferTooSmall guard
    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    * = MvPlan.formatVerboseLine("long-src.txt", "long-dst.txt", tiny[0..]) catch {
        buf_err = true;
    };
    expect(buf_err).toBe(true);
});
