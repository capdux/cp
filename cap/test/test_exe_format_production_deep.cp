// Source: cp/cap/test/test_exe_format_production_deep.cp
import std from "std";
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    ExeKind,
    ExeArch,
    detectExeKind,
    parseElf64Header,
    listElf64Sections,
    findElf64Section,
    parsePeHeader,
    parseMachO64Header,
    appendStandaloneTrailer,
    extractStandaloneTrailer,
} from "cap:binfmt";

fn putU16Le(buf: []u8, off: usize, val: u16) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, val: u32) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
    buf[off + 2] = ((val >> 16) & 0xFF) as u8;
    buf[off + 3] = ((val >> 24) & 0xFF) as u8;
}

fn putU64Le(buf: []u8, off: usize, val: u64) {
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        buf[off + i] = ((val >> ((i * 8) as u6)) & 0xFF) as u8;
    }
}

test("exe_format: magic detection for 5 formats and malformed header bounds") {
    // 1. ELF64 & ELF32
    let elf64_buf: [64]u8 = [0; 64];
    elf64_buf[0] = 0x7F;
    elf64_buf[1] = 'E';
    elf64_buf[2] = 'L';
    elf64_buf[3] = 'F';
    elf64_buf[4] = 2;
    elf64_buf[5] = 1;
    expect_eq(detectExeKind(elf64_buf[0..]), ExeKind.Elf64);

    let elf32_buf: [8]u8 = [0x7F, 'E', 'L', 'F', 1, 1, 1, 0];
    expect_eq(detectExeKind(elf32_buf[0..]), ExeKind.Elf32);

    // 2. Mach-O 64 & Fat
    let macho_buf: [32]u8 = [0; 32];
    putU32Le(macho_buf[0..], 0, 0xFEEDFACF);
    expect_eq(detectExeKind(macho_buf[0..]), ExeKind.MachO64);

    let fat_buf: [8]u8 = [0xCA, 0xFE, 0xBA, 0xBE, 0, 0, 0, 2];
    expect_eq(detectExeKind(fat_buf[0..]), ExeKind.MachOFat);

    // 3. PE32+ & PE32
    let pe_buf: [128]u8 = [0; 128];
    pe_buf[0] = 'M';
    pe_buf[1] = 'Z';
    putU32Le(pe_buf[0..], 60, 64);
    putU32Le(pe_buf[0..], 64, 0x00004550);
    putU16Le(pe_buf[0..], 88, 0x020B);
    expect_eq(detectExeKind(pe_buf[0..]), ExeKind.Pe32Plus);

    putU16Le(pe_buf[0..], 88, 0x010B);
    expect_eq(detectExeKind(pe_buf[0..]), ExeKind.Pe32);

    // 4. Wasm & Shebang
    let wasm_buf: [8]u8 = [0x00, 'a', 's', 'm', 1, 0, 0, 0];
    expect_eq(detectExeKind(wasm_buf[0..]), ExeKind.Wasm);

    const shebang = "#!/usr/bin/env cp\n";
    expect_eq(detectExeKind(shebang), ExeKind.Shebang);

    // 5. Truncated / malformed headers must be safely rejected
    const tiny = "MZ";
    expect_eq(detectExeKind(tiny), ExeKind.Unknown);

    let bad_elf: [16]u8 = [0; 16];
    bad_elf[0] = 0x7F;
    bad_elf[1] = 'E';
    bad_elf[2] = 'L';
    bad_elf[3] = 'F';
    let elf_err = false;
    * = parseElf64Header(bad_elf[0..]) catch { elf_err = true; };
    expect_eq(elf_err, true);

    // Malformed ELF64 with out-of-bounds e_shoff
    putU16Le(elf64_buf[0..], 58, 64);
    putU16Le(elf64_buf[0..], 60, 10);
    putU64Le(elf64_buf[0..], 40, 9999);
    let oob_err = false;
    * = parseElf64Header(elf64_buf[0..]) catch { oob_err = true; };
    expect_eq(oob_err, true);
}

