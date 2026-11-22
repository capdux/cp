// Source: cp/cap/test/test_gcode_production_deep.cp (Production deep tests for cap:gcode)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GcodeWord,
    GcodeLine,
    GcodeStats,
    gcodeChecksum,
    verifyGcodeChecksum,
    GcodeWordIter,
    parseGcodeLine,
    GcodeIter,
    summarizeGcode,
} from "cap:science";

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < 0.000001;
}

test("1. Compact & spaced G-Code word iteration with semicolon and parenthesis comments", () => {
    const compact = "g1X10.5y-2.25Z0.2E1.5F1800 ; compact move";
    let wit = GcodeWordIter.init(compact);

    const w0 = wit.next() ?? unreachable;
    expect_eq(w0.letter, 'G');
    expect_eq(approxEq(w0.value, 1.0), true);

    const w1 = wit.next() ?? unreachable;
    expect_eq(w1.letter, 'X');
    expect_eq(approxEq(w1.value, 10.5), true);
    expect_eq(std.mem.eql(u8, w1.raw_value, "10.5"), true);

    const w2 = wit.next() ?? unreachable;
    expect_eq(w2.letter, 'Y');
    expect_eq(approxEq(w2.value, -2.25), true);

    const w3 = wit.next() ?? unreachable;
    expect_eq(w3.letter, 'Z');
    expect_eq(approxEq(w3.value, 0.2), true);

    const w4 = wit.next() ?? unreachable;
    expect_eq(w4.letter, 'E');
    expect_eq(approxEq(w4.value, 1.5), true);

    const w5 = wit.next() ?? unreachable;
    expect_eq(w5.letter, 'F');
    expect_eq(approxEq(w5.value, 1800.0), true);

    expect_eq(wit.next() == null, true);

    // Inline parenthesis comment stripping
    const spaced = "N42 (inline note) G0 X+12.0 Y3.5 (tail note) S255";
    const gl = parseGcodeLine(spaced);
    expect_eq(gl.line_number ?? 0, 42);
    expect_eq(gl.has_command, true);
    expect_eq(gl.command_letter, 'G');
    expect_eq(gl.command_code, 0);
    expect_eq(approxEq(gl.x, 12.0), true);
    expect_eq(approxEq(gl.y, 3.5), true);
    expect_eq(approxEq(gl.s, 255.0), true);
    expect_eq(std.mem.eql(u8, gl.comment, "inline note"), true);
});

test("2. RepRap/Marlin XOR checksum calculation and verification", () => {
    const prefix = "N123 G1 X10";
    const cs = gcodeChecksum(prefix);
    // Compute expected manually via loop check and verify full formatted line
    let expected: u8 = 0;
    let i: usize = 0;
    while (i < prefix.len) {
        expected = expected ^ prefix[i];
        i += 1;
    }
    expect_eq(cs, expected);

    // "N123 G1 X10" XOR == 113 -> "N123 G1 X10*113"
    expect_eq(cs, 113);
    expect_eq(verifyGcodeChecksum("N123 G1 X10*113"), true);
    expect_eq(verifyGcodeChecksum("N123 G1 X10*113 ; trailing comment\r\n"), true);

    // Wrong checksum / missing '*' / non-numeric checksum
    expect_eq(verifyGcodeChecksum("N123 G1 X10*114"), false);
    expect_eq(verifyGcodeChecksum("N123 G1 X10"), false);
    expect_eq(verifyGcodeChecksum("N123 G1 X10*abc"), false);
    expect_eq(verifyGcodeChecksum("*0"), false);
    expect_eq(verifyGcodeChecksum(""), false);
});

