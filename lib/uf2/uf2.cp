// Origin: Pure-CP Stdlib (Zero-Alloc Microsoft UF2 Microcontroller Firmware Codec) | Created: 2026-11-22

/// UF2 固定块大小（512 字节）
pub const UF2_BLOCK_SIZE: usize = 512;

/// UF2 单块最大有效负载字节数（476 字节）
pub const UF2_MAX_PAYLOAD: usize = 476;

/// UF2 三重魔数（小端序）
pub const UF2_MAGIC_START0: u32 = 0x0A324655; // "UF2\n"
pub const UF2_MAGIC_START1: u32 = 0x9E5D5157;
pub const UF2_MAGIC_END: u32 = 0x0AB16F30;

/// UF2 标志位常量
pub const UF2_FLAG_NOT_MAIN_FLASH: u32 = 0x00000001;
pub const UF2_FLAG_FILE_CONTAINER: u32 = 0x00001000;
pub const UF2_FLAG_FAMILY_ID_PRESENT: u32 = 0x00002000;
pub const UF2_FLAG_MD5_PRESENT: u32 = 0x00004000;
pub const UF2_FLAG_EXTENSION_TAGS: u32 = 0x00008000;

/// 常见微控制器 Family ID 常量
pub const UF2_FAMILY_RP2040: u32 = 0xE48BFF56;
pub const UF2_FAMILY_RP2350_ARM_S: u32 = 0xE48BFF59;
pub const UF2_FAMILY_RP2350_RISCV: u32 = 0xE48BFF5A;
pub const UF2_FAMILY_ESP32S2: u32 = 0xBFDD4EEE;
pub const UF2_FAMILY_ESP32S3: u32 = 0xC47E5767;
pub const UF2_FAMILY_STM32F4: u32 = 0x57755A57;
pub const UF2_FAMILY_NRF52840: u32 = 0xADA52840;

/// 单个 UF2 512 字节块解码视图（零拷贝切片指向原始块内负载区）
pub struct Uf2Block {
    flags: u32,
    target_addr: u32,
    payload_size: u32,
    block_no: u32,
    num_blocks: u32,
    family_or_file_size: u32,
    payload: []const u8,

    pub fn hasFamilyId(): bool {
        return (_.flags & UF2_FLAG_FAMILY_ID_PRESENT) != 0;
    }

    pub fn familyId(): u32? {
        if (_.hasFamilyId()) {
            return _.family_or_file_size;
        }
        return null;
    }

    pub fn isNotMainFlash(): bool {
        return (_.flags & UF2_FLAG_NOT_MAIN_FLASH) != 0;
    }

    pub fn isFileContainer(): bool {
        return (_.flags & UF2_FLAG_FILE_CONTAINER) != 0;
    }
}

/// 完整 UF2 固件镜像单遍扫描汇总结果
pub struct Uf2Summary {
    block_count: u32,
    total_payload_bytes: u64,
    min_addr: u32,
    max_addr: u32,
    family_id: u32?,
    is_contiguous: bool,
}

// ---------------------------------------------------------------------------
// 内部小端序读写辅助函数
// ---------------------------------------------------------------------------

