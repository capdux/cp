// Origin: Pure-CP Stdlib (Zero-Alloc RFC 9669 eBPF 64-bit/128-bit Instruction Codec & Program Validator) | Created: 2026-11-22

// Core Constants
pub const BPF_INSN_SIZE: usize = 8;
pub const BPF_WIDE_INSN_SIZE: usize = 16;
pub const BPF_MAX_REG: u8 = 10;

// Instruction Classes (low 3 bits of opcode)
pub const BPF_LD: u8 = 0x00;
pub const BPF_LDX: u8 = 0x01;
pub const BPF_ST: u8 = 0x02;
pub const BPF_STX: u8 = 0x03;
pub const BPF_ALU: u8 = 0x04;
pub const BPF_JMP: u8 = 0x05;
pub const BPF_JMP32: u8 = 0x06;
pub const BPF_ALU64: u8 = 0x07;

// Source Operand Bit (bit 3)
pub const BPF_K: u8 = 0x00;
pub const BPF_X: u8 = 0x08;

// Size Modifier (bits 3..4 for LD/LDX/ST/STX)
pub const BPF_W: u8 = 0x00;
pub const BPF_H: u8 = 0x08;
pub const BPF_B: u8 = 0x10;
pub const BPF_DW: u8 = 0x18;

// Mode Modifier (bits 5..7 for LD/LDX/ST/STX)
pub const BPF_IMM: u8 = 0x00;
pub const BPF_MEM: u8 = 0x60;
pub const BPF_ATOMIC: u8 = 0xC0;

// ALU / ALU64 Operation Codes (high 4 bits)
pub const ALU_ADD: u8 = 0x00;
pub const ALU_SUB: u8 = 0x10;
pub const ALU_MUL: u8 = 0x20;
pub const ALU_DIV: u8 = 0x30;
pub const ALU_OR: u8 = 0x40;
pub const ALU_AND: u8 = 0x50;
pub const ALU_LSH: u8 = 0x60;
pub const ALU_RSH: u8 = 0x70;
pub const ALU_NEG: u8 = 0x80;
pub const ALU_MOD: u8 = 0x90;
pub const ALU_XOR: u8 = 0xA0;
pub const ALU_MOV: u8 = 0xB0;
pub const ALU_ARSH: u8 = 0xC0;
pub const ALU_END: u8 = 0xD0;

// JMP / JMP32 Operation Codes (high 4 bits)
pub const JMP_JA: u8 = 0x00;
pub const JMP_JEQ: u8 = 0x10;
pub const JMP_JGT: u8 = 0x20;
pub const JMP_JGE: u8 = 0x30;
pub const JMP_JSET: u8 = 0x40;
pub const JMP_JNE: u8 = 0x50;
pub const JMP_JSGT: u8 = 0x60;
pub const JMP_JSGE: u8 = 0x70;
pub const JMP_CALL: u8 = 0x80;
pub const JMP_EXIT: u8 = 0x90;
pub const JMP_JLT: u8 = 0xA0;
pub const JMP_JLE: u8 = 0xB0;
pub const JMP_JSLT: u8 = 0xC0;
pub const JMP_JSLE: u8 = 0xD0;

