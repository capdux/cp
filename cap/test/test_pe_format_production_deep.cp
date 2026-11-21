// Source: cp/cap/test/test_pe_format_production_deep.cp (Production deep tests for cap:pe_format)

import { test, expect_eq } from "cap:test";
import {
    PEFile,
    DOSHeader,
    PEHeader,
    DataDirectory,
    OptionalHeader64,
    SectionHeader,
    StripMode,
    StripOpts,
    DOS_SIGNATURE,
    PE_SIGNATURE,
    OPTIONAL_HEADER_MAGIC_64,
    IMAGE_FILE_MACHINE_AMD64,
    IMAGE_SCN_CNT_INITIALIZED_DATA,
    IMAGE_SCN_MEM_READ,
    IMAGE_DIRECTORY_ENTRY_SECURITY,
    IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY,
    isPow2,
    alignUpU32,
    alignUpUsize,
    encodeSectionName,
    computePEChecksum,
    isPE,
} from "cap:pe_format";

test("PE64 C-ABI header sizes, alignment helpers and section name encoding") {
    expect_eq(@sizeOf(DOSHeader), @as(usize, 64));
    expect_eq(@sizeOf(PEHeader), @as(usize, 24));
    expect_eq(@sizeOf(DataDirectory), @as(usize, 8));
    expect_eq(@sizeOf(OptionalHeader64), @as(usize, 240));
    expect_eq(@sizeOf(SectionHeader), @as(usize, 40));

    expect_eq(isPow2(0), false);
    expect_eq(isPow2(1), true);
    expect_eq(isPow2(512), true);
    expect_eq(isPow2(4096), true);
    expect_eq(isPow2(3), false);
    expect_eq(isPow2(4097), false);

    expect_eq(alignUpU32(0, 512) catch 999, @as(u32, 0));
    expect_eq(alignUpU32(1, 512) catch 999, @as(u32, 512));
    expect_eq(alignUpU32(512, 512) catch 999, @as(u32, 512));
    expect_eq(alignUpU32(513, 512) catch 999, @as(u32, 1024));
    expect_eq(alignUpUsize(4097, 4096) catch 0, @as(usize, 8192));

    const bad_align = switch (alignUpU32(100, 3)) {
        err.BadAlignment => true,
        else => false,
    };
    expect_eq(bad_align, true);

    const ovf_hit = switch (alignUpU32(0xFFFFFFFF, 4096)) {
        err.Overflow => true,
        else => false,
    };
    expect_eq(ovf_hit, true);

    const empty_name_err = switch (encodeSectionName("")) {
        err.InvalidSectionName => true,
        else => false,
    };
    expect_eq(empty_name_err, true);

    const long_name_err = switch (encodeSectionName(".toolong9")) {
        err.InvalidSectionName => true,
        else => false,
    };
    expect_eq(long_name_err, true);

    const ok_name = encodeSectionName(".text") catch [0; 8];
    expect_eq(std.mem.eql(u8, ok_name[0..5], ".text"), true);
    expect_eq(ok_name[5], @as(u8, 0));
}

test("Minimal PE64 synthesis, parse roundtrip, validation and checksum") {
    const alloc = std.testing.allocator;
    const code: [4]u8 = [0x48, 0x31, 0xC0, 0xC3]; // xor rax, rax; ret

    const pe = PEFile.buildMinimalPE64(alloc, code[0..4]) catch unreachable;
    expect_eq(isPE(pe.bytes()), true);
    expect_eq(pe.sectionCount(), @as(u16, 1));

    const val_ok = switch (pe.validate()) {
        err(_) => false,
        else => true,
    };
    expect_eq(val_ok, true);

    const dos = pe.getDosHeader() catch unreachable;
    expect_eq(dos.e_magic, DOS_SIGNATURE);
    expect_eq(dos.e_lfanew, @as(u32, 0x80));

    const hdr = pe.getPEHeader() catch unreachable;
    expect_eq(hdr.signature, PE_SIGNATURE);
    expect_eq(hdr.machine, IMAGE_FILE_MACHINE_AMD64);
    expect_eq(hdr.number_of_sections, @as(u16, 1));

    const opt = pe.getOptionalHeader() catch unreachable;
    expect_eq(opt.magic, OPTIONAL_HEADER_MAGIC_64);
    expect_eq(opt.checksum != 0, true);
    const recomputed = computePEChecksum(pe.bytes(), 0x80 + 24 + 64);
    expect_eq(opt.checksum, recomputed);

    const reparsed = PEFile.parse(alloc, pe.bytes()) catch unreachable;
    expect_eq(reparsed.sectionCount(), @as(u16, 1));
    const reval_ok = switch (reparsed.validate()) {
        err(_) => false,
        else => true,
    };
    expect_eq(reval_ok, true);
}

