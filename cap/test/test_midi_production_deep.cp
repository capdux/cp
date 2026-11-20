// Source: cp/cap/test/test_midi_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:midi & cap.midi

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MidiError,
    MidiFormat,
    NoteEventData,
    PolyPressureData,
    ControlChangeData,
    ProgramChangeData,
    ChannelPressureData,
    PitchBendData,
    MetaOtherData,
    MidiEventKind,
    MidiEvent,
    MidiTrack,
    MidiFile,
    parse,
    encode,
} from "cap:midi";

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testFormat0And1Roundtrip(): !void {
    const alloc = std.testing.allocator;

    // 1. Format 0 SingleTrack roundtrip (without explicit EndOfTrack -> auto-appended)
    let f0_events: [6]MidiEvent = [
        MidiEvent {
            delta_ticks: 0,
            kind: MidiEventKind { meta_tempo: 500000 },
        },
        MidiEvent {
            delta_ticks: 0,
            kind: MidiEventKind { program_change: ProgramChangeData { channel: 0, program: 42 } },
        },
        MidiEvent {
            delta_ticks: 0,
            kind: MidiEventKind { note_on: NoteEventData { channel: 0, note: 60, velocity: 110 } },
        },
        MidiEvent {
            delta_ticks: 120,
            kind: MidiEventKind { control_change: ControlChangeData { channel: 0, controller: 64, value: 127 } },
        },
        MidiEvent {
            delta_ticks: 120,
            kind: MidiEventKind { pitch_bend: PitchBendData { channel: 0, value: 12288 } },
        },
        MidiEvent {
            delta_ticks: 240,
            kind: MidiEventKind { note_off: NoteEventData { channel: 0, note: 60, velocity: 0 } },
        },
    ];
    let f0_tracks: [1]MidiTrack = [MidiTrack { events: &f0_events }];
    const f0_src = MidiFile {
        format: MidiFormat.SingleTrack,
        ticks_per_quarter: 480,
        tracks: &f0_tracks,
    };

    const f0_bytes = encode(alloc, f0_src)?;
    defer alloc.free(f0_bytes);

    let f0_dec = parse(alloc, f0_bytes)?;
    defer f0_dec.deinit(alloc);

    expect_eq(f0_dec.format == MidiFormat.SingleTrack, true);
    expect_eq(f0_dec.ticks_per_quarter, 480);
    expect_eq(f0_dec.tracks.len, 1);
    // 6 events + auto-appended meta_end_of_track
    expect_eq(f0_dec.tracks[0].events.len, 7);
    expect_eq(f0_dec.totalTicks(), 480);

    switch (f0_dec.tracks[0].events[0].kind) {
        .meta_tempo => |us| expect_eq(us, 500000),
        else => return error.TestFailed,
    }
    switch (f0_dec.tracks[0].events[1].kind) {
        .program_change => |pc| {
            expect_eq(pc.channel, 0);
            expect_eq(pc.program, 42);
        },
        else => return error.TestFailed,
    }
    switch (f0_dec.tracks[0].events[2].kind) {
        .note_on => |no| {
            expect_eq(no.channel, 0);
            expect_eq(no.note, 60);
            expect_eq(no.velocity, 110);
        },
        else => return error.TestFailed,
    }
    switch (f0_dec.tracks[0].events[3].kind) {
        .control_change => |cc| {
            expect_eq(cc.controller, 64);
            expect_eq(cc.value, 127);
        },
        else => return error.TestFailed,
    }
    switch (f0_dec.tracks[0].events[4].kind) {
        .pitch_bend => |pb| {
            expect_eq(pb.channel, 0);
            expect_eq(pb.value, 12288);
        },
        else => return error.TestFailed,
    }
    switch (f0_dec.tracks[0].events[5].kind) {
        .note_off => |nf| {
            expect_eq(nf.note, 60);
            expect_eq(nf.velocity, 0);
        },
        else => return error.TestFailed,
    }
    switch (f0_dec.tracks[0].events[6].kind) {
        .meta_end_of_track => {},
        else => return error.TestFailed,
    }

    // 2. Format 1 MultiTrackSync roundtrip with meta_other, poly_pressure, channel_pressure, sysex
    let t0_events: [3]MidiEvent = [
        MidiEvent {
            delta_ticks: 0,
            kind: MidiEventKind { meta_tempo: 400000 },
        },
        MidiEvent {
            delta_ticks: 0,
            kind: MidiEventKind { meta_other: MetaOtherData { meta_type: 0x03, data: "Lead" } },
        },
        MidiEvent {
            delta_ticks: 0,
            kind: MidiEventKind.meta_end_of_track,
        },
    ];
    let sysex_payload: [4]u8 = [0x7E, 0x7F, 0x09, 0xF7];
    let t1_events: [5]MidiEvent = [
        MidiEvent {
            delta_ticks: 0,
            kind: MidiEventKind { note_on: NoteEventData { channel: 2, note: 64, velocity: 95 } },
        },
        MidiEvent {
            delta_ticks: 60,
            kind: MidiEventKind { poly_pressure: PolyPressureData { channel: 2, note: 64, pressure: 50 } },
        },
        MidiEvent {
            delta_ticks: 60,
            kind: MidiEventKind { channel_pressure: ChannelPressureData { channel: 2, pressure: 70 } },
        },
        MidiEvent {
            delta_ticks: 120,
            kind: MidiEventKind { sysex: &sysex_payload },
        },
        MidiEvent {
            delta_ticks: 720,
            kind: MidiEventKind { note_off: NoteEventData { channel: 2, note: 64, velocity: 0 } },
        },
    ];
    let f1_tracks: [2]MidiTrack = [
        MidiTrack { events: &t0_events },
        MidiTrack { events: &t1_events },
    ];
    const f1_src = MidiFile {
        format: MidiFormat.MultiTrackSync,
        ticks_per_quarter: 960,
        tracks: &f1_tracks,
    };

    const f1_bytes = encode(alloc, f1_src)?;
    defer alloc.free(f1_bytes);

    let f1_dec = parse(alloc, f1_bytes)?;
    defer f1_dec.deinit(alloc);

    expect_eq(f1_dec.format == MidiFormat.MultiTrackSync, true);
    expect_eq(f1_dec.tracks.len, 2);
    expect_eq(f1_dec.totalTicks(), 960);

    switch (f1_dec.tracks[0].events[1].kind) {
        .meta_other => |m| {
            expect_eq(m.meta_type, 0x03);
            expect_eq(std.mem.eql(u8, m.data, "Lead"), true);
        },
        else => return error.TestFailed,
    }
    switch (f1_dec.tracks[1].events[3].kind) {
        .sysex => |s| {
            expect_eq(s.len, 4);
            expect_eq(s[0], 0x7E);
            expect_eq(s[3], 0xF7);
        },
        else => return error.TestFailed,
    }
}

