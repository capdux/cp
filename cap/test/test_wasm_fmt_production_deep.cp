// Source: cp/cap/test/test_wasm_fmt_production_deep.cp (Deep production tests for cap:wasm_fmt)
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    WasmError,
    ValType,
    FuncType,
    WasmLimits,
    WasmImport,
    WasmExport,
    WasmMemory,
    WasmGlobal,
    CustomSection,
    WasmModule,
    readUleb128,
    writeUleb128,
    readSleb128,
    writeSleb128,
    parseWasm,
    encodeWasm,
    findExport,
    findExportFuncType,
    findCustomSection,
    addCustomSection,
} from "cap:wasm_fmt";

const std = @import("std");

test("wasm_fmt leb128 u32/i32 boundary and overflow") {
    const alloc = std.testing.allocator;

    let buf: std.ArrayList(u8) = .empty;
    defer buf.deinit(alloc);

    // Unsigned boundaries: 0, 624485, u32 max
    try writeUleb128(&buf, alloc, 0);
    try writeUleb128(&buf, alloc, 624485);
    try writeUleb128(&buf, alloc, 0xFFFFFFFF);

    let off: usize = 0;
    expect_eq(try readUleb128(buf.items, &off), @as(u32, 0));
    expect_eq(try readUleb128(buf.items, &off), @as(u32, 624485));
    expect_eq(try readUleb128(buf.items, &off), @as(u32, 0xFFFFFFFF));
    expect_eq(off, buf.items.len);

    // Signed boundaries: 0, -1, i32 min, i32 max
    buf.clearRetainingCapacity();
    const i32_min: i32 = -2147483648;
    const i32_max: i32 = 2147483647;
    try writeSleb128(&buf, alloc, 0);
    try writeSleb128(&buf, alloc, -1);
    try writeSleb128(&buf, alloc, i32_min);
    try writeSleb128(&buf, alloc, i32_max);

    off = 0;
    expect_eq(try readSleb128(buf.items, &off), @as(i32, 0));
    expect_eq(try readSleb128(buf.items, &off), @as(i32, -1));
    expect_eq(try readSleb128(buf.items, &off), i32_min);
    expect_eq(try readSleb128(buf.items, &off), i32_max);
    expect_eq(off, buf.items.len);

    // 6-byte overflow interception for uleb128 and sleb128
    const over6 = "\x80\x80\x80\x80\x80\x01";
    let off_u: usize = 0;
    let u_overflow = false;
    * = readUleb128(over6, &off_u) catch |e| {
        if (e == WasmError.Leb128Overflow) {
            u_overflow = true;
        }
    };
    expect_eq(u_overflow, true);

    let off_s: usize = 0;
    let s_overflow = false;
    * = readSleb128(over6, &off_s) catch |e| {
        if (e == WasmError.Leb128Overflow) {
            s_overflow = true;
        }
    };
    expect_eq(s_overflow, true);
}

test("wasm_fmt encodeWasm and parseWasm full module roundtrip") {
    const alloc = std.testing.allocator;

    const p0: [2]ValType = .{ .I32, .I32 };
    const r0: [1]ValType = .{.I32};
    const types: [2]FuncType = .{
        FuncType{ .params = &p0, .results = &r0 },
        FuncType{ .params = &.{}, .results = &.{} },
    };

    const imports: [1]WasmImport = .{
        WasmImport{
            .module = "env",
            .name = "log",
            .kind = .Func,
            .type_index = 1,
        },
    };

    const funcs: [1]u32 = .{@as(u32, 0)};
    const mems: [1]WasmMemory = .{
        WasmMemory{ .limits = WasmLimits{ .min = 1, .max = 4 } },
    };

    const g_init = "\x41\x2A\x0B";
    const globals: [1]WasmGlobal = .{
        WasmGlobal{
            .val_type = .I32,
            .mutable = true,
            .init_expr = g_init,
        },
    };

    const exports: [2]WasmExport = .{
        WasmExport{ .name = "add", .kind = .Func, .index = 1 },
        WasmExport{ .name = "memory", .kind = .Memory, .index = 0 },
    };

    const body = "\x00\x20\x00\x20\x01\x6A\x0B";
    const codes: [1][]const u8 = .{body};

    const custom_data = "\xDE\xAD\xBE\xEF";
    const customs: [1]CustomSection = .{
        CustomSection{ .name = "meta", .data = custom_data },
    };

    const mod = WasmModule{
        .types = &types,
        .imports = &imports,
        .functions = &funcs,
        .memories = &mems,
        .globals = &globals,
        .exports = &exports,
        .start = 0,
        .codes = &codes,
        .custom_sections = &customs,
    };

    const bytes = try encodeWasm(alloc, mod);
    defer alloc.free(bytes);

    let parsed = try parseWasm(alloc, bytes);
    defer parsed.deinit(alloc);

    expect_eq(parsed.types.len, @as(usize, 2));
    expect_eq(parsed.types[0].params.len, @as(usize, 2));
    expect_eq(parsed.types[0].results[0].toByte(), ValType.I32.toByte());
    expect_eq(parsed.imports.len, @as(usize, 1));
    expect_eq(std.mem.eql(u8, parsed.imports[0].module, "env"), true);
    expect_eq(parsed.functions.len, @as(usize, 1));
    expect_eq(parsed.functions[0], @as(u32, 0));
    expect_eq(parsed.memories.len, @as(usize, 1));
    expect_eq(parsed.memories[0].limits.min, @as(u32, 1));
    expect_eq(parsed.memories[0].limits.max.?, @as(u32, 4));
    expect_eq(parsed.globals.len, @as(usize, 1));
    expect_eq(parsed.globals[0].mutable, true);
    expect_eq(parsed.exports.len, @as(usize, 2));
    expect_eq(parsed.start.?, @as(u32, 0));
    expect_eq(parsed.codes.len, @as(usize, 1));
    expect_eq(std.mem.eql(u8, parsed.codes[0], body), true);
    expect_eq(parsed.custom_sections.len, @as(usize, 1));
    expect_eq(std.mem.eql(u8, parsed.custom_sections[0].name, "meta"), true);
}

