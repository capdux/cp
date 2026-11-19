// cap/test/test_bench_cli_napi_bigint_event.cp
// Comprehensive Offline Regression Test Suite for:
//   1. cap:bench   (HwTimer, Stopwatch, BenchStats, LatencyHistogram)
//   2. cap:cli     (SliceIterator, CliArgs, TtyMode, AnsiColor, ProgressBar, CliTable)
//   3. cap:napi    (NapiStatus, NapiValueType, NapiTypedArrayType, NapiModule, NapiPropertyDescriptor, NapiAddonRegistry, Napi)
//   4. cap:bigint  (BigInt arbitrary precision, BigInt256 EVM/Web3 arithmetic)
//   5. cap:event   (EventBus, EventEmitter, Event, AnyTask, ManagedTask, DeferredTaskQueue, ConcurrentQueue, TimerHeap, EventLoop)

import { test, expect_eq } from "cap:test";
import { HwTimer, Stopwatch, BenchStats, LatencyHistogram } from "cap:bench";
import {
    SliceIterator,
    CliArgs,
    TtyMode,
    AnsiColor,
    ProgressBar,
    CliTableAlign,
    CliTable
} from "cap:cli";
import {
    NAPI_VERSION,
    NAPI_MODULE_VERSION,
    NapiStatus,
    NapiValueType,
    NapiTypedArrayType,
    NapiPropertyAttributes,
    NapiAddonRegistry,
    Napi
} from "cap:napi";
import { BigInt, BigInt256 } from "cap:bigint";
import {
    EventBus,
    EventEmitter,
    Event,
    AnyTask,
    ManagedTask,
    DeferredTaskQueue,
    ConcurrentTask,
    ConcurrentQueue,
    TimerHeap,
    EventLoop
} from "cap:event";

const std = @import("std");

// --- Helper callbacks for cap:event ---
struct EventTestCounter {
    pub total: i32 = 0;
    pub last_len: usize = 0;
}

fn onBusMessage(payload: []const u8, ctx: (*anyopaque)?) {
    if (c := ctx) {
        let counter: *EventTestCounter = @alignCast(@ptrCast(c));
        counter.total += 1;
        counter.last_len = payload.len;
    }
}

fn onBusOnce(payload: []const u8, ctx: (*anyopaque)?) {
    if (c := ctx) {
        let counter: *EventTestCounter = @alignCast(@ptrCast(c));
        counter.total += 10;
        counter.last_len = payload.len;
    }
}

fn onSimpleTask(ctx: *!?) {
    if (c := ctx) {
        let counter: *EventTestCounter = @alignCast(@ptrCast(c));
        counter.total += 1;
    }
}

fn testBenchSuite(): !void {
    // 1. HwTimer monotonic clock
    const t0 = HwTimer.nowNs();
    const t1 = HwTimer.nowNs();
    expect_eq(t1 >= t0, true);
    expect_eq(HwTimer.nowUs() > 0, true);
    expect_eq(HwTimer.nowMs() > 0, true);

    // 2. Stopwatch
    let sw = Stopwatch.start();
    const lap1 = sw.lap();
    expect_eq(lap1 >= 0, true);
    expect_eq(sw.readNs() >= 0, true);
    sw.reset();
    expect_eq(sw.readMs() < 1000, true);

    // 3. BenchStats sorted percentiles & ops/sec
    let stats = BenchStats.init();
    stats.record(500);
    stats.record(100);
    stats.record(900);
    stats.record(300);
    stats.record(700);
    expect_eq(stats.count, 5);
    expect_eq(stats.min_ns, 100);
    expect_eq(stats.max_ns, 900);
    expect_eq(stats.meanNs(), 500);
    expect_eq(stats.p50Ns(), 500);
    expect_eq(stats.p95Ns(), 700);
    expect_eq(stats.p99Ns(), 700);
    expect_eq(stats.percentileNs(100), 900);
    expect_eq(stats.opsPerSec(), 2000000);

    // 4. LatencyHistogram logarithmic buckets
    let hist = LatencyHistogram.init();
    hist.record(50);          // bucket 0 (<100ns)
    hist.record(250);         // bucket 1 (<1us)
    hist.record(4000);        // bucket 2 (<10us)
    hist.record(50000);       // bucket 3 (<100us)
    hist.record(200000000);   // bucket 7 (>=100ms)
    expect_eq(hist.count(), 5);
    expect_eq(hist.bucketCount(0), 1);
    expect_eq(hist.bucketCount(1), 1);
    expect_eq(hist.bucketCount(2), 1);
    expect_eq(hist.bucketCount(3), 1);
    expect_eq(hist.bucketCount(7), 1);
    expect_eq(hist.minNs(), 50);
    expect_eq(hist.maxNs(), 200000000);
    expect_eq(hist.estimatePercentileBoundNs(50), 10000);
    expect_eq(hist.estimatePercentileBoundNs(99), 200000000);

    hist.reset();
    expect_eq(hist.count(), 0);
    expect_eq(hist.meanNs(), 0);
}

