// cap/test/testTestRunnerDebugTree.cp
// Production-grade deep tests for cap:test_runner debugTree.cp (Bun test_runner/debug.cp tracer)

import { test, assert, assertEq } from "cap:test";
import {
    DEBUG_TREE_MAX_DEPTH,
    DEBUG_TREE_DEFAULT_INDENT,
    DebugTreeError,
    DebugRunMode,
    DebugHookPhase,
    DebugEntryMeta,
    DebugSequenceMeta,
    DebugTreeWriter,
} from "cap:test_runner";

test("DebugTree: nested begin/log/end & empty group closing suppression", () => {
    let buf: [512]u8 = [0; 512];
    let w = DebugTreeWriter.init(buf[0..], true, false);
    assert(w.isEnabled());
    assertEq(w.depth(), @as(usize, 0));
    assertEq(DEBUG_TREE_DEFAULT_INDENT, "│ ");

    assert(w.begin("root") catch false);
    assertEq(w.depth(), @as(usize, 1));
    assert(w.log("step 1") catch false);

    // Empty nested group: end() should suppress redundant "--"
    assert(w.begin("emptyChild") catch false);
    assertEq(w.depth(), @as(usize, 2));
    assert(w.end() catch false);
    assertEq(w.depth(), @as(usize, 1));

    // Close root (had log/child output -> emits "--")
    assert(w.end() catch false);
    assertEq(w.depth(), @as(usize, 0));

    const expected = "++ root\n│ step 1\n│ ++ emptyChild\n--\n";
    assertEq(w.slice(), expected);
    assertEq(w.len(), expected.len);
    assertEq(w.remaining(), 512 - expected.len);
});

test("DebugTree: beginLocation plain & ANSI colored rendering", () => {
    let plain_buf: [256]u8 = [0; 256];
    let wp = DebugTreeWriter.init(plain_buf[0..], true, false);
    assert(wp.beginLocation("suite.cp", 42, 7, "runAll") catch false);
    assert(wp.log("ok") catch false);
    assert(wp.end() catch false);
    assertEq(wp.slice(), "++ suite.cp:42:7: runAll\n│ ok\n--\n");

    let ansi_buf: [512]u8 = [0; 512];
    let wa = DebugTreeWriter.init(ansi_buf[0..], true, true);
    assert(wa.beginLocation("a.cp", 10, 2, "fnA") catch false);
    assert(wa.log("hi") catch false);
    assert(wa.end() catch false);
    const out = wa.slice();
    assert(out.len > 30);
    // Contains ANSI ESC prefix
    assertEq(out[0], @as(u8, 0x1b));
});

test("DebugTree: dumpDescribeHeader / dumpTest / escaping & unnamed fallback", () => {
    let buf: [1024]u8 = [0; 1024];
    let w = DebugTreeWriter.init(buf[0..], true, false);

    assert(w.dumpDescribeHeader(DebugEntryMeta{
        name: "auth \"v2\"\nline",
        concurrent: true,
        only: false,
        mode: .Normal,
        has_callback: true,
    }) catch false);

    assert(w.dumpTest(DebugEntryMeta{
        name: "",
        label: DebugHookPhase.BeforeEach.asStr(),
        concurrent: false,
        only: false,
    }) catch false);

    assert(w.dumpTest(DebugEntryMeta{
        name: "logs\\in\tok",
        label: DebugHookPhase.Test.asStr(),
        concurrent: true,
        only: true,
    }) catch false);

    assert(w.end() catch false);

    const expected =
        "++ describe \"auth \\\"v2\\\"\\nline\" (concurrent=true, mode=normal, only=false, has_callback=true)\n" ++
        "│ ++ beforeEach \"(unnamed)\" (concurrent=false, only=false)\n" ++
        "│ ++ test \"logs\\\\in\\tok\" (concurrent=true, only=true)\n" ++
        "--\n";
    assertEq(w.slice(), expected);
});