test("Section lookup, addSection injection, duplicate guard and data integrity") {
    const alloc = std.testing.allocator;
    const code: [1]u8 = [0xC3];
    let pe = PEFile.buildMinimalPE64(alloc, code[0..1]) catch unreachable;

    expect_eq(pe.findSection(".text") != null, true);
    expect_eq(pe.findSection(".missing") == null, true);

    const missing_err = switch (pe.getSectionRawData(".missing")) {
        err.SectionNotFound => true,
        else => false,
    };
    expect_eq(missing_err, true);

    const payload1 = "CP_EMBEDDED_PAYLOAD_V1";
    const flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
    pe.addSection(".cap", payload1, flags, .none) catch unreachable;
    expect_eq(pe.sectionCount(), @as(u16, 2));

    const payload2 = "READONLY_TABLE_0123456789";
    pe.addSection(".rdata", payload2, flags, .none) catch unreachable;
    expect_eq(pe.sectionCount(), @as(u16, 3));

    const val_ok = switch (pe.validate()) {
        err(_) => false,
        else => true,
    };
    expect_eq(val_ok, true);

    const cap_raw = pe.getSectionRawData(".cap") catch unreachable;
    expect_eq(std.mem.eql(u8, cap_raw[0..payload1.len], payload1), true);
    const rdata_raw = pe.getSectionRawData(".rdata") catch unreachable;
    expect_eq(std.mem.eql(u8, rdata_raw[0..payload2.len], payload2), true);

    const opt = pe.getOptionalHeader() catch unreachable;
    expect_eq(opt.size_of_image, @as(u32, 0x4000));
    expect_eq(opt.checksum, computePEChecksum(pe.bytes(), 0x80 + 24 + 64));

    const dup_err = switch (pe.addSection(".cap", "dup", flags, .none)) {
        err.SectionExists => true,
        else => false,
    };
    expect_eq(dup_err, true);
}

test("Authenticode security directory stripping and checksum recomputation") {
    const alloc = std.testing.allocator;
    const code: [1]u8 = [0xC3];
    const pe = PEFile.buildMinimalPE64(alloc, code[0..1]) catch unreachable;
    const orig_len = pe.bytes().len;

    let signed_buf = std.array_list.Managed(u8).init(alloc);
    defer signed_buf.deinit();
    signed_buf.appendSlice(pe.bytes()) catch unreachable;
    let fake_cert: [32]u8 = [0xAA; 32];
    signed_buf.appendSlice(fake_cert[0..32]) catch unreachable;

    let signed_pe = PEFile.parse(alloc, signed_buf.items) catch unreachable;
    let opt = signed_pe.getOptionalHeader() catch unreachable;
    opt.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].virtual_address = orig_len as u32;
    opt.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].size = 32;
    opt.dll_characteristics |= IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY;
    signed_pe.writeOptionalHeader(opt) catch unreachable;

    signed_pe.stripAuthenticode(StripOpts{ require_overlay: true, recompute_checksum: true }) catch unreachable;
    expect_eq(signed_pe.bytes().len, orig_len);

    const opt_after = signed_pe.getOptionalHeader() catch unreachable;
    expect_eq(opt_after.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].virtual_address, @as(u32, 0));
    expect_eq(opt_after.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].size, @as(u32, 0));
    expect_eq((opt_after.dll_characteristics & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY) == 0, true);
    expect_eq(opt_after.checksum, computePEChecksum(signed_pe.bytes(), 0x80 + 24 + 64));

    const val_ok = switch (signed_pe.validate()) {
        err(_) => false,
        else => true,
    };
    expect_eq(val_ok, true);
}

