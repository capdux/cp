// Source: cp/cap/test/test_dwarf_production_deep.cp (Deep production tests for cap:dwarf)

const std = @import("std");
const cap = @import("cap");
import { test, expect_eq } from "cap:test";
import {
    DwarfError,
    DwTag,
    DwAt,
    DwForm,
    writeUleb128,
    writeU16,
    writeU32,
    writeU64,
    FileEntry,
    LineEncodeEntry,
    parseDebugLine,
    encodeDebugLineV4,
    lookupAddress,
    parseDebugInfoCu,
    AttrValueKind,
} from "cap:dwarf";

test("dwarf debug_line v4 encode and state machine decode roundtrip") {
    const alloc = std.testing.allocator;
    const dirs: [2]string = [ "src", "include" ];
    const files: [2]FileEntry = [
        FileEntry{ .name = "main.cp", .dir_index = 1, .mod_time = 0, .length = 120 },
        FileEntry{ .name = "util.cp", .dir_index = 2, .mod_time = 0, .length = 80 },
    ];
    const entries: [4]LineEncodeEntry = [
        LineEncodeEntry{ .address = 0x1000, .file_index = 1, .line = 10, .column = 4, .is_stmt = true, .end_sequence = false },
        LineEncodeEntry{ .address = 0x1004, .file_index = 1, .line = 12, .column = 8, .is_stmt = true, .end_sequence = false },
        LineEncodeEntry{ .address = 0x1020, .file_index = 2, .line = 45, .column = 2, .is_stmt = false, .end_sequence = false },
        LineEncodeEntry{ .address = 0x1030, .file_index = 2, .line = 45, .column = 2, .is_stmt = false, .end_sequence = true },
    ];

    const encoded = try encodeDebugLineV4(alloc, &dirs, &files, &entries, 8);
    defer alloc.free(encoded);

    let table = try parseDebugLine(alloc, encoded, 8);
    defer table.deinit(alloc);

    expect_eq(@as(u16, 4), table.header.version);
    expect_eq(@as(usize, 2), table.header.include_directories.len);
    expect_eq(@as(usize, 2), table.header.file_names.len);
    expect_eq(@as(usize, 4), table.rows.len);

    expect_eq(@as(u64, 0x1000), table.rows[0].address);
    expect_eq(@as(i64, 10), table.rows[0].line);
    expect_eq(@as(u32, 4), table.rows[0].column);
    expect_eq(true, table.rows[0].is_stmt);
    expect_eq(false, table.rows[0].end_sequence);

    expect_eq(@as(u64, 0x1004), table.rows[1].address);
    expect_eq(@as(i64, 12), table.rows[1].line);

    expect_eq(@as(u64, 0x1020), table.rows[2].address);
    expect_eq(@as(u32, 2), table.rows[2].file_index);
    expect_eq(@as(i64, 45), table.rows[2].line);
    expect_eq(false, table.rows[2].is_stmt);

    expect_eq(true, table.rows[3].end_sequence);
}

test("dwarf lookupAddress exact and in-range binary search") {
    const alloc = std.testing.allocator;
    const dirs: [1]string = ["lib"];
    const files: [1]FileEntry = [
        FileEntry{ .name = "engine.cp", .dir_index = 1, .mod_time = 0, .length = 200 },
    ];
    const entries: [4]LineEncodeEntry = [
        LineEncodeEntry{ .address = 0x4000, .file_index = 1, .line = 100, .column = 1, .is_stmt = true, .end_sequence = false },
        LineEncodeEntry{ .address = 0x4008, .file_index = 1, .line = 105, .column = 5, .is_stmt = true, .end_sequence = false },
        LineEncodeEntry{ .address = 0x4014, .file_index = 1, .line = 110, .column = 9, .is_stmt = true, .end_sequence = false },
        LineEncodeEntry{ .address = 0x4020, .file_index = 1, .line = 110, .column = 9, .is_stmt = true, .end_sequence = true },
    ];

    const encoded = try encodeDebugLineV4(alloc, &dirs, &files, &entries, 8);
    defer alloc.free(encoded);

    let table = try parseDebugLine(alloc, encoded, 8);
    defer table.deinit(alloc);

    // Before first address
    expect_eq(true, lookupAddress(&table, 0x3fff) == null);

    // Exact match
    const loc1 = lookupAddress(&table, 0x4000).?;
    expect_eq(true, std.mem.eql(u8, loc1.file, "engine.cp"));
    expect_eq(true, std.mem.eql(u8, loc1.directory, "lib"));
    expect_eq(@as(i64, 100), loc1.line);
    expect_eq(@as(u32, 1), loc1.column);

    // In-range between 0x4008 and 0x4014
    const loc2 = table.lookupAddress(0x400c).?;
    expect_eq(@as(i64, 105), loc2.line);
    expect_eq(@as(u32, 5), loc2.column);

    // Past end_sequence
    expect_eq(true, lookupAddress(&table, 0x4025) == null);
}

