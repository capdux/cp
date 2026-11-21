// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Khronos SPIR-V Binary Shader Parser)

/// SPIR-V 二进制魔数 (`0x07230203`)
pub const SPV_MAGIC: u32 = 0x07230203;
/// SPIR-V 文件头固定字节数（5 words = 20 字节）
pub const SPV_HEADER_BYTES: usize = 20;

/// 常用 SPIR-V Opcode 常量
pub const OP_NOP: u16 = 0;
pub const OP_SOURCE: u16 = 3;
pub const OP_NAME: u16 = 5;
pub const OP_EXT_INST_IMPORT: u16 = 11;
pub const OP_MEMORY_MODEL: u16 = 14;
pub const OP_ENTRY_POINT: u16 = 15;
pub const OP_EXECUTION_MODE: u16 = 16;
pub const OP_CAPABILITY: u16 = 17;
pub const OP_TYPE_VOID: u16 = 19;
pub const OP_FUNCTION: u16 = 54;
pub const OP_FUNCTION_END: u16 = 56;

/// SPIR-V 5-word (20 字节) 模块文件头
pub struct SpvHeader {
    is_big_endian: bool,
    version_major: u8,
    version_minor: u8,
    generator_magic: u32,
    bound: u32,
    schema: u32,
    word_count: usize,
}

/// SPIR-V 单条指令零拷贝视图
pub struct SpvInst {
    opcode: u16,
    word_count: u16,
    word_offset: usize,
    raw: []const u8,
    is_big_endian: bool,

    /// 操作数 word 数量（`word_count - 1`）
    fn operandCount(): u16 {
        return _.word_count - 1;
    }

    /// 读取第 `idx` 个 32-bit 操作数 word（自动适配大小端）；越界返回 `null`
    fn operandU32(idx: u16): u32? {
        if (idx >= _.operandCount()) {
            return null;
        }
        const byte_off = (1 + usize(idx)) * 4;
        if (byte_off + 4 > _.raw.len) {
            return null;
        }
        return readU32Endian(_.raw, byte_off, _.is_big_endian);
    }
}

/// SPIR-V `OpEntryPoint` 执行模型
pub enum SpvExecutionModel {
    vertex,
    tess_control,
    tess_eval,
    geometry,
    fragment,
    gl_compute,
    kernel,
    unknown,
}

/// 解析后的 `OpEntryPoint` 入口信息
pub struct SpvEntryPoint {
    execution_model: SpvExecutionModel,
    entry_point_id: u32,
    name: []const u8,
}

/// SPIR-V 模块单遍扫描统计摘要
pub struct SpvStats {
    instruction_count: u32,
    capability_count: u32,
    entry_point_count: u32,
    function_count: u32,
}

fn readU32Le(data: []const u8, pos: usize): u32 {
    return u32(data[pos])
        | (u32(data[pos + 1]) << 8)
        | (u32(data[pos + 2]) << 16)
        | (u32(data[pos + 3]) << 24);
}

fn readU32Be(data: []const u8, pos: usize): u32 {
    return (u32(data[pos]) << 24)
        | (u32(data[pos + 1]) << 16)
        | (u32(data[pos + 2]) << 8)
        | u32(data[pos + 3]);
}

fn readU32Endian(data: []const u8, pos: usize, big_endian: bool): u32 {
    return if (big_endian) readU32Be(data, pos) else readU32Le(data, pos);
}

fn decodeExecutionModel(raw: u32): SpvExecutionModel {
    if (raw == 0) return SpvExecutionModel.vertex;
    if (raw == 1) return SpvExecutionModel.tess_control;
    if (raw == 2) return SpvExecutionModel.tess_eval;
    if (raw == 3) return SpvExecutionModel.geometry;
    if (raw == 4) return SpvExecutionModel.fragment;
    if (raw == 5) return SpvExecutionModel.gl_compute;
    if (raw == 6) return SpvExecutionModel.kernel;
    return SpvExecutionModel.unknown;
}

/// 快速校验 SPIR-V 魔数（自动支持 LE/BE）、最小长度 `>= 20` 及 4 字节对齐
pub fn isSpv(data: []const u8): bool {
    if (data.len < SPV_HEADER_BYTES || (data.len % 4) != 0) {
        return false;
    }
    const w0_le = readU32Le(data, 0);
    if (w0_le == SPV_MAGIC) return true;
    const w0_be = readU32Be(data, 0);
    return w0_be == SPV_MAGIC;
}

