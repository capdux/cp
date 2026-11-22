// Source: cp/cap/test/test_macho_production_deep.cp (Production deep tests for cap:macho)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MachoKind,
    CPU_TYPE_X86_64,
    CPU_TYPE_ARM64,
    MH_EXECUTE,
    MH_DYLIB,
    MachoHeader,
    MachoLoadCmd,
    FatArchEntry,
    detectMachoKind,
    parseMachoHeader,
    MachoLoadCmdIter,
    FatArchIter,
} from "cap:binfmt";

fn putU32LE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU32BE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putU64BE(buf: *[]u8, off: usize, v: u64) {
    putU32BE(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    putU32BE(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

test("1. detectMachoKind identifies all 8 Mach-O and Fat magic signatures and rejects unknown/short inputs", () => {
    const m_t32_be: [4]u8 = [0xFE, 0xED, 0xFA, 0xCE];
    const m_t32_le: [4]u8 = [0xCE, 0xFA, 0xED, 0xFE];
    const m_t64_be: [4]u8 = [0xFE, 0xED, 0xFA, 0xCF];
    const m_t64_le: [4]u8 = [0xCF, 0xFA, 0xED, 0xFE];
    const m_f32_be: [4]u8 = [0xCA, 0xFE, 0xBA, 0xBE];
    const m_f32_le: [4]u8 = [0xBE, 0xBA, 0xFE, 0xCA];
    const m_f64_be: [4]u8 = [0xCA, 0xFE, 0xBA, 0xBF];
    const m_f64_le: [4]u8 = [0xBF, 0xBA, 0xFE, 0xCA];
    const m_elf: [4]u8 = [0x7F, 'E', 'L', 'F'];
    const m_short: [2]u8 = [0xCF, 0xFA];

    expect_eq(detectMachoKind(m_t32_be[0..4]) == MachoKind.thin32, true);
    expect_eq(detectMachoKind(m_t32_le[0..4]) == MachoKind.thin32, true);
    expect_eq(detectMachoKind(m_t64_be[0..4]) == MachoKind.thin64, true);
    expect_eq(detectMachoKind(m_t64_le[0..4]) == MachoKind.thin64, true);
    expect_eq(detectMachoKind(m_f32_be[0..4]) == MachoKind.fat32, true);
    expect_eq(detectMachoKind(m_f32_le[0..4]) == MachoKind.fat32, true);
    expect_eq(detectMachoKind(m_f64_be[0..4]) == MachoKind.fat64, true);
    expect_eq(detectMachoKind(m_f64_le[0..4]) == MachoKind.fat64, true);
    expect_eq(detectMachoKind(m_elf[0..4]) == MachoKind.unknown, true);
    expect_eq(detectMachoKind(m_short[0..2]) == MachoKind.unknown, true);
});

test("2. parseMachoHeader decodes little-endian 64-bit ARM64 executable and big-endian 32-bit dylib", () => {
    // 64-bit LE ARM64 executable
    let bin64: [32]u8 = undefined;
    let s64 = bin64[0..32];
    putU32BE(*s64, 0, 0xCFFAEDFE); // MH_MAGIC_64 in LE
    putU32LE(*s64, 4, CPU_TYPE_ARM64);
    putU32LE(*s64, 8, 0);
    putU32LE(*s64, 12, MH_EXECUTE);
    putU32LE(*s64, 16, 12);
    putU32LE(*s64, 20, 1024);
    putU32LE(*s64, 24, 0x00200085);
    putU32LE(*s64, 28, 0);

    const h64 = parseMachoHeader(s64) ?? unreachable;
    expect_eq(h64.is_64bit, true);
    expect_eq(h64.is_big_endian, false);
    expect_eq(h64.isArm64(), true);
    expect_eq(h64.isX86_64(), false);
    expect_eq(h64.isExecutable(), true);
    expect_eq(h64.isDylib(), false);
    expect_eq(h64.ncmds, 12);
    expect_eq(h64.sizeofcmds, 1024);
    expect_eq(h64.headerSize(), 32);

    // Truncated 64-bit header (28 bytes < 32) must return null
    expect_eq(parseMachoHeader(s64[0..28]) == null, true);

    // 32-bit BE x86_64-typed dylib
    let bin32: [28]u8 = undefined;
    let s32 = bin32[0..28];
    putU32BE(*s32, 0, 0xFEEDFACE); // MH_MAGIC in BE
    putU32BE(*s32, 4, CPU_TYPE_X86_64);
    putU32BE(*s32, 8, 3);
    putU32BE(*s32, 12, MH_DYLIB);
    putU32BE(*s32, 16, 4);
    putU32BE(*s32, 20, 128);
    putU32BE(*s32, 24, 0x10);

    const h32 = parseMachoHeader(s32) ?? unreachable;
    expect_eq(h32.is_64bit, false);
    expect_eq(h32.is_big_endian, true);
    expect_eq(h32.isX86_64(), true);
    expect_eq(h32.isArm64(), false);
    expect_eq(h32.isDylib(), true);
    expect_eq(h32.isExecutable(), false);
    expect_eq(h32.headerSize(), 28);
});

test("3. MachoLoadCmdIter iterates LC_SEGMENT_64 and LC_UUID payloads cleanly", () => {
    // 32-byte header + cmd0 (16 bytes) + cmd1 (24 bytes) = 72 bytes
    let raw: [72]u8 = undefined;
    let i: usize = 0;
    while (i < 72) {
        raw[i] = 0;
        i += 1;
    }
    let s = raw[0..72];
    putU32BE(*s, 0, 0xCFFAEDFE);
    putU32LE(*s, 4, CPU_TYPE_ARM64);
    putU32LE(*s, 12, MH_EXECUTE);
    putU32LE(*s, 16, 2);   // ncmds = 2
    putU32LE(*s, 20, 40);  // sizeofcmds = 16 + 24

    // Command 0: LC_SEGMENT_64 (0x19), cmdsize = 16, payload = "__TEXT\0\0"
    putU32LE(*s, 32, 0x19);
    putU32LE(*s, 36, 16);
    s[40] = '_';
    s[41] = '_';
    s[42] = 'T';
    s[43] = 'E';
    s[44] = 'X';
    s[45] = 'T';

    // Command 1: LC_UUID (0x1B), cmdsize = 24, 16-byte UUID payload
    putU32LE(*s, 48, 0x1B);
    putU32LE(*s, 52, 24);
    s[56] = 0xDE;
    s[57] = 0xAD;
    s[71] = 0xEF;

    let it = MachoLoadCmdIter.init(s);

    const c0 = it.next() ?? unreachable;
    expect_eq(c0.cmd, 0x19);
    expect_eq(c0.cmdsize, 16);
    expect_eq(c0.payload.len, 8);
    expect_eq(std.mem.eql(u8, c0.payload[0..6], "__TEXT"), true);

    const c1 = it.next() ?? unreachable;
    expect_eq(c1.cmd, 0x1B);
    expect_eq(c1.cmdsize, 24);
    expect_eq(c1.payload.len, 16);
    expect_eq(c1.payload[0], 0xDE);
    expect_eq(c1.payload[15], 0xEF);

    expect_eq(it.next() == null, true);
});

test("4. MachoLoadCmdIter guards against malformed cmdsize (< 8 or out-of-bounds)", () => {
    let bad_small: [48]u8 = undefined;
    let i: usize = 0;
    while (i < 48) {
        bad_small[i] = 0;
        i += 1;
    }
    let s1 = bad_small[0..48];
    putU32BE(*s1, 0, 0xCFFAEDFE);
    putU32LE(*s1, 16, 1);
    putU32LE(*s1, 20, 16);
    putU32LE(*s1, 32, 0x19);
    putU32LE(*s1, 36, 4); // invalid cmdsize < 8

    let it1 = MachoLoadCmdIter.init(s1);
    expect_eq(it1.next() == null, true);

    // cmdsize exceeds remaining buffer
    putU32LE(*s1, 36, 9999);
    let it2 = MachoLoadCmdIter.init(s1);
    expect_eq(it2.next() == null, true);
});

test("5. FatArchIter parses big-endian Fat32 and Fat64 Universal Binary architecture tables", () => {
    // Fat32: 8-byte header + 2 * 20-byte entries = 48 bytes
    let fat32: [48]u8 = undefined;
    let s32 = fat32[0..48];
    putU32BE(*s32, 0, 0xCAFEBABE);
    putU32BE(*s32, 4, 2);

    // Arch 0: x86_64
    putU32BE(*s32, 8, CPU_TYPE_X86_64);
    putU32BE(*s32, 12, 3);
    putU32BE(*s32, 16, 4096);
    putU32BE(*s32, 20, 65536);
    putU32BE(*s32, 24, 12);

    // Arch 1: arm64
    putU32BE(*s32, 28, CPU_TYPE_ARM64);
    putU32BE(*s32, 32, 0);
    putU32BE(*s32, 36, 73728);
    putU32BE(*s32, 40, 81920);
    putU32BE(*s32, 44, 14);

    let it32 = FatArchIter.init(s32);
    const a0 = it32.next() ?? unreachable;
    expect_eq(a0.cputype, CPU_TYPE_X86_64);
    expect_eq(a0.cpusubtype, 3);
    expect_eq(a0.offset, 4096);
    expect_eq(a0.size, 65536);
    expect_eq(a0.align_pow2, 12);

    const a1 = it32.next() ?? unreachable;
    expect_eq(a1.cputype, CPU_TYPE_ARM64);
    expect_eq(a1.offset, 73728);
    expect_eq(a1.size, 81920);
    expect_eq(a1.align_pow2, 14);
    expect_eq(it32.next() == null, true);

    // Fat64: 8-byte header + 1 * 32-byte entry = 40 bytes
    let fat64: [40]u8 = undefined;
    let s64 = fat64[0..40];
    putU32BE(*s64, 0, 0xCAFEBABF);
    putU32BE(*s64, 4, 1);
    putU32BE(*s64, 8, CPU_TYPE_ARM64);
    putU32BE(*s64, 12, 0);
    putU64BE(*s64, 16, 0x0000000100004000);
    putU64BE(*s64, 24, 0x0000000000100000);
    putU32BE(*s64, 32, 14);
    putU32BE(*s64, 36, 0);

    let it64 = FatArchIter.init(s64);
    const f0 = it64.next() ?? unreachable;
    expect_eq(f0.cputype, CPU_TYPE_ARM64);
    expect_eq(f0.offset, 0x0000000100004000);
    expect_eq(f0.size, 0x0000000000100000);
    expect_eq(f0.align_pow2, 14);
    expect_eq(it64.next() == null, true);
});