test("exe_format: synthetic ELF64 section table, shstrtab and findElf64Section") {
    const alloc = std.testing.allocator;
    // Layout:
    // [0..64): Elf64_Ehdr
    // [64..80): .shstrtab bytes ("\0.shstrtab\0.cp_data\0")
    // [80..96): .cp_data payload ("CP_HELLO_SECTION")
    // [96..288): 3 Elf64_Shdr entries (0: null, 1: .shstrtab, 2: .cp_data)
    let bin: [288]u8 = [0; 288];
    bin[0] = 0x7F;
    bin[1] = 'E';
    bin[2] = 'L';
    bin[3] = 'F';
    bin[4] = 2; // ELFCLASS64
    bin[5] = 1; // ELFDATA2LSB
    putU16Le(bin[0..], 16, 2); // ET_EXEC
    putU16Le(bin[0..], 18, 62); // EM_X86_64
    putU64Le(bin[0..], 24, 0x401000); // e_entry
    putU64Le(bin[0..], 40, 96); // e_shoff
    putU16Le(bin[0..], 58, 64); // e_shentsize
    putU16Le(bin[0..], 60, 3);  // e_shnum
    putU16Le(bin[0..], 62, 1);  // e_shstrndx

    const shstrtab = "\x00.shstrtab\x00.cp_data";
    // Note: shstrtab length is 19 bytes; fits in [64..84), let's place .cp_data at 84..100?
    // Wait: section table is at 96..288! Let's use exact offsets:
    // Let's put shstrtab at 64..80? 1 + 9 + 1 + 8 + 1 = 20 bytes -> too big for 16 bytes.
    // Let's keep names short or check: "\x00.str\x00.cp\x00" = 10 bytes!
    // Even better, let's just use a 320-byte buffer!
    let elf: [320]u8 = [0; 320];
    @memcpy(elf[0..64], bin[0..64]);
    putU64Le(elf[0..], 40, 128); // e_shoff = 128

    const str_bytes = "\x00.shstrtab\x00.cp_data\x00";
    @memcpy(elf[64 .. 64 + str_bytes.len], str_bytes);

    const sec_payload = "CP_ZERO_COPY";
    @memcpy(elf[96 .. 96 + sec_payload.len], sec_payload);

    // Shdr 1 (.shstrtab) at 128 + 64 = 192
    putU32Le(elf[0..], 192, 1); // sh_name = 1 (".shstrtab")
    putU32Le(elf[0..], 196, 3); // SHT_STRTAB
    putU64Le(elf[0..], 216, 64); // sh_offset
    putU64Le(elf[0..], 224, str_bytes.len as u64); // sh_size

    // Shdr 2 (.cp_data) at 128 + 128 = 256
    putU32Le(elf[0..], 256, 11); // sh_name = 11 (".cp_data")
    putU32Le(elf[0..], 260, 1); // SHT_PROGBITS
    putU64Le(elf[0..], 280, 96); // sh_offset
    putU64Le(elf[0..], 288, sec_payload.len as u64); // sh_size

    const hdr = parseElf64Header(elf[0..])?;
    expect_eq(hdr.arch, ExeArch.X86_64);
    expect_eq(hdr.e_shnum, 3);

    const sections = listElf64Sections(alloc, elf[0..])?;
    defer alloc.free(sections);
    expect_eq(sections.len, 3);
    expect_eq(std.mem.eql(u8, sections[1].name, ".shstrtab"), true);
    expect_eq(std.mem.eql(u8, sections[2].name, ".cp_data"), true);
    expect_eq(std.mem.eql(u8, sections[2].data, sec_payload), true);

    const found = findElf64Section(elf[0..], ".cp_data")?;
    expect_eq(found != null, true);
    expect_eq(std.mem.eql(u8, found.?.data, sec_payload), true);

    const missing = findElf64Section(elf[0..], ".missing")?;
    expect_eq(missing == null, true);
}

