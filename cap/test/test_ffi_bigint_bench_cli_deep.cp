// Source: /data/dev/bun/src/runtime/ffi/ & /data/dev/bun/src/jsc/JSBigInt.cp & /data/dev/bun/src/perf/ & /data/dev/bun/src/cli/
// cap/test/test_ffi_bigint_bench_cli_deep.cp
// Comprehensive Deep Audit Test Suite for cap:ffi, cap:bigint, cap:bench, cap:cli

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    FFIType,
    DlFlags,
    FFISignature,
    FfiCallbackDesc,
    DynLib,
    CString,
    CStructField,
    CStructLayout,
} from "cap:ffi";
import { BigInt, BigInt256 } from "cap:bigint";
import {
    HwTimer,
    Stopwatch,
    BenchStats,
    LatencyHistogram,
    TracePhase,
    TraceEvent,
    TraceRingBuffer,
    PerfTrace,
    BenchSuite,
} from "cap:bench";
import {
    TtyMode,
    TerminalSize,
    Terminal,
    AnsiColor,
    SpinnerStyle,
    SpinnerState,
    Spinner,
    ProgressBar,
    CliTableAlign,
    CliTableBorder,
    CliTable,
} from "cap:cli";

type StrlenFn = @c ((cstr): usize);
type AbsFn = @c ((i32): i32);

fn dummyBenchWork() {
    let acc: u64 = 0;
    let i: u64 = 0;
    while (i < 32) {
        acc = acc +% (i * 3 + 1);
        i += 1;
    }
    std.mem.doNotOptimizeAway(acc);
}

