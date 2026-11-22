// Source: cp/cap/test/test_spv_production_deep.cp (Production deep tests for cap:spv)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SPV_MAGIC,
    SPV_HEADER_BYTES,
    OP_MEMORY_MODEL,
    OP_ENTRY_POINT,
    OP_CAPABILITY,
    OP_TYPE_VOID,
    OP_FUNCTION,
    OP_FUNCTION_END,
    SpvHeader,
    SpvInst,
    SpvExecutionModel,
    SpvEntryPoint,
    SpvStats,
    isSpv,
    parseSpvHeader,
    SpvInstIter,
    parseSpvEntryPoint,
    summarizeSpv,
} from "cap:compiler";

fn pushWordLe(buf: []u8, word_idx: usize, v: u32) {
    const p = word_idx * 4;
    buf[p] = u8(v & 0xFF);
    buf[p + 1] = u8((v >> 8) & 0xFF);
    buf[p + 2] = u8((v >> 16) & 0xFF);
    buf[p + 3] = u8((v >> 24) & 0xFF);
}

fn pushWordBe(buf: []u8, word_idx: usize, v: u32) {
    const p = word_idx * 4;
    buf[p] = u8((v >> 24) & 0xFF);
    buf[p + 1] = u8((v >> 16) & 0xFF);
    buf[p + 2] = u8((v >> 8) & 0xFF);
    buf[p + 3] = u8(v & 0xFF);
}

fn makeInstHead(wc: u16, op: u16): u32 {
    return (u32(wc) << 16) | u32(op);
}

test("1. isSpv and parseSpvHeader support both LE and BE SPIR-V 1.5 headers", () => {
    expect_eq(SPV_HEADER_BYTES, 20);

    // Little-endian SPIR-V 1.5
    let le: [20]u8 = std.mem.zeroes([20]u8);
    pushWordLe(le[0..], 0, SPV_MAGIC);
    pushWordLe(le[0..], 1, 0x00010500); // v1.5
    pushWordLe(le[0..], 2, 0x00080001); // generator
    pushWordLe(le[0..], 3, 42);         // bound
    pushWordLe(le[0..], 4, 0);          // schema

    expect_eq(isSpv(le[0..]), true);
    const h_le = parseSpvHeader(le[0..]) ?? unreachable;
    expect_eq(h_le.is_big_endian, false);
    expect_eq(h_le.version_major, 1);
    expect_eq(h_le.version_minor, 5);
    expect_eq(h_le.generator_magic, 0x00080001);
    expect_eq(h_le.bound, 42);
    expect_eq(h_le.schema, 0);
    expect_eq(h_le.word_count, 5);

    // Big-endian SPIR-V 1.6
    let be: [20]u8 = std.mem.zeroes([20]u8);
    pushWordBe(be[0..], 0, SPV_MAGIC);
    pushWordBe(be[0..], 1, 0x00010600); // v1.6
    pushWordBe(be[0..], 2, 0x000D0000);
    pushWordBe(be[0..], 3, 128);
    pushWordBe(be[0..], 4, 0);

    expect_eq(isSpv(be[0..]), true);
    const h_be = parseSpvHeader(be[0..]) ?? unreachable;
    expect_eq(h_be.is_big_endian, true);
    expect_eq(h_be.version_major, 1);
    expect_eq(h_be.version_minor, 6);
    expect_eq(h_be.bound, 128);
});