test("wasm_fmt findExport, findExportFuncType, findCustomSection, addCustomSection") {
    const alloc = std.testing.allocator;

    const p0: [2]ValType = .{ .I32, .I64 };
    const r0: [1]ValType = .{.F64};
    const types: [1]FuncType = .{
        FuncType{ .params = &p0, .results = &r0 },
    };
    const funcs: [1]u32 = .{@as(u32, 0)};
    const exports: [1]WasmExport = .{
        WasmExport{ .name = "calc", .kind = .Func, .index = 0 },
    };
    const body = "\x00\x0B";
    const codes: [1][]const u8 = .{body};

    const mod = WasmModule{
        .types = &types,
        .functions = &funcs,
        .exports = &exports,
        .codes = &codes,
    };

    const base_bytes = try encodeWasm(alloc, mod);
    defer alloc.free(base_bytes);

    const payload = "\x01\x02\x03";
    const with_custom = try addCustomSection(alloc, base_bytes, "name", payload);
    defer alloc.free(with_custom);

    let parsed = try parseWasm(alloc, with_custom);
    defer parsed.deinit(alloc);

    const exp = findExport(parsed, "calc").?;
    expect_eq(exp.index, @as(u32, 0));

    const sig = findExportFuncType(parsed, "calc").?;
    expect_eq(sig.params.len, @as(usize, 2));
    expect_eq(sig.params[1].toByte(), ValType.I64.toByte());
    expect_eq(sig.results[0].toByte(), ValType.F64.toByte());

    const cs = findCustomSection(parsed, "name").?;
    expect_eq(std.mem.eql(u8, cs.data, payload), true);
}

test("wasm_fmt rejects invalid magic, version, section order, duplicate, and bounds") {
    const alloc = std.testing.allocator;

    // 1. Invalid magic
    const bad_magic = "\x00as\x00\x01\x00\x00\x00";
    let got_bad_magic = false;
    * = parseWasm(alloc, bad_magic) catch |e| {
        if (e == WasmError.InvalidMagic) {
            got_bad_magic = true;
        }
    };
    expect_eq(got_bad_magic, true);

    // 2. Unsupported version
    const bad_ver = "\x00asm\x02\x00\x00\x00";
    let got_bad_ver = false;
    * = parseWasm(alloc, bad_ver) catch |e| {
        if (e == WasmError.UnsupportedVersion) {
            got_bad_ver = true;
        }
    };
    expect_eq(got_bad_ver, true);

    // 3. Out-of-order section (Export=7 before Type=1)
    const out_of_order = "\x00asm\x01\x00\x00\x00\x07\x01\x00\x01\x01\x00";
    let got_ooo = false;
    * = parseWasm(alloc, out_of_order) catch |e| {
        if (e == WasmError.OutOfOrderSection) {
            got_ooo = true;
        }
    };
    expect_eq(got_ooo, true);

    // 4. Duplicate section (Type=1 twice)
    const dup_sec = "\x00asm\x01\x00\x00\x00\x01\x01\x00\x01\x01\x00";
    let got_dup = false;
    * = parseWasm(alloc, dup_sec) catch |e| {
        if (e == WasmError.DuplicateSection) {
            got_dup = true;
        }
    };
    expect_eq(got_dup, true);

    // 5. Out-of-bounds payload length
    const oob_len = "\x00asm\x01\x00\x00\x00\x01\x7F\x00";
    let got_eof = false;
    * = parseWasm(alloc, oob_len) catch |e| {
        if (e == WasmError.UnexpectedEof) {
            got_eof = true;
        }
    };
    expect_eq(got_eof, true);
}

test("wasm_fmt cap.wasm_fmt dynamic mount and zero leaks") {
    const alloc = std.testing.allocator;

    const exports: [1]WasmExport = .{
        WasmExport{ .name = "mem", .kind = .Memory, .index = 0 },
    };
    const mems: [1]WasmMemory = .{
        WasmMemory{ .limits = WasmLimits{ .min = 2, .max = null } },
    };
    const mod = WasmModule{
        .memories = &mems,
        .exports = &exports,
    };

    const encoded = try cap.wasm_fmt.encodeWasm(alloc, mod);
    defer alloc.free(encoded);

    let decoded = try cap.wasm_fmt.parseWasm(alloc, encoded);
    defer decoded.deinit(alloc);

    const found = cap.wasm_fmt.findExport(decoded, "mem").?;
    expect_eq(found.index, @as(u32, 0));
    expect_eq(decoded.memories[0].limits.min, @as(u32, 2));
}
