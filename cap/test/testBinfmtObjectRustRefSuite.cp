// Reference-hardened test suite against gimli-rs/object (MIT/Apache-2.0)
// Covers Thin Mach-O 64/32 (LE+BE), Fat/Universal Binary, COFF .obj & PE32+ synthetic images,
// strict bounds/alignment/overflow/overlap rejection guards, 16-lane @Vector(16, u8) SIMD vs scalar
// 0-diff parity, and 1,600 deterministic PRNG fuzz cases (800 Mach-O + 800 COFF).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MachoKind,
    MH_MAGIC,
    MH_CIGAM,
    MH_MAGIC_64,
    MH_CIGAM_64,
    FAT_MAGIC,
    FAT_MAGIC_64,
    LC_SEGMENT,
    LC_SEGMENT_64,
    CPU_TYPE_X86_64,
    CPU_TYPE_ARM64,
    MH_EXECUTE,
    MACHO_SEG32_CMD_SIZE,
    MACHO_SECT32_SIZE,
    MACHO_SEG64_CMD_SIZE,
    MACHO_SECT64_SIZE,
    MachoError,
    simdObjTrimNul16,
    scalarObjTrimNul16,
    simdObjBytesEqual,
    scalarObjBytesEqual,
    detectMachoKind,
    parseMachoHeader,
    MachoLoadCmdIter,
    findMachoSection,
    extractFatArchSlice,
    FatArchIter,
    validateMachoStrict,
} from "../../lib/binfmt/macho.cp";
import {
    COFF_HEADER_SIZE,
    COFF_SECTION_SIZE,
    COFF_SYMBOL_SIZE,
    PE_DOS_MAGIC,
    PE32_OPT_MAGIC,
    PE32PLUS_OPT_MAGIC,
    PE32PLUS_MIN_OPT_HEADER_SIZE,
    MACHINE_AMD64,
    MACHINE_ARM64,
    SCN_CNT_CODE,
    SCN_MEM_EXECUTE,
    SCN_MEM_READ,
    SYM_CLASS_EXTERNAL,
    CoffError,
    parseCoff,
    parsePeOrCoff,
    validateCoffStrict,
} from "../../lib/binfmt/coff.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

fn putU16LE(buf: []u8, off: usize, v: u16) {
    buf[off] = @as(u8, @truncate(v));
    buf[off + 1] = @as(u8, @truncate(v >> 8));
}

fn putU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = @as(u8, @truncate(v));
    buf[off + 1] = @as(u8, @truncate(v >> 8));
    buf[off + 2] = @as(u8, @truncate(v >> 16));
    buf[off + 3] = @as(u8, @truncate(v >> 24));
}

fn putU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = @as(u8, @truncate(v >> 24));
    buf[off + 1] = @as(u8, @truncate(v >> 16));
    buf[off + 2] = @as(u8, @truncate(v >> 8));
    buf[off + 3] = @as(u8, @truncate(v));
}

fn putU32(buf: []u8, off: usize, v: u32, is_be: bool) {
    if (is_be) {
        putU32BE(buf, off, v);
    } else {
        putU32LE(buf, off, v);
    }
}

fn putU64(buf: []u8, off: usize, v: u64, is_be: bool) {
    if (is_be) {
        putU32BE(buf, off, @as(u32, @truncate(v >> 32)));
        putU32BE(buf, off + 4, @as(u32, @truncate(v)));
    } else {
        putU32LE(buf, off, @as(u32, @truncate(v)));
        putU32LE(buf, off + 4, @as(u32, @truncate(v >> 32)));
    }
}

fn putStr(buf: []u8, off: usize, s: string) {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        buf[off + i] = s[i];
    }
}

