// Source: cp/cap/test/test_vcd_production_deep.cp (Production deep tests for cap:vcd)

import { test, expect_eq } from "cap:test";
import {
    VcdTimeUnit,
    VcdVarType,
    VcdEventKind,
    VcdTimescale,
    VcdVarDef,
    VcdHeader,
    VcdEvent,
    parseVcdTimescale,
    parseVcdHeader,
    VcdVarIter,
    VcdValueIter,
} from "cap:vcd";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

const SAMPLE_VCD: []const u8 =
    "$date\n  Nov 22 2026\n$end\n" +
    "$version\n  Verilator 5.024\n$end\n" +
    "$timescale 10ns $end\n" +
    "$scope module top $end\n" +
    "  $var wire 1 ! clk $end\n" +
    "  $var reg 8 \" data [7:0] $end\n" +
    "  $scope module alu $end\n" +
    "    $var real 64 # voltage $end\n" +
    "    $var integer 32 $ count $end\n" +
    "  $upscope $end\n" +
    "$upscope $end\n" +
    "$enddefinitions $end\n" +
    "$dumpvars\n" +
    "0!\n" +
    "b00000000 \"\n" +
    "r3.30 #\n" +
    "$end\n" +
    "#10\n" +
    "1!\n" +
    "b10101100 \"\n" +
    "#25\n" +
    "x!\n" +
    "z!\n" +
    "r1.25e-3 #\n" +
    "$dumpoff\n" +
    "$dumpon\n" +
    "$dumpall\n";

test("1. parses Verilator/Icarus VCD header with date, version, timescale, nested scopes, and body_offset", () => {
    if (hdr := parseVcdHeader(SAMPLE_VCD)) {
        expect_eq(sliceEq(hdr.date, "Nov 22 2026"), true);
        expect_eq(sliceEq(hdr.version, "Verilator 5.024"), true);
        expect_eq(hdr.timescale.magnitude, 10);
        expect_eq(hdr.timescale.unit == VcdTimeUnit.ns, true);
        expect_eq(hdr.var_count, 4);
        expect_eq(hdr.body_offset > 0, true);
        expect_eq(hdr.body_offset < SAMPLE_VCD.len, true);
    } else {
        expect_eq(true, false);
    }
});