fn testRunningStatusDecoding(): !void {
    const alloc = std.testing.allocator;

    // Hand-crafted SMF Format 0 with Running Status:
    // Header: MThd len=6 fmt=0 tracks=1 div=480 (0x01E0)
    // Track payload (25 bytes):
    //   00 90 3C 64        (delta=0, NoteOn ch0, note 60, vel 100) -> sets running_status = 0x90
    //   78 40 5A           (delta=120, omitted status -> 0x90, note 64, vel 90)
    //   78 43 50           (delta=120, omitted status -> 0x90, note 67, vel 80)
    //   81 70 80 3C 00     (delta=240, NoteOff ch0, note 60, vel 0) -> sets running_status = 0x80
    //   00 40 00           (delta=0, omitted status -> 0x80, note 64, vel 0)
    //   00 FF 51 03 07 A1 20 (delta=0, Meta Tempo 500000) -> clears running_status
    //   00 FF 2F 00        (delta=0, End of Track)
    let smf: [51]u8 = [
        'M', 'T', 'h', 'd', 0x00, 0x00, 0x00, 0x06,
        0x00, 0x00, 0x00, 0x01, 0x01, 0xE0,
        'M', 'T', 'r', 'k', 0x00, 0x00, 0x00, 29,
        0x00, 0x90, 0x3C, 0x64,
        0x78, 0x40, 0x5A,
        0x78, 0x43, 0x50,
        0x81, 0x70, 0x80, 0x3C, 0x00,
        0x00, 0x40, 0x00,
        0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20,
        0x00, 0xFF, 0x2F, 0x00,
    ];

    let mf = parse(alloc, &smf)?;
    defer mf.deinit(alloc);

    expect_eq(mf.tracks[0].events.len, 7);
    expect_eq(mf.tracks[0].events[1].delta_ticks, 120);
    switch (mf.tracks[0].events[1].kind) {
        .note_on => |n| {
            expect_eq(n.note, 64);
            expect_eq(n.velocity, 90);
        },
        else => return error.TestFailed,
    }
    switch (mf.tracks[0].events[2].kind) {
        .note_on => |n| {
            expect_eq(n.note, 67);
            expect_eq(n.velocity, 80);
        },
        else => return error.TestFailed,
    }
    expect_eq(mf.tracks[0].events[3].delta_ticks, 240);
    switch (mf.tracks[0].events[4].kind) {
        .note_off => |n| {
            expect_eq(n.note, 64);
            expect_eq(n.velocity, 0);
        },
        else => return error.TestFailed,
    }
    switch (mf.tracks[0].events[5].kind) {
        .meta_tempo => |us| expect_eq(us, 500000),
        else => return error.TestFailed,
    }

    // Verify Meta event clears Running Status: data byte after Meta without status -> InvalidStatusByte
    let cleared_rs: [36]u8 = [
        'M', 'T', 'h', 'd', 0x00, 0x00, 0x00, 0x06,
        0x00, 0x00, 0x00, 0x01, 0x01, 0xE0,
        'M', 'T', 'r', 'k', 0x00, 0x00, 0x00, 14,
        0x00, 0x90, 0x3C, 0x64,
        0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20,
        0x00, 0x40, 0x50,
    ];
    let mut_ok = parse(alloc, &cleared_rs) catch |e| {
        expect_eq(e == MidiError.InvalidStatusByte, true);
        return;
    };
    mut_ok.deinit(alloc);
    return error.TestFailed;
}

