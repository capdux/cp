// Source: cp/cap/test/test_bvh_production_deep.cp (Production deep tests for cap:bvh)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BvhChannel,
    BvhJoint,
    BvhHeader,
    parseBvhChannel,
    isBvh,
    BvhJointIter,
    parseBvhHeader,
    BvhFloatIter,
    BvhFrameIter,
} from "cap:bvh";

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < 0.0001;
}

const SAMPLE_BVH: []const u8 =
    "HIERARCHY\n" ++
    "ROOT Hips\n" ++
    "{\n" ++
    "  OFFSET 0.00 0.00 0.00\n" ++
    "  CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n" ++
    "  JOINT Spine\n" ++
    "  {\n" ++
    "    OFFSET 0.00 10.50 0.00\n" ++
    "    CHANNELS 3 Zrotation Xrotation Yrotation\n" ++
    "    JOINT Head\n" ++
    "    {\n" ++
    "      OFFSET 0.00 12.25 0.50\n" ++
    "      CHANNELS 3 Zrotation Xrotation Yrotation\n" ++
    "      End Site\n" ++
    "      {\n" ++
    "        OFFSET 0.00 6.00 0.00\n" ++
    "      }\n" ++
    "    }\n" ++
    "  }\n" ++
    "  JOINT LeftLeg\n" ++
    "  {\n" ++
    "    OFFSET -5.00 0.00 0.00\n" ++
    "    CHANNELS 3 Zrotation Xrotation Yrotation\n" ++
    "    End Site\n" ++
    "    {\n" ++
    "      OFFSET 0.00 -30.00 0.00\n" ++
    "    }\n" ++
    "  }\n" ++
    "}\n" ++
    "MOTION\n" ++
    "Frames: 3\n" ++
    "Frame Time: 0.0333333\n" ++
    "0.0 90.0 0.0 1.5 -2.5 3.0 0.1 0.2 0.3 10.0 20.0 30.0 -5.0 0.0 1.5e-2\n" ++
    "0.5 90.1 0.2 1.6 -2.4 3.1 0.2 0.3 0.4 11.0 21.0 31.0 -4.5 0.5 2.0e-2\n" ++
    "1.0 90.2 0.4 1.7 -2.3 3.2 0.3 0.4 0.5 12.0 22.0 32.0 -4.0 1.0 2.5e-2\n";

test("1. isBvh and parseBvhChannel recognize standard BVH header and all 6 channel types", () => {
    expect_eq(isBvh(SAMPLE_BVH), true);
    expect_eq(isBvh("   \n\tHIERARCHY\n"), true);
    expect_eq(isBvh("MOTION\nFrames: 1\n"), false);
    expect_eq(isBvh(""), false);

    expect_eq(parseBvhChannel("Xposition"), BvhChannel.x_pos);
    expect_eq(parseBvhChannel("Yposition"), BvhChannel.y_pos);
    expect_eq(parseBvhChannel("Zposition"), BvhChannel.z_pos);
    expect_eq(parseBvhChannel("Xrotation"), BvhChannel.x_rot);
    expect_eq(parseBvhChannel("Yrotation"), BvhChannel.y_rot);
    expect_eq(parseBvhChannel("Zrotation"), BvhChannel.z_rot);
    expect_eq(parseBvhChannel("ScaleX"), BvhChannel.unknown);
});

test("2. BvhJointIter traverses hierarchy depth-first with accurate depth and channel_offset", () => {
    let it = BvhJointIter.init(SAMPLE_BVH);

    // 0: ROOT Hips (depth 0, 6 channels, offset 0)
    const j0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, j0.name, "Hips"), true);
    expect_eq(j0.depth, 0);
    expect_eq(j0.is_root, true);
    expect_eq(j0.is_end_site, false);
    expect_eq(j0.channel_count, 6);
    expect_eq(j0.channel_offset, 0);
    expect_eq(j0.channels[0], BvhChannel.x_pos);
    expect_eq(j0.channels[3], BvhChannel.z_rot);

    // 1: JOINT Spine (depth 1, 3 channels, offset 6)
    const j1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, j1.name, "Spine"), true);
    expect_eq(j1.depth, 1);
    expect_eq(approxEq(j1.offset_y, 10.5), true);
    expect_eq(j1.channel_count, 3);
    expect_eq(j1.channel_offset, 6);

    // 2: JOINT Head (depth 2, 3 channels, offset 9)
    const j2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, j2.name, "Head"), true);
    expect_eq(j2.depth, 2);
    expect_eq(approxEq(j2.offset_y, 12.25), true);
    expect_eq(approxEq(j2.offset_z, 0.5), true);
    expect_eq(j2.channel_offset, 9);

    // 3: End Site under Head (depth 3, 0 channels, offset 12)
    const j3 = it.next() ?? unreachable;
    expect_eq(j3.is_end_site, true);
    expect_eq(j3.depth, 3);
    expect_eq(j3.channel_count, 0);
    expect_eq(j3.channel_offset, 12);
    expect_eq(approxEq(j3.offset_y, 6.0), true);

    // 4: JOINT LeftLeg (depth 1, 3 channels, offset 12)
    const j4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, j4.name, "LeftLeg"), true);
    expect_eq(j4.depth, 1);
    expect_eq(approxEq(j4.offset_x, -5.0), true);
    expect_eq(j4.channel_count, 3);
    expect_eq(j4.channel_offset, 12);

    // 5: End Site under LeftLeg (depth 2, 0 channels, offset 15)
    const j5 = it.next() ?? unreachable;
    expect_eq(j5.is_end_site, true);
    expect_eq(j5.depth, 2);
    expect_eq(approxEq(j5.offset_y, -30.0), true);
    expect_eq(j5.channel_offset, 15);

    expect_eq(it.next() == null, true);
    expect_eq(it.malformed, false);
});