fn testFfiDeep(): !void {
    // 1. Complete 16-type C-ABI type matrix
    expect_eq(FFIType.sizeOf(FFIType.Void), 0);
    expect_eq(FFIType.alignOf(FFIType.Void), 1);
    expect_eq(FFIType.sizeOf(FFIType.Bool), 1);
    expect_eq(FFIType.sizeOf(FFIType.I8), 1);
    expect_eq(FFIType.sizeOf(FFIType.U8), 1);
    expect_eq(FFIType.sizeOf(FFIType.I16), 2);
    expect_eq(FFIType.alignOf(FFIType.I16), 2);
    expect_eq(FFIType.sizeOf(FFIType.U16), 2);
    expect_eq(FFIType.sizeOf(FFIType.I32), 4);
    expect_eq(FFIType.sizeOf(FFIType.U32), 4);
    expect_eq(FFIType.sizeOf(FFIType.F32), 4);
    expect_eq(FFIType.sizeOf(FFIType.I64), 8);
    expect_eq(FFIType.sizeOf(FFIType.U64), 8);
    expect_eq(FFIType.sizeOf(FFIType.F64), 8);
    expect_eq(FFIType.sizeOf(FFIType.Ptr), 8);
    expect_eq(FFIType.sizeOf(FFIType.CString), 8);
    expect_eq(FFIType.sizeOf(FFIType.Buffer), 8);
    expect_eq(FFIType.sizeOf(FFIType.Function), 8);
    expect_eq(FFIType.alignOf(FFIType.Buffer), 8);
    expect_eq(FFIType.alignOf(FFIType.Function), 8);

    expect_eq(FFIType.isInteger(FFIType.I8), true);
    expect_eq(FFIType.isInteger(FFIType.U64), true);
    expect_eq(FFIType.isInteger(FFIType.F64), false);
    expect_eq(FFIType.isInteger(FFIType.Buffer), false);
    expect_eq(FFIType.isFloat(FFIType.F32), true);
    expect_eq(FFIType.isFloat(FFIType.F64), true);
    expect_eq(FFIType.isFloat(FFIType.I32), false);
    expect_eq(FFIType.isSigned(FFIType.I16), true);
    expect_eq(FFIType.isSigned(FFIType.U16), false);
    expect_eq(FFIType.isPointer(FFIType.Buffer), true);
    expect_eq(FFIType.isPointer(FFIType.Function), true);
    expect_eq(FFIType.fromName("buffer").? == FFIType.Buffer, true);
    expect_eq(FFIType.fromName("function").? == FFIType.Function, true);
    expect_eq(std.mem.eql(u8, FFIType.typeName(FFIType.Function), "function"), true);

    // 2. CStructLayout: natural alignment, #packed, nested sub-struct, and >64 fields
    let inner = CStructLayout.init();
    expect_eq(inner.addNamedField("tag", FFIType.U8), 0);
    expect_eq(inner.addNamedField("val", FFIType.U64), 8);
    expect_eq(inner.finalize(), 16);
    expect_eq(inner.paddingBytes(), 7);

    let outer = CStructLayout.init();
    expect_eq(outer.addNamedField("head", FFIType.U8), 0);
    expect_eq(outer.addNamedStruct("inner", &inner), 8);
    expect_eq(outer.addNamedField("tail", FFIType.U8), 24);
    expect_eq(outer.finalize(), 32);
    expect_eq(outer.max_align, 8);
    expect_eq(outer.trailingPadding(), 7);
    expect_eq(outer.offsetOf("inner").?, 8);
    expect_eq(outer.offsetOf("tail").?, 24);

    let packed_layout = CStructLayout.initPacked();
    expect_eq(packed_layout.addField(FFIType.U8), 0);
    expect_eq(packed_layout.addField(FFIType.U32), 1);
    expect_eq(packed_layout.addField(FFIType.F64), 5);
    expect_eq(packed_layout.addField(FFIType.U8), 13);
    expect_eq(packed_layout.finalize(), 14);
    expect_eq(packed_layout.max_align, 1);
    expect_eq(packed_layout.paddingBytes(), 0);

    let many_layout = CStructLayout.init();
    let fi: usize = 0;
    while (fi < 80) {
        * = many_layout.addField(FFIType.U32);
        fi += 1;
    }
    expect_eq(many_layout.field_count, 80);
    expect_eq(many_layout.finalize(), 320);

    // 3. Real POSIX dlopen / dlsym / dlclose / dlerror
    let self_lib = DynLib.openSelf(DlFlags.RTLD_LAZY);
    expect_eq(self_lib.is_open, true);
    const strlen_sym = self_lib.symbol("strlen");
    const abs_sym = self_lib.symbol("abs");
    expect_eq(strlen_sym != null, true);
    expect_eq(abs_sym != null, true);
    expect_eq(self_lib.symbol("cp_nonexistent_symbol_xyz_999") == null, true);
    expect_eq(self_lib.lastError() != null, true);

    // Invoke real resolved libc functions through dlsym function pointers
    unsafe {
        const c_strlen: StrlenFn = @ptrCast(@alignCast(strlen_sym.?));
        const c_abs: AbsFn = @ptrCast(@alignCast(abs_sym.?));
        expect_eq(c_strlen("hello cp"), 8);
        expect_eq(c_abs(-42), 42);
    }
    self_lib.close();
    expect_eq(self_lib.is_open, false);

    let libc = DynLib.open("libc.so.6", DlFlags.RTLD_LAZY);
    expect_eq(libc.is_open, true);
    expect_eq(libc.hasSymbol("strlen"), true);
    expect_eq(libc.hasSymbol("abs"), true);
    libc.close();

    let missing = DynLib.open("lib_missing_cp_test_404.so", DlFlags.RTLD_LAZY);
    expect_eq(missing.is_open, false);
    expect_eq(missing.lastError() != null, true);

    // 4. CString & FfiCallbackDesc
    const sample_z: [:0]const u8 = "cap-ffi-cstr";
    const cstr_obj = CString.fromSlice(sample_z);
    expect_eq(cstr_obj.len(), 12);
    expect_eq(cstr_obj.eql("cap-ffi-cstr"), true);
    expect_eq(cstr_obj.byteAt(0).?, 'c');
    expect_eq(cstr_obj.byteAt(12) == null, true);

    let cb_sig = FFISignature.init(FFIType.I32);
    * = cb_sig.addArg(FFIType.Ptr);
    let cb = FfiCallbackDesc.init(cb_sig, abs_sym, null, true);
    expect_eq(cb.isValid(), true);
    expect_eq(cb.threadsafe, true);
    cb.close();
    expect_eq(cb.isValid(), false);
}

