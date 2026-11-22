// Source: cp/cap/test/test_bpf_production_deep.cp (Production deep tests for cap:bpf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BPF_INSN_SIZE,
    BPF_WIDE_INSN_SIZE,
    BPF_MAX_REG,
    BPF_LD,
    BPF_LDX,
    BPF_ST,
    BPF_STX,
    BPF_ALU,
    BPF_JMP,
    BPF_JMP32,
    BPF_ALU64,
    BPF_K,
    BPF_X,
    BPF_W,
    BPF_H,
    BPF_B,
    BPF_DW,
    BPF_IMM,
    BPF_MEM,
    BPF_ATOMIC,
    ALU_ADD,
    ALU_SUB,
    ALU_MUL,
    ALU_DIV,
    ALU_OR,
    ALU_AND,
    ALU_LSH,
    ALU_RSH,
    ALU_NEG,
    ALU_MOD,
    ALU_XOR,
    ALU_MOV,
    ALU_ARSH,
    ALU_END,
    JMP_JA,
    JMP_JEQ,
    JMP_JGT,
    JMP_JGE,
    JMP_JSET,
    JMP_JNE,
    JMP_JSGT,
    JMP_JSGE,
    JMP_CALL,
    JMP_EXIT,
    JMP_JLT,
    JMP_JLE,
    JMP_JSLT,
    JMP_JSLE,
    OP_LD_IMM64,
    OP_CALL,
    OP_EXIT,
    BpfInsn,
    decodeBpfInsn,
    encodeBpfInsn,
    encodeLdImm64,
    insnMov64Imm,
    insnMov64Reg,
    insnAlu64Reg,
    insnCall,
    insnExit,
    BpfProgIter,
    validateBpfProgram,
} from "cap:sys";

test("1. Standard 8B ALU64 / JMP / CALL / EXIT encode & decode roundtrip (with negative offset & imm)", () => {
    expect_eq(BPF_INSN_SIZE, 8 as usize);
    expect_eq(BPF_WIDE_INSN_SIZE, 16 as usize);
    expect_eq(BPF_MAX_REG, 10 as u8);
    expect_eq(BPF_LD, 0x00 as u8);
    expect_eq(BPF_LDX, 0x01 as u8);
    expect_eq(BPF_ST, 0x02 as u8);
    expect_eq(BPF_STX, 0x03 as u8);
    expect_eq(BPF_ALU, 0x04 as u8);
    expect_eq(BPF_JMP32, 0x06 as u8);
    expect_eq(BPF_W, 0x00 as u8);
    expect_eq(BPF_H, 0x08 as u8);
    expect_eq(BPF_B, 0x10 as u8);
    expect_eq(BPF_DW, 0x18 as u8);
    expect_eq(BPF_IMM, 0x00 as u8);
    expect_eq(BPF_MEM, 0x60 as u8);
    expect_eq(BPF_ATOMIC, 0xC0 as u8);
    expect_eq(ALU_SUB, 0x10 as u8);
    expect_eq(ALU_MUL, 0x20 as u8);
    expect_eq(ALU_DIV, 0x30 as u8);
    expect_eq(ALU_OR, 0x40 as u8);
    expect_eq(ALU_AND, 0x50 as u8);
    expect_eq(ALU_LSH, 0x60 as u8);
    expect_eq(ALU_RSH, 0x70 as u8);
    expect_eq(ALU_NEG, 0x80 as u8);
    expect_eq(ALU_MOD, 0x90 as u8);
    expect_eq(ALU_XOR, 0xA0 as u8);
    expect_eq(ALU_ARSH, 0xC0 as u8);
    expect_eq(ALU_END, 0xD0 as u8);
    expect_eq(JMP_JA, 0x00 as u8);
    expect_eq(JMP_JGT, 0x20 as u8);
    expect_eq(JMP_JGE, 0x30 as u8);
    expect_eq(JMP_JSET, 0x40 as u8);
    expect_eq(JMP_JNE, 0x50 as u8);
    expect_eq(JMP_JSGT, 0x60 as u8);
    expect_eq(JMP_JSGE, 0x70 as u8);
    expect_eq(JMP_CALL, 0x80 as u8);
    expect_eq(JMP_EXIT, 0x90 as u8);
    expect_eq(JMP_JLT, 0xA0 as u8);
    expect_eq(JMP_JLE, 0xB0 as u8);
    expect_eq(JMP_JSLT, 0xC0 as u8);
    expect_eq(JMP_JSLE, 0xD0 as u8);

    // ALU64 ADD r1, r2
    let buf: [8]u8 = std.mem.zeroes([8]u8);
    const n1 = encodeBpfInsn(BPF_ALU64 | BPF_X | ALU_ADD, 1, 2, 0, 0, buf[0..]) ?? unreachable;
    expect_eq(n1, 8 as usize);
    const d1 = decodeBpfInsn(buf[0..]) ?? unreachable;
    expect_eq(d1.insnClass(), BPF_ALU64);
    expect_eq(d1.aluOp(), ALU_ADD);
    expect_eq(d1.isRegSource(), true);
    expect_eq(d1.dst_reg, 1 as u8);
    expect_eq(d1.src_reg, 2 as u8);
    expect_eq(d1.isValidRegs(), true);

    // Conditional JMP with negative relative offset (-4) and negative immediate (-1)
    const n2 = encodeBpfInsn(BPF_JMP | BPF_K | JMP_JEQ, 3, 0, -4, -1, buf[0..]) ?? unreachable;
    expect_eq(n2, 8 as usize);
    const d2 = decodeBpfInsn(buf[0..]) ?? unreachable;
    expect_eq(d2.insnClass(), BPF_JMP);
    expect_eq(d2.jmpOp(), JMP_JEQ);
    expect_eq(d2.isRegSource(), false);
    expect_eq(d2.offset, -4 as i16);
    expect_eq(d2.imm, -1 as i32);
    expect_eq(d2.imm64, 0xFFFFFFFF as u64);

    // CALL & EXIT
    const call_insn = insnCall(5); // bpf_ktime_get_ns
    expect_eq(call_insn.isCall(), true);
    expect_eq(call_insn.isExit(), false);
    const exit_insn = insnExit();
    expect_eq(exit_insn.isExit(), true);
    expect_eq(exit_insn.opcode, OP_EXIT);
});