fn readU32LE(data: []const u8, offset: usize): u32 {
    const b0 = u32(data[offset]);
    const b1 = u32(data[offset + 1]);
    const b2 = u32(data[offset + 2]);
    const b3 = u32(data[offset + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn writeU32LE(buf: []u8, offset: usize, val: u32) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((val >> 24) & 0xFF) as u8;
}

// ---------------------------------------------------------------------------
// 1. 快速识别与单块解码
// ---------------------------------------------------------------------------

/// 快速校验数据是否为 UF2 格式（长度为 512 的正整数倍且首块三重魔数匹配）
pub fn isUf2(data: []const u8): bool {
    if (data.len < UF2_BLOCK_SIZE || (data.len % UF2_BLOCK_SIZE) != 0) {
        return false;
    }
    return readU32LE(data, 0) == UF2_MAGIC_START0 &&
        readU32LE(data, 4) == UF2_MAGIC_START1 &&
        readU32LE(data, 508) == UF2_MAGIC_END;
}

/// 零分配解析单个 512 字节 UF2 块，校验三重魔数、`payload_size <= 476`、`num_blocks > 0 && block_no < num_blocks`
pub fn parseUf2Block(block_512: []const u8): Uf2Block? {
    if (block_512.len != UF2_BLOCK_SIZE) {
        return null;
    }
    if (readU32LE(block_512, 0) != UF2_MAGIC_START0 ||
        readU32LE(block_512, 4) != UF2_MAGIC_START1 ||
        readU32LE(block_512, 508) != UF2_MAGIC_END)
    {
        return null;
    }
    const flags = readU32LE(block_512, 8);
    const target_addr = readU32LE(block_512, 12);
    const payload_size = readU32LE(block_512, 16);
    const block_no = readU32LE(block_512, 20);
    const num_blocks = readU32LE(block_512, 24);
    const family_or_file_size = readU32LE(block_512, 28);

    if (usize(payload_size) > UF2_MAX_PAYLOAD) {
        return null;
    }
    if (num_blocks == 0 || block_no >= num_blocks) {
        return null;
    }

    const payload_end = 32 + usize(payload_size);
    return Uf2Block {
        flags: flags,
        target_addr: target_addr,
        payload_size: payload_size,
        block_no: block_no,
        num_blocks: num_blocks,
        family_or_file_size: family_or_file_size,
        payload: block_512[32..payload_end],
    };
}

// ---------------------------------------------------------------------------
// 2. 零分配逐块迭代器
// ---------------------------------------------------------------------------

/// 零分配 UF2 固件逐块迭代器
pub struct Uf2Iter {
    data: []const u8,
    offset: usize,
    malformed: bool,

    pub fn init(data: []const u8): Uf2Iter {
        if (data.len == 0 || (data.len % UF2_BLOCK_SIZE) != 0) {
            return Uf2Iter {
                data: data,
                offset: data.len,
                malformed: data.len != 0,
            };
        }
        return Uf2Iter {
            data: data,
            offset: 0,
            malformed: false,
        };
    }

    pub fn next(*_): Uf2Block? {
        if (_.offset >= _.data.len) {
            return null;
        }
        const blk_slice = _.data[_.offset .. _.offset + UF2_BLOCK_SIZE];
        if (blk := parseUf2Block(blk_slice)) {
            _.offset += UF2_BLOCK_SIZE;
            return blk;
        }
        _.malformed = true;
        _.offset = _.data.len;
        return null;
    }
}

// ---------------------------------------------------------------------------
// 3. 零分配单块编码器
// ---------------------------------------------------------------------------

/// 零分配将单个固件块编码为 512 字节标准 UF2 块，剩余负载区自动清零并写入尾魔数
pub fn encodeUf2Block(
    flags: u32,
    target_addr: u32,
    block_no: u32,
    num_blocks: u32,
    family_or_file_size: u32,
    payload: []const u8,
    out_512: []u8,
): bool {
    if (out_512.len < UF2_BLOCK_SIZE || payload.len > UF2_MAX_PAYLOAD) {
        return false;
    }
    if (num_blocks == 0 || block_no >= num_blocks) {
        return false;
    }

    writeU32LE(out_512, 0, UF2_MAGIC_START0);
    writeU32LE(out_512, 4, UF2_MAGIC_START1);
    writeU32LE(out_512, 8, flags);
    writeU32LE(out_512, 12, target_addr);
    writeU32LE(out_512, 16, payload.len as u32);
    writeU32LE(out_512, 20, block_no);
    writeU32LE(out_512, 24, num_blocks);
    writeU32LE(out_512, 28, family_or_file_size);

    let i: usize = 0;
    while (i < payload.len) {
        out_512[32 + i] = payload[i];
        i += 1;
    }
    let z: usize = payload.len + 32;
    while (z < 508) {
        out_512[z] = 0;
        z += 1;
    }

    writeU32LE(out_512, 508, UF2_MAGIC_END);
    return true;
}

// ---------------------------------------------------------------------------
// 4. 固件单遍汇总分析
// ---------------------------------------------------------------------------

/// 单遍扫描完整 UF2 固件镜像，统计块数、总负载字节数、地址范围、Family ID 与地址连续性；
/// 若输入为空、非 512 倍数或包含任意非法块，返回 `null`
pub fn summarizeUf2(data: []const u8): Uf2Summary? {
    if (data.len < UF2_BLOCK_SIZE || (data.len % UF2_BLOCK_SIZE) != 0) {
        return null;
    }
    let it = Uf2Iter.init(data);
    let block_count: u32 = 0;
    let total_payload_bytes: u64 = 0;
    let min_addr: u32 = 0;
    let max_addr: u32 = 0;
    let family_id: u32? = null;
    let is_contiguous = true;
    let prev_end_addr: u64 = 0;
    let prev_block_no: u32 = 0;

    while (blk := it.next()) {
        const end_addr_u64 = u64(blk.target_addr) + u64(blk.payload_size);
        const end_addr: u32 = if (end_addr_u64 > 0xFFFFFFFF) 0xFFFFFFFF else (end_addr_u64 as u32);

        if (block_count == 0) {
            min_addr = blk.target_addr;
            max_addr = end_addr;
        } else {
            if (blk.target_addr < min_addr) {
                min_addr = blk.target_addr;
            }
            if (end_addr > max_addr) {
                max_addr = end_addr;
            }
            if (u64(blk.target_addr) != prev_end_addr || blk.block_no != prev_block_no + 1) {
                is_contiguous = false;
            }
        }

        if (family_id == null) {
            if (fid := blk.familyId()) {
                family_id = fid;
            }
        }

        total_payload_bytes += u64(blk.payload_size);
        prev_end_addr = end_addr_u64;
        prev_block_no = blk.block_no;
        block_count += 1;
    }

    if (it.malformed || block_count == 0) {
        return null;
    }

    return Uf2Summary {
        block_count: block_count,
        total_payload_bytes: total_payload_bytes,
        min_addr: min_addr,
        max_addr: max_addr,
        family_id: family_id,
        is_contiguous: is_contiguous,
    };
}
