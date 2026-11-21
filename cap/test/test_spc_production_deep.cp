// Source: cp/cap/test/test_spc_production_deep.cp
// Deep Production Test Suite for cap:spc
// Covers:
//   1. Text ID666 full 256B header parsing (song/game/dumper/comments/artist/play_seconds/fade_ms)
//   2. Binary ID666 auto-detection & 24-bit play_seconds / 32-bit fade_ms / artist parsing
//   3. SPC700 CPU registers (pc/a/x/y/psw/sp) & has_id666 == false (offset 35 == 27)
//   4. Complete 66048B+ snapshot ramSlice/dspSlice/extraRamSlice & 4B-aligned xid6 sub-chunk iteration
//   5. Bad magic, missing 0x1A 0x1A, truncated header & out-of-bounds xid6 guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SPC_MAGIC,
    SPC_HEADER_SIZE,
    SPC_RAM_OFFSET,
    SPC_RAM_SIZE,
    SPC_DSP_OFFSET,
    SPC_DSP_SIZE,
    SPC_EXTRA_RAM_OFFSET,
    SPC_EXTRA_RAM_SIZE,
    SPC_MIN_FILE_SIZE,
    SPC_XID6_MAGIC,
    SpcId666Format,
    SpcRegs,
    SpcHeader,
    SpcXid6Item,
    SpcXid6Iter,
    isSpc,
    parseSpcHeader,
} from "cap:spc";

let g_full_spc: [66120]u8 = [0; 66120];