test("DebugTree: dumpOrder concurrent group, sequence & ExecutionEntry", () => {
    let buf: [1024]u8 = [0; 1024];
    let w = DebugTreeWriter.init(buf[0..], true, false);

    assert(w.begin("dumpOrder") catch false);
    assert(w.dumpConcurrentGroupHeader(DebugSequenceMeta{
        concurrent_group_id: 0,
        sequence_start: 0,
        sequence_end: 2,
    }) catch false);
    assert(w.dumpSequenceHeader(DebugSequenceMeta{
        index: 1,
        repeat_count: 3,
    }) catch false);
    assert(w.dumpExecutionEntry(DebugEntryMeta{
        name: "smoke",
        concurrent: false,
        mode: .Only,
        only: true,
        has_callback: true,
    }) catch false);
    assert(w.end() catch false);
    assert(w.end() catch false);
    assert(w.end() catch false);

    const expected =
        "++ dumpOrder\n" ++
        "│ ++ 0 ConcurrentGroup (0-2)\n" ++
        "│ │ ++ 1 Sequence (3x)\n" ++
        "│ │ │ ExecutionEntry \"smoke\" (concurrent=false, mode=only, only=true, has_callback=true)\n" ++
        "│ │ --\n" ++
        "│ --\n" ++
        "--\n";
    assertEq(w.slice(), expected);
});

test("DebugTree: disabled mode is zero-output no-op & reset clears state", () => {
    let buf: [128]u8 = [0; 128];
    let wd = DebugTreeWriter.init(buf[0..], false, false);
    assert(!wd.isEnabled());
    assert(!(wd.begin("ignored") catch true));
    assert(!(wd.log("ignored") catch true));
    assert(!(wd.end() catch true));
    assertEq(wd.len(), @as(usize, 0));
    assertEq(wd.depth(), @as(usize, 0));

    let wa = DebugTreeWriter.init(buf[0..], true, false);
    assert(wa.begin("a") catch false);
    assert(wa.log("b") catch false);
    assert(wa.len() > 0);
    wa.reset();
    assertEq(wa.len(), @as(usize, 0));
    assertEq(wa.depth(), @as(usize, 0));
    assertEq(wa.slice(), "");
});

test("DebugTree: enum labels for DebugRunMode & DebugHookPhase", () => {
    assertEq(DebugRunMode.Normal.asStr(), "normal");
    assertEq(DebugRunMode.Skip.asStr(), "skip");
    assertEq(DebugRunMode.Todo.asStr(), "todo");
    assertEq(DebugRunMode.Only.asStr(), "only");
    assertEq(DebugRunMode.Fail.asStr(), "fail");

    assertEq(DebugHookPhase.BeforeAll.asStr(), "beforeAll");
    assertEq(DebugHookPhase.BeforeEach.asStr(), "beforeEach");
    assertEq(DebugHookPhase.Test.asStr(), "test");
    assertEq(DebugHookPhase.AfterEach.asStr(), "afterEach");
    assertEq(DebugHookPhase.AfterAll.asStr(), "afterAll");
    assertEq(DebugHookPhase.OnTestFinished.asStr(), "onTestFinished");
});

test("DebugTree: error bounds (UnbalancedEnd, MaxDepthExceeded, BufferTooSmall rollback)", () => {
    // 1. UnbalancedEnd on empty stack
    let buf1: [64]u8 = [0; 64];
    let w1 = DebugTreeWriter.init(buf1[0..], true, false);
    let got_unbalanced = false;
    * = w1.end() catch |e| {
        if (e == DebugTreeError.UnbalancedEnd) {
            got_unbalanced = true;
        }
        false
    };
    assert(got_unbalanced);

    // 2. BufferTooSmall atomic rollback (no partial garbage line written)
    let tiny: [12]u8 = [0; 12];
    let w2 = DebugTreeWriter.init(tiny[0..], true, false);
    assert(w2.begin("ok") catch false); // writes "++ ok\n" (6 bytes)
    const len_before = w2.len();
    assertEq(len_before, @as(usize, 6));
    let got_too_small = false;
    * = w2.log("exceeds_remaining_capacity") catch |e| {
        if (e == DebugTreeError.BufferTooSmall) {
            got_too_small = true;
        }
        false
    };
    assert(got_too_small);
    assertEq(w2.len(), len_before);
    assertEq(w2.depth(), @as(usize, 1));

    // 3. MaxDepthExceeded guard
    let big: [8192]u8 = [0; 8192];
    let w3 = DebugTreeWriter.init(big[0..], true, false);
    let d: usize = 0;
    while (d < DEBUG_TREE_MAX_DEPTH) {
        assert(w3.begin("x") catch false);
        d += 1;
    }
    assertEq(w3.depth(), DEBUG_TREE_MAX_DEPTH);
    let got_max_depth = false;
    * = w3.begin("overflow") catch |e| {
        if (e == DebugTreeError.MaxDepthExceeded) {
            got_max_depth = true;
        }
        false
    };
    assert(got_max_depth);
    assertEq(w3.depth(), DEBUG_TREE_MAX_DEPTH);
});
