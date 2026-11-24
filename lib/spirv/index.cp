// lib/spirv/index.cp — Vulkan/WebGPU SPIR-V binary header & instruction stream parser (cap:spirv)
// Pure CP, zero heap allocation, strict bounds and word-alignment checks.

pub err SpirvError {
    Truncated,
    UnalignedLength,
    InvalidMagic,
    InvalidWordCount,
    InstructionOutOfBounds,
    OperandOutOfBounds,
}

pub const SPIRV_MAGIC: u32 = 0x07230203;
pub const SPIRV_MAGIC_SWAPPED: u32 = 0x03022307;
pub const HEADER_SIZE: usize = 20;
pub const HEADER_WORDS: usize = 5;

// Common SPIR-V Op constants (Khronos SPIR-V Unified Specification)
pub const OP_NOP: u16 = 0;
pub const OP_SOURCE: u16 = 3;
pub const OP_NAME: u16 = 5;
pub const OP_EXTENSION: u16 = 10;
pub const OP_EXT_INST_IMPORT: u16 = 11;
pub const OP_MEMORY_MODEL: u16 = 14;
pub const OP_ENTRY_POINT: u16 = 15;
pub const OP_EXECUTION_MODE: u16 = 16;
pub const OP_CAPABILITY: u16 = 17;
pub const OP_TYPE_VOID: u16 = 19;
pub const OP_TYPE_BOOL: u16 = 20;
pub const OP_TYPE_INT: u16 = 21;
pub const OP_TYPE_FLOAT: u16 = 22;
pub const OP_TYPE_VECTOR: u16 = 23;
pub const OP_TYPE_FUNCTION: u16 = 33;
pub const OP_CONSTANT: u16 = 43;
pub const OP_FUNCTION: u16 = 54;
pub const OP_FUNCTION_END: u16 = 56;
pub const OP_VARIABLE: u16 = 59;
pub const OP_DECORATE: u16 = 71;
pub const OP_LABEL: u16 = 248;
pub const OP_RETURN: u16 = 253;
pub const OP_RETURN_VALUE: u16 = 254;