test("Malformed PE64 signatures, format, alignment and SizeOfImage error guards") {
    const alloc = std.testing.allocator;

    // 1. 空/截断输入 -> InvalidPEFile & isPE == false
    const empty_slice: []const u8 = "";
    expect_eq(isPE(empty_slice), false);
    const e1 = switch (PEFile.parse(alloc, empty_slice)) {
        err.InvalidPEFile => true,
        else => false,
    };
    expect_eq(e1, true);

    const code: [1]u8 = [0xC3];
    const base = PEFile.buildMinimalPE64(alloc, code[0..1]) catch unreachable;
    let buf = alloc.alloc(u8, base.bytes().len) catch unreachable;
    defer alloc.free(buf);

    // 2. 破坏 DOS 签名 -> InvalidDOSSignature
    @memcpy(buf, base.bytes());
    buf[0] = 0x00;
    expect_eq(isPE(buf), false);
    const e2 = switch (PEFile.parse(alloc, buf)) {
        err.InvalidDOSSignature => true,
        else => false,
    };
    expect_eq(e2, true);

    // 3. 破坏 PE 签名 -> InvalidPESignature
    @memcpy(buf, base.bytes());
    buf[0x80] = 0xFF;
    expect_eq(isPE(buf), false);
    const e3 = switch (PEFile.parse(alloc, buf)) {
        err.InvalidPESignature => true,
        else => false,
    };
    expect_eq(e3, true);

    // 4. 修改 OptionalHeader magic 为 PE32 (0x010B) -> UnsupportedPEFormat
    @memcpy(buf, base.bytes());
    const opt_off: usize = 0x80 + 24;
    buf[opt_off] = 0x0B;
    buf[opt_off + 1] = 0x01;
    const e4 = switch (PEFile.parse(alloc, buf)) {
        err.UnsupportedPEFormat => true,
        else => false,
    };
    expect_eq(e4, true);

    // 5. 修改 file_alignment 为非 2 的幂 (300) -> BadAlignment
    @memcpy(buf, base.bytes());
    std.mem.writeInt(u32, buf[opt_off + 36 .. opt_off + 40], 300, .little);
    const e5 = switch (PEFile.parse(alloc, buf)) {
        err.BadAlignment => true,
        else => false,
    };
    expect_eq(e5, true);

    // 6. 修改 number_of_sections 为 97 (> MAX_SECTIONS=96) -> TooManySections
    @memcpy(buf, base.bytes());
    std.mem.writeInt(u16, buf[0x80 + 6 .. 0x80 + 8], 97, .little);
    const e6 = switch (PEFile.parse(alloc, buf)) {
        err.TooManySections => true,
        else => false,
    };
    expect_eq(e6, true);

    // 7. 篡改 SizeOfImage -> validate() 返回 SizeOfImageMismatch
    @memcpy(buf, base.bytes());
    std.mem.writeInt(u32, buf[opt_off + 56 .. opt_off + 60], 0x9000, .little);
    const bad_img = PEFile.parse(alloc, buf) catch unreachable;
    const e7 = switch (bad_img.validate()) {
        err.SizeOfImageMismatch => true,
        else => false,
    };
    expect_eq(e7, true);
}

test("Multi-round RAII lifecycle and parse-mutate-reparse loop") {
    const alloc = std.testing.allocator;
    const code: [2]u8 = [0x90, 0xC3];
    let pe = PEFile.buildMinimalPE64(alloc, code[0..2]) catch unreachable;

    const names: [3]string = [".s1", ".s2", ".s3"];
    const flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
    let i: usize = 0;
    while (i < 3) {
        pe.addSection(names[i], names[i], flags, .none) catch unreachable;
        const snapshot = PEFile.parse(alloc, pe.bytes()) catch unreachable;
        expect_eq(snapshot.sectionCount(), @as(u16, 2 + (i as u16)));
        const ok = switch (snapshot.validate()) {
            err(_) => false,
            else => true,
        };
        expect_eq(ok, true);
        i += 1;
    }

    expect_eq(pe.sectionCount(), @as(u16, 4));
    expect_eq(pe.findSection(".s3") != null, true);
}