test("dwarf debug_abbrev and debug_info compile unit DIE parsing") {
    const alloc = std.testing.allocator;

    // Build .debug_str
    const debug_str = "cp-compiler\x00main_fn\x00";

    let abbrev = std.ArrayList(u8).empty;
    defer abbrev.deinit(alloc);

    try writeUleb128(alloc, &abbrev, 1);
    try writeUleb128(alloc, &abbrev, DwTag.compile_unit);
    try abbrev.append(alloc, 1);
    try writeUleb128(alloc, &abbrev, DwAt.producer);
    try writeUleb128(alloc, &abbrev, DwForm.strp);
    try writeUleb128(alloc, &abbrev, DwAt.name);
    try writeUleb128(alloc, &abbrev, DwForm.string);
    try writeUleb128(alloc, &abbrev, DwAt.low_pc);
    try writeUleb128(alloc, &abbrev, DwForm.addr);
    try writeUleb128(alloc, &abbrev, 0);
    try writeUleb128(alloc, &abbrev, 0);

    try writeUleb128(alloc, &abbrev, 2);
    try writeUleb128(alloc, &abbrev, DwTag.subprogram);
    try abbrev.append(alloc, 0);
    try writeUleb128(alloc, &abbrev, DwAt.name);
    try writeUleb128(alloc, &abbrev, DwForm.strp);
    try writeUleb128(alloc, &abbrev, DwAt.decl_line);
    try writeUleb128(alloc, &abbrev, DwForm.data1);
    try writeUleb128(alloc, &abbrev, DwAt.external);
    try writeUleb128(alloc, &abbrev, DwForm.flag_present);
    try writeUleb128(alloc, &abbrev, 0);
    try writeUleb128(alloc, &abbrev, 0);

    try writeUleb128(alloc, &abbrev, 0); // end of abbrev table

    let body = std.ArrayList(u8).empty;
    defer body.deinit(alloc);

    try writeU16(alloc, &body, 4); // version = 4
    try writeU32(alloc, &body, 0); // debug_abbrev_offset = 0
    try body.append(alloc, 8); // address_size = 8

    // DIE 1 (CU)
    try writeUleb128(alloc, &body, 1);
    try writeU32(alloc, &body, 0); // producer strp -> "cp-compiler"
    try body.appendSlice(alloc, "root.cp\x00"); // name string
    try writeU64(alloc, &body, 0x20000); // low_pc addr

    // DIE 2 (subprogram child)
    try writeUleb128(alloc, &body, 2);
    try writeU32(alloc, &body, 12); // name strp -> "main_fn"
    try body.append(alloc, 42); // decl_line data1 = 42

    // End of children for CU
    try writeUleb128(alloc, &body, 0);

    let info = std.ArrayList(u8).empty;
    defer info.deinit(alloc);
    try writeU32(alloc, &info, @intCast(body.items.len));
    try info.appendSlice(alloc, body.items);

    let cu = try parseDebugInfoCu(alloc, info.items, abbrev.items, debug_str, 0);
    defer cu.deinit(alloc);

    expect_eq(@as(u16, 4), cu.header.version);
    expect_eq(@as(u8, 8), cu.header.address_size);
    expect_eq(@as(usize, 2), cu.nodes.len);

    const cu_node = cu.nodes[0];
    expect_eq(DwTag.compile_unit, cu_node.tag);
    expect_eq(@as(u32, 0), cu_node.depth);
    const prod = cu_node.getAttr(DwAt.producer).?;
    expect_eq(AttrValueKind.str, prod.kind);
    expect_eq(true, std.mem.eql(u8, prod.str_val, "cp-compiler"));
    const cu_name = cu_node.getAttr(DwAt.name).?;
    expect_eq(true, std.mem.eql(u8, cu_name.str_val, "root.cp"));
    const low_pc = cu_node.getAttr(DwAt.low_pc).?;
    expect_eq(@as(u64, 0x20000), low_pc.uint_val);

    const fn_node = cu.nodes[1];
    expect_eq(DwTag.subprogram, fn_node.tag);
    expect_eq(@as(u32, 1), fn_node.depth);
    expect_eq(true, std.mem.eql(u8, fn_node.getAttr(DwAt.name).?.str_val, "main_fn"));
    expect_eq(@as(u64, 42), fn_node.getAttr(DwAt.decl_line).?.uint_val);
    expect_eq(true, fn_node.getAttr(DwAt.external).?.flag_val);
}