// Synthesizes a valid Thin Mach-O 64-bit image (LE or BE) with 1 LC_SEGMENT_64 (__TEXT) & 1 section (__text)
fn buildMacho64(buf: []u8, is_be: bool, cputype: u32): usize {
    const cmd_size: usize = MACHO_SEG64_CMD_SIZE + MACHO_SECT64_SIZE; // 72 + 80 = 152
    const data_off: usize = cmd_size + 32; // 184
    const sec_len: usize = 16;
    const total: usize = data_off + sec_len; // 200
    @memset(buf[0..total], 0);

    // Header (32B)
    putU32BE(buf, 0, if (is_be) MH_MAGIC_64 else MH_CIGAM_64);
    putU32(buf, 4, cputype, is_be);
    putU32(buf, 8, 0, is_be);
    putU32(buf, 12, MH_EXECUTE, is_be);
    putU32(buf, 16, 1, is_be); // ncmds
    putU32(buf, 20, @as(u32, @intCast(cmd_size)), is_be); // sizeofcmds
    putU32(buf, 24, 0, is_be);

    // LC_SEGMENT_64 (72B)
    const c_off: usize = 32;
    putU32(buf, c_off, LC_SEGMENT_64, is_be);
    putU32(buf, c_off + 4, @as(u32, @intCast(cmd_size)), is_be);
    putStr(buf, c_off + 8, "__TEXT");
    putU64(buf, c_off + 24, 0x100000000, is_be); // vmaddr
    putU64(buf, c_off + 32, 0x1000, is_be); // vmsize
    putU64(buf, c_off + 40, @as(u64, @intCast(data_off)), is_be); // fileoff
    putU64(buf, c_off + 48, @as(u64, @intCast(sec_len)), is_be); // filesize
    putU32(buf, c_off + 56, 5, is_be); // maxprot
    putU32(buf, c_off + 60, 5, is_be); // initprot
    putU32(buf, c_off + 64, 1, is_be); // nsects
    putU32(buf, c_off + 68, 0, is_be); // flags

    // section_64 (80B)
    const s_off: usize = c_off + MACHO_SEG64_CMD_SIZE;
    putStr(buf, s_off, "__text");
    putStr(buf, s_off + 16, "__TEXT");
    putU64(buf, s_off + 32, 0x100000000, is_be); // addr
    putU64(buf, s_off + 40, @as(u64, @intCast(sec_len)), is_be); // size
    putU32(buf, s_off + 48, @as(u32, @intCast(data_off)), is_be); // offset
    putU32(buf, s_off + 52, 4, is_be); // align

    putStr(buf, data_off, "MACHO64_PAYLOAD!");
    return total;
}

// Synthesizes a valid Thin Mach-O 32-bit image (LE or BE) with 1 LC_SEGMENT (__TEXT) & 1 section (__text)
fn buildMacho32(buf: []u8, is_be: bool): usize {
    const cmd_size: usize = MACHO_SEG32_CMD_SIZE + MACHO_SECT32_SIZE; // 56 + 68 = 124
    const data_off: usize = cmd_size + 28; // 152
    const sec_len: usize = 8;
    const total: usize = data_off + sec_len; // 160
    @memset(buf[0..total], 0);

    putU32BE(buf, 0, if (is_be) MH_MAGIC else MH_CIGAM);
    putU32(buf, 4, 7, is_be); // CPU_TYPE_I386
    putU32(buf, 12, MH_EXECUTE, is_be);
    putU32(buf, 16, 1, is_be);
    putU32(buf, 20, @as(u32, @intCast(cmd_size)), is_be);

    const c_off: usize = 28;
    putU32(buf, c_off, LC_SEGMENT, is_be);
    putU32(buf, c_off + 4, @as(u32, @intCast(cmd_size)), is_be);
    putStr(buf, c_off + 8, "__TEXT");
    putU32(buf, c_off + 32, @as(u32, @intCast(data_off)), is_be); // fileoff
    putU32(buf, c_off + 36, @as(u32, @intCast(sec_len)), is_be); // filesize
    putU32(buf, c_off + 48, 1, is_be); // nsects

    const s_off: usize = c_off + MACHO_SEG32_CMD_SIZE;
    putStr(buf, s_off, "__text");
    putStr(buf, s_off + 16, "__TEXT");
    putU32(buf, s_off + 36, @as(u32, @intCast(sec_len)), is_be); // size
    putU32(buf, s_off + 40, @as(u32, @intCast(data_off)), is_be); // offset
    putStr(buf, data_off, "MACHO32!");
    return total;
}