// Special Opcodes
pub const OP_LD_IMM64: u8 = 0x18; // BPF_LD | BPF_DW | BPF_IMM
pub const OP_CALL: u8 = 0x85;     // BPF_JMP | JMP_CALL
pub const OP_EXIT: u8 = 0x95;     // BPF_JMP | JMP_EXIT

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readI16Le(data: []const u8, off: usize): i16 {
    const bits = readU16Le(data, off);
    return @bitCast(bits);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readI32Le(data: []const u8, off: usize): i32 {
    const bits = readU32Le(data, off);
    return @bitCast(bits);
}

fn writeU16Le(out: []u8, off: usize, v: u16) {
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeI16Le(out: []u8, off: usize, v: i16) {
    const bits: u16 = @bitCast(v);
    writeU16Le(out, off, bits);
}

fn writeU32Le(out: []u8, off: usize, v: u32) {
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
    out[off + 2] = ((v >> 16) & 0xFF) as u8;
    out[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeI32Le(out: []u8, off: usize, v: i32) {
    const bits: u32 = @bitCast(v);
    writeU32Le(out, off, bits);
}

/// Decoded eBPF 64-bit or wide 128-bit (`LD_IMM64`) instruction.
pub struct BpfInsn {
    opcode: u8,
    dst_reg: u8,
    src_reg: u8,
    offset: i16,
    imm: i32,
    imm64: u64,
    is_wide: bool,
    pc: usize,

    /// Instruction class (low 3 bits: `BPF_LD`..`BPF_ALU64`).
    fn insnClass(): u8 {
        return _.opcode & 0x07;
    }

    /// ALU operation code (high 4 bits: `ALU_ADD`..`ALU_END`).
    fn aluOp(): u8 {
        return _.opcode & 0xF0;
    }

    /// JMP operation code (high 4 bits: `JMP_JA`..`JMP_JSLE`).
    fn jmpOp(): u8 {
        return _.opcode & 0xF0;
    }

    /// Returns `true` if the source operand is a register (`BPF_X`).
    fn isRegSource(): bool {
        return (_.opcode & BPF_X) != 0;
    }

    /// Returns `true` if `opcode == OP_EXIT (0x95)`.
    fn isExit(): bool {
        return _.opcode == OP_EXIT;
    }

    /// Returns `true` if `opcode == OP_CALL (0x85)`.
    fn isCall(): bool {
        return _.opcode == OP_CALL;
    }

    /// Returns `true` if `opcode == OP_LD_IMM64 (0x18)`.
    fn isLdImm64(): bool {
        return _.opcode == OP_LD_IMM64;
    }

    /// Returns `true` if both `dst_reg` and `src_reg` are within `0..=10`.
    fn isValidRegs(): bool {
        return _.dst_reg <= BPF_MAX_REG && _.src_reg <= BPF_MAX_REG;
    }

    /// Encodes this instruction into `out` (8 bytes for normal, 16 bytes for wide `LD_IMM64`).
    fn encode(out: []u8): usize? {
        if (_.is_wide || _.opcode == OP_LD_IMM64) {
            return encodeLdImm64(_.dst_reg, _.src_reg, _.imm64, out);
        }
        return encodeBpfInsn(_.opcode, _.dst_reg, _.src_reg, _.offset, _.imm, out);
    }
}

/// Decodes a single 8-byte eBPF instruction (little-endian).
pub fn decodeBpfInsn(bytes8: []const u8): BpfInsn? {
    if (bytes8.len < BPF_INSN_SIZE) {
        return null;
    }
    const opcode = bytes8[0];
    const regs = bytes8[1];
    const dst_reg = regs & 0x0F;
    const src_reg = (regs >> 4) & 0x0F;
    const offset = readI16Le(bytes8, 2);
    const imm = readI32Le(bytes8, 4);
    const imm_u32 = readU32Le(bytes8, 4);

    return BpfInsn{
        .opcode = opcode,
        .dst_reg = dst_reg,
        .src_reg = src_reg,
        .offset = offset,
        .imm = imm,
        .imm64 = imm_u32 as u64,
        .is_wide = false,
        .pc = 0,
    };
}

/// Encodes a standard 8-byte eBPF instruction into `out`.
/// Validates `dst_reg <= 10`, `src_reg <= 10`, and `out.len >= 8`.
pub fn encodeBpfInsn(opcode: u8, dst_reg: u8, src_reg: u8, offset: i16, imm: i32, out: []u8): usize? {
    if (out.len < BPF_INSN_SIZE || dst_reg > BPF_MAX_REG || src_reg > BPF_MAX_REG) {
        return null;
    }
    out[0] = opcode;
    out[1] = (dst_reg & 0x0F) | ((src_reg & 0x0F) << 4);
    writeI16Le(out, 2, offset);
    writeI32Le(out, 4, imm);
    return BPF_INSN_SIZE;
}

/// Encodes a standard 16-byte wide `LD_IMM64` (`0x18`) instruction into `out`.
/// Validates `dst_reg <= 10`, `src_reg <= 10`, and `out.len >= 16`.
pub fn encodeLdImm64(dst_reg: u8, src_reg: u8, imm64: u64, out: []u8): usize? {
    if (out.len < BPF_WIDE_INSN_SIZE || dst_reg > BPF_MAX_REG || src_reg > BPF_MAX_REG) {
        return null;
    }
    const lo = (imm64 & 0xFFFFFFFF) as u32;
    const hi = ((imm64 >> 32) & 0xFFFFFFFF) as u32;

    out[0] = OP_LD_IMM64;
    out[1] = (dst_reg & 0x0F) | ((src_reg & 0x0F) << 4);
    writeU16Le(out, 2, 0);
    writeU32Le(out, 4, lo);

    out[8] = 0;
    out[9] = 0;
    writeU16Le(out, 10, 0);
    writeU32Le(out, 12, hi);
    return BPF_WIDE_INSN_SIZE;
}

/// Helper constructor: `r[dst] = imm` (`ALU64 | ALU_MOV | BPF_K`).
pub fn insnMov64Imm(dst: u8, imm: i32): BpfInsn {
    const imm_u32: u32 = @bitCast(imm);
    return BpfInsn{
        .opcode = BPF_ALU64 | BPF_K | ALU_MOV,
        .dst_reg = dst,
        .src_reg = 0,
        .offset = 0,
        .imm = imm,
        .imm64 = imm_u32 as u64,
        .is_wide = false,
        .pc = 0,
    };
}

/// Helper constructor: `r[dst] = r[src]` (`ALU64 | ALU_MOV | BPF_X`).
pub fn insnMov64Reg(dst: u8, src: u8): BpfInsn {
    return BpfInsn{
        .opcode = BPF_ALU64 | BPF_X | ALU_MOV,
        .dst_reg = dst,
        .src_reg = src,
        .offset = 0,
        .imm = 0,
        .imm64 = 0,
        .is_wide = false,
        .pc = 0,
    };
}

/// Helper constructor: `r[dst] op= r[src]` (`BPF_ALU64 | BPF_X | (op & 0xF0)`).
pub fn insnAlu64Reg(op: u8, dst: u8, src: u8): BpfInsn {
    return BpfInsn{
        .opcode = BPF_ALU64 | BPF_X | (op & 0xF0),
        .dst_reg = dst,
        .src_reg = src,
        .offset = 0,
        .imm = 0,
        .imm64 = 0,
        .is_wide = false,
        .pc = 0,
    };
}

/// Helper constructor: `call helper_id` (`OP_CALL`).
pub fn insnCall(helper_id: i32): BpfInsn {
    const imm_u32: u32 = @bitCast(helper_id);
    return BpfInsn{
        .opcode = OP_CALL,
        .dst_reg = 0,
        .src_reg = 0,
        .offset = 0,
        .imm = helper_id,
        .imm64 = imm_u32 as u64,
        .is_wide = false,
        .pc = 0,
    };
}

/// Helper constructor: `exit` (`OP_EXIT`).
pub fn insnExit(): BpfInsn {
    return BpfInsn{
        .opcode = OP_EXIT,
        .dst_reg = 0,
        .src_reg = 0,
        .offset = 0,
        .imm = 0,
        .imm64 = 0,
        .is_wide = false,
        .pc = 0,
    };
}

/// Zero-alloc streaming iterator over an eBPF bytecode program.
/// Automatically merges 16-byte `LD_IMM64` (`0x18`) wide instructions and advances `pc` by 2.
/// Sets `hasError()` on unaligned trailing bytes or malformed/truncated wide instructions.
pub struct BpfProgIter {
    code: []const u8,
    pos: usize,
    pc: usize,
    has_err: bool,

    pub static fn init(code: []const u8): BpfProgIter {
        return BpfProgIter{
            .code = code,
            .pos = 0,
            .pc = 0,
            .has_err = false,
        };
    }

    fn hasError(): bool {
        return _.has_err;
    }

    fn next(*_): BpfInsn? {
        if (_.has_err || _.pos >= _.code.len) {
            return null;
        }
        if (_.code.len - _.pos < BPF_INSN_SIZE) {
            _.has_err = true;
            return null;
        }
        const cur_pc = _.pc;
        let insn = decodeBpfInsn(_.code[_.pos..(_.pos + BPF_INSN_SIZE)]) ?? return null;
        insn.pc = cur_pc;
        _.pos += BPF_INSN_SIZE;
        _.pc += 1;

        if (insn.opcode == OP_LD_IMM64) {
            if (_.code.len - _.pos < BPF_INSN_SIZE) {
                _.has_err = true;
                return null;
            }
            const op2 = _.code[_.pos];
            const regs2 = _.code[_.pos + 1];
            const off2 = readU16Le(_.code, _.pos + 2);
            if (op2 != 0 || regs2 != 0 || off2 != 0) {
                _.has_err = true;
                return null;
            }
            const hi32 = readU32Le(_.code, _.pos + 4) as u64;
            insn.imm64 = (insn.imm64 & 0xFFFFFFFF) | (hi32 << 32);
            insn.is_wide = true;
            _.pos += BPF_INSN_SIZE;
            _.pc += 1;
        }

        return insn;
    }
}

/// Validates an eBPF bytecode program:
/// - Non-empty and 8-byte aligned
/// - All instructions decode cleanly (including 16-byte `LD_IMM64` wide instructions)
/// - All register indices are within `0..=10`
/// - Contains at least one `OP_EXIT` instruction
pub fn validateBpfProgram(code: []const u8): bool {
    if (code.len == 0 || (code.len & 7) != 0) {
        return false;
    }
    let it = BpfProgIter.init(code);
    let has_exit = false;
    while (true) {
        if (insn := it.next()) {
            if (!insn.isValidRegs()) {
                return false;
            }
            if (insn.isExit()) {
                has_exit = true;
            }
        } else {
            break;
        }
    }
    return !it.hasError() && has_exit;
}
