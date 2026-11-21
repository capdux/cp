// Source: cp/cap/test/test_abc_production_deep.cp (Production deep tests for cap:abc)

import { test, expect_eq } from "cap:test";
import {
    AbcFraction,
    AbcHeader,
    AbcEventKind,
    AbcEvent,
    AbcStats,
    AbcEventIter,
    parseAbcHeader,
    isAbc,
    summarizeAbc,
} from "cap:abc";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. ABC v2.1 header parsing (X/T/C/M/L/Q/K, comments, and C / C| meters)", () => {
    const tune =
        "% Sample ABC v2.1 tune\n" +
        "X:1 % reference number\n" +
        "T:Ode to Joy\n" +
        "C:L. van Beethoven\n" +
        "M:4/4\n" +
        "L:1/8\n" +
        "Q:1/4=120\n" +
        "K:C\n" +
        "E2 E2 F2 G2 |\n";

    expect_eq(isAbc(tune), true);
    const hdr = parseAbcHeader(tune) ?? unreachable;
    expect_eq(hdr.reference_num, 1);
    expect_eq(bytesEq(hdr.title, "Ode to Joy"), true);
    expect_eq(bytesEq(hdr.composer, "L. van Beethoven"), true);
    expect_eq(hdr.meter.num, 4);
    expect_eq(hdr.meter.den, 4);
    expect_eq(hdr.unit_note_len.num, 1);
    expect_eq(hdr.unit_note_len.den, 8);
    expect_eq(hdr.tempo_bpm, 120);
    expect_eq(bytesEq(hdr.key, "C"), true);
    expect_eq(hdr.body_offset < tune.len, true);

    // Common time "C" (4/4) and Cut time "C|" (2/2) + plain integer tempo Q:140
    const cut_tune = "X:2\nT:Reel\nM:C|\nQ:140\nK:G\nG4|\n";
    const h2 = parseAbcHeader(cut_tune) ?? unreachable;
    expect_eq(h2.meter.num, 2);
    expect_eq(h2.meter.den, 2);
    expect_eq(h2.tempo_bpm, 140);

    const common_tune = "X:3\nM:C\nK:D\nD4|\n";
    const h3 = parseAbcHeader(common_tune) ?? unreachable;
    expect_eq(h3.meter.num, 4);
    expect_eq(h3.meter.den, 4);
});

test("2. Exact MIDI pitch conversion for accidentals and octave modifiers (^F, _B, C,, c')", () => {
    const tune =
        "X:1\nK:C\n" +
        "C c ^F _B C, c' ^^C __B =F\n";

    let it = AbcEventIter.init(tune) ?? unreachable;

    const e0 = it.next() ?? unreachable; // C = 60
    expect_eq(e0.kind, AbcEventKind.note);
    expect_eq(e0.midi_note ?? 0, 60);

    const e1 = it.next() ?? unreachable; // c = 72
    expect_eq(e1.midi_note ?? 0, 72);

    const e2 = it.next() ?? unreachable; // ^F = 66
    expect_eq(e2.accidental, 1);
    expect_eq(e2.midi_note ?? 0, 66);

    const e3 = it.next() ?? unreachable; // _B = 70
    expect_eq(e3.accidental, -1);
    expect_eq(e3.midi_note ?? 0, 70);

    const e4 = it.next() ?? unreachable; // C, = 48
    expect_eq(e4.octave_shift, -1);
    expect_eq(e4.midi_note ?? 0, 48);

    const e5 = it.next() ?? unreachable; // c' = 84
    expect_eq(e5.octave_shift, 1);
    expect_eq(e5.midi_note ?? 0, 84);

    const e6 = it.next() ?? unreachable; // ^^C = 62
    expect_eq(e6.accidental, 2);
    expect_eq(e6.midi_note ?? 0, 62);

    const e7 = it.next() ?? unreachable; // __B = 69
    expect_eq(e7.accidental, -2);
    expect_eq(e7.midi_note ?? 0, 69);

    const e8 = it.next() ?? unreachable; // =F = 65
    expect_eq(e8.accidental, 0);
    expect_eq(e8.midi_note ?? 0, 65);

    expect_eq(it.next() == null, true);
});