test("dwarf error guards: unsupported version, truncated length, invalid file_index, missing abbrev") {
    const alloc = std.testing.allocator;

    // 1. Truncated unit_length
    const trunc: [4]u8 = [ 0x20, 0x00, 0x00, 0x00 ];
    try std.testing.expectError(DwarfError.TruncatedData, parseDebugLine(alloc, &trunc, 8));

    // 2. Unsupported DWARF version (v2)
    const bad_ver: [10]u8 = [ 6, 0, 0, 0, 2, 0, 0, 0, 0, 0 ];
    try std.testing.expectError(DwarfError.UnsupportedVersion, parseDebugLine(alloc, &bad_ver, 8));

    // 3. Invalid file_index (file_index = 5 when only 1 file exists)
    const dirs: [1]string = ["src"];
    const files: [1]FileEntry = [
        FileEntry{ .name = "a.cp", .dir_index = 1, .mod_time = 0, .length = 10 },
    ];
    const bad_entries: [1]LineEncodeEntry = [
        LineEncodeEntry{ .address = 0x100, .file_index = 5, .line = 1, .column = 0, .is_stmt = true, .end_sequence = false },
    ];
    const bad_line_buf = try encodeDebugLineV4(alloc, &dirs, &files, &bad_entries, 8);
    defer alloc.free(bad_line_buf);
    try std.testing.expectError(DwarfError.InvalidFileIndex, parseDebugLine(alloc, bad_line_buf, 8));

    // 4. Missing abbrev code
    const empty_abbrev: [1]u8 = [0];
    const missing_info: [12]u8 = [ 8, 0, 0, 0, 4, 0, 0, 0, 0, 0, 8, 99 ];
    try std.testing.expectError(DwarfError.MissingAbbrev, parseDebugInfoCu(alloc, &missing_info, &empty_abbrev, "", 0));
}

test("dwarf cap.dwarf dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;
    const dirs: [1]string = ["app"];
    const files: [1]FileEntry = [
        FileEntry{ .name = "boot.cp", .dir_index = 1, .mod_time = 0, .length = 42 },
    ];
    const entries: [2]LineEncodeEntry = [
        LineEncodeEntry{ .address = 0x8000, .file_index = 1, .line = 7, .column = 3, .is_stmt = true, .end_sequence = false },
        LineEncodeEntry{ .address = 0x8010, .file_index = 1, .line = 7, .column = 3, .is_stmt = true, .end_sequence = true },
    ];

    const buf = try cap.dwarf.encodeDebugLineV4(alloc, &dirs, &files, &entries, 8);
    defer alloc.free(buf);

    let table = try cap.dwarf.parseDebugLine(alloc, buf, 8);
    defer table.deinit(alloc);

    const loc = cap.dwarf.lookupAddress(&table, 0x8004).?;
    expect_eq(true, std.mem.eql(u8, loc.file, "boot.cp"));
    expect_eq(@as(i64, 7), loc.line);
    expect_eq(@as(u32, 3), loc.column);
}