// Synthesizes a valid standalone COFF .obj with 1 section (.text), 1 symbol (_main), and a COFF string table
fn buildCoffObj(buf: []u8): usize {
    const sec_off: usize = COFF_HEADER_SIZE; // 20
    const raw_off: usize = sec_off + COFF_SECTION_SIZE; // 60
    const raw_len: usize = 16;
    const sym_off: usize = raw_off + raw_len; // 76
    const str_off: usize = sym_off + COFF_SYMBOL_SIZE; // 94
    const str_size: usize = 4 + 12; // 4B size + "long_symbol\0"
    const total: usize = str_off + str_size; // 110
    @memset(buf[0..total], 0);

    putU16LE(buf, 0, MACHINE_AMD64);
    putU16LE(buf, 2, 1); // num_sections
    putU32LE(buf, 8, @as(u32, @intCast(sym_off)));
    putU32LE(buf, 12, 1); // num_symbols
    putU16LE(buf, 16, 0); // size_of_opt_header

    // Section .text
    putStr(buf, sec_off, ".text");
    putU32LE(buf, sec_off + 16, @as(u32, @intCast(raw_len)));
    putU32LE(buf, sec_off + 20, @as(u32, @intCast(raw_off)));
    putU32LE(buf, sec_off + 36, SCN_CNT_CODE | SCN_MEM_EXECUTE | SCN_MEM_READ);

    putStr(buf, raw_off, "COFF_CODE_BYTES!");

    // Symbol pointing to string table offset 4
    putU32LE(buf, sym_off, 0);
    putU32LE(buf, sym_off + 4, 4);
    putU16LE(buf, sym_off + 12, 1); // section 1
    buf[sym_off + 16] = SYM_CLASS_EXTERNAL;

    // String table
    putU32LE(buf, str_off, @as(u32, @intCast(str_size)));
    putStr(buf, str_off + 4, "long_symbol");
    buf[str_off + 15] = 0;

    return total;
}

// Synthesizes a valid PE32+ image (MZ + PE\0\0 + COFF + OptionalHeader64 + 1 section)
fn buildPe32Plus(buf: []u8): usize {
    const pe_off: usize = 64;
    const coff_off: usize = pe_off + 4; // 68
    const opt_off: usize = coff_off + COFF_HEADER_SIZE; // 88
    const opt_size: usize = PE32PLUS_MIN_OPT_HEADER_SIZE as usize; // 112
    const sec_off: usize = opt_off + opt_size; // 200
    const raw_off: usize = sec_off + COFF_SECTION_SIZE; // 240
    const raw_len: usize = 16;
    const total: usize = raw_off + raw_len; // 256
    @memset(buf[0..total], 0);

    putU16LE(buf, 0, PE_DOS_MAGIC);
    putU32LE(buf, 60, @as(u32, @intCast(pe_off)));
    putStr(buf, pe_off, "PE");
    buf[pe_off + 2] = 0;
    buf[pe_off + 3] = 0;

    putU16LE(buf, coff_off, MACHINE_ARM64);
    putU16LE(buf, coff_off + 2, 1);
    putU16LE(buf, coff_off + 16, PE32PLUS_MIN_OPT_HEADER_SIZE);

    putU16LE(buf, opt_off, PE32PLUS_OPT_MAGIC);

    putStr(buf, sec_off, ".rdata");
    putU32LE(buf, sec_off + 16, @as(u32, @intCast(raw_len)));
    putU32LE(buf, sec_off + 20, @as(u32, @intCast(raw_off)));
    putU32LE(buf, sec_off + 36, SCN_MEM_READ);
    putStr(buf, raw_off, "PE32PLUS_RDATA!!");
    return total;
}