fn copyAt(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn writeU16LeAt(dst: []u8, off: usize, v: u16) {
    dst[off] = (v & 0xFF) as u8;
    dst[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32LeAt(dst: []u8, off: usize, v: u32) {
    dst[off] = (v & 0xFF) as u8;
    dst[off + 1] = ((v >> 8) & 0xFF) as u8;
    dst[off + 2] = ((v >> 16) & 0xFF) as u8;
    dst[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn initBaseHeader(buf: []u8, has_id666: bool) {
    let i: usize = 0;
    while (i < 256) {
        buf[i] = 0;
        i += 1;
    }
    copyAt(buf, 0, SPC_MAGIC);
    buf[33] = 0x1A;
    buf[34] = 0x1A;
    buf[35] = if (has_id666) 26 else 27;
    buf[36] = 30; // version minor 30
    // CPU regs: PC=0x0400, A=0x12, X=0x34, Y=0x56, PSW=0x82, SP=0xEF
    writeU16LeAt(buf, 37, 0x0400);
    buf[39] = 0x12;
    buf[40] = 0x34;
    buf[41] = 0x56;
    buf[42] = 0x82;
    buf[43] = 0xEF;
}

fn testTextId666Header() {
    let buf: [256]u8 = [0; 256];
    initBaseHeader(buf[0..], true);

    copyAt(buf[0..], 46, "Corridors of Time");
    copyAt(buf[0..], 78, "Chrono Trigger");
    copyAt(buf[0..], 110, "DGen");
    copyAt(buf[0..], 126, "Zeal Kingdom theme");
    copyAt(buf[0..], 158, "03/11/1995");
    copyAt(buf[0..], 169, "180");
    copyAt(buf[0..], 172, "10000");
    copyAt(buf[0..], 177, "Yasunori Mitsuda");
    buf[210] = '2'; // ZSNES

    expect_eq(isSpc(buf[0..]), true);
    const hdr = parseSpcHeader(buf[0..]).?;
    expect_eq(hdr.has_id666, true);
    expect_eq(hdr.version_minor, 30);
    expect_eq(hdr.id666_format == SpcId666Format.text, true);
    expect_eq(std.mem.eql(u8, hdr.song_title, "Corridors of Time"), true);
    expect_eq(std.mem.eql(u8, hdr.game_title, "Chrono Trigger"), true);
    expect_eq(std.mem.eql(u8, hdr.dumper_name, "DGen"), true);
    expect_eq(std.mem.eql(u8, hdr.comments, "Zeal Kingdom theme"), true);
    expect_eq(hdr.play_seconds, 180);
    expect_eq(hdr.fade_ms, 10000);
    expect_eq(std.mem.eql(u8, hdr.artist, "Yasunori Mitsuda"), true);
    expect_eq(hdr.emulator_id, 2);

    // 256B-only buffer has no full RAM/DSP/ExtraRAM
    expect_eq(hdr.ramSlice(buf[0..]) == null, true);
    expect_eq(hdr.dspSlice(buf[0..]) == null, true);
    expect_eq(hdr.extraRamSlice(buf[0..]) == null, true);
}

fn testBinaryId666Header() {
    let buf: [256]u8 = [0; 256];
    initBaseHeader(buf[0..], true);

    copyAt(buf[0..], 46, "Terra's Theme");
    copyAt(buf[0..], 78, "Final Fantasy VI");
    copyAt(buf[0..], 110, "SNES9x");
    copyAt(buf[0..], 126, "Binary ID666");
    // Binary date (non-ASCII bytes at 158..162)
    writeU32LeAt(buf[0..], 158, 0x019940402);
    // 24-bit LE play_seconds = 240 (0x0000F0 -> contains non-ASCII byte 0xF0)
    buf[169] = 0xF0;
    buf[170] = 0x00;
    buf[171] = 0x00;
    // 32-bit LE fade_ms = 5000 (0x00001388)
    writeU32LeAt(buf[0..], 172, 5000);
    copyAt(buf[0..], 176, "Nobuo Uematsu");
    buf[209] = 3; // Snes9x

    const hdr = parseSpcHeader(buf[0..]).?;
    expect_eq(hdr.id666_format == SpcId666Format.binary, true);
    expect_eq(hdr.play_seconds, 240);
    expect_eq(hdr.fade_ms, 5000);
    expect_eq(std.mem.eql(u8, hdr.artist, "Nobuo Uematsu"), true);
    expect_eq(hdr.emulator_id, 3);
}

fn testCpuRegsAndNoId666Flag() {
    let buf: [256]u8 = [0; 256];
    initBaseHeader(buf[0..], false); // offset 35 == 27 (no ID666)
    copyAt(buf[0..], 46, "ShouldBeIgnored");

    const hdr = parseSpcHeader(buf[0..]).?;
    expect_eq(hdr.has_id666, false);
    expect_eq(hdr.id666_format == SpcId666Format.none, true);
    expect_eq(hdr.song_title.len, 0);
    expect_eq(hdr.regs.pc, 0x0400);
    expect_eq(hdr.regs.a, 0x12);
    expect_eq(hdr.regs.x, 0x34);
    expect_eq(hdr.regs.y, 0x56);
    expect_eq(hdr.regs.psw, 0x82);
    expect_eq(hdr.regs.sp, 0xEF);
}

fn testFullSnapshotSlicesAndXid6Iter() {
    initBaseHeader(g_full_spc[0..], true);
    g_full_spc[SPC_RAM_OFFSET] = 0xAA;
    g_full_spc[SPC_RAM_OFFSET + SPC_RAM_SIZE - 1] = 0x55;
    g_full_spc[SPC_DSP_OFFSET] = 0x7F;
    g_full_spc[SPC_EXTRA_RAM_OFFSET] = 0xCD;

    // Append xid6 chunk at offset 66048:
    //   Item 0: id=0x01, type=1 (string), len=5 ("OST!\0") -> padded to 8B (total 12B)
    //   Item 1: id=0x14, type=0 (inline u16), val=1995 (total 4B)
    //   Item 2: id=0x30, type=4 (u32 int), val=64000 (total 8B)
    //   Total xid6 body = 12 + 4 + 8 = 24 bytes
    const xoff = SPC_MIN_FILE_SIZE;
    copyAt(g_full_spc[0..], xoff, SPC_XID6_MAGIC);
    writeU32LeAt(g_full_spc[0..], xoff + 4, 24);

    let p = xoff + 8;
    // Item 0: string len 5 ("OST!\0") + 3 pad bytes
    g_full_spc[p] = 0x01;
    g_full_spc[p + 1] = 1;
    writeU16LeAt(g_full_spc[0..], p + 2, 5);
    copyAt(g_full_spc[0..], p + 4, "OST!");
    g_full_spc[p + 8] = 0;
    g_full_spc[p + 9] = 0;
    g_full_spc[p + 10] = 0;
    g_full_spc[p + 11] = 0;
    p += 12;

    // Item 1: inline u16 = 1995
    g_full_spc[p] = 0x14;
    g_full_spc[p + 1] = 0;
    writeU16LeAt(g_full_spc[0..], p + 2, 1995);
    p += 4;

    // Item 2: u32 int = 64000
    g_full_spc[p] = 0x30;
    g_full_spc[p + 1] = 4;
    writeU16LeAt(g_full_spc[0..], p + 2, 4);
    writeU32LeAt(g_full_spc[0..], p + 4, 64000);
    p += 8;

    const full_slice = g_full_spc[0..p];
    const hdr = parseSpcHeader(full_slice).?;

    const ram = hdr.ramSlice(full_slice).?;
    expect_eq(ram.len, SPC_RAM_SIZE);
    expect_eq(ram[0], 0xAA);
    expect_eq(ram[SPC_RAM_SIZE - 1], 0x55);

    const dsp = hdr.dspSlice(full_slice).?;
    expect_eq(dsp.len, SPC_DSP_SIZE);
    expect_eq(dsp[0], 0x7F);

    const extra = hdr.extraRamSlice(full_slice).?;
    expect_eq(extra.len, SPC_EXTRA_RAM_SIZE);
    expect_eq(extra[0], 0xCD);

    let xit = SpcXid6Iter.init(full_slice);
    const it0 = xit.next().?;
    expect_eq(it0.id, 0x01);
    expect_eq(it0.data_type, 1);
    expect_eq(std.mem.eql(u8, it0.str_val, "OST!"), true);

    const it1 = xit.next().?;
    expect_eq(it1.id, 0x14);
    expect_eq(it1.data_type, 0);
    expect_eq(it1.int_val, 1995);

    const it2 = xit.next().?;
    expect_eq(it2.id, 0x30);
    expect_eq(it2.data_type, 4);
    expect_eq(it2.int_val, 64000);

    expect_eq(xit.next() == null, true);
}

fn testMalformedSpcAndXid6Guards() {
    // Truncated / bad magic
    expect_eq(isSpc(""), false);
    expect_eq(isSpc("SNES-SPC700 Sound File Data v0.30"), false);

    // Missing 0x1A 0x1A terminator at 33..35
    let bad: [256]u8 = [0; 256];
    initBaseHeader(bad[0..], true);
    bad[34] = 0x00;
    expect_eq(isSpc(bad[0..]), false);
    expect_eq(parseSpcHeader(bad[0..]) == null, true);

    // Out-of-bounds xid6 chunk_size
    const xoff = SPC_MIN_FILE_SIZE;
    copyAt(g_full_spc[0..], xoff, SPC_XID6_MAGIC);
    writeU32LeAt(g_full_spc[0..], xoff + 4, 999999); // exceeds file len
    let bad_xit = SpcXid6Iter.init(g_full_spc[0 .. xoff + 16]);
    expect_eq(bad_xit.next() == null, true);
}

test("1. Text ID666 Full 256B Header Parsing", () => {
    testTextId666Header();
});

test("2. Binary ID666 Auto-Detection & Duration/Fade Decoding", () => {
    testBinaryId666Header();
});

test("3. SPC700 CPU Registers & has_id666 == false Handling", () => {
    testCpuRegsAndNoId666Flag();
});

test("4. Full 66048B+ Snapshot Slices & 4B-Aligned xid6 Sub-Chunk Iteration", () => {
    testFullSnapshotSlicesAndXid6Iter();
});

test("5. Bad Magic, Missing 0x1A 0x1A & Out-of-Bounds xid6 Guards", () => {
    testMalformedSpcAndXid6Guards();
});