test("3. Duration multipliers (A2, B/, B//, C3/2), rests, and barlines (|:, :|, |])", () => {
    const tune =
        "X:1\nK:C\n" +
        "|: A2 B/ B// C3/2 | z2 x/ :| D |]\n";

    let it = AbcEventIter.init(tune) ?? unreachable;

    const b0 = it.next() ?? unreachable;
    expect_eq(b0.kind, AbcEventKind.barline);
    expect_eq(bytesEq(b0.bar_text, "|:"), true);

    const n0 = it.next() ?? unreachable; // A2
    expect_eq(n0.dur_num, 2);
    expect_eq(n0.dur_den, 1);

    const n1 = it.next() ?? unreachable; // B/
    expect_eq(n1.dur_num, 1);
    expect_eq(n1.dur_den, 2);

    const n2 = it.next() ?? unreachable; // B//
    expect_eq(n2.dur_num, 1);
    expect_eq(n2.dur_den, 4);

    const n3 = it.next() ?? unreachable; // C3/2
    expect_eq(n3.dur_num, 3);
    expect_eq(n3.dur_den, 2);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.kind, AbcEventKind.barline);
    expect_eq(bytesEq(b1.bar_text, "|"), true);

    const r0 = it.next() ?? unreachable; // z2
    expect_eq(r0.kind, AbcEventKind.rest);
    expect_eq(r0.dur_num, 2);
    expect_eq(r0.dur_den, 1);

    const r1 = it.next() ?? unreachable; // x/
    expect_eq(r1.kind, AbcEventKind.rest);
    expect_eq(r1.dur_num, 1);
    expect_eq(r1.dur_den, 2);

    const b2 = it.next() ?? unreachable;
    expect_eq(b2.kind, AbcEventKind.barline);
    expect_eq(bytesEq(b2.bar_text, ":|"), true);

    const n4 = it.next() ?? unreachable; // D
    expect_eq(n4.kind, AbcEventKind.note);

    const b3 = it.next() ?? unreachable;
    expect_eq(b3.kind, AbcEventKind.barline);
    expect_eq(bytesEq(b3.bar_text, "|]"), true);

    expect_eq(it.next() == null, true);
});

test("4. Chords [CEG]2, guitar chord \"Am\", decorations !trill!, inline [M:3/4], and summarizeAbc", () => {
    const tune =
        "X:1\nT:Chord Demo\nK:C\n" +
        "\"Am\" !trill! [CEG]2 [M:3/4] (A-A) z | % end comment\n";

    let it = AbcEventIter.init(tune) ?? unreachable;

    const cs = it.next() ?? unreachable;
    expect_eq(cs.kind, AbcEventKind.chord_start);

    const c = it.next() ?? unreachable;
    expect_eq(c.midi_note ?? 0, 60);
    const e = it.next() ?? unreachable;
    expect_eq(e.midi_note ?? 0, 64);
    const g = it.next() ?? unreachable;
    expect_eq(g.midi_note ?? 0, 67);

    const ce = it.next() ?? unreachable;
    expect_eq(ce.kind, AbcEventKind.chord_end);
    expect_eq(ce.dur_num, 2);
    expect_eq(ce.dur_den, 1);

    const ss = it.next() ?? unreachable;
    expect_eq(ss.kind, AbcEventKind.slur_start);
    * = it.next() ?? unreachable; // A
    const tie = it.next() ?? unreachable;
    expect_eq(tie.kind, AbcEventKind.tie);
    * = it.next() ?? unreachable; // A
    const se = it.next() ?? unreachable;
    expect_eq(se.kind, AbcEventKind.slur_end);

    const rest = it.next() ?? unreachable;
    expect_eq(rest.kind, AbcEventKind.rest);
    const bar = it.next() ?? unreachable;
    expect_eq(bar.kind, AbcEventKind.barline);
    expect_eq(it.next() == null, true);

    const stats = summarizeAbc(tune);
    expect_eq(stats.note_count, 5);
    expect_eq(stats.rest_count, 1);
    expect_eq(stats.bar_count, 1);
    expect_eq(stats.chord_count, 1);
    expect_eq(stats.min_midi, 60);
    expect_eq(stats.max_midi, 69);
});

test("5. Fault tolerance: missing X:, missing K:, empty input, and non-ABC text", () => {
    expect_eq(isAbc(""), false);
    expect_eq(parseAbcHeader("") == null, true);

    // Missing X:
    expect_eq(isAbc("T:No Ref\nK:C\nCDEF|\n"), false);

    // Missing K:
    expect_eq(isAbc("X:1\nT:No Key\nM:4/4\n"), false);

    // Non-ABC text
    expect_eq(isAbc("Hello world, not a music score"), false);
    expect_eq(AbcEventIter.init("Not ABC") == null, true);

    const bad = summarizeAbc("Not ABC");
    expect_eq(bad.note_count, 0);
    expect_eq(bad.bar_count, 0);
    expect_eq(bad.min_midi, 0);
    expect_eq(bad.max_midi, 0);
});