test("3. parseBvhHeader computes counts, fps, and durationSec accurately", () => {
    const hdr = parseBvhHeader(SAMPLE_BVH) ?? unreachable;
    expect_eq(hdr.joint_count, 4);
    expect_eq(hdr.end_site_count, 2);
    expect_eq(hdr.total_channels, 15);
    expect_eq(hdr.frame_count, 3);
    expect_eq(approxEq(hdr.frame_time_sec, 0.0333333), true);
    expect_eq(approxEq(hdr.fps(), 30.00003), true);
    expect_eq(approxEq(hdr.durationSec(), 0.0999999), true);
});

test("4. BvhFrameIter & BvhFloatIter decode every frame and channel including scientific notation", () => {
    const hdr = parseBvhHeader(SAMPLE_BVH) ?? unreachable;
    let fit = BvhFrameIter.init(SAMPLE_BVH, hdr);

    const f0 = fit.next() ?? unreachable;
    let vals0 = BvhFloatIter.init(f0);
    let idx: u32 = 0;
    let last_val: f64 = 0.0;
    let second_val: f64 = 0.0;
    while (v := vals0.next()) {
        if (idx == 1) second_val = v;
        last_val = v;
        idx += 1;
    }
    expect_eq(idx, 15);
    expect_eq(vals0.malformed, false);
    expect_eq(approxEq(second_val, 90.0), true);
    expect_eq(approxEq(last_val, 0.015), true);

    const f1 = fit.next() ?? unreachable;
    let vals1 = BvhFloatIter.init(f1);
    const f1_first = vals1.next() ?? unreachable;
    expect_eq(approxEq(f1_first, 0.5), true);

    const f2 = fit.next() ?? unreachable;
    let vals2 = BvhFloatIter.init(f2);
    const f2_first = vals2.next() ?? unreachable;
    expect_eq(approxEq(f2_first, 1.0), true);

    expect_eq(fit.next() == null, true);
    expect_eq(fit.malformed, false);
});

test("5. Malformed BVH (missing MOTION, mismatched braces, invalid CHANNELS, wrong frame channel count) fail safely", () => {
    // Missing MOTION
    const no_motion = "HIERARCHY\nROOT Hips { OFFSET 0 0 0 CHANNELS 3 Xposition Yposition Zposition }\n";
    expect_eq(parseBvhHeader(no_motion) == null, true);

    // Unclosed brace before MOTION
    const unclosed = "HIERARCHY\nROOT Hips { OFFSET 0 0 0 CHANNELS 3 Xposition Yposition Zposition\nMOTION\nFrames: 1\nFrame Time: 0.033\n0 0 0\n";
    expect_eq(parseBvhHeader(unclosed) == null, true);

    // Invalid CHANNELS count (> 6)
    const bad_ch_cnt = "HIERARCHY\nROOT Hips { OFFSET 0 0 0 CHANNELS 7 Xposition Yposition Zposition Xrotation Yrotation Zrotation Xposition }\nMOTION\nFrames: 0\nFrame Time: 0.033\n";
    expect_eq(parseBvhHeader(bad_ch_cnt) == null, true);

    // Unknown channel name
    const bad_ch_name = "HIERARCHY\nROOT Hips { OFFSET 0 0 0 CHANNELS 1 BadChannel }\nMOTION\nFrames: 0\nFrame Time: 0.033\n";
    expect_eq(parseBvhHeader(bad_ch_name) == null, true);

    // Frame with wrong channel count detected by BvhFrameIter
    const bad_frame =
        "HIERARCHY\nROOT Hips { OFFSET 0 0 0 CHANNELS 3 Xposition Yposition Zposition }\n" ++
        "MOTION\nFrames: 1\nFrame Time: 0.04\n1.0 2.0\n";
    const hdr = parseBvhHeader(bad_frame) ?? unreachable;
    let fit = BvhFrameIter.init(bad_frame, hdr);
    expect_eq(fit.next() == null, true);
    expect_eq(fit.malformed, true);
});