fn testCliSuite(): !void {
    // 1. SliceIterator & CliArgs
    let argv: [8][]const u8 = undefined;
    argv[0] = "build";
    argv[1] = "--release";
    argv[2] = "--target=x86_64-linux";
    argv[3] = "-vf";
    argv[4] = "-p=8080";
    argv[5] = "--";
    argv[6] = "--raw-positional";
    argv[7] = "src/main.cp";

    let siter = SliceIterator.init(&argv);
    expect_eq(siter.remainingCount(), 8);
    expect_eq(std.mem.eql(u8, siter.peek().?, "build"), true);
    expect_eq(std.mem.eql(u8, siter.next().?, "build"), true);
    expect_eq(siter.remainingCount(), 7);

    const parsed = CliArgs.parse(&argv);
    expect_eq(parsed.hasFlag("release"), true);
    expect_eq(parsed.hasFlag("v"), true);
    expect_eq(parsed.hasFlag("f"), true);
    expect_eq(parsed.hasFlag("unknown"), false);
    expect_eq(std.mem.eql(u8, parsed.getOption("target").?, "x86_64-linux"), true);
    expect_eq(std.mem.eql(u8, parsed.getOption("p").?, "8080"), true);
    expect_eq(std.mem.eql(u8, parsed.getOptionOr("missing", "fallback"), "fallback"), true);
    expect_eq(parsed.positional_count, 3);
    expect_eq(std.mem.eql(u8, parsed.positional(0).?, "build"), true);
    expect_eq(std.mem.eql(u8, parsed.positional(1).?, "--raw-positional"), true);
    expect_eq(std.mem.eql(u8, parsed.positional(2).?, "src/main.cp"), true);

    // 2. TtyMode & AnsiColor
    const mode = TtyMode.Raw;
    expect_eq(@intFromEnum(mode), 1);

    let stripped_buf: [64]u8 = undefined;
    const colored = "\x1b[1m\x1b[32mPASS\x1b[0m: 42ms";
    const plain = AnsiColor.stripAnsi(colored, &stripped_buf);
    expect_eq(std.mem.eql(u8, plain, "PASS: 42ms"), true);

    // 3. ProgressBar
    let bar = ProgressBar.init(200, 10);
    expect_eq(bar.percent(), 0);
    expect_eq(bar.isComplete(), false);
    bar.tick(100);
    expect_eq(bar.percent(), 50);

    let bar_buf: [32]u8 = undefined;
    expect_eq(std.mem.eql(u8, bar.renderBar(&bar_buf), "[#####-----]"), true);
    bar.set(250);
    expect_eq(bar.isComplete(), true);
    expect_eq(bar.percent(), 100);
    expect_eq(std.mem.eql(u8, bar.renderBar(&bar_buf), "[##########]"), true);

    // 4. CliTable
    let tbl = CliTable.init(3);
    tbl.setHeader(0, "Name", CliTableAlign.Left);
    tbl.setHeader(1, "Status", CliTableAlign.Center);
    tbl.setHeader(2, "Time", CliTableAlign.Right);
    expect_eq(tbl.addRow3("http", "OK", "12ms"), true);
    expect_eq(tbl.addRow3("bench", "PASS", "3ms"), true);

    let tbl_buf: [512]u8 = undefined;
    const written = tbl.renderToBuf(&tbl_buf);
    const rendered = tbl_buf[0..written];
    expect_eq(written > 40, true);
    expect_eq(std.mem.indexOf(u8, rendered, "| Name  | Status | Time |") != null, true);
    expect_eq(std.mem.indexOf(u8, rendered, "| http  |   OK   | 12ms |") != null, true);
    expect_eq(std.mem.indexOf(u8, rendered, "| bench |  PASS  |  3ms |") != null, true);
}