fn checkValidMachoFatAndCoff(): u32 {
    // 1. Mach-O 64 LE & BE
    let m64_le: [256]u8 = [0; 256];
    const n64_le = buildMacho64(m64_le[0..], false, CPU_TYPE_ARM64);
    validateMachoStrict(m64_le[0..n64_le]) catch return 1;
    const h64 = parseMachoHeader(m64_le[0..n64_le]) ?? return 2;
    if (!h64.is_64bit || h64.is_big_endian || !h64.isArm64() || !h64.isExecutable()) return 3;
    const s64 = findMachoSection(m64_le[0..n64_le], "__TEXT", "__text") ?? return 4;
    if (!simdObjBytesEqual(s64.sectName(), "__text") || s64.size != 16) return 5;

    let m64_be: [256]u8 = [0; 256];
    const n64_be = buildMacho64(m64_be[0..], true, CPU_TYPE_X86_64);
    validateMachoStrict(m64_be[0..n64_be]) catch return 6;
    const h64b = parseMachoHeader(m64_be[0..n64_be]) ?? return 7;
    if (!h64b.is_big_endian || !h64b.isX86_64()) return 8;
    if (findMachoSection(m64_be[0..n64_be], "__TEXT", "__text") == null) return 9;

    // 2. Mach-O 32 LE & BE
    let m32_le: [200]u8 = [0; 200];
    const n32_le = buildMacho32(m32_le[0..], false);
    validateMachoStrict(m32_le[0..n32_le]) catch return 10;
    if (findMachoSection(m32_le[0..n32_le], "__TEXT", "__text") == null) return 11;

    let m32_be: [200]u8 = [0; 200];
    const n32_be = buildMacho32(m32_be[0..], true);
    validateMachoStrict(m32_be[0..n32_be]) catch return 12;
    if (detectMachoKind(m32_be[0..n32_be]) != MachoKind.thin32) return 13;

    // 3. Fat Universal Binary with 2 slices (ARM64 + x86_64)
    let fat: [512]u8 = [0; 512];
    const off1: usize = 48; // >= 8 + 2*20
    const off2: usize = off1 + n64_le;
    const fat_total: usize = off2 + n64_be;
    putU32BE(fat[0..], 0, FAT_MAGIC);
    putU32BE(fat[0..], 4, 2);
    // arch 0
    putU32BE(fat[0..], 8, CPU_TYPE_ARM64);
    putU32BE(fat[0..], 16, @as(u32, @intCast(off1)));
    putU32BE(fat[0..], 20, @as(u32, @intCast(n64_le)));
    // arch 1
    putU32BE(fat[0..], 28, CPU_TYPE_X86_64);
    putU32BE(fat[0..], 36, @as(u32, @intCast(off2)));
    putU32BE(fat[0..], 40, @as(u32, @intCast(n64_be)));
    @memcpy(fat[off1 .. off1 + n64_le], m64_le[0..n64_le]);
    @memcpy(fat[off2 .. off2 + n64_be], m64_be[0..n64_be]);

    validateMachoStrict(fat[0..fat_total]) catch return 14;
    let fit = FatArchIter.init(fat[0..fat_total]);
    const e0 = fit.next() ?? return 15;
    const sl0 = extractFatArchSlice(fat[0..fat_total], e0) ?? return 16;
    if (findMachoSection(sl0, "__TEXT", "__text") == null) return 17;
    const e1 = fit.next() ?? return 18;
    if (e1.cputype != CPU_TYPE_X86_64) return 19;

    // 4. Standalone COFF .obj + symbol string table lookup
    let coff_buf: [128]u8 = [0; 128];
    const coff_len = buildCoffObj(coff_buf[0..]);
    validateCoffStrict(coff_buf[0..coff_len]) catch return 20;
    const obj = parseCoff(coff_buf[0..coff_len]) ?? return 21;
    const text_sec = obj.findSectionByName(".text") ?? return 22;
    if (!text_sec.isCode() || !text_sec.isExecutable()) return 23;
    const sym0 = obj.symbolAt(0) ?? return 24;
    const sym_name = obj.symbolName(sym0) ?? return 25;
    if (!simdObjBytesEqual(sym_name, "long_symbol")) return 26;

    // 5. PE32+ image (MZ + PE\0\0 + COFF + OptionalHeader64)
    let pe_buf: [280]u8 = [0; 280];
    const pe_len = buildPe32Plus(pe_buf[0..]);
    validateCoffStrict(pe_buf[0..pe_len]) catch return 27;
    const pe = parsePeOrCoff(pe_buf[0..pe_len]) ?? return 28;
    const rdata = pe.findSectionByName(".rdata") ?? return 29;
    const rdata_bytes = pe.sectionData(rdata) ?? return 30;
    if (!simdObjBytesEqual(rdata_bytes, "PE32PLUS_RDATA!!")) return 31;

    return 0;
}