test("2. SpvInstIter walks OpCapability, OpMemoryModel, OpTypeVoid, OpFunction, OpFunctionEnd", () => {
    // 5 header + 2 (Cap) + 3 (MemModel) + 2 (TypeVoid) + 5 (Function) + 1 (FunctionEnd) = 18 words = 72 bytes
    let mod: [72]u8 = std.mem.zeroes([72]u8);
    pushWordLe(mod[0..], 0, SPV_MAGIC);
    pushWordLe(mod[0..], 1, 0x00010500);
    pushWordLe(mod[0..], 3, 10);

    // OpCapability Shader (1)
    pushWordLe(mod[0..], 5, makeInstHead(2, OP_CAPABILITY));
    pushWordLe(mod[0..], 6, 1);
    // OpMemoryModel Logical(0) GLSL450(1)
    pushWordLe(mod[0..], 7, makeInstHead(3, OP_MEMORY_MODEL));
    pushWordLe(mod[0..], 8, 0);
    pushWordLe(mod[0..], 9, 1);
    // OpTypeVoid %1
    pushWordLe(mod[0..], 10, makeInstHead(2, OP_TYPE_VOID));
    pushWordLe(mod[0..], 11, 1);
    // OpFunction %1 %4 None(0) %3
    pushWordLe(mod[0..], 12, makeInstHead(5, OP_FUNCTION));
    pushWordLe(mod[0..], 13, 1);
    pushWordLe(mod[0..], 14, 4);
    pushWordLe(mod[0..], 15, 0);
    pushWordLe(mod[0..], 16, 3);
    // OpFunctionEnd
    pushWordLe(mod[0..], 17, makeInstHead(1, OP_FUNCTION_END));

    let it = SpvInstIter.init(mod[0..]);

    const i0 = it.next() ?? unreachable;
    expect_eq(i0.opcode, OP_CAPABILITY);
    expect_eq(i0.operandCount(), 1);
    expect_eq(i0.operandU32(0) ?? 0, 1);
    expect_eq(i0.operandU32(1) == null, true);

    const i1 = it.next() ?? unreachable;
    expect_eq(i1.opcode, OP_MEMORY_MODEL);
    expect_eq(i1.operandCount(), 2);
    expect_eq(i1.operandU32(0) ?? 99, 0);
    expect_eq(i1.operandU32(1) ?? 99, 1);

    const i2 = it.next() ?? unreachable;
    expect_eq(i2.opcode, OP_TYPE_VOID);
    expect_eq(i2.operandU32(0) ?? 0, 1);

    const i3 = it.next() ?? unreachable;
    expect_eq(i3.opcode, OP_FUNCTION);
    expect_eq(i3.operandU32(1) ?? 0, 4);

    const i4 = it.next() ?? unreachable;
    expect_eq(i4.opcode, OP_FUNCTION_END);
    expect_eq(i4.operandCount(), 0);

    expect_eq(it.next() == null, true);
    expect_eq(it.failed, false);
});

test("3. parseSpvEntryPoint extracts execution model, function ID, and UTF-8 entry name", () => {
    // 5 header + 5 words OpEntryPoint (head, model=5 GLCompute, id=7, "main\0\0\0\0" = 2 words? "main\0" is 5 bytes -> 2 words)
    // Let's use "fs_main\0" (8 bytes = 2 words) => total OpEntryPoint wc = 3 + 2 = 5 words -> 10 words total = 40 bytes
    let mod: [40]u8 = std.mem.zeroes([40]u8);
    pushWordLe(mod[0..], 0, SPV_MAGIC);
    pushWordLe(mod[0..], 1, 0x00010500);
    pushWordLe(mod[0..], 3, 20);

    pushWordLe(mod[0..], 5, makeInstHead(5, OP_ENTRY_POINT));
    pushWordLe(mod[0..], 6, 4); // Fragment
    pushWordLe(mod[0..], 7, 7); // %7
    const ep_name: []const u8 = "fs_main";
    let k: usize = 0;
    while (k < ep_name.len) {
        mod[32 + k] = ep_name[k];
        k += 1;
    }
    mod[39] = 0;

    let it = SpvInstIter.init(mod[0..]);
    const inst = it.next() ?? unreachable;
    const ep = parseSpvEntryPoint(inst, mod[0..], false) ?? unreachable;
    expect_eq(ep.execution_model == SpvExecutionModel.fragment, true);
    expect_eq(ep.entry_point_id, 7);
    expect_eq(std.mem.eql(u8, ep.name, "fs_main"), true);
});