fn testNapiSuite(): !void {
    // 1. Constants & Status helpers
    expect_eq(NAPI_VERSION, 9);
    expect_eq(NAPI_MODULE_VERSION, 1);
    expect_eq(Napi.isOk(NapiStatus.Ok), true);
    expect_eq(Napi.isOk(NapiStatus.InvalidArg), false);
    Napi.checkStatus(NapiStatus.Ok)?;
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.PendingException), "napi_pending_exception"), true);

    // 2. Value types & TypedArray element sizes
    expect_eq(std.mem.eql(u8, Napi.valueTypeName(NapiValueType.BigInt), "bigint"), true);
    expect_eq(std.mem.eql(u8, Napi.valueTypeName(NapiValueType.Function), "function"), true);
    expect_eq(Napi.typedArrayElementSize(NapiTypedArrayType.Uint8), 1);
    expect_eq(Napi.typedArrayElementSize(NapiTypedArrayType.Int16), 2);
    expect_eq(Napi.typedArrayElementSize(NapiTypedArrayType.Float32), 4);
    expect_eq(Napi.typedArrayElementSize(NapiTypedArrayType.Float64), 8);
    expect_eq(Napi.typedArrayElementSize(NapiTypedArrayType.BigUint64), 8);

    // 3. Property attributes & C-ABI descriptors
    expect_eq(NapiPropertyAttributes.DefaultJsProperty, 7);
    const mod_name = "cp_native_addon";
    const file_name = "addon.cp";
    const mod_desc = Napi.createModuleDescriptor(mod_name.ptr, file_name.ptr, 0, null);
    expect_eq(mod_desc.nm_version, 1);
    expect_eq(mod_desc.nm_flags, 0);

    const prop_name = "computeHash";
    const prop_desc = Napi.createPropertyDescriptor(prop_name.ptr, NapiPropertyAttributes.DefaultMethod);
    expect_eq(prop_desc.attributes, NapiPropertyAttributes.DefaultMethod);

    // 4. Addon Registry
    let reg = NapiAddonRegistry.init();
    expect_eq(reg.register("crypto_native", "crypto_addon.node", 1), true);
    expect_eq(reg.register("sqlite_native", "sqlite_addon.node", 0), true);
    expect_eq(reg.count, 2);

    const found = reg.find("crypto_native");
    expect_eq(found != null, true);
    expect_eq(std.mem.eql(u8, found.?.filename, "crypto_addon.node"), true);
    expect_eq(found.?.napi_version, NAPI_VERSION);
    expect_eq(reg.find("nonexistent") == null, true);
}

fn testBigIntSuite(): !void {
    // 1. Arbitrary-precision BigInt (> 128-bit)
    let a = BigInt.fromString("1234567890123456789012345678901234567890", 10)?;
    defer a.deinit();
    let b = BigInt.fromString("9876543210987654321098765432109876543210", 10)?;
    defer b.deinit();

    let sum = a.add(&b)?;
    defer sum.deinit();
    expect_eq(std.mem.eql(u8, sum.toString(10), "11111111101111111110111111111011111111100"), true);

    let diff = a.sub(&b)?;
    defer diff.deinit();
    expect_eq(diff.isNegative(), true);
    expect_eq(a.order(&b), -1);
    expect_eq(b.order(&a), 1);
    expect_eq(a.eq(&a), true);

    let zero = BigInt.fromInt(0)?;
    defer zero.deinit();
    expect_eq(zero.eqZero(), true);
    expect_eq(zero.isNegative(), false);

    let x = BigInt.fromUint(1000000)?;
    defer x.deinit();
    let y = BigInt.fromUint(3000)?;
    defer y.deinit();
    let q = x.divTrunc(&y)?;
    defer q.deinit();
    let r = x.rem(&y)?;
    defer r.deinit();
    expect_eq(std.mem.eql(u8, q.toString(10), "333"), true);
    expect_eq(std.mem.eql(u8, r.toString(10), "1000"), true);
    expect_eq(x.bitCount(), 20);

    // 2. BigInt256 (Web3 / EVM uint256)
    const v0 = BigInt256.zero();
    const v1 = BigInt256.fromU64(1000);
    const v2 = BigInt256.fromU64(2500);
    expect_eq(v0.cmp(v1), -1);
    expect_eq(v1.add(v2).limbs[0], 3500);
    expect_eq(v2.sub(v1).limbs[0], 1500);
    expect_eq(v1.mul(v2).limbs[0], 2500000);

    const h1 = BigInt256.fromHex("0x10000000000000000")?;
    const h2 = BigInt256.fromHex("0xffffffffffffffff")?;
    expect_eq(h2.add(BigInt256.fromU64(1)).eq(h1), true);
    expect_eq(std.mem.eql(u8, h1.toHex(), "10000000000000000"), true);
}