fn testBigIntDeep(): !void {
    // 1. Bidirectional radix conversions with prefixes & negatives
    let h_neg = BigInt.fromHex("-0x1a2b3c4d5e6f708090a0b0c0d0e0f0")?;
    defer h_neg.deinit();
    expect_eq(h_neg.isNegative(), true);
    expect_eq(std.mem.eql(u8, h_neg.toHex(), "-1a2b3c4d5e6f708090a0b0c0d0e0f0"), true);
    expect_eq(std.mem.eql(u8, h_neg.toHexPrefixed(), "-0x1a2b3c4d5e6f708090a0b0c0d0e0f0"), true);

    let b_val = BigInt.fromBinary("0b11010110")?;
    defer b_val.deinit();
    expect_eq(std.mem.eql(u8, b_val.toDecimal(), "214"), true);
    expect_eq(std.mem.eql(u8, b_val.toBinary(), "11010110"), true);

    // 2. divMod, mod (including negative dividend), cmp/lt/gt
    let neg_div = BigInt.fromDecimal("-17")?;
    defer neg_div.deinit();
    let divisor = BigInt.fromDecimal("5")?;
    defer divisor.deinit();

    let dm = neg_div.divMod(&divisor)?;
    defer dm.deinit();
    expect_eq(std.mem.eql(u8, dm.quotient.toDecimal(), "-3"), true);
    expect_eq(std.mem.eql(u8, dm.remainder.toDecimal(), "-2"), true);

    let floored_m = neg_div.mod(&divisor)?;
    defer floored_m.deinit();
    expect_eq(std.mem.eql(u8, floored_m.toDecimal(), "3"), true);
    expect_eq(neg_div.lt(&divisor), true);
    expect_eq(divisor.gt(&neg_div), true);

    // 3. Bitwise, pow, gcd, and RSA-style modPow
    let bw1 = BigInt.fromHex("0xff00f0")?;
    defer bw1.deinit();
    let bw2 = BigInt.fromHex("0x0ff0aa")?;
    defer bw2.deinit();
    let band = bw1.bitAnd(&bw2)?;
    defer band.deinit();
    let bor = bw1.bitOr(&bw2)?;
    defer bor.deinit();
    let bxor = bw1.bitXor(&bw2)?;
    defer bxor.deinit();
    expect_eq(std.mem.eql(u8, band.toHex(), "f00a0"), true);
    expect_eq(std.mem.eql(u8, bor.toHex(), "fff0fa"), true);
    expect_eq(std.mem.eql(u8, bxor.toHex(), "f0f05a"), true);

    let two = BigInt.fromUint(2)?;
    defer two.deinit();
    let p100 = two.pow(100)?;
    defer p100.deinit();
    let shl100 = BigInt.fromUint(1)?;
    defer shl100.deinit();
    let shifted = shl100.shl(100)?;
    defer shifted.deinit();
    expect_eq(p100.eq(&shifted), true);
    let back = shifted.shr(100)?;
    defer back.deinit();
    expect_eq(std.mem.eql(u8, back.toDecimal(), "1"), true);

    let g1 = BigInt.fromDecimal("123456789012345678900")?;
    defer g1.deinit();
    let g2 = BigInt.fromDecimal("987654321098765432100")?;
    defer g2.deinit();
    let g_res = g1.gcd(&g2)?;
    defer g_res.deinit();
    expect_eq(std.mem.eql(u8, g_res.toDecimal(), "9000000000900"), true);

    // RSA roundtrip: p=61, q=53 => n=3233, e=17, d=2753; also 64-bit+ modPow
    let msg = BigInt.fromUint(65)?;
    defer msg.deinit();
    let e_exp = BigInt.fromUint(17)?;
    defer e_exp.deinit();
    let d_exp = BigInt.fromUint(2753)?;
    defer d_exp.deinit();
    let n_mod = BigInt.fromUint(3233)?;
    defer n_mod.deinit();
    let cipher = msg.modPow(&e_exp, &n_mod)?;
    defer cipher.deinit();
    expect_eq(std.mem.eql(u8, cipher.toDecimal(), "2790"), true);
    let plain = cipher.modPow(&d_exp, &n_mod)?;
    defer plain.deinit();
    expect_eq(plain.eq(&msg), true);

    // Large 128-bit+ modPow verification: (base^e) mod m
    let big_base = BigInt.fromDecimal("98765432109876543210987654321")?;
    defer big_base.deinit();
    let big_exp = BigInt.fromDecimal("65537")?;
    defer big_exp.deinit();
    let big_mod = BigInt.fromDecimal("340282366920938463463374607431768211297")?;
    defer big_mod.deinit();
    let mp_res = big_base.modPow(&big_exp, &big_mod)?;
    defer mp_res.deinit();
    expect_eq(mp_res.lt(&big_mod), true);
    expect_eq(mp_res.isPositive(), true);

    // 4. BigInt256 full coverage
    const d256 = BigInt256.fromDecimal("1000000000000000000000000")?;
    expect_eq(std.mem.eql(u8, d256.toDecimal(), "1000000000000000000000000"), true);
    const b256 = BigInt256.fromBinary("0b10101010")?;
    expect_eq(b256.limbs[0], 170);

    const dv = d256.divMod(BigInt256.fromU64(97))?;
    const recon = dv.quotient.mul(BigInt256.fromU64(97)).add(dv.remainder);
    expect_eq(recon.eq(d256), true);

    const sh = BigInt256.one().shl(200);
    expect_eq(sh.shr(200).eq(BigInt256.one()), true);
    expect_eq(BigInt256.one().shl(256).isZero(), true);

    const max_u256 = BigInt256.max();
    const ov_add = max_u256.addOverflow(BigInt256.one());
    expect_eq(ov_add.overflow, true);
    expect_eq(ov_add.value.isZero(), true);
    const ov_sub = BigInt256.zero().subOverflow(BigInt256.one());
    expect_eq(ov_sub.overflow, true);
    const ov_mul = max_u256.mulOverflow(BigInt256.fromU64(2));
    expect_eq(ov_mul.overflow, true);
    const ok_add = BigInt256.fromU64(10).addChecked(BigInt256.fromU64(20))?;
    expect_eq(ok_add.limbs[0], 30);
}

