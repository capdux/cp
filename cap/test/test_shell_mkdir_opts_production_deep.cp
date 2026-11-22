// Production Deep Tests for cap:shell POSIX mkdir Options & ParentDirIter (lib/shell/mkdir_opts.cp)
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_MKDIR_MODE,
    MkdirParseError,
    parseOctalMode,
    MkdirOpts,
    ParentDirIter,
} from "cap:shell";

test("shell mkdir_opts: short flags, combined clusters, and -m mode attachment", fn() {
    const a1 = ["-pv", "dir1", "dir2"];
    const o1 = MkdirOpts.parse(a1[0..]) catch unreachable;
    expect_eq(o1.parents, true);
    expect_eq(o1.verbose, true);
    expect_eq(o1.mode == null, true);
    expect_eq(o1.effectiveMode(), DEFAULT_MKDIR_MODE);
    expect_eq(o1.paths_start, @as(usize, 1));

    // Attached mode in cluster `-pm750`
    const a2 = ["-pm750", "out"];
    const o2 = MkdirOpts.parse(a2[0..]) catch unreachable;
    expect_eq(o2.parents, true);
    expect_eq(o2.effectiveMode(), @as(u32, 0o750));
    expect_eq(o2.paths_start, @as(usize, 1));

    // Separate mode argument `-m 0755`
    const a3 = ["-m", "0755", "target"];
    const o3 = MkdirOpts.parse(a3[0..]) catch unreachable;
    expect_eq(o3.effectiveMode(), @as(u32, 0o755));
    expect_eq(o3.paths_start, @as(usize, 2));
});

test("shell mkdir_opts: long flags and -- option terminator", fn() {
    const a1 = ["--parents", "--verbose", "--mode=0700", "a/b/c"];
    const o1 = MkdirOpts.parse(a1[0..]) catch unreachable;
    expect_eq(o1.parents, true);
    expect_eq(o1.verbose, true);
    expect_eq(o1.effectiveMode(), @as(u32, 0o700));
    expect_eq(o1.paths_start, @as(usize, 3));

    const a2 = ["--mode", "1777", "--", "-dash-dir"];
    const o2 = MkdirOpts.parse(a2[0..]) catch unreachable;
    expect_eq(o2.effectiveMode(), @as(u32, 0o1777));
    expect_eq(o2.paths_start, @as(usize, 3));
    expect_eq(a2[o2.paths_start], "-dash-dir");
});

test("shell mkdir_opts: parseOctalMode valid bounds and malformed rejections", fn() {
    expect_eq(parseOctalMode("0") catch unreachable, @as(u32, 0));
    expect_eq(parseOctalMode("755") catch unreachable, @as(u32, 0o755));
    expect_eq(parseOctalMode("0755") catch unreachable, @as(u32, 0o755));
    expect_eq(parseOctalMode("7777") catch unreachable, @as(u32, 0o7777));

    const bad_inputs = ["", "888", "0900", "12345", "rwx"];
    let i: usize = 0;
    while (i < bad_inputs.len) {
        let is_bad = false;
        * = parseOctalMode(bad_inputs[i]) catch |e| {
            is_bad = (e == MkdirParseError.InvalidMode);
            @as(u32, 0)
        };
        expect_eq(is_bad, true);
        i += 1;
    }
});

test("shell mkdir_opts: missing operand, missing mode value, and illegal options", fn() {
    const empty: [0]string = [];
    let miss_op = false;
    * = MkdirOpts.parse(empty[0..]) catch |e| {
        miss_op = (e == MkdirParseError.MissingOperand);
        MkdirOpts{}
    };
    expect_eq(miss_op, true);

    const flags_only = ["-p", "-v"];
    let miss_op2 = false;
    * = MkdirOpts.parse(flags_only[0..]) catch |e| {
        miss_op2 = (e == MkdirParseError.MissingOperand);
        MkdirOpts{}
    };
    expect_eq(miss_op2, true);

    const no_mode_val = ["-m"];
    let miss_mode = false;
    * = MkdirOpts.parse(no_mode_val[0..]) catch |e| {
        miss_mode = (e == MkdirParseError.MissingModeValue);
        MkdirOpts{}
    };
    expect_eq(miss_mode, true);

    const bad_short = ["-pz", "dir"];
    let bad_opt = false;
    * = MkdirOpts.parse(bad_short[0..]) catch |e| {
        bad_opt = (e == MkdirParseError.IllegalOption);
        MkdirOpts{}
    };
    expect_eq(bad_opt, true);

    const bad_long = ["--unknown", "dir"];
    let bad_opt2 = false;
    * = MkdirOpts.parse(bad_long[0..]) catch |e| {
        bad_opt2 = (e == MkdirParseError.IllegalOption);
        MkdirOpts{}
    };
    expect_eq(bad_opt2, true);
});

test("shell mkdir_opts: ParentDirIter iterates absolute and relative paths cleanly", fn() {
    let it_abs = ParentDirIter.init("/var/log/app");
    expect_eq(it_abs.next() ?? "", "/var");
    expect_eq(it_abs.next() ?? "", "/var/log");
    expect_eq(it_abs.next() ?? "", "/var/log/app");
    expect_eq(it_abs.next() == null, true);

    let it_rel = ParentDirIter.init("a/b/c");
    expect_eq(it_rel.next() ?? "", "a");
    expect_eq(it_rel.next() ?? "", "a/b");
    expect_eq(it_rel.next() ?? "", "a/b/c");
    expect_eq(it_rel.next() == null, true);
});

test("shell mkdir_opts: ParentDirIter handles trailing/duplicate slashes, root, and empty", fn() {
    let it_multi = ParentDirIter.init("x//y///");
    expect_eq(it_multi.next() ?? "", "x");
    expect_eq(it_multi.next() ?? "", "x//y");
    expect_eq(it_multi.next() == null, true);

    let it_empty = ParentDirIter.init("");
    expect_eq(it_empty.next() == null, true);

    let it_root = ParentDirIter.init("/");
    expect_eq(it_root.next() == null, true);
});
