// cap/test/testZigBinFormatsSuite.cp
// Comprehensive tests for cap:coff, cap:macho, cap:pdb, cap:spirv

import { test, expect_eq } from "cap:test";
import {
    DOS_MAGIC,
    PE_POINTER_OFFSET,
    OPT_MAGIC_PE32_PLUS,
    MACHINE_AMD64,
    SUBSYSTEM_WINDOWS_CUI,
    FLAG_EXECUTABLE_IMAGE,
    FLAG_LARGE_ADDRESS_AWARE,
    parsePeHeader,
    isPe,
} from "cap:coff";
import {
    MH_MAGIC_64,
    FAT_MAGIC,
    CPU_TYPE_ARM64,
    CPU_TYPE_X86_64,
    MH_EXECUTE,
    MH_DYLIB,
    parseMachoHeader,
    isMacho,
} from "cap:macho";
import {
    MSF_MAGIC,
    MSF_MAGIC_LEN,
    parsePdbHeader,
    isPdb,
} from "cap:pdb";
import {
    SPIRV_MAGIC,
    OP_CAPABILITY,
    OP_MEMORY_MODEL,
    OP_RETURN,
    SpirvInstIter,
    parseSpirvHeader,
    isSpirv,
} from "cap:spirv";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU64Le(buf: []u8, off: usize, v: u64) {
    putU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn main() {
    test("1. coff: valid PE32+ (AMD64) header, optional header & section table", () => {
        // DOS (64B) + PE sig (4B) + COFF (20B) + Optional PE32+ (112B) + 2 sections (80B) = 280B
        let buf: [280]u8 = [0; 280];
        putU16Le(buf[0..], 0, DOS_MAGIC);
        putU32Le(buf[0..], PE_POINTER_OFFSET, 64);

        // PE\0\0 signature at 64
        buf[64] = 0x50;
        buf[65] = 0x45;
        buf[66] = 0x00;
        buf[67] = 0x00;

        // COFF header at 68
        putU16Le(buf[0..], 68, MACHINE_AMD64);
        putU16Le(buf[0..], 70, 2);
        putU32Le(buf[0..], 72, 0x65000001);
        putU16Le(buf[0..], 84, 112);
        putU16Le(buf[0..], 86, FLAG_EXECUTABLE_IMAGE | FLAG_LARGE_ADDRESS_AWARE);

        // PE32+ Optional header at 88
        putU16Le(buf[0..], 88, OPT_MAGIC_PE32_PLUS);
        putU32Le(buf[0..], 88 + 16, 0x1000);
        putU64Le(buf[0..], 88 + 24, 0x0000000140000000);
        putU32Le(buf[0..], 88 + 32, 0x1000);
        putU32Le(buf[0..], 88 + 36, 0x0200);
        putU32Le(buf[0..], 88 + 56, 0x5000);
        putU32Le(buf[0..], 88 + 60, 0x0400);
        putU16Le(buf[0..], 88 + 68, SUBSYSTEM_WINDOWS_CUI);
        putU16Le(buf[0..], 88 + 70, 0x8160);

        // Section 0: ".text" at offset 200
        buf[200] = 0x2E; buf[201] = 0x74; buf[202] = 0x65; buf[203] = 0x78; buf[204] = 0x74;
        putU32Le(buf[0..], 200 + 8, 0x0800);
        putU32Le(buf[0..], 200 + 12, 0x1000);
        putU32Le(buf[0..], 200 + 16, 0x0800);
        putU32Le(buf[0..], 200 + 20, 0x0400);
        putU32Le(buf[0..], 200 + 36, 0x60000020);

        // Section 1: ".rdata" at offset 240
        buf[240] = 0x2E; buf[241] = 0x72; buf[242] = 0x64; buf[243] = 0x61; buf[244] = 0x74; buf[245] = 0x61;
        putU32Le(buf[0..], 240 + 8, 0x0200);
        putU32Le(buf[0..], 240 + 12, 0x2000);
        putU32Le(buf[0..], 240 + 36, 0x40000040);

        const info = parsePeHeader(buf[0..]) catch unreachable;
        expect_eq(info.pe_offset, 64 as u32);
        expect_eq(info.coff.machine, MACHINE_AMD64);
        expect_eq(info.coff.isExecutable(), true);
        expect_eq(info.coff.isDll(), false);
        expect_eq(info.is_pe32_plus, true);
        expect_eq(info.entry_point_rva, 0x1000 as u32);
        expect_eq(info.image_base, 0x0000000140000000 as u64);
        expect_eq(info.subsystem, SUBSYSTEM_WINDOWS_CUI);
        expect_eq(info.sectionCount(), 2 as usize);
        expect_eq(isPe(buf[0..]), true);

        const sec0 = info.parseSection(buf[0..], 0) catch unreachable;
        expect_eq(sec0.nameEquals(".text"), true);
        expect_eq(sec0.virtual_address, 0x1000 as u32);
        expect_eq(sec0.isCode(), true);
        expect_eq(sec0.isExecutable(), true);
        expect_eq(sec0.isReadable(), true);
        expect_eq(sec0.isWritable(), false);

        const sec1 = info.parseSection(buf[0..], 1) catch unreachable;
        expect_eq(sec1.nameEquals(".rdata"), true);
        expect_eq(sec1.virtual_address, 0x2000 as u32);
        expect_eq(sec1.isExecutable(), false);

        let oob_err = false;
        * = info.parseSection(buf[0..], 2) catch {
            oob_err = true;
        };
        expect_eq(oob_err, true);
    });

    test("2. coff: reject malformed DOS magic, PE signature & OOB offsets", () => {
        let short_buf: [16]u8 = [0; 16];
        expect_eq(isPe(short_buf[0..]), false);

        let bad_dos: [128]u8 = [0; 128];
        putU16Le(bad_dos[0..], 0, 0x1234);
        putU32Le(bad_dos[0..], PE_POINTER_OFFSET, 64);
        expect_eq(isPe(bad_dos[0..]), false);

        let bad_pe: [128]u8 = [0; 128];
        putU16Le(bad_pe[0..], 0, DOS_MAGIC);
        putU32Le(bad_pe[0..], PE_POINTER_OFFSET, 64);
        bad_pe[64] = 0x50; bad_pe[65] = 0x58;
        expect_eq(isPe(bad_pe[0..]), false);

        let oob_pe: [128]u8 = [0; 128];
        putU16Le(oob_pe[0..], 0, DOS_MAGIC);
        putU32Le(oob_pe[0..], PE_POINTER_OFFSET, 4096);
        expect_eq(isPe(oob_pe[0..]), false);
    });

    test("3. macho: Mach-O 64-bit ARM64 & x86_64 header parsing & truncation guard", () => {
        let arm64_buf: [32]u8 = [0; 32];
        putU32Le(arm64_buf[0..], 0, MH_MAGIC_64);
        putU32Le(arm64_buf[0..], 4, CPU_TYPE_ARM64);
        putU32Le(arm64_buf[0..], 8, 0);
        putU32Le(arm64_buf[0..], 12, MH_EXECUTE);
        putU32Le(arm64_buf[0..], 16, 18);
        putU32Le(arm64_buf[0..], 20, 1520);
        putU32Le(arm64_buf[0..], 24, 0x00200085);

        const info_arm = parseMachoHeader(arm64_buf[0..]) catch unreachable;
        expect_eq(info_arm.is_64bit, true);
        expect_eq(info_arm.is_fat, false);
        expect_eq(info_arm.is_big_endian, false);
        expect_eq(info_arm.cpu_type, CPU_TYPE_ARM64);
        expect_eq(info_arm.file_type, MH_EXECUTE);
        expect_eq(info_arm.ncmds, 18 as u32);
        expect_eq(info_arm.sizeofcmds, 1520 as u32);

        let x64_buf: [32]u8 = [0; 32];
        putU32Le(x64_buf[0..], 0, MH_MAGIC_64);
        putU32Le(x64_buf[0..], 4, CPU_TYPE_X86_64);
        putU32Le(x64_buf[0..], 8, 3);
        putU32Le(x64_buf[0..], 12, MH_DYLIB);
        putU32Le(x64_buf[0..], 16, 12);
        putU32Le(x64_buf[0..], 20, 960);

        const info_x64 = parseMachoHeader(x64_buf[0..]) catch unreachable;
        expect_eq(info_x64.cpu_type, CPU_TYPE_X86_64);
        expect_eq(info_x64.file_type, MH_DYLIB);
        expect_eq(isMacho(x64_buf[0..]), true);

        expect_eq(isMacho(arm64_buf[0..28]), false);
    });

    test("4. macho: Universal Fat binary (ARM64 + x86_64) & malformed magic rejection", () => {
        let fat_buf: [48]u8 = [0; 48];
        putU32Be(fat_buf[0..], 0, FAT_MAGIC);
        putU32Be(fat_buf[0..], 4, 2);

        putU32Be(fat_buf[0..], 8, CPU_TYPE_X86_64);
        putU32Be(fat_buf[0..], 12, 3);
        putU32Be(fat_buf[0..], 16, 0x4000);
        putU32Be(fat_buf[0..], 20, 0x12000);
        putU32Be(fat_buf[0..], 24, 14);

        putU32Be(fat_buf[0..], 28, CPU_TYPE_ARM64);
        putU32Be(fat_buf[0..], 32, 0);
        putU32Be(fat_buf[0..], 36, 0x18000);
        putU32Be(fat_buf[0..], 40, 0x10000);
        putU32Be(fat_buf[0..], 44, 14);

        const fat_info = parseMachoHeader(fat_buf[0..]) catch unreachable;
        expect_eq(fat_info.is_fat, true);
        expect_eq(fat_info.is_big_endian, true);
        expect_eq(fat_info.nfat_arch, 2 as u32);

        const a0 = fat_info.parseFatArch(fat_buf[0..], 0) catch unreachable;
        expect_eq(a0.cpu_type, CPU_TYPE_X86_64);
        expect_eq(a0.offset, 0x4000 as u64);
        expect_eq(a0.size, 0x12000 as u64);

        const a1 = fat_info.parseFatArch(fat_buf[0..], 1) catch unreachable;
        expect_eq(a1.cpu_type, CPU_TYPE_ARM64);
        expect_eq(a1.offset, 0x18000 as u64);
        expect_eq(a1.alignment, 14 as u32);

        let bad: [32]u8 = [0; 32];
        putU32Le(bad[0..], 0, 0xDEADBEEF);
        expect_eq(isMacho(bad[0..]), false);
        expect_eq(isMacho(fat_buf[0..2]), false);
    });

    test("5. pdb: MSF 7.0 superblock metadata parsing & malformed guards", () => {
        let buf: [64]u8 = [0; 64];
        let idx: usize = 0;
        while (idx < MSF_MAGIC_LEN) : (idx += 1) {
            buf[idx] = MSF_MAGIC[idx];
        }
        putU32Le(buf[0..], 32, 4096);
        putU32Le(buf[0..], 36, 2);
        putU32Le(buf[0..], 40, 64);
        putU32Le(buf[0..], 44, 5000);
        putU32Le(buf[0..], 48, 0);
        putU32Le(buf[0..], 52, 10);

        const sb = parsePdbHeader(buf[0..]) catch unreachable;
        expect_eq(sb.block_size, 4096 as u32);
        expect_eq(sb.free_block_map_block, 2 as u32);
        expect_eq(sb.num_blocks, 64 as u32);
        expect_eq(sb.num_directory_bytes, 5000 as u32);
        expect_eq(sb.block_map_addr, 10 as u32);
        expect_eq(sb.expectedFileSize(), (4096 * 64) as u64);
        expect_eq(sb.directoryBlockCount(), 2 as u32);
        expect_eq(sb.blockMapOffset(), 40960 as u64);
        expect_eq(isPdb(buf[0..]), true);

        let bad_bs: [64]u8 = buf;
        putU32Le(bad_bs[0..], 32, 1234);
        expect_eq(isPdb(bad_bs[0..]), false);

        let bad_fbm: [64]u8 = buf;
        putU32Le(bad_fbm[0..], 36, 3);
        expect_eq(isPdb(bad_fbm[0..]), false);

        let bad_addr: [64]u8 = buf;
        putU32Le(bad_addr[0..], 52, 64);
        expect_eq(isPdb(bad_addr[0..]), false);

        expect_eq(isPdb(buf[0..32]), false);
    });

    test("6. spirv: SPIR-V 1.6 header, instruction stream iterator & malformed guards", () => {
        let buf: [44]u8 = [0; 44];
        putU32Le(buf[0..], 0, SPIRV_MAGIC);
        putU32Le(buf[0..], 4, 0x00010600);
        putU32Le(buf[0..], 8, 0x0008000B);
        putU32Le(buf[0..], 12, 42);
        putU32Le(buf[0..], 16, 0);

        // Inst 0: OpCapability Shader
        putU32Le(buf[0..], 20, (2 << 16) | (OP_CAPABILITY as u32));
        putU32Le(buf[0..], 24, 1);

        // Inst 1: OpMemoryModel Logical GLSL450
        putU32Le(buf[0..], 28, (3 << 16) | (OP_MEMORY_MODEL as u32));
        putU32Le(buf[0..], 32, 0);
        putU32Le(buf[0..], 36, 1);

        // Inst 2: OpReturn
        putU32Le(buf[0..], 40, (1 << 16) | (OP_RETURN as u32));

        const hdr = parseSpirvHeader(buf[0..]) catch unreachable;
        expect_eq(hdr.magic, SPIRV_MAGIC);
        expect_eq(hdr.version_major, 1 as u8);
        expect_eq(hdr.version_minor, 6 as u8);
        expect_eq(hdr.generatorId(), 8 as u16);
        expect_eq(hdr.generatorVersion(), 11 as u16);
        expect_eq(hdr.bound, 42 as u32);
        expect_eq(hdr.is_big_endian, false);
        expect_eq(hdr.total_words, 11 as usize);
        expect_eq(isSpirv(buf[0..]), true);

        let iter = SpirvInstIter.init(buf[0..], hdr);
        expect_eq(iter.hasNext(), true);

        const inst0 = iter.next() catch unreachable;
        expect_eq(inst0.opcode, OP_CAPABILITY);
        expect_eq(inst0.word_count, 2 as u16);
        expect_eq(inst0.operand_count, 1 as usize);
        expect_eq(inst0.readOperand(buf[0..], hdr.is_big_endian, 0) catch unreachable, 1 as u32);

        const inst1 = iter.next() catch unreachable;
        expect_eq(inst1.opcode, OP_MEMORY_MODEL);
        expect_eq(inst1.word_count, 3 as u16);
        expect_eq(inst1.readOperand(buf[0..], hdr.is_big_endian, 0) catch unreachable, 0 as u32);
        expect_eq(inst1.readOperand(buf[0..], hdr.is_big_endian, 1) catch unreachable, 1 as u32);

        const inst2 = iter.next() catch unreachable;
        expect_eq(inst2.opcode, OP_RETURN);
        expect_eq(inst2.word_count, 1 as u16);
        expect_eq(inst2.operand_count, 0 as usize);
        expect_eq(iter.hasNext(), false);

        expect_eq(isSpirv(buf[0..42]), false);
        expect_eq(isSpirv(buf[0..16]), false);
        let bad_magic: [20]u8 = [0; 20];
        putU32Le(bad_magic[0..], 0, 0x11223344);
        expect_eq(isSpirv(bad_magic[0..]), false);

        let bad_inst: [24]u8 = [0; 24];
        putU32Le(bad_inst[0..], 0, SPIRV_MAGIC);
        putU32Le(bad_inst[0..], 4, 0x00010600);
        putU32Le(bad_inst[0..], 20, 0);
        const bad_hdr = parseSpirvHeader(bad_inst[0..]) catch unreachable;
        let bad_iter = SpirvInstIter.init(bad_inst[0..], bad_hdr);
        let inst_err = false;
        * = bad_iter.next() catch {
            inst_err = true;
        };
        expect_eq(inst_err, true);
    });
}