fn spirvReadU32Le(bytes: []const u8, off: usize): u32 {
    const b0: u32 = bytes[off];
    const b1: u32 = bytes[off + 1];
    const b2: u32 = bytes[off + 2];
    const b3: u32 = bytes[off + 3];
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn spirvReadU32Be(bytes: []const u8, off: usize): u32 {
    const b0: u32 = bytes[off];
    const b1: u32 = bytes[off + 1];
    const b2: u32 = bytes[off + 2];
    const b3: u32 = bytes[off + 3];
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn spirvReadWord(bytes: []const u8, off: usize, big_endian: bool): u32 {
    if (big_endian) {
        return spirvReadU32Be(bytes, off);
    }
    return spirvReadU32Le(bytes, off);
}

pub struct SpirvHeader {
    pub magic: u32 = 0;
    pub version_raw: u32 = 0;
    pub version_major: u8 = 0;
    pub version_minor: u8 = 0;
    pub generator: u32 = 0;
    pub bound: u32 = 0;
    pub schema: u32 = 0;
    pub is_big_endian: bool = false;
    pub total_words: usize = 0;

    pub fn generatorId(): u16 {
        return ((_.generator >> 16) & 0xFFFF) as u16;
    }

    pub fn generatorVersion(): u16 {
        return (_.generator & 0xFFFF) as u16;
    }
}

pub struct SpirvInst {
    pub opcode: u16 = 0;
    pub word_count: u16 = 0;
    pub word_offset: usize = 0;
    pub operand_count: usize = 0;

    pub fn readOperand(bytes: []const u8, is_big_endian: bool, operand_idx: usize): u32! {
        if (operand_idx >= _.operand_count) {
            return error.OperandOutOfBounds;
        }
        const byte_off = (_.word_offset + 1 + operand_idx) * 4;
        if (byte_off + 4 > bytes.len) {
            return error.InstructionOutOfBounds;
        }
        return spirvReadWord(bytes, byte_off, is_big_endian);
    }
}

pub struct SpirvInstIter {
    pub bytes: []const u8 = "";
    pub is_big_endian: bool = false;
    pub offset: usize = HEADER_SIZE;

    pub static fn init(bytes: []const u8, header: SpirvHeader): SpirvInstIter {
        return SpirvInstIter{
            bytes: bytes,
            is_big_endian: header.is_big_endian,
            offset: HEADER_SIZE,
        };
    }

    pub fn hasNext(): bool {
        return _.offset < _.bytes.len;
    }

    pub fn next(*_): SpirvInst! {
        if (_.offset + 4 > _.bytes.len) {
            return error.InstructionOutOfBounds;
        }
        const head = spirvReadWord(_.bytes, _.offset, _.is_big_endian);
        const opcode: u16 = (head & 0xFFFF) as u16;
        const word_count: u16 = ((head >> 16) & 0xFFFF) as u16;
        if (word_count == 0) {
            return error.InvalidWordCount;
        }
        const byte_len: usize = (word_count as usize) * 4;
        if (byte_len > _.bytes.len - _.offset) {
            return error.InstructionOutOfBounds;
        }
        const inst = SpirvInst{
            opcode: opcode,
            word_count: word_count,
            word_offset: _.offset / 4,
            operand_count: (word_count as usize) - 1,
        };
        _.offset += byte_len;
        return inst;
    }
}

pub fn parseSpirvHeader(bytes: []const u8): SpirvHeader! {
    if (bytes.len < HEADER_SIZE) {
        return error.Truncated;
    }
    if ((bytes.len % 4) != 0) {
        return error.UnalignedLength;
    }

    const magic_le = spirvReadU32Le(bytes, 0);
    const magic_be = spirvReadU32Be(bytes, 0);
    let big_endian = false;
    if (magic_le == SPIRV_MAGIC) {
        big_endian = false;
    } else if (magic_be == SPIRV_MAGIC) {
        big_endian = true;
    } else {
        return error.InvalidMagic;
    }

    const version_raw = spirvReadWord(bytes, 4, big_endian);
    const major: u8 = ((version_raw >> 16) & 0xFF) as u8;
    const minor: u8 = ((version_raw >> 8) & 0xFF) as u8;
    const generator = spirvReadWord(bytes, 8, big_endian);
    const bound = spirvReadWord(bytes, 12, big_endian);
    const schema = spirvReadWord(bytes, 16, big_endian);

    return SpirvHeader{
        magic: SPIRV_MAGIC,
        version_raw: version_raw,
        version_major: major,
        version_minor: minor,
        generator: generator,
        bound: bound,
        schema: schema,
        is_big_endian: big_endian,
        total_words: bytes.len / 4,
    };
}

pub fn isSpirv(bytes: []const u8): bool {
    const hdr = parseSpirvHeader(bytes) catch return false;
    return hdr.magic == SPIRV_MAGIC;
}

pub fn opName(opcode: u16): []const u8 {
    if (opcode == OP_NOP) { return "OpNop"; }
    if (opcode == OP_SOURCE) { return "OpSource"; }
    if (opcode == OP_NAME) { return "OpName"; }
    if (opcode == OP_MEMORY_MODEL) { return "OpMemoryModel"; }
    if (opcode == OP_ENTRY_POINT) { return "OpEntryPoint"; }
    if (opcode == OP_EXECUTION_MODE) { return "OpExecutionMode"; }
    if (opcode == OP_CAPABILITY) { return "OpCapability"; }
    if (opcode == OP_TYPE_VOID) { return "OpTypeVoid"; }
    if (opcode == OP_TYPE_FUNCTION) { return "OpTypeFunction"; }
    if (opcode == OP_FUNCTION) { return "OpFunction"; }
    if (opcode == OP_FUNCTION_END) { return "OpFunctionEnd"; }
    if (opcode == OP_LABEL) { return "OpLabel"; }
    if (opcode == OP_RETURN) { return "OpReturn"; }
    return "OpUnknown";
}
