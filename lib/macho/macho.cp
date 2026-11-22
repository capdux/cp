// Origin: Pure-CP Stdlib (Zero-Alloc Apple Mach-O & Fat Binary Parser) | Created: 2026-11-22

/// Apple Mach-O / Fat Universal 二进制格式种类枚举
pub enum MachoKind: u8 {
    thin32 = 0,
    thin64 = 1,
    fat32 = 2,
    fat64 = 3,
    unknown = 4,
}

/// CPU 类型常量
pub const CPU_TYPE_X86_64: u32 = 0x01000007;
pub const CPU_TYPE_ARM64: u32 = 0x0100000C;

/// Mach-O 文件类型常量
pub const MH_EXECUTE: u32 = 2;
pub const MH_DYLIB: u32 = 6;

/// Mach-O 32/64-bit 二进制头元数据
pub struct MachoHeader {
    is_64bit: bool,
    is_big_endian: bool,
    cputype: u32,
    cpusubtype: u32,
    filetype: u32,
    ncmds: u32,
    sizeofcmds: u32,
    flags: u32,

    /// 是否为 ARM64 架构（`CPU_TYPE_ARM64 = 0x0100000C`）
    pub fn isArm64(): bool {
        return _.cputype == CPU_TYPE_ARM64;
    }

    /// 是否为 x86_64 架构（`CPU_TYPE_X86_64 = 0x01000007`）
    pub fn isX86_64(): bool {
        return _.cputype == CPU_TYPE_X86_64;
    }

    /// 是否为可执行文件（`MH_EXECUTE = 2`）
    pub fn isExecutable(): bool {
        return _.filetype == MH_EXECUTE;
    }

    /// 是否为动态库（`MH_DYLIB = 6`）
    pub fn isDylib(): bool {
        return _.filetype == MH_DYLIB;
    }

    /// 返回头部字节大小（64-bit 为 32 字节，32-bit 为 28 字节）
    pub fn headerSize(): usize {
        if (_.is_64bit) return 32;
        return 28;
    }
}

/// 零拷贝 Mach-O Load Command 视图
pub struct MachoLoadCmd {
    cmd: u32,
    cmdsize: u32,
    /// Load Command 负载切片（不含前 8 字节的 `cmd` 与 `cmdsize`）
    payload: []const u8,
}

/// Apple Fat Universal Binary 架构切片条目
pub struct FatArchEntry {
    cputype: u32,
    cpusubtype: u32,
    offset: u64,
    size: u64,
    align_pow2: u32,
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

fn readU32(data: []const u8, off: usize, is_be: bool): u32 {
    if (is_be) return readU32BE(data, off);
    return readU32LE(data, off);
}

fn readU64BE(data: []const u8, off: usize): u64 {
    const hi: u64 = readU32BE(data, off) as u64;
    const lo: u64 = readU32BE(data, off + 4) as u64;
    return (hi << 32) | lo;
}

fn readU64LE(data: []const u8, off: usize): u64 {
    const lo: u64 = readU32LE(data, off) as u64;
    const hi: u64 = readU32LE(data, off + 4) as u64;
    return (hi << 32) | lo;
}

fn readU64(data: []const u8, off: usize, is_be: bool): u64 {
    if (is_be) return readU64BE(data, off);
    return readU64LE(data, off);
}

/// 根据前 4 字节魔数探测 Mach-O 32/64-bit 或 Fat Universal 32/64-bit 二进制格式
pub fn detectMachoKind(data: []const u8): MachoKind {
    if (data.len < 4) return MachoKind.unknown;
    const magic_be = readU32BE(data, 0);
    if (magic_be == 0xFEEDFACE || magic_be == 0xCEFAEDFE) return MachoKind.thin32;
    if (magic_be == 0xFEEDFACF || magic_be == 0xCFFAEDFE) return MachoKind.thin64;
    if (magic_be == 0xCAFEBABE || magic_be == 0xBEBAFECA) return MachoKind.fat32;
    if (magic_be == 0xCAFEBABF || magic_be == 0xBFBAFECA) return MachoKind.fat64;
    return MachoKind.unknown;
}

/// 解析 Thin Mach-O 32/64-bit 头部（自动识别大端/小端字节序并校验 `>= 28/32` 字节长度）
pub fn parseMachoHeader(data: []const u8): MachoHeader? {
    if (data.len < 28) return null;
    const magic_be = readU32BE(data, 0);

    let is_64bit = false;
    let is_big_endian = false;

    if (magic_be == 0xFEEDFACE) {
        is_64bit = false;
        is_big_endian = true;
    } else if (magic_be == 0xCEFAEDFE) {
        is_64bit = false;
        is_big_endian = false;
    } else if (magic_be == 0xFEEDFACF) {
        is_64bit = true;
        is_big_endian = true;
    } else if (magic_be == 0xCFFAEDFE) {
        is_64bit = true;
        is_big_endian = false;
    } else {
        return null;
    }

    if (is_64bit && data.len < 32) return null;

    return MachoHeader {
        is_64bit: is_64bit,
        is_big_endian: is_big_endian,
        cputype: readU32(data, 4, is_big_endian),
        cpusubtype: readU32(data, 8, is_big_endian),
        filetype: readU32(data, 12, is_big_endian),
        ncmds: readU32(data, 16, is_big_endian),
        sizeofcmds: readU32(data, 20, is_big_endian),
        flags: readU32(data, 24, is_big_endian),
    };
}

/// 零堆分配 Mach-O Load Command 流式迭代器（防越界与非法 `cmdsize` 保护）
pub struct MachoLoadCmdIter {
    data: []const u8,
    pos: usize = 0,
    limit: usize = 0,
    remaining: u32 = 0,
    is_big_endian: bool = false,