fn testBenchDeep(): !void {
    // 1. BenchStats complete quantiles & Welford stdDev
    let stats = BenchStats.init();
    let i: u64 = 1;
    while (i <= 100) {
        stats.record(i * 100);
        i += 1;
    }
    expect_eq(stats.count, 100);
    expect_eq(stats.minNs(), 100);
    expect_eq(stats.maxNs(), 10000);
    expect_eq(stats.meanNs(), 5050);
    expect_eq(stats.medianNs(), 5000);
    expect_eq(stats.p75Ns(), 7500);
    expect_eq(stats.p90Ns(), 9000);
    expect_eq(stats.p95Ns(), 9500);
    expect_eq(stats.p99Ns(), 9900);
    expect_eq(stats.p999Ns(), 9900);
    expect_eq(stats.stdDevNs() > 2800 && stats.stdDevNs() < 3000, true);
    expect_eq(stats.opsPerSec() > 190000, true);

    // 2. PerfTrace / TraceRingBuffer & Chrome Trace JSON export
    let ring = TraceRingBuffer<4>.initWithIds(100, 1);
    const sp = ring.begin("parse", "bundler");
    ring.instant("cache_miss", "io");
    * = ring.endSpan(sp);
    ring.complete("emit", "codegen", 1000, 2500);
    ring.complete("link", "codegen", 3500, 1500); // overwrites oldest slot (cap=4, total=5)
    expect_eq(ring.len(), 4);
    expect_eq(ring.totalRecorded(), 5);
    expect_eq(ring.droppedCount(), 1);

    let trace_json_buf: [2048]u8 = undefined;
    const trace_json = ring.exportChromeTraceJson(&trace_json_buf);
    expect_eq(std.mem.indexOf(u8, trace_json, "\"traceEvents\":[") != null, true);
    expect_eq(std.mem.indexOf(u8, trace_json, "\"name\":\"cache_miss\"") != null, true);
    expect_eq(std.mem.indexOf(u8, trace_json, "\"ph\":\"X\"") != null, true);

    let pt = PerfTrace.init();
    pt.instant("boot", "runtime");
    expect_eq(pt.len(), 1);

    // 3. BenchSuite warmup, execution, Markdown & JSON export
    let suite = BenchSuite.init("Crypto & Loop Bench", 3, 20, 10000, 200);
    suite.run("dummy_loop", dummyBenchWork);
    suite.recordCase("synthetic", stats);
    expect_eq(suite.caseCount(), 2);

    let md_buf: [2048]u8 = undefined;
    const md = suite.toMarkdown(&md_buf);
    expect_eq(std.mem.indexOf(u8, md, "| Benchmark | Iters |") != null, true);
    expect_eq(std.mem.indexOf(u8, md, "| dummy_loop |") != null, true);
    expect_eq(std.mem.indexOf(u8, md, "| synthetic |") != null, true);

    let json_buf: [2048]u8 = undefined;
    const js = suite.toJson(&json_buf);
    expect_eq(std.mem.indexOf(u8, js, "\"suite\":\"Crypto & Loop Bench\"") != null, true);
    expect_eq(std.mem.indexOf(u8, js, "\"p999_ns\":9900") != null, true);
}