fn testVlqExtremesAndTicksToMs(): !void {
    const alloc = std.testing.allocator;

    let evs: [5]MidiEvent = [
        MidiEvent { delta_ticks: 0, kind: MidiEventKind { note_on: NoteEventData { channel: 0, note: 60, velocity: 80 } } },
        MidiEvent { delta_ticks: 127, kind: MidiEventKind { note_off: NoteEventData { channel: 0, note: 60, velocity: 0 } } },
        MidiEvent { delta_ticks: 16383, kind: MidiEventKind { note_on: NoteEventData { channel: 0, note: 62, velocity: 80 } } },
        MidiEvent { delta_ticks: 2097151, kind: MidiEventKind { note_off: NoteEventData { channel: 0, note: 62, velocity: 0 } } },
        MidiEvent { delta_ticks: 0x0FFFFFFF, kind: MidiEventKind.meta_end_of_track },
    ];
    let trs: [1]MidiTrack = [MidiTrack { events: &evs }];
    const mf = MidiFile {
        format: MidiFormat.SingleTrack,
        ticks_per_quarter: 480,
        tracks: &trs,
    };

    const bytes = encode(alloc, mf)?;
    defer alloc.free(bytes);

    let parsed = parse(alloc, bytes)?;
    defer parsed.deinit(alloc);

    expect_eq(parsed.tracks[0].events[0].delta_ticks, 0);
    expect_eq(parsed.tracks[0].events[1].delta_ticks, 127);
    expect_eq(parsed.tracks[0].events[2].delta_ticks, 16383);
    expect_eq(parsed.tracks[0].events[3].delta_ticks, 2097151);
    expect_eq(parsed.tracks[0].events[4].delta_ticks, 0x0FFFFFFF);

    // ticksToMs at 120 BPM (500_000 us/quarter) and 480 TPQ: 480 ticks = 500.0 ms
    expect_eq(approxF64(parsed.ticksToMs(480, 500000), 500.0, 0.0001), true);
    expect_eq(approxF64(parsed.ticksToMs(960, 500000), 1000.0, 0.0001), true);
    expect_eq(approxF64(parsed.ticksToMs(240, 500000), 250.0, 0.0001), true);
}

fn expectParseError(buf: []const u8, expected: MidiError): !void {
    const alloc = std.testing.allocator;
    let mut_ok = parse(alloc, buf) catch |e| {
        expect_eq(e == expected, true);
        return;
    };
    mut_ok.deinit(alloc);
    return error.TestFailed;
}

