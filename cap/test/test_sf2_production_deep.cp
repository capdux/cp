// Source: cp/cap/test/test_sf2_production_deep.cp (Production deep tests for cap:sf2)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SF2_PRESET_HEADER_SIZE,
    SF2_INST_HEADER_SIZE,
    SF2_SAMPLE_HEADER_SIZE,
    SF2_BAG_SIZE,
    SF2_GEN_SIZE,
    Sf2Version,
    Sf2Info,
    Sf2Sections,
    Sf2Preset,
    Sf2Instrument,
    Sf2Sample,
    Sf2PresetIter,
    Sf2InstIter,
    Sf2SampleIter,
    isSf2,
    parseSf2,
} from "cap:sf2";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn wU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn wU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn wCopy(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

fn appendChunk(buf: []u8, pos: *usize, tag: []const u8, payload: []const u8) {
    const p = pos.*;
    wCopy(buf, p, tag);
    wU32Le(buf, p + 4, payload.len as u32);
    wCopy(buf, p + 8, payload);
    let next = p + 8 + payload.len;
    if ((payload.len & 1) != 0) {
        buf[next] = 0;
        next += 1;
    }
    pos.* = next;
}

fn buildSampleSf2(out: []u8): usize {
    // 1. Build LIST/INFO body
    let info_buf: [256]u8 = std.mem.zeroes([256]u8);
    let ipos: usize = 0;
    const ifil: [4]u8 = [0x02, 0x00, 0x04, 0x00]; // 2.04
    appendChunk(info_buf[0..], *ipos, "ifil", ifil[0..]);
    appendChunk(info_buf[0..], *ipos, "isng", "EMU8000\x00");
    appendChunk(info_buf[0..], *ipos, "INAM", "Grand Piano Bank\x00\x00");
    appendChunk(info_buf[0..], *ipos, "irom", "ROM1\x00\x00");
    appendChunk(info_buf[0..], *ipos, "ICRD", "2026-11-21\x00\x00");
    appendChunk(info_buf[0..], *ipos, "IENG", "CP Audio\x00\x00");
    appendChunk(info_buf[0..], *ipos, "IPRD", "CAP Synth\x00");
    appendChunk(info_buf[0..], *ipos, "ICOP", "MIT\x00");
    appendChunk(info_buf[0..], *ipos, "ICMT", "Zero-alloc SF2\x00\x00");
    appendChunk(info_buf[0..], *ipos, "ISFT", "Polyphone\x00");

    // 2. Build LIST/sdta body (8 x 16-bit PCM samples = 16 bytes + 8 bytes sm24)
    let sdta_buf: [64]u8 = std.mem.zeroes([64]u8);
    let spos: usize = 0;
    const pcm16: [16]u8 = [
        0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x30, 0x00,
        0x40, 0x00, 0x50, 0x00, 0x60, 0x00, 0x70, 0x00,
    ];
    const sm24: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    appendChunk(sdta_buf[0..], *spos, "smpl", pcm16[0..]);
    appendChunk(sdta_buf[0..], *spos, "sm24", sm24[0..]);

    // 3. Build LIST/pdta body
    let pdta_buf: [512]u8 = std.mem.zeroes([512]u8);
    let ppos: usize = 0;

    // phdr: 2 presets + EOP (3 * 38 = 114 bytes)
    let phdr: [114]u8 = std.mem.zeroes([114]u8);
    wCopy(phdr[0..], 0, "Acoustic Grand");
    wU16Le(phdr[0..], 20, 0);   // preset=0
    wU16Le(phdr[0..], 22, 0);   // bank=0
    wU16Le(phdr[0..], 24, 0);   // bag_idx=0
    wCopy(phdr[0..], 38, "Strings Ensemble");
    wU16Le(phdr[0..], 38 + 20, 48); // preset=48
    wU16Le(phdr[0..], 38 + 22, 1);  // bank=1
    wU16Le(phdr[0..], 38 + 24, 1);  // bag_idx=1
    wCopy(phdr[0..], 76, "EOP");
    appendChunk(pdta_buf[0..], *ppos, "phdr", phdr[0..]);

    const pbag: [8]u8 = [0, 0, 0, 0, 1, 0, 0, 0];
    appendChunk(pdta_buf[0..], *ppos, "pbag", pbag[0..]);

    // inst: 1 instrument + EOI (2 * 22 = 44 bytes)
    let inst: [44]u8 = std.mem.zeroes([44]u8);
    wCopy(inst[0..], 0, "Piano Inst");
    wU16Le(inst[0..], 20, 0);
    wCopy(inst[0..], 22, "EOI");
    appendChunk(pdta_buf[0..], *ppos, "inst", inst[0..]);

    const ibag: [8]u8 = [0, 0, 0, 0, 1, 0, 0, 0];
    appendChunk(pdta_buf[0..], *ppos, "ibag", ibag[0..]);

    // shdr: 2 samples + EOS (3 * 46 = 138 bytes)
    let shdr: [138]u8 = std.mem.zeroes([138]u8);
    wCopy(shdr[0..], 0, "PianoC4");
    wU32Le(shdr[0..], 20, 1);     // start=1
    wU32Le(shdr[0..], 24, 5);     // end=5 (4 samples = 8 bytes)
    wU32Le(shdr[0..], 28, 2);     // start_loop=2
    wU32Le(shdr[0..], 32, 4);     // end_loop=4
    wU32Le(shdr[0..], 36, 44100); // sample_rate=44100
    shdr[40] = 60;                // original_pitch=C4 (60)
    shdr[41] = 0xF4;              // pitch_correction = -12 cents
    wU16Le(shdr[0..], 42, 0);
    wU16Le(shdr[0..], 44, 1);     // monoSample=1

    wCopy(shdr[0..], 46, "OutOfBoundsSmp");
    wU32Le(shdr[0..], 46 + 20, 2);
    wU32Le(shdr[0..], 46 + 24, 99); // exceeds 8-sample smpl pool!
    wU32Le(shdr[0..], 46 + 36, 48000);
    shdr[46 + 40] = 69;

    wCopy(shdr[0..], 92, "EOS");
    appendChunk(pdta_buf[0..], *ppos, "shdr", shdr[0..]);

    // 4. Assemble RIFF sfbk
    wCopy(out, 0, "RIFF");
    wCopy(out, 8, "sfbk");
    let pos: usize = 12;

    // LIST INFO
    wCopy(out, pos, "LIST");
    wU32Le(out, pos + 4, (ipos + 4) as u32);
    wCopy(out, pos + 8, "INFO");
    wCopy(out, pos + 12, info_buf[0..ipos]);
    pos = pos + 12 + ipos;

    // LIST sdta
    wCopy(out, pos, "LIST");
    wU32Le(out, pos + 4, (spos + 4) as u32);
    wCopy(out, pos + 8, "sdta");
    wCopy(out, pos + 12, sdta_buf[0..spos]);
    pos = pos + 12 + spos;

    // LIST pdta
    wCopy(out, pos, "LIST");
    wU32Le(out, pos + 4, (ppos + 4) as u32);
    wCopy(out, pos + 8, "pdta");
    wCopy(out, pos + 12, pdta_buf[0..ppos]);
    pos = pos + 12 + ppos;

    wU32Le(out, 4, (pos - 8) as u32);
    return pos;
}

test("1. Full RIFF sfbk parsing and constants verification", () => {
    expect_eq(SF2_PRESET_HEADER_SIZE, 38);
    expect_eq(SF2_INST_HEADER_SIZE, 22);
    expect_eq(SF2_SAMPLE_HEADER_SIZE, 46);
    expect_eq(SF2_BAG_SIZE, 4);
    expect_eq(SF2_GEN_SIZE, 4);

    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleSf2(buf[0..]);
    expect_eq(isSf2(buf[0..flen]), true);

    const sec = parseSf2(buf[0..flen]) ?? unreachable;
    expect_eq(sec.smpl_pcm16.len, 16);
    expect_eq(sec.sm24_lsb.len, 8);
    expect_eq(sec.phdr.len, 114);
    expect_eq(sec.inst.len, 44);
    expect_eq(sec.shdr.len, 138);
});

test("2. Sf2Info version 2.04 and NUL-trimmed metadata fields", () => {
    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleSf2(buf[0..]);
    const sec = parseSf2(buf[0..flen]) ?? unreachable;

    expect_eq(sec.info.version.major, 2);
    expect_eq(sec.info.version.minor, 4);
    expect_eq(bytesEq(sec.info.sound_engine, "EMU8000"), true);
    expect_eq(bytesEq(sec.info.bank_name, "Grand Piano Bank"), true);
    expect_eq(bytesEq(sec.info.rom_name, "ROM1"), true);
    expect_eq(bytesEq(sec.info.creation_date, "2026-11-21"), true);
    expect_eq(bytesEq(sec.info.author, "CP Audio"), true);
    expect_eq(bytesEq(sec.info.product, "CAP Synth"), true);
    expect_eq(bytesEq(sec.info.copyright, "MIT"), true);
    expect_eq(bytesEq(sec.info.comment, "Zero-alloc SF2"), true);
    expect_eq(bytesEq(sec.info.software, "Polyphone"), true);
});

test("3. Hydra Preset/Instrument/Sample iteration with EOP/EOI/EOS sentinel stripping", () => {
    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleSf2(buf[0..]);
    const sec = parseSf2(buf[0..flen]) ?? unreachable;

    // Presets (3 records in phdr -> 2 valid presets, EOP stripped)
    let pit = Sf2PresetIter.init(sec.phdr);
    expect_eq(pit.len(), 2);
    const p0 = pit.next() ?? unreachable;
    expect_eq(bytesEq(p0.name, "Acoustic Grand"), true);
    expect_eq(p0.preset, 0);
    expect_eq(p0.bank, 0);
    expect_eq(p0.preset_bag_idx, 0);

    const p1 = pit.next() ?? unreachable;
    expect_eq(bytesEq(p1.name, "Strings Ensemble"), true);
    expect_eq(p1.preset, 48);
    expect_eq(p1.bank, 1);
    expect_eq(p1.preset_bag_idx, 1);
    expect_eq(pit.next() == null, true);

    // Instruments (2 records in inst -> 1 valid instrument, EOI stripped)
    let iit = Sf2InstIter.init(sec.inst);
    expect_eq(iit.len(), 1);
    const inst0 = iit.next() ?? unreachable;
    expect_eq(bytesEq(inst0.name, "Piano Inst"), true);
    expect_eq(inst0.inst_bag_idx, 0);
    expect_eq(iit.next() == null, true);

    // Samples (3 records in shdr -> 2 valid samples, EOS stripped)
    let sit = Sf2SampleIter.init(sec.shdr);
    expect_eq(sit.len(), 2);
    const s0 = sit.next() ?? unreachable;
    expect_eq(bytesEq(s0.name, "PianoC4"), true);
    expect_eq(s0.start, 1);
    expect_eq(s0.end, 5);
    expect_eq(s0.start_loop, 2);
    expect_eq(s0.end_loop, 4);
    expect_eq(s0.sample_rate, 44100);
    expect_eq(s0.original_pitch, 60);
    expect_eq(s0.pitch_correction, -12);
    expect_eq(s0.sample_type, 1);

    * = sit.next() ?? unreachable;
    expect_eq(sit.next() == null, true);
});

test("4. Sf2Sample.samplePcmSlice zero-copy extraction and bounds protection", () => {
    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleSf2(buf[0..]);
    const sec = parseSf2(buf[0..flen]) ?? unreachable;

    let sit = Sf2SampleIter.init(sec.shdr);
    const s0 = sit.next() ?? unreachable;
    const pcm = s0.samplePcmSlice(sec) ?? unreachable;
    // start=1..end=5 -> bytes 2..10 = [0x10, 0x00, 0x20, 0x00, 0x30, 0x00, 0x40, 0x00]
    expect_eq(pcm.len, 8);
    expect_eq(pcm[0], 0x10);
    expect_eq(pcm[2], 0x20);
    expect_eq(pcm[6], 0x40);

    // Out-of-bounds sample (end=99 > 8) returns null
    const s1 = sit.next() ?? unreachable;
    expect_eq(s1.samplePcmSlice(sec) == null, true);
});

test("5. Fault tolerance: non-sfbk RIFF, truncated size, and misaligned Hydra chunk", () => {
    // WAV RIFF is NOT sfbk
    const wav: [12]u8 = ['R', 'I', 'F', 'F', 4, 0, 0, 0, 'W', 'A', 'V', 'E'];
    expect_eq(isSf2(wav[0..]), false);
    expect_eq(parseSf2(wav[0..]) == null, true);

    // Declared RIFF size exceeds actual buffer length
    const trunc: [12]u8 = ['R', 'I', 'F', 'F', 0xFF, 0x00, 0x00, 0x00, 's', 'f', 'b', 'k'];
    expect_eq(isSf2(trunc[0..]), false);

    // Misaligned phdr chunk (size 10 is not a multiple of 38) -> parseSf2 returns null
    let bad: [64]u8 = std.mem.zeroes([64]u8);
    wCopy(bad[0..], 0, "RIFF");
    wU32Le(bad[0..], 4, 4 + 8 + 4 + 8 + 10); // sfbk + LIST(pdta + phdr(10))
    wCopy(bad[0..], 8, "sfbk");
    wCopy(bad[0..], 12, "LIST");
    wU32Le(bad[0..], 16, 4 + 8 + 10);
    wCopy(bad[0..], 20, "pdta");
    wCopy(bad[0..], 24, "phdr");
    wU32Le(bad[0..], 28, 10);
    expect_eq(isSf2(bad[0..42]), true);
    expect_eq(parseSf2(bad[0..42]) == null, true);
});