fn testEventSuite(): !void {
    // 1. EventBus deterministic pub/sub dispatch
    let bus = EventBus.init();
    let counter = EventTestCounter{ total: 0, last_len: 0 };

    expect_eq(bus.on("data", onBusMessage, @ptrCast(&counter)), true);
    expect_eq(bus.once("data", onBusOnce, @ptrCast(&counter)), true);
    expect_eq(bus.listenerCount("data"), 2);

    const fired1 = bus.emit("data", "hello");
    expect_eq(fired1, 2);
    expect_eq(counter.total, 11);
    expect_eq(counter.last_len, 5);
    expect_eq(bus.listenerCount("data"), 1); // once listener auto-removed

    const fired2 = bus.emit("data", "cp!");
    expect_eq(fired2, 1);
    expect_eq(counter.total, 12);
    expect_eq(counter.last_len, 3);

    expect_eq(bus.off("data", onBusMessage), true);
    expect_eq(bus.listenerCount("data"), 0);
    expect_eq(bus.emit("data", "none"), 0);

    expect_eq(bus.on("close", onBusMessage, @ptrCast(&counter)), true);
    expect_eq(bus.removeAllListeners("close"), 1);
    expect_eq(bus.listenerCount("close"), 0);

    // 2. EventEmitter & Event coroutine signal
    let ee = new EventEmitter();
    expect_eq(ee.listenerCount("ready"), 0);
    expect_eq(ee.emit("ready"), false);
    * = ee.on("ready", null);
    expect_eq(ee.listenerCount("ready"), 1);
    expect_eq(ee.emit("ready"), true);
    ee.removeAllListeners("ready");
    expect_eq(ee.listenerCount("ready"), 0);

    let sig = new Event();
    expect_eq(sig.isSet(), false);
    sig.set();
    expect_eq(sig.isSet(), true);
    sig.reset();
    expect_eq(sig.isSet(), false);

    // 3. AnyTask & ManagedTask
    let task_counter = EventTestCounter{ total: 0, last_len: 0 };
    let any_t = AnyTask.init(@ptrCast(&task_counter), onSimpleTask);
    any_t.run();
    expect_eq(task_counter.total, 1);

    let m1 = ManagedTask.create(@ptrCast(&task_counter), onSimpleTask);
    m1.cancel();
    m1.run(); // cancelled -> skipped
    expect_eq(task_counter.total, 1);

    let m2 = ManagedTask.create(@ptrCast(&task_counter), onSimpleTask);
    m2.run();
    expect_eq(task_counter.total, 2);

    // 4. DeferredTaskQueue deduplication & execution
    let dq = DeferredTaskQueue.init();
    expect_eq(dq.postTask(@ptrCast(&task_counter), onSimpleTask), false);
    expect_eq(dq.postTask(@ptrCast(&task_counter), onSimpleTask), true); // deduplicated
    expect_eq(dq.count(), 1);
    expect_eq(dq.run(), 1);
    expect_eq(task_counter.total, 3);
    expect_eq(dq.count(), 0);

    // 5. ConcurrentQueue & EventLoop unified tick
    let cq = ConcurrentQueue.init();
    const ct = ConcurrentTask.create(onSimpleTask, @ptrCast(&task_counter), true);
    cq.push(ct);
    expect_eq(cq.len(), 1);
    expect_eq(cq.drain(8), 1);
    expect_eq(task_counter.total, 4);

    let loop = EventLoop.init();
    * = loop.setTimeout(onSimpleTask, 0, @ptrCast(&task_counter));
    * = loop.deferTask(onSimpleTask, @ptrCast(&task_counter));
    loop.postTask(onSimpleTask, @ptrCast(&task_counter));
    expect_eq(loop.hasActiveHandles(), true);
    const processed = loop.tick(0);
    expect_eq(processed >= 3, true);
    expect_eq(task_counter.total, 7);
    expect_eq(TimerHeap.nowMs() > 0, true);
}

fn main() {
    test("1. cap:bench — HwTimer, Stopwatch, BenchStats & LatencyHistogram", () => {
        testBenchSuite()?;
    });

    test("2. cap:cli — SliceIterator, CliArgs, TtyMode, AnsiColor, ProgressBar & CliTable", () => {
        testCliSuite()?;
    });

    test("3. cap:napi — N-API v9 C-ABI Types, Status Mapping, Descriptors & Addon Registry", () => {
        testNapiSuite()?;
    });

    test("4. cap:bigint — Arbitrary-Precision BigInt & Web3 BigInt256", () => {
        testBigIntSuite()?;
    });

    test("5. cap:event — EventBus, EventEmitter, Event, Tasks, Queues & EventLoop", () => {
        testEventSuite()?;
    });
}