test("2. 16B LD_IMM64 encode/decode & BpfProgIter wide merging with pc + 2 stepping", () => {
    let prog: [32]u8 = std.mem.zeroes([32]u8);

    // pc=0: r1 = 0xCAFEBABE_12345678 (16 bytes -> occupies pc 0 & 1)
    const w0 = encodeLdImm64(1, 0, 0xCAFEBABE12345678, prog[0..16]) ?? unreachable;
    expect_eq(w0, 16 as usize);

    // pc=2: r0 = r1
    const w1 = insnMov64Reg(0, 1).encode(prog[16..24]) ?? unreachable;
    expect_eq(w1, 8 as usize);

    // pc=3: exit
    const w2 = insnExit().encode(prog[24..32]) ?? unreachable;
    expect_eq(w2, 8 as usize);

    expect_eq(validateBpfProgram(prog[0..]), true);

    let it = BpfProgIter.init(prog[0..]);

    const i0 = it.next() ?? unreachable;
    expect_eq(i0.pc, 0 as usize);
    expect_eq(i0.isLdImm64(), true);
    expect_eq(i0.is_wide, true);
    expect_eq(i0.dst_reg, 1 as u8);
    expect_eq(i0.src_reg, 0 as u8);
    expect_eq(i0.imm64, 0xCAFEBABE12345678 as u64);

    const i1 = it.next() ?? unreachable;
    expect_eq(i1.pc, 2 as usize); // stepped by +2!
    expect_eq(i1.is_wide, false);
    expect_eq(i1.dst_reg, 0 as u8);
    expect_eq(i1.src_reg, 1 as u8);
    expect_eq(i1.aluOp(), ALU_MOV);

    const i2 = it.next() ?? unreachable;
    expect_eq(i2.pc, 3 as usize);
    expect_eq(i2.isExit(), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("3. Realistic XDP program (helper call + ALU64 + return XDP_PASS) construction & validation", () => {
    let prog: [40]u8 = std.mem.zeroes([40]u8);
    // pc=0: r6 = r1 (save xdp_md ctx)
    * = insnMov64Reg(6, 1).encode(prog[0..8]);
    // pc=1: call 5 (bpf_ktime_get_ns)
    * = insnCall(5).encode(prog[8..16]);
    // pc=2: r2 = 2 (XDP_PASS)
    * = insnMov64Imm(2, 2).encode(prog[16..24]);
    // pc=3: r0 = r0 + r2 (or just r0 = r2)
    * = insnAlu64Reg(ALU_ADD, 0, 2).encode(prog[24..32]);
    // pc=4: exit
    * = insnExit().encode(prog[32..40]);

    expect_eq(validateBpfProgram(prog[0..]), true);

    let it = BpfProgIter.init(prog[0..]);
    let count: usize = 0;
    let saw_call = false;
    let saw_exit = false;
    while (true) {
        if (ins := it.next()) {
            expect_eq(ins.isValidRegs(), true);
            if (ins.isCall()) {
                expect_eq(ins.imm, 5 as i32);
                saw_call = true;
            }
            if (ins.isExit()) {
                saw_exit = true;
            }
            count += 1;
        } else {
            break;
        }
    }
    expect_eq(count, 5 as usize);
    expect_eq(saw_call, true);
    expect_eq(saw_exit, true);
    expect_eq(it.hasError(), false);
});

test("4. Invalid register (r11..r15), missing EXIT, empty & unaligned bytecode rejection", () => {
    // Empty & unaligned
    let unaligned: [12]u8 = std.mem.zeroes([12]u8);
    expect_eq(validateBpfProgram(""), false);
    expect_eq(validateBpfProgram(unaligned[0..]), false);

    // Encoder rejects r11..r15 and short output buffers
    let small: [4]u8 = [0; 4];
    let buf: [16]u8 = std.mem.zeroes([16]u8);
    expect_eq(encodeBpfInsn(OP_EXIT, 0, 0, 0, 0, small[0..]) == null, true);
    expect_eq(encodeBpfInsn(BPF_ALU64 | BPF_K | ALU_MOV, 11, 0, 0, 1, buf[0..]) == null, true);
    expect_eq(encodeBpfInsn(BPF_ALU64 | BPF_X | ALU_MOV, 0, 15, 0, 0, buf[0..]) == null, true);
    expect_eq(encodeLdImm64(11, 0, 1, buf[0..]) == null, true);
    expect_eq(encodeLdImm64(0, 0, 1, buf[0..8]) == null, true);

    // Program with illegal dst_reg = 11 followed by EXIT -> rejected
    * = insnMov64Imm(0, 1).encode(buf[0..8]);
    buf[1] = 0x0B; // dst_reg = 11
    * = insnExit().encode(buf[8..16]);
    const bad_ins = decodeBpfInsn(buf[0..8]) ?? unreachable;
    expect_eq(bad_ins.isValidRegs(), false);
    expect_eq(validateBpfProgram(buf[0..]), false);

    // Program with no EXIT instruction -> rejected
    let no_exit: [8]u8 = std.mem.zeroes([8]u8);
    * = insnMov64Imm(0, 0).encode(no_exit[0..]);
    expect_eq(validateBpfProgram(no_exit[0..]), false);
});

test("5. Truncated & malformed LD_IMM64 wide instruction rejection", () => {
    // 1. Truncated LD_IMM64 (only 8 bytes, missing second 8B slot)
    let trunc: [8]u8 = std.mem.zeroes([8]u8);
    trunc[0] = OP_LD_IMM64;
    trunc[1] = 0x01;
    let it1 = BpfProgIter.init(trunc[0..]);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.hasError(), true);
    expect_eq(validateBpfProgram(trunc[0..]), false);

    // 2. Malformed second 8B slot (non-zero opcode/regs/offset in second half)
    let bad_wide: [24]u8 = std.mem.zeroes([24]u8);
    * = encodeLdImm64(1, 0, 0x1122334455667788, bad_wide[0..16]);
    * = insnExit().encode(bad_wide[16..24]);
    bad_wide[8] = 0x01; // non-zero opcode in second half -> invalid
    let it2 = BpfProgIter.init(bad_wide[0..]);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.hasError(), true);
    expect_eq(validateBpfProgram(bad_wide[0..]), false);

    // 3. decodeBpfInsn on < 8B buffer returns null
    expect_eq(decodeBpfInsn(trunc[0..7]) == null, true);
});
