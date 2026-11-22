import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_SEQ_MAX_STEPS,
    SeqError,
    SeqOptions,
    SeqIter,
    formatSeq,
    formatSeqArgs,
} from "cap:shell";

test("1. SeqOptions.parse supports 1/2/3 operands, attached/long flags, and -- separator", () => {
    const a1: [1][]const u8 = ["5"];
    const o1 = SeqOptions.parse(a1[0..]) catch unreachable;
    expect_eq(o1.start == 1.0, true);
    expect_eq(o1.end == 5.0, true);
    expect_eq(o1.increment == 1.0, true);
    expect_eq(o1.estimatedCount(), usize(5));

    // 单参数小于 1 时自动递减
    const a2: [1][]const u8 = ["-2"];
    const o2 = SeqOptions.parse(a2[0..]) catch unreachable;
    expect_eq(o2.start == 1.0, true);
    expect_eq(o2.end == -2.0, true);
    expect_eq(o2.increment == -1.0, true);
    expect_eq(o2.estimatedCount(), usize(4));

    // 组合选项与 -- 分隔
    const a3: [7][]const u8 = ["-w", "-s,", "--terminator=!", "--", "-3", "2", "3"];
    const o3 = SeqOptions.parse(a3[0..]) catch unreachable;
    expect_eq(o3.fixed_width, true);
    expect_eq(std.mem.eql(u8, o3.separator, ","), true);
    expect_eq(std.mem.eql(u8, o3.terminator, "!"), true);
    expect_eq(o3.start == -3.0, true);
    expect_eq(o3.increment == 2.0, true);
    expect_eq(o3.end == 3.0, true);
    expect_eq(o3.estimatedCount(), usize(4));
});

test("2. SeqOptions.parse rejects malformed operands, NaN/Inf, zero increment, and wrong direction", () => {
    const empty: [0][]const u8 = [];
    let got_missing = false;
    * = SeqOptions.parse(empty[0..]) catch |e| {
        if (e == SeqError.MissingOperand) got_missing = true;
    };
    expect_eq(got_missing, true);

    const missing_opt: [1][]const u8 = ["-s"];
    let got_opt = false;
    * = SeqOptions.parse(missing_opt[0..]) catch |e| {
        if (e == SeqError.MissingOptionArg) got_opt = true;
    };
    expect_eq(got_opt, true);

    const bad_num: [1][]const u8 = ["NaN"];
    let got_nan = false;
    * = SeqOptions.parse(bad_num[0..]) catch |e| {
        if (e == SeqError.InvalidNumber) got_nan = true;
    };
    expect_eq(got_nan, true);

    const zero_inc: [3][]const u8 = ["1", "0", "5"];
    let got_zero = false;
    * = SeqOptions.parse(zero_inc[0..]) catch |e| {
        if (e == SeqError.ZeroIncrement) got_zero = true;
    };
    expect_eq(got_zero, true);

    const wrong_dir: [3][]const u8 = ["10", "2", "1"];
    let got_wrong = false;
    * = SeqOptions.parse(wrong_dir[0..]) catch |e| {
        if (e == SeqError.WrongDirection) got_wrong = true;
    };
    expect_eq(got_wrong, true);

    const too_many: [4][]const u8 = ["1", "1", "5", "9"];
    let got_many = false;
    * = SeqOptions.parse(too_many[0..]) catch |e| {
        if (e == SeqError.TooManyOperands) got_many = true;
    };
    expect_eq(got_many, true);
});

test("3. SeqIter lazy iteration eliminates floating-point accumulation drift and supports reset", () => {
    const args: [3][]const u8 = ["0.1", "0.1", "0.3"];
    const opts = SeqOptions.parse(args[0..]) catch unreachable;
    let it = SeqIter.init(opts);

    const v1 = it.next() ?? unreachable;
    const v2 = it.next() ?? unreachable;
    const v3 = it.next() ?? unreachable;
    expect_eq(@abs(v1 - 0.1) < 1.0e-9, true);
    expect_eq(@abs(v2 - 0.2) < 1.0e-9, true);
    expect_eq(@abs(v3 - 0.3) < 1.0e-9, true);
    expect_eq(it.next() == null, true);

    it.reset();
    const again = it.next() ?? unreachable;
    expect_eq(@abs(again - 0.1) < 1.0e-9, true);
});

test("4. formatSeqArgs formats custom separators, terminators, decimals, and negative countdown", () => {
    const alloc = std.testing.allocator;

    const a1: [5][]const u8 = ["-s", ", ", "-t", "\n", "3"];
    const s1 = formatSeqArgs(alloc, a1[0..]) catch unreachable;
    defer alloc.free(s1);
    expect_eq(std.mem.eql(u8, s1, "1, 2, 3, \n"), true);

    const a2: [4][]const u8 = ["-s:", "2", "-1", "-1"];
    const s2 = formatSeqArgs(alloc, a2[0..]) catch unreachable;
    defer alloc.free(s2);
    expect_eq(std.mem.eql(u8, s2, "2:1:0:-1:"), true);

    const a3: [4][]const u8 = ["-s|", "0", "0.5", "1.5"];
    const s3 = formatSeqArgs(alloc, a3[0..]) catch unreachable;
    defer alloc.free(s3);
    expect_eq(std.mem.eql(u8, s3, "0|0.5|1|1.5|"), true);
});

test("5. formatSeqArgs equal-width (-w) zero-pads positive and negative numbers cleanly", () => {
    const alloc = std.testing.allocator;

    const a1: [4][]const u8 = ["-w", "-s,", "8", "10"];
    const s1 = formatSeqArgs(alloc, a1[0..]) catch unreachable;
    defer alloc.free(s1);
    expect_eq(std.mem.eql(u8, s1, "08,09,10,"), true);

    const a2: [5][]const u8 = ["--equal-width", "-s,", "-2", "2", "2"];
    const s2 = formatSeqArgs(alloc, a2[0..]) catch unreachable;
    defer alloc.free(s2);
    expect_eq(std.mem.eql(u8, s2, "-2,00,02,"), true);
});

test("6. formatSeq enforces max_steps resource exhaustion protection", () => {
    const alloc = std.testing.allocator;
    expect_eq(DEFAULT_SEQ_MAX_STEPS, usize(1000000));

    const opts = SeqOptions{
        start: 1.0,
        end: 500.0,
        increment: 1.0,
        max_steps: 10,
    };

    let hit_limit = false;
    * = formatSeq(alloc, opts) catch |e| {
        if (e == SeqError.StepLimitExceeded) {
            hit_limit = true;
        }
    };
    expect_eq(hit_limit, true);
});