fn testCliDeep(): !void {
    // 1. AnsiColor 16/256/RGB & stripAnsi
    let fg256_buf: [32]u8 = undefined;
    const c256 = AnsiColor.fg256(196, &fg256_buf);
    expect_eq(std.mem.eql(u8, c256, "\x1b[38;5;196m"), true);

    let rgb_buf: [32]u8 = undefined;
    const crgb = AnsiColor.fgRgb(255, 128, 0, &rgb_buf);
    expect_eq(std.mem.eql(u8, crgb, "\x1b[38;2;255;128;0m"), true);

    expect_eq(std.mem.eql(u8, AnsiColor.STRIKETHROUGH, "\x1b[9m"), true);
    expect_eq(std.mem.eql(u8, AnsiColor.BRIGHT_CYAN, "\x1b[96m"), true);
    expect_eq(std.mem.eql(u8, AnsiColor.BG_BLUE, "\x1b[44m"), true);

    let strip_buf: [128]u8 = undefined;
    const styled = "\x1b[1m\x1b[38;2;255;0;0mERR\x1b[0m \x1b]8;;https://cp-lang.org\x07link\x1b]8;;\x07";
    const plain = AnsiColor.stripAnsi(styled, &strip_buf);
    expect_eq(std.mem.eql(u8, plain, "ERR link"), true);

    // 2. Terminal ioctl fallback & size probing
    const invalid_sz = Terminal.getSize(-1);
    expect_eq(invalid_sz.cols, 80);
    expect_eq(invalid_sz.rows, 24);
    expect_eq(Terminal.isTty(-1), false);
    const out_sz = Terminal.getSize(1);
    expect_eq(out_sz.cols > 0, true);
    expect_eq(out_sz.rows > 0, true);

    // 3. Spinner animation & ProgressBar ETA/rate
    let sp = Spinner.init("Compiling", SpinnerStyle.Dots);
    expect_eq(std.mem.eql(u8, sp.tick(), "⠋"), true);
    expect_eq(std.mem.eql(u8, sp.tick(), "⠙"), true);
    sp.succeed("Built");
    let sp_buf: [64]u8 = undefined;
    expect_eq(std.mem.eql(u8, sp.render(&sp_buf), "✔ Built"), true);
    sp.fail("Error");
    expect_eq(std.mem.eql(u8, sp.render(&sp_buf), "✖ Error"), true);

    let pb = ProgressBar.initWithTime(100, 10, 1000000000, "DL");
    pb.tickAt(50, 3000000000); // 50 items in 2s => 25/s, ETA 2s
    expect_eq(pb.percent(), 50);
    expect_eq(pb.itemsPerSec(), 25);
    expect_eq(pb.etaSeconds(), 2);
    let pb_buf: [128]u8 = undefined;
    const line = pb.renderLine(&pb_buf);
    expect_eq(std.mem.indexOf(u8, line, "DL [#####-----] 50% (50/100, 25/s, ETA 2s)") != null, true);

    // 4. CliTable with CJK/Emoji display width & Box/Markdown borders
    let tbl_box = CliTable.init(3);
    tbl_box.setBorder(CliTableBorder.Box);
    tbl_box.setHeader(0, "名称", CliTableAlign.Left);
    tbl_box.setHeader(1, "图标", CliTableAlign.Center);
    tbl_box.setHeader(2, "耗时", CliTableAlign.Right);
    expect_eq(tbl_box.addRow3("测试", "🚀", "12ms"), true);

    let box_buf: [512]u8 = undefined;
    const box_len = tbl_box.renderToBuf(&box_buf);
    const box_out = box_buf[0..box_len];
    expect_eq(std.mem.indexOf(u8, box_out, "┌") != null, true);
    expect_eq(std.mem.indexOf(u8, box_out, "│ 测试 │  🚀  │ 12ms │") != null, true);
    expect_eq(std.mem.indexOf(u8, box_out, "└") != null, true);

    let tbl_md = CliTable.init(2);
    tbl_md.setBorder(CliTableBorder.Markdown);
    tbl_md.setHeader(0, "Lang", CliTableAlign.Left);
    tbl_md.setHeader(1, "Score", CliTableAlign.Right);
    expect_eq(tbl_md.addRow2("CP", "100"), true);
    let md_buf: [256]u8 = undefined;
    const md_len = tbl_md.renderToBuf(&md_buf);
    const md_out = md_buf[0..md_len];
    expect_eq(std.mem.indexOf(u8, md_out, "| :--") != null, true);
    expect_eq(std.mem.indexOf(u8, md_out, "---: |") != null, true);
}

fn main() {
    test("1. cap:ffi — Full 16-type C-ABI, CStructLayout (#packed/nested), real POSIX dlopen/dlsym & CString", () => {
        testFfiDeep()?;
    });
    test("2. cap:bigint — BigInt divMod/mod/bitwise/pow/gcd/modPow, radix prefixes & BigInt256 overflow", () => {
        testBigIntDeep()?;
    });
    test("3. cap:bench — Quantiles (p50..p999/stdDev), PerfTrace Chrome JSON & BenchSuite Markdown/JSON", () => {
        testBenchDeep()?;
    });
    test("4. cap:cli — 16/256/RGB ANSI, Terminal ioctl, Spinner/ProgressBar ETA & CJK/Box/Markdown CliTable", () => {
        testCliDeep()?;
    });
}
