// Source: cp/cap/test/test_dls_production_deep.cp
// Deep Production Test Suite for cap:dls
// Covers:
//   1. Complete RIFF "DLS " (colh + vers + LIST/INFO + LIST/lins + ptbl + LIST/wvpl) parsing
//   2. Melodic vs percussion drum kit (0x80000000) instrument detection & INAM names
//   3. DlsWaveIter extracting fmt , wsmp (signed fine_tune & attenuation) & data PCM slices
//   4. 2-byte RIFF even-alignment padding on odd-sized chunks
//   5. Non-DLS RIFF ("WAVE"), truncated header & out-of-bounds chunk size guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DLS_RIFF_MAGIC,
    DLS_FORM_TYPE,
    DLS_BANK_DRUM_FLAG,
    DlsVersion,
    DlsInfo,
    DlsHeader,
    DlsInstrument,
    DlsWave,
    DlsInstIter,
    DlsWaveIter,
    isDls,
    parseDls,
} from "cap:dls";

struct RiffBuilder {
    buf: [2048]u8 = [0; 2048],
    len: usize = 0,

    fn writeU8(*_, v: u8) {
        _.buf[_.len] = v;
        _.len += 1;
    }

    fn writeU16Le(*_, v: u16) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
    }

    fn writeU32Le(*_, v: u32) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
        _.writeU8(((v >> 16) & 0xFF) as u8);
        _.writeU8(((v >> 24) & 0xFF) as u8);
    }

    fn writeBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeU8(s[i]);
            i += 1;
        }
    }

    fn patchU32Le(*_, off: usize, v: u32) {
        _.buf[off] = (v & 0xFF) as u8;
        _.buf[off + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[off + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[off + 3] = ((v >> 24) & 0xFF) as u8;
    }

    fn beginChunk(*_, fourcc: []const u8): usize {
        _.writeBytes(fourcc);
        const sz_off = _.len;
        _.writeU32Le(0);
        return sz_off;
    }

    fn endChunk(*_, sz_off: usize) {
        const payload_len = _.len - (sz_off + 4);
        _.patchU32Le(sz_off, payload_len as u32);
        if ((payload_len & 1) != 0) {
            _.writeU8(0); // 2-byte even pad
        }
    }

    fn beginList(*_, list_type: []const u8): usize {
        const sz_off = _.beginChunk("LIST");
        _.writeBytes(list_type);
        return sz_off;
    }

    fn writeStrChunk(*_, fourcc: []const u8, s: []const u8) {
        const sz_off = _.beginChunk(fourcc);
        _.writeBytes(s);
        _.writeU8(0);
        _.endChunk(sz_off);
    }

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

fn buildSampleDls(w: *RiffBuilder) {
    const riff_sz = w.beginChunk("RIFF");
    w.writeBytes("DLS ");

    // colh: 2 instruments
    const colh_sz = w.beginChunk("colh");
    w.writeU32Le(2);
    w.endChunk(colh_sz);

    // vers: v2.1 (ms = 0x00020001, ls = 0x00000000)
    const vers_sz = w.beginChunk("vers");
    w.writeU32Le(0x00020001);
    w.writeU32Le(0x00000000);
    w.endChunk(vers_sz);

    // LIST/INFO (includes odd-sized string chunks to exercise 2B even padding)
    const info_sz = w.beginList("INFO");
    w.writeStrChunk("INAM", "GM Bank");     // 7 + 1 = 8 (even)
    w.writeStrChunk("ICOP", "CP 2026");     // 7 + 1 = 8
    w.writeStrChunk("IENG", "SynthEng");    // 8 + 1 = 9 (odd -> +1 pad!)
    w.writeStrChunk("ICMT", "Level 2");     // 7 + 1 = 8
    w.writeStrChunk("ISFT", "CapSynth");    // 8 + 1 = 9 (odd -> +1 pad!)
    w.writeStrChunk("ICRD", "2026-11-21");  // 10 + 1 = 11 (odd -> +1 pad!)
    w.endChunk(info_sz);

    // LIST/lins: 2 instruments (Grand Piano + Standard Drum Kit)
    const lins_sz = w.beginList("lins");

    // Instrument 0: Acoustic Grand Piano (melodic, bank 0, program 0, 2 regions)
    const ins0_sz = w.beginList("ins ");
    const insh0 = w.beginChunk("insh");
    w.writeU32Le(2); // regions
    w.writeU32Le(0); // bank (melodic)
    w.writeU32Le(0); // program
    w.endChunk(insh0);
    const ins0_info = w.beginList("INFO");
    w.writeStrChunk("INAM", "Grand Piano");
    w.endChunk(ins0_info);
    w.endChunk(ins0_sz);

    // Instrument 1: Standard Kit (drum kit, bank 0x80000000, program 0, 16 regions)
    const ins1_sz = w.beginList("ins ");
    const insh1 = w.beginChunk("insh");
    w.writeU32Le(16);
    w.writeU32Le(DLS_BANK_DRUM_FLAG | 0x0000);
    w.writeU32Le(0);
    w.endChunk(insh1);
    const ins1_info = w.beginList("INFO");
    w.writeStrChunk("INAM", "Standard Kit");
    w.endChunk(ins1_info);
    w.endChunk(ins1_sz);

    w.endChunk(lins_sz);

    // ptbl: cbSize=8, cCues=2, 2 cue offsets
    const ptbl_sz = w.beginChunk("ptbl");
    w.writeU32Le(8);
    w.writeU32Le(2);
    w.writeU32Le(0);
    w.writeU32Le(64);
    w.endChunk(ptbl_sz);

    // LIST/wvpl: 2 waves (one with wsmp, one with odd-sized PCM data)
    const wvpl_sz = w.beginList("wvpl");

    // Wave 0: 44100Hz mono 16-bit PCM + wsmp
    const w0_sz = w.beginList("wave");
    const fmt0 = w.beginChunk("fmt ");
    w.writeU16Le(1);     // PCM
    w.writeU16Le(1);     // mono
    w.writeU32Le(44100); // sample_rate
    w.writeU32Le(88200); // avg bytes/sec
    w.writeU16Le(2);     // block align
    w.writeU16Le(16);    // bits per sample
    w.endChunk(fmt0);
    const wsmp0 = w.beginChunk("wsmp");
    w.writeU32Le(20); // cbSize
    w.writeU16Le(60); // unity_note = Middle C
    const neg_tune: i16 = -25;
    const neg_tune_u: u16 = @bitCast(neg_tune);
    w.writeU16Le(neg_tune_u);
    const atten: i32 = -65536;
    const atten_u: u32 = @bitCast(atten);
    w.writeU32Le(atten_u);
    w.writeU32Le(0); // options
    w.writeU32Le(0); // loops
    w.endChunk(wsmp0);
    const data0 = w.beginChunk("data");
    w.writeBytes("\x01\x02\x03\x04\x05"); // 5 bytes (odd -> +1 pad!)
    w.endChunk(data0);
    const w0_info = w.beginList("INFO");
    w.writeStrChunk("INAM", "PianoC4");
    w.endChunk(w0_info);
    w.endChunk(w0_sz);

    // Wave 1: 48000Hz stereo 16-bit PCM (no wsmp)
    const w1_sz = w.beginList("wave");
    const fmt1 = w.beginChunk("fmt ");
    w.writeU16Le(1);
    w.writeU16Le(2);
    w.writeU32Le(48000);
    w.writeU32Le(192000);
    w.writeU16Le(4);
    w.writeU16Le(16);
    w.endChunk(fmt1);
    const data1 = w.beginChunk("data");
    w.writeBytes("\xAA\xBB\xCC\xDD");
    w.endChunk(data1);
    w.endChunk(w1_sz);

    w.endChunk(wvpl_sz);
    w.endChunk(riff_sz);
}

fn testFullDlsHeaderAndInfo() {
    let w = RiffBuilder {};
    buildSampleDls(*w);
    const data = w.slice();

    expect_eq(isDls(data), true);
    const hdr = parseDls(data).?;
    expect_eq(hdr.instrument_count, 2);
    expect_eq(hdr.has_version, true);
    expect_eq(hdr.version.major(), 2);
    expect_eq(hdr.version.minor(), 1);
    expect_eq(hdr.cue_count, 2);
    expect_eq(std.mem.eql(u8, hdr.info.inam, "GM Bank"), true);
    expect_eq(std.mem.eql(u8, hdr.info.icop, "CP 2026"), true);
    expect_eq(std.mem.eql(u8, hdr.info.ieng, "SynthEng"), true);
    expect_eq(std.mem.eql(u8, hdr.info.icmt, "Level 2"), true);
    expect_eq(std.mem.eql(u8, hdr.info.isft, "CapSynth"), true);
    expect_eq(std.mem.eql(u8, hdr.info.icrd, "2026-11-21"), true);
}

fn testInstrumentsMelodicAndDrumKit() {
    let w = RiffBuilder {};
    buildSampleDls(*w);
    const hdr = parseDls(w.slice()).?;

    let it = hdr.instruments();
    const inst0 = it.next().?;
    expect_eq(std.mem.eql(u8, inst0.name, "Grand Piano"), true);
    expect_eq(inst0.regions, 2);
    expect_eq(inst0.bank, 0);
    expect_eq(inst0.instrument_program, 0);
    expect_eq(inst0.isDrum(), false);

    const inst1 = it.next().?;
    expect_eq(std.mem.eql(u8, inst1.name, "Standard Kit"), true);
    expect_eq(inst1.regions, 16);
    expect_eq(inst1.isDrum(), true);

    expect_eq(it.next() == null, true);
}

fn testWavePoolFmtWsmpAndPcm() {
    let w = RiffBuilder {};
    buildSampleDls(*w);
    const hdr = parseDls(w.slice()).?;

    let it = hdr.waves();
    const w0 = it.next().?;
    expect_eq(std.mem.eql(u8, w0.name, "PianoC4"), true);
    expect_eq(w0.format_tag, 1);
    expect_eq(w0.channels, 1);
    expect_eq(w0.sample_rate, 44100);
    expect_eq(w0.bits_per_sample, 16);
    expect_eq(w0.has_wsmp, true);
    expect_eq(w0.unity_note, 60);
    expect_eq(w0.fine_tune, -25);
    expect_eq(w0.attenuation, -65536);
    expect_eq(w0.pcm_data.len, 5);
    expect_eq(w0.pcm_data[0], 0x01);
    expect_eq(w0.pcm_data[4], 0x05);

    const w1 = it.next().?;
    expect_eq(w1.channels, 2);
    expect_eq(w1.sample_rate, 48000);
    expect_eq(w1.has_wsmp, false);
    expect_eq(w1.pcm_data.len, 4);
    expect_eq(w1.pcm_data[0], 0xAA);
    expect_eq(w1.pcm_data[3], 0xDD);

    expect_eq(it.next() == null, true);
}

fn testOddChunkPaddingAlignment() {
    // Minimal DLS with odd-sized unknown chunk followed immediately by colh
    let w = RiffBuilder {};
    const riff_sz = w.beginChunk("RIFF");
    w.writeBytes("DLS ");

    const odd_sz = w.beginChunk("JUNK");
    w.writeBytes("123"); // 3 bytes + 1 pad byte
    w.endChunk(odd_sz);

    const colh_sz = w.beginChunk("colh");
    w.writeU32Le(7);
    w.endChunk(colh_sz);

    w.endChunk(riff_sz);

    const hdr = parseDls(w.slice()).?;
    expect_eq(hdr.instrument_count, 7);
}

fn testMalformedDlsGuards() {
    // Non-DLS RIFF ("WAVE")
    expect_eq(isDls("RIFF\x04\x00\x00\x00WAVE"), false);
    expect_eq(parseDls("RIFF\x04\x00\x00\x00WAVE") == null, true);

    // Truncated / empty
    expect_eq(isDls(""), false);
    expect_eq(isDls("RIFF\x04\x00\x00\x00DLS"), false);

    // Out-of-bounds RIFF size
    expect_eq(isDls("RIFF\xFF\xFF\x00\x00DLS "), false);

    // Out-of-bounds child chunk size inside DLS
    let w = RiffBuilder {};
    w.writeBytes("RIFF");
    w.writeU32Le(12); // body = 8 bytes
    w.writeBytes("DLS ");
    w.writeBytes("colh");
    w.writeU32Le(9999); // exceeds body
    expect_eq(isDls(w.slice()), true);
    expect_eq(parseDls(w.slice()) == null, true);
}

test("1. Complete RIFF DLS Header (colh, vers, LIST/INFO, ptbl) Parsing", () => {
    testFullDlsHeaderAndInfo();
});

test("2. DlsInstIter Melodic vs Drum Kit (0x80000000) & INAM Extraction", () => {
    testInstrumentsMelodicAndDrumKit();
});

test("3. DlsWaveIter fmt , wsmp (Signed FineTune/Attenuation) & PCM Data", () => {
    testWavePoolFmtWsmpAndPcm();
});

test("4. 2-Byte RIFF Even Alignment Padding on Odd-Sized Chunks", () => {
    testOddChunkPaddingAlignment();
});

test("5. Non-DLS RIFF, Truncated Header & Out-of-Bounds Chunk Guards", () => {
    testMalformedDlsGuards();
});