test("exe_format: synthetic PE32+ and Mach-O 64 header & architecture") {
    // Synthetic PE32+ (ARM64)
    let pe: [256]u8 = [0; 256];
    pe[0] = 'M';
    pe[1] = 'Z';
    putU32Le(pe[0..], 60, 64); // e_lfanew = 64
    putU32Le(pe[0..], 64, 0x00004550); // PE\0\0
    putU16Le(pe[0..], 68, 0xAA64); // IMAGE_FILE_MACHINE_ARM64
    putU16Le(pe[0..], 70, 2); // num_sections
    putU16Le(pe[0..], 84, 64); // size_of_optional_header
    // OptionalHeader64 at 88
    putU16Le(pe[0..], 88, 0x020B); // PE32+
    putU32Le(pe[0..], 88 + 16, 0x1000); // entry_point_rva
    putU64Le(pe[0..], 88 + 24, 0x140000000); // image_base

    const pe_info = parsePeHeader(pe[0..])?;
    expect_eq(pe_info.is_pe32_plus, true);
    expect_eq(pe_info.arch, ExeArch.Aarch64);
    expect_eq(pe_info.num_sections, 2);
    expect_eq(pe_info.entry_point_rva, 0x1000);
    expect_eq(pe_info.image_base, 0x140000000);

    // Synthetic Mach-O 64 (x86_64)
    let macho: [64]u8 = [0; 64];
    putU32Le(macho[0..], 0, 0xFEEDFACF);
    putU32Le(macho[0..], 4, 0x01000007); // CPU_TYPE_X86_64
    putU32Le(macho[0..], 12, 2); // MH_EXECUTE
    putU32Le(macho[0..], 16, 2); // ncmds
    putU32Le(macho[0..], 20, 32); // sizeofcmds

    const mo_info = parseMachO64Header(macho[0..])?;
    expect_eq(mo_info.arch, ExeArch.X86_64);
    expect_eq(mo_info.ncmds, 2);
    expect_eq(mo_info.sizeofcmds, 32);
}

test("exe_format: StandaloneTrailer append, zero-copy extract & tamper interception") {
    const alloc = std.testing.allocator;
    const base_exe = "\x7FELF\x02\x01\x01\x00DUMMY_EXE";
    const payload = "{\"entry\":\"main.cp\",\"version\":1}";
    const magic: [8]u8 = ['C', 'P', 'B', 'U', 'N', 'D', 'L', 'E'];

    let bundled = appendStandaloneTrailer(alloc, base_exe, payload, magic)?;
    defer alloc.free(bundled);

    const trailer = extractStandaloneTrailer(bundled, magic)?;
    expect_eq(trailer.payload_len, payload.len as u64);
    expect_eq(std.mem.eql(u8, trailer.payload, payload), true);

    // Tamper 1 byte inside payload and verify ChecksumMismatch
    bundled[base_exe.len + 2] ^= 0x01;
    let tampered = false;
    * = extractStandaloneTrailer(bundled, magic) catch { tampered = true; };
    expect_eq(tampered, true);
}

test("exe_format: cap.exe_format dynamic mount & zero leak") {
    const alloc = std.testing.allocator;
    const wasm_hdr: [8]u8 = [0x00, 'a', 's', 'm', 1, 0, 0, 0];
    expect_eq(cap.binfmt.detectExeKind(wasm_hdr[0..]), ExeKind.Wasm);

    const magic: [8]u8 = ['C', 'P', 'T', 'E', 'S', 'T', '0', '1'];
    const bundled = cap.binfmt.appendStandaloneTrailer(alloc, wasm_hdr[0..], "PAYLOAD", magic)?;
    defer alloc.free(bundled);

    const extracted = cap.binfmt.extractStandaloneTrailer(bundled, magic)?;
    expect_eq(std.mem.eql(u8, extracted.payload, "PAYLOAD"), true);
}