test("3. GcodeIter skips blank/comment-only lines and handles CRLF", () => {
    const prog =
        "; Header comment\r\n" ++
        "\r\n" ++
        "   (Another comment only line)\n" ++
        "G21 ; millimeters\r\n" ++
        "M104 S200 ; hotend temp\n" ++
        "T0\r\n";

    let it = GcodeIter.init(prog);

    const l1 = it.next() ?? unreachable;
    expect_eq(l1.command_letter, 'G');
    expect_eq(l1.command_code, 21);
    expect_eq(std.mem.eql(u8, l1.comment, "millimeters"), true);

    const l2 = it.next() ?? unreachable;
    expect_eq(l2.command_letter, 'M');
    expect_eq(l2.command_code, 104);
    expect_eq(approxEq(l2.s, 200.0), true);

    const l3 = it.next() ?? unreachable;
    expect_eq(l3.command_letter, 'T');
    expect_eq(l3.command_code, 0);

    expect_eq(it.next() == null, true);
});

test("4. summarizeGcode tracks G90/G91, M82/M83, G92, G20/G21 for 3D bounding box and extrusion", () => {
    const prog =
        "G21 ; metric\n" ++
        "G90 ; absolute XYZ & E\n" ++
        "G92 E0\n" ++
        "G1 Z0.2 F600\n" ++
        "G1 X10 Y20 E2.5 F1800\n" ++
        "G1 X-5 Y30 E5.0\n" ++
        "G92 E0 ; reset extruder origin\n" ++
        "M83 ; relative extrusion\n" ++
        "G91 ; relative XYZ & E\n" ++
        "G1 Z0.2\n" ++
        "G1 X25 Y-10 E3.25\n" ++
        "G90 ; back to absolute\n" ++
        "M82 ; absolute extrusion\n" ++
        "G92 E0\n" ++
        "G1 Z0.6 E1.25\n";

    const st = summarizeGcode(prog);
    expect_eq(st.is_metric, true);
    expect_eq(st.command_count, 15);
    expect_eq(st.move_count, 6);
    expect_eq(st.layer_changes, 3); // 0->0.2, 0.2->0.4, 0.4->0.6

    // X visited: 0, 10, -5, 20 (-5+25) -> min=-5, max=20
    expect_eq(approxEq(st.min_x, -5.0), true);
    expect_eq(approxEq(st.max_x, 20.0), true);
    // Y visited: 0, 20, 30, 20 (30-10) -> min=0, max=30
    expect_eq(approxEq(st.min_y, 0.0), true);
    expect_eq(approxEq(st.max_y, 30.0), true);
    // Z visited: 0.2, 0.4, 0.6 -> min=0.2, max=0.6
    expect_eq(approxEq(st.min_z, 0.2), true);
    expect_eq(approxEq(st.max_z, 0.6), true);

    // Extrusion: 2.5 + 2.5 (=5.0) + 3.25 + 1.25 = 9.5
    expect_eq(approxEq(st.total_extrusion_e, 9.5), true);
});

test("5. Imperial mode G20, empty program, and malformed numbers fail safely", () => {
    const imperial = "G20\nG0 X1.5 Y2.5\nG1 Xbad Y.5 Z-1.\n";
    const st = summarizeGcode(imperial);
    expect_eq(st.is_metric, false);
    expect_eq(st.move_count, 2);
    expect_eq(approxEq(st.min_x, 1.5), true);
    expect_eq(approxEq(st.max_x, 1.5), true);
    expect_eq(approxEq(st.min_y, 0.5), true);
    expect_eq(approxEq(st.max_y, 2.5), true);
    expect_eq(approxEq(st.min_z, -1.0), true);

    // Empty / comment-only program
    const empty_st = summarizeGcode("; nothing here\n   \n");
    expect_eq(empty_st.command_count, 0);
    expect_eq(empty_st.move_count, 0);
    expect_eq(approxEq(empty_st.total_extrusion_e, 0.0), true);

    // Unterminated parenthesis comment and lone symbols do not infinite-loop
    const broken = "G1 (unclosed comment X99";
    const gl = parseGcodeLine(broken);
    expect_eq(gl.has_command, true);
    expect_eq(gl.command_code, 1);
    expect_eq(gl.has_x, false);
});