    pub static fn init(data: []const u8): MachoLoadCmdIter {
        if (hdr := parseMachoHeader(data)) {
            const h_size = hdr.headerSize();
            const avail = data.len - h_size;
            const declared = hdr.sizeofcmds as usize;
            const bounded = if (declared <= avail) declared else avail;
            return MachoLoadCmdIter {
                data: data,
                pos: h_size,
                limit: h_size + bounded,
                remaining: hdr.ncmds,
                is_big_endian: hdr.is_big_endian,
            };
        }
        return MachoLoadCmdIter {
            data: data,
            pos: 0,
            limit: 0,
            remaining: 0,
            is_big_endian: false,
        };
    }

    /// 读取下一条 Load Command；到达末尾或遇到非法 `cmdsize` / 越界时安全返回 `null`
    pub fn next(*_): MachoLoadCmd? {
        if (_.remaining == 0) return null;
        if (_.limit - _.pos < 8) {
            _.remaining = 0;
            return null;
        }

        const cmd = readU32(_.data, _.pos, _.is_big_endian);
        const cmdsize = readU32(_.data, _.pos + 4, _.is_big_endian);
        const cmdsize_usize = cmdsize as usize;

        if (cmdsize < 8 || cmdsize_usize > (_.limit - _.pos)) {
            _.remaining = 0;
            return null;
        }

        const payload = _.data[_.pos + 8 .. _.pos + cmdsize_usize];
        _.pos += cmdsize_usize;
        _.remaining -= 1;

        return MachoLoadCmd {
            cmd: cmd,
            cmdsize: cmdsize,
            payload: payload,
        };
    }
}

/// 零堆分配 Apple Fat Universal Binary 架构列表迭代器（支持 Fat 32-bit 与 Fat 64-bit）
pub struct FatArchIter {
    data: []const u8,
    pos: usize = 8,
    remaining: u32 = 0,
    is_64bit: bool = false,
    is_big_endian: bool = true,

    pub static fn init(data: []const u8): FatArchIter {
        if (data.len < 8) {
            return FatArchIter {
                data: data,
                pos: 0,
                remaining: 0,
                is_64bit: false,
                is_big_endian: true,
            };
        }

        const magic_be = readU32BE(data, 0);
        let is_64 = false;
        let is_be = true;

        if (magic_be == 0xCAFEBABE) {
            is_64 = false;
            is_be = true;
        } else if (magic_be == 0xBEBAFECA) {
            is_64 = false;
            is_be = false;
        } else if (magic_be == 0xCAFEBABF) {
            is_64 = true;
            is_be = true;
        } else if (magic_be == 0xBFBAFECA) {
            is_64 = true;
            is_be = false;
        } else {
            return FatArchIter {
                data: data,
                pos: 0,
                remaining: 0,
                is_64bit: false,
                is_big_endian: true,
            };
        }

        const nfat_arch = readU32(data, 4, is_be);
        return FatArchIter {
            data: data,
            pos: 8,
            remaining: nfat_arch,
            is_64bit: is_64,
            is_big_endian: is_be,
        };
    }

    /// 读取下一条 Fat 架构条目；到达末尾或缓冲区截断时返回 `null`
    pub fn next(*_): FatArchEntry? {
        if (_.remaining == 0) return null;
        const entry_size: usize = if (_.is_64bit) 32 else 20;
        if (_.data.len - _.pos < entry_size) {
            _.remaining = 0;
            return null;
        }

        const cputype = readU32(_.data, _.pos, _.is_big_endian);
        const cpusubtype = readU32(_.data, _.pos + 4, _.is_big_endian);
        let offset: u64 = 0;
        let size: u64 = 0;
        let align_pow2: u32 = 0;

        if (_.is_64bit) {
            offset = readU64(_.data, _.pos + 8, _.is_big_endian);
            size = readU64(_.data, _.pos + 16, _.is_big_endian);
            align_pow2 = readU32(_.data, _.pos + 24, _.is_big_endian);
        } else {
            offset = readU32(_.data, _.pos + 8, _.is_big_endian) as u64;
            size = readU32(_.data, _.pos + 12, _.is_big_endian) as u64;
            align_pow2 = readU32(_.data, _.pos + 16, _.is_big_endian);
        }

        _.pos += entry_size;
        _.remaining -= 1;

        return FatArchEntry {
            cputype: cputype,
            cpusubtype: cpusubtype,
            offset: offset,
            size: size,
            align_pow2: align_pow2,
        };
    }
}