fn expectMachoFail(data: []const u8, expected: MachoError): bool {
    let hit = false;
    validateMachoStrict(data) catch |e| {
        if (e == expected) hit = true;
    };
    return hit;
}

fn expectCoffFail(data: []const u8, expected: CoffError): bool {
    let hit = false;
    validateCoffStrict(data) catch |e| {
        if (e == expected) hit = true;
    };
    return hit;
}

fn checkStrictRejectionGuards(): u32 {
    // 1. Mach-O invalid magic & truncated header
    if (!expectMachoFail("NOPE", MachoError.InvalidMagic)) return 1;
    if (!expectMachoFail("\xCF\xFA\xED\xFE\x00\x00", MachoError.TruncatedHeader)) return 2;

    // 2. Mach-O sizeofcmds out of bounds
    let bad: [256]u8 = [0; 256];
    const n = buildMacho64(bad[0..], false, CPU_TYPE_ARM64);
    putU32LE(bad[0..], 20, 0xFFFF0000);
    if (!expectMachoFail(bad[0..n], MachoError.SizeofCmdsOutOfBounds)) return 3;

    // 3. Mach-O cmdsize < 8
    * = buildMacho64(bad[0..], false, CPU_TYPE_ARM64);
    putU32LE(bad[0..], 36, 4);
    if (!expectMachoFail(bad[0..n], MachoError.InvalidCmdSize)) return 4;

    // 4. Mach-O unaligned cmdsize (not 8-byte aligned on 64-bit)
    * = buildMacho64(bad[0..], false, CPU_TYPE_ARM64);
    putU32LE(bad[0..], 36, 12);
    if (!expectMachoFail(bad[0..n], MachoError.UnalignedCmdSize)) return 5;

    // 5. Mach-O nsects multiplication overflow
    * = buildMacho64(bad[0..], false, CPU_TYPE_ARM64);
    putU32LE(bad[0..], 32 + 64, 0x10000000);
    if (!expectMachoFail(bad[0..n], MachoError.InvalidSegmentCommand)) return 6;

    // 6. Mach-O section offset+size out of bounds
    * = buildMacho64(bad[0..], false, CPU_TYPE_ARM64);
    putU32LE(bad[0..], 32 + MACHO_SEG64_CMD_SIZE + 48, 9999);
    if (!expectMachoFail(bad[0..n], MachoError.SectionOutOfBounds)) return 7;

    // 7. Fat Binary overlapping slices rejected
    let fat: [512]u8 = [0; 512];
    const off1: usize = 48;
    const total_fat: usize = off1 + n;
    putU32BE(fat[0..], 0, FAT_MAGIC);
    putU32BE(fat[0..], 4, 2);
    putU32BE(fat[0..], 8, CPU_TYPE_ARM64);
    putU32BE(fat[0..], 16, @as(u32, @intCast(off1)));
    putU32BE(fat[0..], 20, @as(u32, @intCast(n)));
    putU32BE(fat[0..], 28, CPU_TYPE_X86_64);
    putU32BE(fat[0..], 36, @as(u32, @intCast(off1))); // overlaps arch 0!
    putU32BE(fat[0..], 40, @as(u32, @intCast(n)));
    * = buildMacho64(bad[0..], false, CPU_TYPE_ARM64);
    @memcpy(fat[off1 .. off1 + n], bad[0..n]);
    if (!expectMachoFail(fat[0..total_fat], MachoError.FatArchOverlap)) return 8;

    // 8. COFF invalid machine & truncated header
    if (!expectCoffFail("short", CoffError.TruncatedHeader)) return 9;
    let cbuf: [128]u8 = [0; 128];
    const clen = buildCoffObj(cbuf[0..]);
    putU16LE(cbuf[0..], 0, 0xDEAD);
    if (!expectCoffFail(cbuf[0..clen], CoffError.InvalidMachine)) return 10;

    // 9. COFF NumberOfSections exceeding limit & out of bounds
    * = buildCoffObj(cbuf[0..]);
    putU16LE(cbuf[0..], 2, 5000);
    if (!expectCoffFail(cbuf[0..clen], CoffError.TooManySections)) return 11;
    * = buildCoffObj(cbuf[0..]);
    putU16LE(cbuf[0..], 2, 100);
    if (!expectCoffFail(cbuf[0..clen], CoffError.SectionTableOutOfBounds)) return 12;

    // 10. COFF symbol table overflow & out of bounds
    * = buildCoffObj(cbuf[0..]);
    putU32LE(cbuf[0..], 12, 0x0FFFFFFF);
    if (!expectCoffFail(cbuf[0..clen], CoffError.TooManySymbols)) return 13;
    * = buildCoffObj(cbuf[0..]);
    putU32LE(cbuf[0..], 8, 9999);
    if (!expectCoffFail(cbuf[0..clen], CoffError.SymbolTableOutOfBounds)) return 14;

    // 11. COFF section PointerToRawData + SizeOfRawData out of bounds
    * = buildCoffObj(cbuf[0..]);
    putU32LE(cbuf[0..], COFF_HEADER_SIZE + 16, 4096);
    if (!expectCoffFail(cbuf[0..clen], CoffError.SectionDataOutOfBounds)) return 15;

    // 12. PE invalid signature & invalid optional header magic
    let pebuf: [280]u8 = [0; 280];
    const pelen = buildPe32Plus(pebuf[0..]);
    pebuf[64] = 'X';
    if (!expectCoffFail(pebuf[0..pelen], CoffError.InvalidPeSignature)) return 16;
    * = buildPe32Plus(pebuf[0..]);
    putU16LE(pebuf[0..], 88, 0x9999);
    if (!expectCoffFail(pebuf[0..pelen], CoffError.InvalidOptionalHeader)) return 17;

    return 0;
}