fn testErrorInterception(): !void {
    // 1. Invalid MThd magic
    let bad_magic: [14]u8 = ['R', 'I', 'F', 'F', 0, 0, 0, 6, 0, 0, 0, 1, 1, 0xE0];
    expectParseError(&bad_magic, MidiError.InvalidHeader)?;

    // 2. Invalid MThd length (!= 6)
    let bad_len: [14]u8 = ['M', 'T', 'h', 'd', 0, 0, 0, 8, 0, 0, 0, 1, 1, 0xE0];
    expectParseError(&bad_len, MidiError.InvalidHeader)?;

    // 3. Unsupported format (format 3)
    let bad_fmt: [14]u8 = ['M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 3, 0, 1, 1, 0xE0];
    expectParseError(&bad_fmt, MidiError.UnsupportedFormat)?;

    // 4. Invalid MTrk magic
    let bad_trk: [22]u8 = [
        'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 0xE0,
        'B', 'A', 'D', 'K', 0, 0, 0, 0,
    ];
    expectParseError(&bad_trk, MidiError.InvalidTrackHeader)?;

    // 5. Malformed >4 byte VLQ (InvalidVlq)
    let bad_vlq: [27]u8 = [
        'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 0xE0,
        'M', 'T', 'r', 'k', 0, 0, 0, 5,
        0x81, 0x80, 0x80, 0x80, 0x00,
    ];
    expectParseError(&bad_vlq, MidiError.InvalidVlq)?;

    // 6. Bare data byte with no Running Status (InvalidStatusByte)
    let bare_data: [25]u8 = [
        'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 0xE0,
        'M', 'T', 'r', 'k', 0, 0, 0, 3,
        0x00, 0x3C, 0x64,
    ];
    expectParseError(&bare_data, MidiError.InvalidStatusByte)?;

    // 7. Truncated EOF (UnexpectedEof)
    let trunc_eof: [25]u8 = [
        'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 0xE0,
        'M', 'T', 'r', 'k', 0, 0, 0, 10,
        0x00, 0x90, 0x3C,
    ];
    expectParseError(&trunc_eof, MidiError.UnexpectedEof)?;
}

fn testCapHubDynamicMountAndZeroLeak(): !void {
    const alloc = std.testing.allocator;

    let evs: [2]cap.midi.MidiEvent = [
        cap.midi.MidiEvent {
            delta_ticks: 0,
            kind: cap.midi.MidiEventKind {
                note_on: cap.midi.NoteEventData { channel: 1, note: 72, velocity: 120 },
            },
        },
        cap.midi.MidiEvent {
            delta_ticks: 240,
            kind: cap.midi.MidiEventKind.meta_end_of_track,
        },
    ];
    let trs: [1]cap.midi.MidiTrack = [cap.midi.MidiTrack { events: &evs }];
    const mf = cap.midi.MidiFile {
        format: cap.midi.MidiFormat.SingleTrack,
        ticks_per_quarter: 480,
        tracks: &trs,
    };

    const encoded = cap.midi.encode(alloc, mf)?;
    defer alloc.free(encoded);

    let decoded = cap.midi.parse(alloc, encoded)?;
    defer decoded.deinit(alloc);

    expect_eq(decoded.ticks_per_quarter, 480);
    expect_eq(decoded.totalTicks(), 240);
    expect_eq(approxF64(decoded.ticksToMs(240, 500000), 250.0, 0.0001), true);
}

test("cap:midi Format 0/1 roundtrip (notes, CC, PC, pitch_bend, meta_tempo, sysex)", testFormat0And1Roundtrip);
test("cap:midi Running Status decoding & Meta status reset", testRunningStatusDecoding);
test("cap:midi VLQ 1..4 byte extremes (0..0x0FFFFFFF) & ticksToMs", testVlqExtremesAndTicksToMs);
test("cap:midi malformed MThd/MTrk, InvalidVlq, InvalidStatusByte & UnexpectedEof", testErrorInterception);
test("cap.midi hub dynamic mount & std.testing.allocator zero-leak", testCapHubDynamicMountAndZeroLeak);
