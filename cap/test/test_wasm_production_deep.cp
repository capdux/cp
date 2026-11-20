// Source: cp/cap/test/test_wasm_production_deep.cp
// Deep Production Test Suite for cap:wasm
// Covers:
//   1. Leb128 U32/I32/U64/I64 boundary encoding/decoding & strict 5B/10B overflow/EOF guards
//   2. WasmBuilder full add(i32, i32) -> i32 module (Import/Memory/Export/Start/Custom) & WasmModule.parse round-trip
//   3. findExport / findCustomSection / getFuncType / getCodeBody & function body bytecode verification
//   4. Malformed binary guards: InvalidMagic, UnsupportedVersion, InvalidSectionOrder, UnexpectedEof, InvalidValType, InvalidExportKind, MalformedSection
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.wasm`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    WasmError,
    Leb128,
    WASM_MAGIC,
    WASM_VERSION,
    WasmValType,
    WasmExternKind,
    WasmLimits,
    WasmFuncType,
    WasmImport,
    WasmExport,
    WasmCustomSection,
    WasmLocalDecl,
    WasmCodeBody,
    WasmModule,
    WasmBuilder,
} from "cap:wasm";
import cap from "cap";

fn testLeb128BoundariesAndOverflow()! {
    const alloc = std.heap.page_allocator;

    // 1. U32 boundary vectors
    const u32_vals: [6]u32 = [0, 127, 128, 624485, 268435455, std.math.maxInt(u32)];
    for (v of u32_vals) {
        const enc = Leb128.encodeU32(alloc, v) catch unreachable;
        defer alloc.free(enc);
        let pos: usize = 0;
        const decoded = Leb128.decodeU32(enc, &pos) catch unreachable;
        expect_eq(decoded, v);
        expect_eq(pos, enc.len);
    }

    // Known Wikipedia U32 vector: 624485 -> 0xE5 0x8E 0x26
    const enc_624485 = Leb128.encodeU32(alloc, 624485) catch unreachable;
    defer alloc.free(enc_624485);
    expect_eq(std.mem.eql(u8, enc_624485, "\xE5\x8E\x26"), true);

    // 2. I32 boundary vectors (including minInt(i32) & maxInt(i32))
    const i32_vals: [7]i32 = [0, 1, -1, 63, -64, -123456, std.math.minInt(i32)];
    for (v of i32_vals) {
        const enc = Leb128.encodeI32(alloc, v) catch unreachable;
        defer alloc.free(enc);
        let pos: usize = 0;
        const decoded = Leb128.decodeI32(enc, &pos) catch unreachable;
        expect_eq(decoded, v);
        expect_eq(pos, enc.len);
    }

    const enc_i32_max = Leb128.encodeI32(alloc, std.math.maxInt(i32)) catch unreachable;
    defer alloc.free(enc_i32_max);
    let pos_i32_max: usize = 0;
    expect_eq(Leb128.decodeI32(enc_i32_max, &pos_i32_max) catch unreachable, std.math.maxInt(i32));

    // Known signed vector: -123456 -> 0xC0 0xBB 0x78
    const enc_neg123456 = Leb128.encodeI32(alloc, -123456) catch unreachable;
    defer alloc.free(enc_neg123456);
    expect_eq(std.mem.eql(u8, enc_neg123456, "\xC0\xBB\x78"), true);

    // 3. U64 & I64 boundary vectors
    const u64_vals: [4]u64 = [0, 16384, 1000000000000, std.math.maxInt(u64)];
    for (v of u64_vals) {
        const enc = Leb128.encodeU64(alloc, v) catch unreachable;
        defer alloc.free(enc);
        let pos: usize = 0;
        const decoded = Leb128.decodeU64(enc, &pos) catch unreachable;
        expect_eq(decoded, v);
        expect_eq(pos, enc.len);
    }

    const i64_vals: [5]i64 = [0, -1, -9876543210123, std.math.minInt(i64), std.math.maxInt(i64)];
    for (v of i64_vals) {
        const enc = Leb128.encodeI64(alloc, v) catch unreachable;
        defer alloc.free(enc);
        let pos: usize = 0;
        const decoded = Leb128.decodeI64(enc, &pos) catch unreachable;
        expect_eq(decoded, v);
        expect_eq(pos, enc.len);
    }

    // 4. 6-byte U32 overflow interception -> Leb128Overflow
    let p_ovf6: usize = 0;
    let ovf6_hit = false;
    * = Leb128.decodeU32("\x80\x80\x80\x80\x80\x00", &p_ovf6) catch |e| {
        if (e == WasmError.Leb128Overflow) ovf6_hit = true;
    };
    expect_eq(ovf6_hit, true);

    // 5-byte U32 exceeding 32 bits (5th byte >= 0x10) -> Leb128Overflow
    let p_ovf32: usize = 0;
    let ovf32_hit = false;
    * = Leb128.decodeU32("\x80\x80\x80\x80\x10", &p_ovf32) catch |e| {
        if (e == WasmError.Leb128Overflow) ovf32_hit = true;
    };
    expect_eq(ovf32_hit, true);

    // 5-byte I32 with invalid sign-extension bits -> Leb128Overflow
    let p_i32_bad: usize = 0;
    let i32_bad_hit = false;
    * = Leb128.decodeI32("\x80\x80\x80\x80\x18", &p_i32_bad) catch |e| {
        if (e == WasmError.Leb128Overflow) i32_bad_hit = true;
    };
    expect_eq(i32_bad_hit, true);

    // 11-byte U64 overflow -> Leb128Overflow
    let p_u64_ovf: usize = 0;
    let u64_ovf_hit = false;
    * = Leb128.decodeU64("\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01", &p_u64_ovf) catch |e| {
        if (e == WasmError.Leb128Overflow) u64_ovf_hit = true;
    };
    expect_eq(u64_ovf_hit, true);

    // Truncated LEB128 -> UnexpectedEof
    let p_eof: usize = 0;
    let eof_hit = false;
    * = Leb128.decodeU32("\x80\x80", &p_eof) catch |e| {
        if (e == WasmError.UnexpectedEof) eof_hit = true;
    };
    expect_eq(eof_hit, true);
}

fn testBuilderAndParserRoundTrip()! {
    const alloc = std.heap.page_allocator;

    let b = WasmBuilder.init(alloc);
    defer b.deinit();

    // Type 0: (i32) -> () for imported host log function
    const log_params: [1]WasmValType = [.I32];
    const t_log = b.addType(&log_params, []) catch unreachable;
    expect_eq(t_log, 0 as u32);

    // Type 1: (i32, i32) -> (i32) for exported `add` function
    const add_params: [2]WasmValType = [.I32, .I32];
    const add_results: [1]WasmValType = [.I32];
    const t_add = b.addType(&add_params, &add_results) catch unreachable;
    expect_eq(t_add, 1 as u32);

    // Type 2: () -> () for start function
    const t_void = b.addType([], []) catch unreachable;
    expect_eq(t_void, 2 as u32);

    // Import func index 0: env.log_i32
    const imp_idx = b.addImportFunc("env", "log_i32", t_log) catch unreachable;
    expect_eq(imp_idx, 0 as u32);

    // Local func index 1: add(i32, i32) -> i32
    // local.get 0 (0x20 0x00), local.get 1 (0x20 0x01), i32.add (0x6A), end (0x0B)
    const add_code = "\x20\x00\x20\x01\x6A\x0B";
    const add_idx = b.addFunction(t_add, add_code) catch unreachable;
    expect_eq(add_idx, 1 as u32);

    // Local func index 2: _start() -> () (nop, end)
    const start_idx = b.addFunction(t_void, "\x01\x0B") catch unreachable;
    expect_eq(start_idx, 2 as u32);

    // Linear memory: min 1 page, max 16 pages
    const mem_idx = b.addMemory(1, 16) catch unreachable;
    expect_eq(mem_idx, 0 as u32);

    // Exports & Start & Custom Section
    b.addExport("add", .Func, add_idx) catch unreachable;
    b.addExport("memory", .Memory, mem_idx) catch unreachable;
    b.setStart(start_idx);
    b.addCustomSection("cp.meta", "{\"sandbox\":true}") catch unreachable;

    const wasm_bytes = b.build(alloc) catch unreachable;
    defer alloc.free(wasm_bytes);

    // Verify header bytes directly
    expect_eq(std.mem.eql(u8, wasm_bytes[0..4], WASM_MAGIC[0..]), true);

    // Lossless parse back via WasmModule.parse
    let mod = WasmModule.parse(alloc, wasm_bytes) catch unreachable;
    defer mod.deinit();

    expect_eq(mod.version, WASM_VERSION);
    expect_eq(mod.types.len, 3 as usize);
    expect_eq(mod.types[1].params.len, 2 as usize);
    expect_eq(mod.types[1].params[0], WasmValType.I32);
    expect_eq(mod.types[1].params[1], WasmValType.I32);
    expect_eq(mod.types[1].results.len, 1 as usize);
    expect_eq(mod.types[1].results[0], WasmValType.I32);

    expect_eq(mod.imports.len, 1 as usize);
    expect_eq(std.mem.eql(u8, mod.imports[0].module, "env"), true);
    expect_eq(std.mem.eql(u8, mod.imports[0].name, "log_i32"), true);
    expect_eq(mod.imports[0].kind, WasmExternKind.Func);
    expect_eq(mod.imports[0].type_idx, t_log);

    expect_eq(mod.functions.len, 2 as usize);
    expect_eq(mod.functions[0], t_add);
    expect_eq(mod.functions[1], t_void);

    expect_eq(mod.memories.len, 1 as usize);
    expect_eq(mod.memories[0].min, 1 as u32);
    expect_eq(mod.memories[0].max.?, 16 as u32);

    expect_eq(mod.start_func.?, start_idx);
    expect_eq(mod.exports.len, 2 as usize);
    expect_eq(mod.custom_sections.len, 1 as usize);
}

fn testIntrospectionAndCodeBodyVerification()! {
    const alloc = std.heap.page_allocator;

    let b = WasmBuilder.init(alloc);
    defer b.deinit();

    const p: [2]WasmValType = [.I32, .I32];
    const r: [1]WasmValType = [.I32];
    const t_add = b.addType(&p, &r) catch unreachable;

    const add_code = "\x20\x00\x20\x01\x6A\x0B";
    const add_idx = b.addFunction(t_add, add_code) catch unreachable;

    // Also add a function with local declarations (2 i64 locals)
    const locs: [1]WasmLocalDecl = [WasmLocalDecl{ count: 2, val_type: .I64 }];
    const r2: [1]WasmValType = [.I64];
    const t_loc = b.addType([], &r2) catch unreachable;
    const loc_code = "\x20\x00\x0B";
    const loc_idx = b.addFunctionWithLocals(t_loc, &locs, loc_code) catch unreachable;

    b.addExport("add", .Func, add_idx) catch unreachable;
    b.addExport("with_locals", .Func, loc_idx) catch unreachable;
    b.addCustomSection("producers", "\x01cp\x031.0") catch unreachable;

    const bytes = b.build(alloc) catch unreachable;
    defer alloc.free(bytes);

    let mod = WasmModule.parse(alloc, bytes) catch unreachable;
    defer mod.deinit();

    // findExport positive & negative
    const exp_add = mod.findExport("add").?;
    expect_eq(exp_add.kind, WasmExternKind.Func);
    expect_eq(exp_add.index, add_idx);
    expect_eq(mod.findExport("nonexistent") == null, true);

    // findCustomSection positive & negative
    const cs = mod.findCustomSection("producers").?;
    expect_eq(std.mem.eql(u8, cs.data, "\x01cp\x031.0"), true);
    expect_eq(mod.findCustomSection("missing") == null, true);

    // Code body bytecode verification
    const add_body = mod.getCodeBody(exp_add.index).?;
    expect_eq(add_body.locals.len, 0 as usize);
    expect_eq(std.mem.eql(u8, add_body.code, add_code), true);

    const ft = mod.getFuncType(exp_add.index).?;
    expect_eq(ft.params.len, 2 as usize);
    expect_eq(ft.results[0], WasmValType.I32);

    const loc_body = mod.getCodeBody(loc_idx).?;
    expect_eq(loc_body.locals.len, 1 as usize);
    expect_eq(loc_body.locals[0].count, 2 as u32);
    expect_eq(loc_body.locals[0].val_type, WasmValType.I64);
    expect_eq(std.mem.eql(u8, loc_body.code, loc_code), true);
}

fn testSecurityAndMalformedBinaryGuards()! {
    const alloc = std.heap.page_allocator;

    // 1. Invalid magic
    let bad_magic = false;
    * = WasmModule.parse(alloc, "\x00bad\x01\x00\x00\x00") catch |e| {
        if (e == WasmError.InvalidMagic) bad_magic = true;
    };
    expect_eq(bad_magic, true);

    // 2. Unsupported version (version 2)
    let bad_ver = false;
    * = WasmModule.parse(alloc, "\x00asm\x02\x00\x00\x00") catch |e| {
        if (e == WasmError.UnsupportedVersion) bad_ver = true;
    };
    expect_eq(bad_ver, true);

    // 3. Truncated header (< 8 bytes)
    let trunc_hdr = false;
    * = WasmModule.parse(alloc, "\x00asm\x01") catch |e| {
        if (e == WasmError.UnexpectedEof) trunc_hdr = true;
    };
    expect_eq(trunc_hdr, true);

    // 4. Out-of-order section IDs (Memory(5) before Type(1)) -> InvalidSectionOrder
    // Header + Memory(id=5, len=2, [1, 0x00, 1] -> actually len=3: count=1, flags=0, min=1) + Type(id=1, len=1, count=0)
    let bad_order = false;
    * = WasmModule.parse(alloc, "\x00asm\x01\x00\x00\x00\x05\x03\x01\x00\x01\x01\x01\x00") catch |e| {
        if (e == WasmError.InvalidSectionOrder) bad_order = true;
    };
    expect_eq(bad_order, true);

    // Duplicate known section ID (Type(1) twice) -> InvalidSectionOrder
    let dup_sec = false;
    * = WasmModule.parse(alloc, "\x00asm\x01\x00\x00\x00\x01\x01\x00\x01\x01\x00") catch |e| {
        if (e == WasmError.InvalidSectionOrder) dup_sec = true;
    };
    expect_eq(dup_sec, true);

    // 5. Truncated section payload length -> UnexpectedEof
    let trunc_sec = false;
    * = WasmModule.parse(alloc, "\x00asm\x01\x00\x00\x00\x01\x10\x00") catch |e| {
        if (e == WasmError.UnexpectedEof) trunc_sec = true;
    };
    expect_eq(trunc_sec, true);

    // 6. Invalid value type (0x55 in Type section) -> InvalidValType
    // Type section: len=5, count=1, 0x60, p_count=1, valtype=0x55, r_count=0
    let bad_vt = false;
    * = WasmModule.parse(alloc, "\x00asm\x01\x00\x00\x00\x01\x05\x01\x60\x01\x55\x00") catch |e| {
        if (e == WasmError.InvalidValType) bad_vt = true;
    };
    expect_eq(bad_vt, true);

    // 7. Invalid export kind (kind=9 in Export section) -> InvalidExportKind
    // Export section: len=5, count=1, name_len=1, 'a', kind=9, idx=0
    let bad_exp_kind = false;
    * = WasmModule.parse(alloc, "\x00asm\x01\x00\x00\x00\x07\x05\x01\x01a\x09\x00") catch |e| {
        if (e == WasmError.InvalidExportKind) bad_exp_kind = true;
    };
    expect_eq(bad_exp_kind, true);

    // 8. Custom(0) sections allowed anywhere (before Type(1) AND after Type(1))
    // Custom(0, len=3, name="x", data="y") + Type(1, len=1, count=0) + Custom(0, len=3, name="z", data="w")
    let custom_any = WasmModule.parse(
        alloc,
        "\x00asm\x01\x00\x00\x00\x00\x03\x01xy\x01\x01\x00\x00\x03\x01zw",
    ) catch unreachable;
    defer custom_any.deinit();
    expect_eq(custom_any.custom_sections.len, 2 as usize);
    expect_eq(std.mem.eql(u8, custom_any.findCustomSection("x").?.data, "y"), true);
    expect_eq(std.mem.eql(u8, custom_any.findCustomSection("z").?.data, "w"), true);
}

fn testCapWasmDynamicLazyMount()! {
    const alloc = std.heap.page_allocator;

    let b = cap.wasm.WasmBuilder.init(alloc);
    defer b.deinit();

    const p: [2]cap.wasm.WasmValType = [.I32, .I32];
    const r: [1]cap.wasm.WasmValType = [.I32];
    const t0 = b.addType(&p, &r) catch unreachable;
    const f0 = b.addFunction(t0, "\x20\x00\x20\x01\x6A\x0B") catch unreachable;
    b.addExport("sum", .Func, f0) catch unreachable;
    b.addCustomSection("sdk", "cap-wasm-v1") catch unreachable;

    const raw = b.build(alloc) catch unreachable;
    defer alloc.free(raw);

    let mod = cap.wasm.WasmModule.parse(alloc, raw) catch unreachable;
    defer mod.deinit();

    const exp = mod.findExport("sum").?;
    expect_eq(exp.index, 0 as u32);
    expect_eq(std.mem.eql(u8, mod.findCustomSection("sdk").?.data, "cap-wasm-v1"), true);

    const enc = cap.wasm.Leb128.encodeU32(alloc, 300) catch unreachable;
    defer alloc.free(enc);
    let pos: usize = 0;
    expect_eq(cap.wasm.Leb128.decodeU32(enc, &pos) catch unreachable, 300 as u32);
}

fn main() {
    test("1. Leb128 U32/I32/U64/I64 Boundary Vectors & Overflow/EOF Protection", () => {
        testLeb128BoundariesAndOverflow()?;
    });

    test("2. WasmBuilder Full add(i32, i32) -> i32 Module & WasmModule.parse Lossless Round-Trip", () => {
        testBuilderAndParserRoundTrip()?;
    });

    test("3. Introspection: findExport, findCustomSection & Function Code Bytecode Verification", () => {
        testIntrospectionAndCodeBodyVerification()?;
    });

    test("4. Malformed Binary Guards: Magic, Version, Section Order, Truncation & Custom Placement", () => {
        testSecurityAndMalformedBinaryGuards()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.wasm`", () => {
        testCapWasmDynamicLazyMount()?;
    });
}