fn checkSimdDifferential(): u32 {
    // 1. All 16B NUL placements (0..16, plus no NUL)
    let name16: [16]u8 = ['A'; 16];
    if (simdObjTrimNul16(name16[0..]).len != scalarObjTrimNul16(name16[0..]).len) return 1;
    let p: usize = 0;
    while (p < 16) : (p += 1) {
        name16[p] = 0;
        const s1 = simdObjTrimNul16(name16[0..]);
        const s2 = scalarObjTrimNul16(name16[0..]);
        if (s1.len != s2.len || s1.len != p) return 2;
        name16[p] = 'A';
    }

    // 2. Full 0..257 length sweep for simdObjTrimNul16 & simdObjBytesEqual
    let prng = XorShift64.init(0x4f42_4a53_494d_4431);
    let a: [260]u8 = [1; 260];
    let b: [260]u8 = [1; 260];
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            const v = prng.nextByte();
            a[i] = if (v == 0) 1 else v;
            b[i] = a[i];
        }
        if (simdObjTrimNul16(a[0..len]).len != scalarObjTrimNul16(a[0..len]).len) return 3;
        if (simdObjBytesEqual(a[0..len], b[0..len]) != scalarObjBytesEqual(a[0..len], b[0..len])) return 4;

        if (len > 0) {
            const spots: [3]usize = [ 0, len / 2, len - 1 ];
            for (sp of spots) {
                const orig = a[sp];
                a[sp] = 0;
                if (simdObjTrimNul16(a[0..len]).len != scalarObjTrimNul16(a[0..len]).len) return 5;
                if (simdObjBytesEqual(a[0..len], b[0..len]) != scalarObjBytesEqual(a[0..len], b[0..len])) return 6;
                a[sp] = orig;
            }
        }
    }
    return 0;
}