/// 解析 20 字节 SPIR-V 文件头（自动适配大小端）
pub fn parseSpvHeader(data: []const u8): SpvHeader? {
    if (data.len < SPV_HEADER_BYTES || (data.len % 4) != 0) {
        return null;
    }
    let is_be = false;
    if (readU32Le(data, 0) == SPV_MAGIC) {
        is_be = false;
    } else if (readU32Be(data, 0) == SPV_MAGIC) {
        is_be = true;
    } else {
        return null;
    }

    const ver_word = readU32Endian(data, 4, is_be);
    if ((ver_word & 0xFF00_00FF) != 0) {
        return null;
    }

    return SpvHeader {
        is_big_endian: is_be,
        version_major: u8((ver_word >> 16) & 0xFF),
        version_minor: u8((ver_word >> 8) & 0xFF),
        generator_magic: readU32Endian(data, 8, is_be),
        bound: readU32Endian(data, 12, is_be),
        schema: readU32Endian(data, 16, is_be),
        word_count: data.len / 4,
    };
}

/// 零堆分配 SPIR-V 指令流迭代器
pub struct SpvInstIter {
    data: []const u8,
    word_pos: usize = 5,
    total_words: usize = 0,
    is_big_endian: bool = false,
    failed: bool = false,

    static fn init(data: []const u8): SpvInstIter {
        if (hdr := parseSpvHeader(data)) {
            return SpvInstIter {
                data: data,
                word_pos: 5,
                total_words: hdr.word_count,
                is_big_endian: hdr.is_big_endian,
                failed: false,
            };
        }
        return SpvInstIter {
            data: data,
            word_pos: 5,
            total_words: 0,
            is_big_endian: false,
            failed: true,
        };
    }

    /// 读取下一条 SPIR-V 指令；正常结束返回 `null`，遇到 `word_count == 0` 或越界时置 `failed = true` 并返回 `null`
    fn next(*_): SpvInst? {
        if (_.failed || _.word_pos >= _.total_words) {
            return null;
        }
        const byte_start = _.word_pos * 4;
        const head = readU32Endian(_.data, byte_start, _.is_big_endian);
        const wc = u16((head >> 16) & 0xFFFF);
        const op = u16(head & 0xFFFF);

        if (wc == 0 || usize(wc) > _.total_words - _.word_pos) {
            _.failed = true;
            _.word_pos = _.total_words;
            return null;
        }

        const start_word = _.word_pos;
        const byte_end = (start_word + usize(wc)) * 4;
        _.word_pos += usize(wc);

        return SpvInst {
            opcode: op,
            word_count: wc,
            word_offset: start_word,
            raw: _.data[byte_start..byte_end],
            is_big_endian: _.is_big_endian,
        };
    }
}

/// 解析 `OpEntryPoint` 指令的执行模型、函数 ID 与 `\0` 结尾 UTF-8 入口名
pub fn parseSpvEntryPoint(inst: SpvInst, data: []const u8, is_be: bool): SpvEntryPoint? {
    if (inst.opcode != OP_ENTRY_POINT || inst.word_count < 4) {
        return null;
    }
    const inst_byte_start = inst.word_offset * 4;
    const inst_byte_len = usize(inst.word_count) * 4;
    if (inst_byte_start > data.len || inst_byte_len > data.len - inst_byte_start) {
        return null;
    }

    const model_raw = readU32Endian(data, inst_byte_start + 4, is_be);
    const fn_id = readU32Endian(data, inst_byte_start + 8, is_be);

    const name_start = inst_byte_start + 12;
    const inst_end = inst_byte_start + inst_byte_len;
    let nul_pos: usize? = null;
    let i = name_start;
    while (i < inst_end) {
        if (data[i] == 0) {
            nul_pos = i;
            break;
        }
        i += 1;
    }

    if (np := nul_pos) {
        return SpvEntryPoint {
            execution_model: decodeExecutionModel(model_raw),
            entry_point_id: fn_id,
            name: data[name_start..np],
        };
    }
    return null;
}

/// 单遍扫描统计 SPIR-V 模块指令总数、Capability、EntryPoint 与 Function 数量；畸形模块返回 `null`
pub fn summarizeSpv(data: []const u8): SpvStats? {
    let it = SpvInstIter.init(data);
    if (it.failed) {
        return null;
    }

    let inst_cnt: u32 = 0;
    let cap_cnt: u32 = 0;
    let ep_cnt: u32 = 0;
    let fn_cnt: u32 = 0;

    while (inst := it.next()) {
        inst_cnt += 1;
        if (inst.opcode == OP_CAPABILITY) {
            cap_cnt += 1;
        } else if (inst.opcode == OP_ENTRY_POINT) {
            ep_cnt += 1;
        } else if (inst.opcode == OP_FUNCTION) {
            fn_cnt += 1;
        }
    }

    if (it.failed) {
        return null;
    }

    return SpvStats {
        instruction_count: inst_cnt,
        capability_count: cap_cnt,
        entry_point_count: ep_cnt,
        function_count: fn_cnt,
    };
}