test("4. summarizeSpv counts instructions, capabilities, entry points, and functions in one pass", () => {
    // 5 header + 2 Cap + 2 Cap + 4 EntryPoint("go\0\0") + 5 Fn + 1 FnEnd = 19 words = 76 bytes
    let mod: [76]u8 = std.mem.zeroes([76]u8);
    pushWordLe(mod[0..], 0, SPV_MAGIC);
    pushWordLe(mod[0..], 1, 0x00010500);

    pushWordLe(mod[0..], 5, makeInstHead(2, OP_CAPABILITY));
    pushWordLe(mod[0..], 6, 1);
    pushWordLe(mod[0..], 7, makeInstHead(2, OP_CAPABILITY));
    pushWordLe(mod[0..], 8, 64);

    pushWordLe(mod[0..], 9, makeInstHead(4, OP_ENTRY_POINT));
    pushWordLe(mod[0..], 10, 5); // GLCompute
    pushWordLe(mod[0..], 11, 2);
    mod[48] = 'g';
    mod[49] = 'o';
    mod[50] = 0;

    pushWordLe(mod[0..], 13, makeInstHead(5, OP_FUNCTION));
    pushWordLe(mod[0..], 18, makeInstHead(1, OP_FUNCTION_END));

    const stats = summarizeSpv(mod[0..]) ?? unreachable;
    expect_eq(stats.instruction_count, 5);
    expect_eq(stats.capability_count, 2);
    expect_eq(stats.entry_point_count, 1);
    expect_eq(stats.function_count, 1);
});

test("5. Malformed SPIR-V protection: unaligned length, word_count=0, out-of-bounds inst, missing NUL", () => {
    // Unaligned length (21 bytes) & short buffer
    let unaligned: [21]u8 = std.mem.zeroes([21]u8);
    pushWordLe(unaligned[0..], 0, SPV_MAGIC);
    expect_eq(isSpv(unaligned[0..]), false);
    expect_eq(parseSpvHeader(unaligned[0..]) == null, true);

    // word_count == 0 instruction
    let zero_wc: [24]u8 = std.mem.zeroes([24]u8);
    pushWordLe(zero_wc[0..], 0, SPV_MAGIC);
    pushWordLe(zero_wc[0..], 1, 0x00010500);
    pushWordLe(zero_wc[0..], 5, makeInstHead(0, OP_CAPABILITY));
    expect_eq(summarizeSpv(zero_wc[0..]) == null, true);

    // Out-of-bounds instruction (claims wc=10 when only 1 word remains)
    let oob: [24]u8 = std.mem.zeroes([24]u8);
    pushWordLe(oob[0..], 0, SPV_MAGIC);
    pushWordLe(oob[0..], 1, 0x00010500);
    pushWordLe(oob[0..], 5, makeInstHead(10, OP_CAPABILITY));
    expect_eq(summarizeSpv(oob[0..]) == null, true);

    // OpEntryPoint missing NUL terminator in name bytes
    let no_nul: [36]u8 = std.mem.zeroes([36]u8);
    pushWordLe(no_nul[0..], 0, SPV_MAGIC);
    pushWordLe(no_nul[0..], 1, 0x00010500);
    pushWordLe(no_nul[0..], 5, makeInstHead(4, OP_ENTRY_POINT));
    pushWordLe(no_nul[0..], 6, 0);
    pushWordLe(no_nul[0..], 7, 1);
    no_nul[32] = 'a'; no_nul[33] = 'b'; no_nul[34] = 'c'; no_nul[35] = 'd';
    let it = SpvInstIter.init(no_nul[0..]);
    const bad_ep = it.next() ?? unreachable;
    expect_eq(parseSpvEntryPoint(bad_ep, no_nul[0..], false) == null, true);
});