fn checkDeterministicFuzz(): u32 {
    let prng = XorShift64.init(0x2026_1124_4f42_4a46);
    let seed_macho: [256]u8 = [0; 256];
    const n_macho = buildMacho64(seed_macho[0..], false, CPU_TYPE_ARM64);
    let seed_coff: [280]u8 = [0; 280];
    const n_coff = buildPe32Plus(seed_coff[0..]);

    let work: [280]u8 = [0; 280];

    // 1. 800 Mach-O fuzz cases (400 mutated valid + 400 raw random)
    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        let cur_len: usize = 0;
        if (iter % 2 == 0) {
            cur_len = @as(usize, @intCast(prng.next() % (n_macho + 1)));
            @memcpy(work[0..cur_len], seed_macho[0..cur_len]);
            if (cur_len > 0) {
                const flips = @as(usize, @intCast(prng.next() % 4)) + 1;
                let fi: usize = 0;
                while (fi < flips) : (fi += 1) {
                    const idx = @as(usize, @intCast(prng.next() % cur_len));
                    work[idx] = prng.nextByte();
                }
            }
        } else {
            cur_len = @as(usize, @intCast(prng.next() % 256));
            prng.fillBytes(work[0..cur_len]);
        }
        const slice = work[0..cur_len];
        * = detectMachoKind(slice);
        * = parseMachoHeader(slice);
        let cit = MachoLoadCmdIter.init(slice);
        let steps: usize = 0;
        while (cit.next() != null && steps < 32) : (steps += 1) {}
        let fit = FatArchIter.init(slice);
        steps = 0;
        while (fit.next() != null && steps < 32) : (steps += 1) {}
        * = findMachoSection(slice, "__TEXT", "__text");
        validateMachoStrict(slice) catch {};
    }

    // 2. 800 COFF/PE fuzz cases (400 mutated valid + 400 raw random)
    iter = 0;
    while (iter < 800) : (iter += 1) {
        let cur_len: usize = 0;
        if (iter % 2 == 0) {
            cur_len = @as(usize, @intCast(prng.next() % (n_coff + 1)));
            @memcpy(work[0..cur_len], seed_coff[0..cur_len]);
            if (cur_len > 0) {
                const flips = @as(usize, @intCast(prng.next() % 4)) + 1;
                let fi: usize = 0;
                while (fi < flips) : (fi += 1) {
                    const idx = @as(usize, @intCast(prng.next() % cur_len));
                    work[idx] = prng.nextByte();
                }
            }
        } else {
            cur_len = @as(usize, @intCast(prng.next() % 256));
            prng.fillBytes(work[0..cur_len]);
        }
        const slice = work[0..cur_len];
        if (obj := parsePeOrCoff(slice)) {
            * = obj.findSectionByName(".text");
            if (sec := obj.sectionAt(0)) {
                * = obj.sectionData(sec);
                * = obj.sectionName(sec);
                * = obj.relocAt(sec, 0);
            }
            if (sym := obj.symbolAt(0)) {
                * = obj.symbolName(sym);
            }
        }
        validateCoffStrict(slice) catch {};
    }

    return 0;
}

fn main() {
    test("1. Valid Mach-O 64/32 (LE+BE), Fat Universal Binary & COFF/PE32+ parsing & queries", () => {
        expect_eq(checkValidMachoFatAndCoff(), 0 as u32);
    });

    test("2. Strict gimli-rs/object bounds, alignment, overflow & Fat overlap rejection guards", () => {
        expect_eq(checkStrictRejectionGuards(), 0 as u32);
    });

    test("3. 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity (0..257 lengths & 16B NUL)", () => {
        expect_eq(checkSimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG fuzzing (800 Mach-O + 800 COFF/PE malformed & mutated images)", () => {
        expect_eq(checkDeterministicFuzz(), 0 as u32);
    });
}