test("2. VcdVarIter iterates multi-scope signal definitions accurately", () => {
    let it = VcdVarIter.init(SAMPLE_VCD);

    if (v0 := it.next()) {
        expect_eq(v0.var_type == VcdVarType.wire, true);
        expect_eq(v0.width, 1);
        expect_eq(sliceEq(v0.id_code, "!"), true);
        expect_eq(sliceEq(v0.reference, "clk"), true);
    } else {
        expect_eq(true, false);
    }

    if (v1 := it.next()) {
        expect_eq(v1.var_type == VcdVarType.reg, true);
        expect_eq(v1.width, 8);
        expect_eq(sliceEq(v1.id_code, "\""), true);
        expect_eq(sliceEq(v1.reference, "data [7:0]"), true);
    } else {
        expect_eq(true, false);
    }

    if (v2 := it.next()) {
        expect_eq(v2.var_type == VcdVarType.real, true);
        expect_eq(v2.width, 64);
        expect_eq(sliceEq(v2.id_code, "#"), true);
        expect_eq(sliceEq(v2.reference, "voltage"), true);
    } else {
        expect_eq(true, false);
    }

    if (v3 := it.next()) {
        expect_eq(v3.var_type == VcdVarType.integer, true);
        expect_eq(v3.width, 32);
        expect_eq(sliceEq(v3.id_code, "$"), true);
        expect_eq(sliceEq(v3.reference, "count"), true);
    } else {
        expect_eq(true, false);
    }

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. VcdValueIter streams timestamps, scalar 0/1/x/z, binary vectors, reals, and dump controls", () => {
    if (hdr := parseVcdHeader(SAMPLE_VCD)) {
        let it = VcdValueIter.fromHeader(SAMPLE_VCD, hdr);

        // $dumpvars
        if (e := it.next()) { expect_eq(e.kind == VcdEventKind.dumpvars, true); } else { expect_eq(true, false); }
        // 0!
        if (e := it.next()) {
            expect_eq(e.kind == VcdEventKind.scalar_change, true);
            expect_eq(e.scalar_val, '0');
            expect_eq(sliceEq(e.id_code, "!"), true);
        } else { expect_eq(true, false); }
        // b00000000 "
        if (e := it.next()) {
            expect_eq(e.kind == VcdEventKind.vector_change, true);
            expect_eq(sliceEq(e.value, "00000000"), true);
            expect_eq(sliceEq(e.id_code, "\""), true);
        } else { expect_eq(true, false); }
        // r3.30 #
        if (e := it.next()) {
            expect_eq(e.kind == VcdEventKind.real_change, true);
            expect_eq(sliceEq(e.value, "3.30"), true);
            expect_eq(sliceEq(e.id_code, "#"), true);
        } else { expect_eq(true, false); }
        // $end
        if (e := it.next()) { expect_eq(e.kind == VcdEventKind.end, true); } else { expect_eq(true, false); }
        // #10
        if (e := it.next()) {
            expect_eq(e.kind == VcdEventKind.timestamp, true);
            expect_eq(e.timestamp, 10);
        } else { expect_eq(true, false); }
        // 1!
        if (e := it.next()) {
            expect_eq(e.kind == VcdEventKind.scalar_change, true);
            expect_eq(e.scalar_val, '1');
        } else { expect_eq(true, false); }
        // b10101100 "
        if (e := it.next()) {
            expect_eq(e.kind == VcdEventKind.vector_change, true);
            expect_eq(sliceEq(e.value, "10101100"), true);
        } else { expect_eq(true, false); }
        // #25
        if (e := it.next()) {
            expect_eq(e.kind == VcdEventKind.timestamp, true);
            expect_eq(e.timestamp, 25);
        } else { expect_eq(true, false); }
        // x! and z!
        if (e := it.next()) { expect_eq(e.scalar_val, 'x'); } else { expect_eq(true, false); }
        if (e := it.next()) { expect_eq(e.scalar_val, 'z'); } else { expect_eq(true, false); }
        // r1.25e-3 #
        if (e := it.next()) {
            expect_eq(e.kind == VcdEventKind.real_change, true);
            expect_eq(sliceEq(e.value, "1.25e-3"), true);
        } else { expect_eq(true, false); }
        // $dumpoff, $dumpon, $dumpall
        if (e := it.next()) { expect_eq(e.kind == VcdEventKind.dumpoff, true); } else { expect_eq(true, false); }
        if (e := it.next()) { expect_eq(e.kind == VcdEventKind.dumpon, true); } else { expect_eq(true, false); }
        if (e := it.next()) { expect_eq(e.kind == VcdEventKind.dumpall, true); } else { expect_eq(true, false); }

        expect_eq(it.next() == null, true);
        expect_eq(it.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("4. parseVcdTimescale covers all IEEE 1364 units (s/ms/us/ns/ps/fs) and magnitudes (1/10/100)", () => {
    if (t := parseVcdTimescale("1s")) {
        expect_eq(t.magnitude, 1);
        expect_eq(t.unit == VcdTimeUnit.s, true);
    } else { expect_eq(true, false); }

    if (t := parseVcdTimescale("10 ms")) {
        expect_eq(t.magnitude, 10);
        expect_eq(t.unit == VcdTimeUnit.ms, true);
    } else { expect_eq(true, false); }

    if (t := parseVcdTimescale("100us")) {
        expect_eq(t.magnitude, 100);
        expect_eq(t.unit == VcdTimeUnit.us, true);
    } else { expect_eq(true, false); }

    if (t := parseVcdTimescale("1ns")) {
        expect_eq(t.unit == VcdTimeUnit.ns, true);
    } else { expect_eq(true, false); }

    if (t := parseVcdTimescale("  10 ps  ")) {
        expect_eq(t.magnitude, 10);
        expect_eq(t.unit == VcdTimeUnit.ps, true);
    } else { expect_eq(true, false); }

    if (t := parseVcdTimescale("100fs")) {
        expect_eq(t.magnitude, 100);
        expect_eq(t.unit == VcdTimeUnit.fs, true);
    } else { expect_eq(true, false); }

    // Invalid magnitudes and units
    expect_eq(parseVcdTimescale("5ns") == null, true);
    expect_eq(parseVcdTimescale("1000ps") == null, true);
    expect_eq(parseVcdTimescale("10ks") == null, true);
    expect_eq(parseVcdTimescale("") == null, true);
});

test("5. rejects missing $enddefinitions, malformed #timestamp, and truncated vector/scalar tokens", () => {
    // Missing $enddefinitions
    const no_enddef = "$timescale 1ns $end\n$var wire 1 ! clk $end\n";
    expect_eq(parseVcdHeader(no_enddef) == null, true);

    // Malformed #timestamp
    let bad_ts = VcdValueIter.init("#12abc\n0!\n", 0);
    expect_eq(bad_ts.next() == null, true);
    expect_eq(bad_ts.had_error, true);

    // Empty #timestamp
    let empty_ts = VcdValueIter.init("#\n", 0);
    expect_eq(empty_ts.next() == null, true);
    expect_eq(empty_ts.had_error, true);

    // Vector missing id_code
    let bad_vec = VcdValueIter.init("b1010", 0);
    expect_eq(bad_vec.next() == null, true);
    expect_eq(bad_vec.had_error, true);

    // Scalar missing id_code
    let bad_scalar = VcdValueIter.init("1", 0);
    expect_eq(bad_scalar.next() == null, true);
    expect_eq(bad_scalar.had_error, true);
});
