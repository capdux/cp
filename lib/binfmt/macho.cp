// Reference-hardened against gimli-rs/object (MIT/Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc Apple Mach-O & Fat Binary Parser + 16-Lane SIMD Scanner) | Created: 2026-11-22

type ObjVec16u8 = @Vector(16, u8);

/// Apple Mach-O / Fat Universal 二进制格式种类枚举
pub enum MachoKind: u8 {
    thin32 = 0,
    thin64 = 1,
    fat32 = 2,
    fat64 = 3,
    unknown = 4,
}

/// Mach-O & Fat Binary 魔数常量（大端视角读取）
pub const MH_MAGIC: u32 = 0xFEEDFACE;
pub const MH_CIGAM: u32 = 0xCEFAEDFE;
pub const MH_MAGIC_64: u32 = 0xFEEDFACF;
pub const MH_CIGAM_64: u32 = 0xCFFAEDFE;
pub const FAT_MAGIC: u32 = 0xCAFEBABE;
pub const FAT_CIGAM: u32 = 0xBEBAFECA;
pub const FAT_MAGIC_64: u32 = 0xCAFEBABF;
pub const FAT_CIGAM_64: u32 = 0xBFBAFECA;

/// Load Command 类型常量
pub const LC_SEGMENT: u32 = 0x00000001;
pub const LC_SEGMENT_64: u32 = 0x00000019;

/// 安全边界上限（防资源耗尽与整数溢出）
pub const MACHO_MAX_CMDS: u32 = 4096;
pub const MACHO_MAX_SECTIONS_PER_SEG: u32 = 4096;
pub const MACHO_MAX_FAT_ARCHS: u32 = 64;

pub const MACHO_SEG32_CMD_SIZE: usize = 56;
pub const MACHO_SECT32_SIZE: usize = 68;
pub const MACHO_SEG64_CMD_SIZE: usize = 72;
pub const MACHO_SECT64_SIZE: usize = 80;

/// CPU 类型常量
pub const CPU_TYPE_X86_64: u32 = 0x01000007;
pub const CPU_TYPE_ARM64: u32 = 0x0100000C;

/// Mach-O 文件类型常量
pub const MH_EXECUTE: u32 = 2;
pub const MH_DYLIB: u32 = 6;

pub err MachoError {
    InvalidMagic,
    TruncatedHeader,
    TooManyLoadCommands,
    SizeofCmdsOutOfBounds,
    InvalidCmdSize,
    UnalignedCmdSize,
    InvalidSegmentCommand,
    SectionOutOfBounds,
    TooManyFatArchs,
    FatArchOutOfBounds,
    FatArchOverlap,
}

// ============================================================================
// 16-Lane @Vector(16, u8) SIMD Primitives + Scalar Differential References
// ============================================================================

fn objLoadVec16(data: []const u8, off: usize): ObjVec16u8 {
    const v: ObjVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD NUL trimmer for Mach-O 16B segname/sectname and COFF 8B names.
/// Returns the sub-slice up to the first `0x00` byte, or the full slice if no NUL is present.
pub fn simdObjTrimNul16(data: []const u8): []const u8 {
    const zero_v: ObjVec16u8 = @splat(0);
    let i: usize = 0;
    while (i + 16 <= data.len) {
        const v = objLoadVec16(data, i);
        if (@reduce(.Or, v == zero_v)) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                if (data[i + j] == 0) {
                    return data[0 .. i + j];
                }
            }
        }
        i += 16;
    }
    while (i < data.len) : (i += 1) {
        if (data[i] == 0) {
            return data[0..i];
        }
    }
    return data;
}

/// Scalar reference for `simdObjTrimNul16` (differential verification).
pub fn scalarObjTrimNul16(data: []const u8): []const u8 {
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (data[i] == 0) {
            return data[0..i];
        }
    }
    return data;
}

/// 16-lane `@Vector(16, u8)` SIMD byte slice equality check for segment/section names & magics.
pub fn simdObjBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va = objLoadVec16(a, i);
        const vb = objLoadVec16(b, i);
        if (!@reduce(.And, va == vb)) return false;
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Scalar reference for `simdObjBytesEqual` (differential verification).
pub fn scalarObjBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

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

/// 零拷贝 Mach-O Segment (`LC_SEGMENT` / `LC_SEGMENT_64`) 视图
pub struct MachoSegment {
    segname_raw: [16]u8,
    vmaddr: u64,
    vmsize: u64,
    fileoff: u64,
    filesize: u64,
    maxprot: u32,
    initprot: u32,
    nsects: u32,
    flags: u32,
    is_64bit: bool,
    is_big_endian: bool,

    pub fn name(): []const u8 {
        return simdObjTrimNul16(_.segname_raw[0..16]);
    }

    pub fn matchesName(target: []const u8): bool {
        return simdObjBytesEqual(_.name(), target);
    }
}

/// 零拷贝 Mach-O Section (`section` / `section_64`) 视图
pub struct MachoSection {
    sectname_raw: [16]u8,
    segname_raw: [16]u8,
    addr: u64,
    size: u64,
    offset: u32,
    align_pow2: u32,
    reloff: u32,
    nreloc: u32,
    flags: u32,

    pub fn sectName(): []const u8 {
        return simdObjTrimNul16(_.sectname_raw[0..16]);
    }

    pub fn segName(): []const u8 {
        return simdObjTrimNul16(_.segname_raw[0..16]);
    }
}

/// Apple Fat Universal Binary 架构切片条目
pub struct FatArchEntry {
    cputype: u32,
    cpusubtype: u32,
    offset: u64,
    size: u64,
    align_pow2: u32,
}

fn copy16(src: []const u8, off: usize): [16]u8 {
    let out: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        out[i] = src[off + i];
    }
    return out;
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
    if (magic_be == MH_MAGIC || magic_be == MH_CIGAM) return MachoKind.thin32;
    if (magic_be == MH_MAGIC_64 || magic_be == MH_CIGAM_64) return MachoKind.thin64;
    if (magic_be == FAT_MAGIC || magic_be == FAT_CIGAM) return MachoKind.fat32;
    if (magic_be == FAT_MAGIC_64 || magic_be == FAT_CIGAM_64) return MachoKind.fat64;
    return MachoKind.unknown;
}

/// 解析 Thin Mach-O 32/64-bit 头部（自动识别大端/小端字节序并校验 `>= 28/32` 字节长度）
pub fn parseMachoHeader(data: []const u8): MachoHeader? {
    if (data.len < 28) return null;
    const magic_be = readU32BE(data, 0);

    let is_64bit = false;
    let is_big_endian = false;

    if (magic_be == MH_MAGIC) {
        is_64bit = false;
        is_big_endian = true;
    } else if (magic_be == MH_CIGAM) {
        is_64bit = false;
        is_big_endian = false;
    } else if (magic_be == MH_MAGIC_64) {
        is_64bit = true;
        is_big_endian = true;
    } else if (magic_be == MH_CIGAM_64) {
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

/// 零堆分配 Mach-O Load Command 流式迭代器（防越界、对齐与非法 `cmdsize` 保护）
pub struct MachoLoadCmdIter {
    data: []const u8,
    pos: usize = 0,
    limit: usize = 0,
    remaining: u32 = 0,
    is_64bit: bool = false,
    is_big_endian: bool = false,

    pub static fn init(data: []const u8): MachoLoadCmdIter {
        if (hdr := parseMachoHeader(data)) {
            if (hdr.ncmds > MACHO_MAX_CMDS) {
                return MachoLoadCmdIter {
                    data: data,
                    pos: 0,
                    limit: 0,
                    remaining: 0,
                    is_64bit: hdr.is_64bit,
                    is_big_endian: hdr.is_big_endian,
                };
            }
            const h_size = hdr.headerSize();
            const avail = data.len - h_size;
            const declared = hdr.sizeofcmds as usize;
            const bounded = if (declared <= avail) declared else avail;
            return MachoLoadCmdIter {
                data: data,
                pos: h_size,
                limit: h_size + bounded,
                remaining: hdr.ncmds,
                is_64bit: hdr.is_64bit,
                is_big_endian: hdr.is_big_endian,
            };
        }
        return MachoLoadCmdIter {
            data: data,
            pos: 0,
            limit: 0,
            remaining: 0,
            is_64bit: false,
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
        const align_req: u32 = if (_.is_64bit) 8 else 4;

        if (cmdsize < 8 || (cmdsize % align_req) != 0 || cmdsize_usize > (_.limit - _.pos)) {
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

/// Parses an `LC_SEGMENT` (32-bit) or `LC_SEGMENT_64` (64-bit) load command payload with overflow guards.
pub fn parseMachoSegment(cmd: MachoLoadCmd, is_64bit: bool, is_big_endian: bool): MachoSegment? {
    if (is_64bit) {
        if (cmd.cmd != LC_SEGMENT_64) return null;
        const payload_hdr: usize = MACHO_SEG64_CMD_SIZE - 8;
        if (cmd.payload.len < payload_hdr) return null;
        const nsects = readU32(cmd.payload, 56, is_big_endian);
        if (nsects > MACHO_MAX_SECTIONS_PER_SEG) return null;
        const sects_bytes: u64 = (nsects as u64) * (MACHO_SECT64_SIZE as u64);
        if (sects_bytes > ((cmd.payload.len - payload_hdr) as u64)) return null;
        return MachoSegment {
            segname_raw: copy16(cmd.payload, 0),
            vmaddr: readU64(cmd.payload, 16, is_big_endian),
            vmsize: readU64(cmd.payload, 24, is_big_endian),
            fileoff: readU64(cmd.payload, 32, is_big_endian),
            filesize: readU64(cmd.payload, 40, is_big_endian),
            maxprot: readU32(cmd.payload, 48, is_big_endian),
            initprot: readU32(cmd.payload, 52, is_big_endian),
            nsects: nsects,
            flags: readU32(cmd.payload, 60, is_big_endian),
            is_64bit: true,
            is_big_endian: is_big_endian,
        };
    }

    if (cmd.cmd != LC_SEGMENT) return null;
    const payload_hdr: usize = MACHO_SEG32_CMD_SIZE - 8;
    if (cmd.payload.len < payload_hdr) return null;
    const nsects = readU32(cmd.payload, 40, is_big_endian);
    if (nsects > MACHO_MAX_SECTIONS_PER_SEG) return null;
    const sects_bytes: u64 = (nsects as u64) * (MACHO_SECT32_SIZE as u64);
    if (sects_bytes > ((cmd.payload.len - payload_hdr) as u64)) return null;
    return MachoSegment {
        segname_raw: copy16(cmd.payload, 0),
        vmaddr: readU32(cmd.payload, 16, is_big_endian) as u64,
        vmsize: readU32(cmd.payload, 20, is_big_endian) as u64,
        fileoff: readU32(cmd.payload, 24, is_big_endian) as u64,
        filesize: readU32(cmd.payload, 28, is_big_endian) as u64,
        maxprot: readU32(cmd.payload, 32, is_big_endian),
        initprot: readU32(cmd.payload, 36, is_big_endian),
        nsects: nsects,
        flags: readU32(cmd.payload, 44, is_big_endian),
        is_64bit: false,
        is_big_endian: is_big_endian,
    };
}

/// Parses section `idx` inside a validated `MachoSegment` load command.
pub fn parseMachoSectionAt(cmd: MachoLoadCmd, seg: MachoSegment, idx: usize): MachoSection? {
    if (idx >= (seg.nsects as usize)) return null;
    if (seg.is_64bit) {
        const base: usize = (MACHO_SEG64_CMD_SIZE - 8) + idx * MACHO_SECT64_SIZE;
        if (base + MACHO_SECT64_SIZE > cmd.payload.len) return null;
        return MachoSection {
            sectname_raw: copy16(cmd.payload, base),
            segname_raw: copy16(cmd.payload, base + 16),
            addr: readU64(cmd.payload, base + 32, seg.is_big_endian),
            size: readU64(cmd.payload, base + 40, seg.is_big_endian),
            offset: readU32(cmd.payload, base + 48, seg.is_big_endian),
            align_pow2: readU32(cmd.payload, base + 52, seg.is_big_endian),
            reloff: readU32(cmd.payload, base + 56, seg.is_big_endian),
            nreloc: readU32(cmd.payload, base + 60, seg.is_big_endian),
            flags: readU32(cmd.payload, base + 64, seg.is_big_endian),
        };
    }

    const base: usize = (MACHO_SEG32_CMD_SIZE - 8) + idx * MACHO_SECT32_SIZE;
    if (base + MACHO_SECT32_SIZE > cmd.payload.len) return null;
    return MachoSection {
        sectname_raw: copy16(cmd.payload, base),
        segname_raw: copy16(cmd.payload, base + 16),
        addr: readU32(cmd.payload, base + 32, seg.is_big_endian) as u64,
        size: readU32(cmd.payload, base + 36, seg.is_big_endian) as u64,
        offset: readU32(cmd.payload, base + 40, seg.is_big_endian),
        align_pow2: readU32(cmd.payload, base + 44, seg.is_big_endian),
        reloff: readU32(cmd.payload, base + 48, seg.is_big_endian),
        nreloc: readU32(cmd.payload, base + 52, seg.is_big_endian),
        flags: readU32(cmd.payload, base + 56, seg.is_big_endian),
    };
}

/// Finds a Mach-O section by segment name and section name using 16-lane SIMD NUL trimming & comparison.
pub fn findMachoSection(data: []const u8, seg_name: []const u8, sect_name: []const u8): MachoSection? {
    const hdr = parseMachoHeader(data) ?? return null;
    let it = MachoLoadCmdIter.init(data);
    while (cmd := it.next()) {
        if (seg := parseMachoSegment(cmd, hdr.is_64bit, hdr.is_big_endian)) {
            if (!seg.matchesName(seg_name)) continue;
            let si: usize = 0;
            while (si < (seg.nsects as usize)) : (si += 1) {
                if (sec := parseMachoSectionAt(cmd, seg, si)) {
                    if (simdObjBytesEqual(sec.sectName(), sect_name)) {
                        return sec;
                    }
                }
            }
        }
    }
    return null;
}

/// Safely slices a Fat/Universal Binary architecture slice with overflow-checked bounds.
pub fn extractFatArchSlice(data: []const u8, entry: FatArchEntry): ([]const u8)? {
    const total: u64 = data.len as u64;
    if (entry.offset > total || entry.size > total - entry.offset) {
        return null;
    }
    const start = entry.offset as usize;
    const len = entry.size as usize;
    return data[start .. start + len];
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

        if (magic_be == FAT_MAGIC) {
            is_64 = false;
            is_be = true;
        } else if (magic_be == FAT_CIGAM) {
            is_64 = false;
            is_be = false;
        } else if (magic_be == FAT_MAGIC_64) {
            is_64 = true;
            is_be = true;
        } else if (magic_be == FAT_CIGAM_64) {
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

struct FatSliceRange {
    start: u64 = 0,
    end: u64 = 0,
}

fn validateThinMachoStrict(data: []const u8): MachoError!void {
    if (data.len < 28) return err.TruncatedHeader;
    const hdr = parseMachoHeader(data) ?? return err.InvalidMagic;
    const h_size = hdr.headerSize();
    if (data.len < h_size) return err.TruncatedHeader;
    if (hdr.ncmds > MACHO_MAX_CMDS) return err.TooManyLoadCommands;

    const avail: u64 = (data.len - h_size) as u64;
    const declared_cmds: u64 = hdr.sizeofcmds as u64;
    if (declared_cmds > avail) return err.SizeofCmdsOutOfBounds;

    const limit: usize = h_size + (hdr.sizeofcmds as usize);
    const align_req: u32 = if (hdr.is_64bit) 8 else 4;
    const total_u64: u64 = data.len as u64;

    let pos: usize = h_size;
    let ci: u32 = 0;
    while (ci < hdr.ncmds) : (ci += 1) {
        if (limit - pos < 8) return err.InvalidCmdSize;
        const cmd_type = readU32(data, pos, hdr.is_big_endian);
        const cmdsize = readU32(data, pos + 4, hdr.is_big_endian);
        if (cmdsize < 8) return err.InvalidCmdSize;
        if ((cmdsize % align_req) != 0) return err.UnalignedCmdSize;
        const cmdsize_usize = cmdsize as usize;
        if (cmdsize_usize > limit - pos) return err.InvalidCmdSize;

        const cmd = MachoLoadCmd {
            cmd: cmd_type,
            cmdsize: cmdsize,
            payload: data[pos + 8 .. pos + cmdsize_usize],
        };

        if (cmd_type == LC_SEGMENT || cmd_type == LC_SEGMENT_64) {
            const seg = parseMachoSegment(cmd, hdr.is_64bit, hdr.is_big_endian) ?? return err.InvalidSegmentCommand;
            if (seg.filesize > 0) {
                if (seg.fileoff > total_u64 || seg.filesize > total_u64 - seg.fileoff) {
                    return err.SectionOutOfBounds;
                }
            }
            let si: usize = 0;
            while (si < (seg.nsects as usize)) : (si += 1) {
                const sec = parseMachoSectionAt(cmd, seg, si) ?? return err.InvalidSegmentCommand;
                // S_ZEROFILL (0x1), S_GB_ZEROFILL (0xC), S_THREAD_LOCAL_ZEROFILL (0x12) have no file bytes
                const sec_type = sec.flags & 0xFF;
                const is_zerofill = (sec_type == 0x1 || sec_type == 0xC || sec_type == 0x12);
                if (!is_zerofill && sec.size > 0) {
                    const off_u64: u64 = sec.offset as u64;
                    if (off_u64 > total_u64 || sec.size > total_u64 - off_u64) {
                        return err.SectionOutOfBounds;
                    }
                }
            }
        }
        pos += cmdsize_usize;
    }
}

/// Zero-allocation strict validator for Thin Mach-O (32/64-bit LE/BE) and Fat/Universal Binaries,
/// aligned with `gimli-rs/object` bounds, alignment, overflow, and non-overlap invariants.
pub fn validateMachoStrict(data: []const u8): MachoError!void {
    if (data.len < 4) return err.TruncatedHeader;
    const kind = detectMachoKind(data);
    if (kind == MachoKind.unknown) return err.InvalidMagic;

    if (kind == MachoKind.thin32 || kind == MachoKind.thin64) {
        return validateThinMachoStrict(data);
    }

    // Fat / Universal Binary (32-bit or 64-bit)
    if (data.len < 8) return err.TruncatedHeader;
    const is_64 = (kind == MachoKind.fat64);
    const magic_be = readU32BE(data, 0);
    const is_be = (magic_be == FAT_MAGIC || magic_be == FAT_MAGIC_64);
    const nfat = readU32(data, 4, is_be);
    if (nfat == 0 || nfat > MACHO_MAX_FAT_ARCHS) return err.TooManyFatArchs;

    const entry_size: u64 = if (is_64) 32 else 20;
    const table_bytes: u64 = (nfat as u64) * entry_size;
    const total_u64: u64 = data.len as u64;
    if (table_bytes > total_u64 - 8) return err.FatArchOutOfBounds;
    const table_end: u64 = table_bytes + (8 as u64);

    let ranges: [MACHO_MAX_FAT_ARCHS]FatSliceRange = [FatSliceRange{}; MACHO_MAX_FAT_ARCHS];
    let count: usize = 0;

    let it = FatArchIter.init(data);
    while (entry := it.next()) {
        if (entry.size == 0 || entry.offset < table_end) {
            return err.FatArchOutOfBounds;
        }
        if (entry.offset > total_u64 || entry.size > total_u64 - entry.offset) {
            return err.FatArchOutOfBounds;
        }
        const s_start = entry.offset;
        const s_end = entry.offset + entry.size;
        let ri: usize = 0;
        while (ri < count) : (ri += 1) {
            if (s_start < ranges[ri].end && ranges[ri].start < s_end) {
                return err.FatArchOverlap;
            }
        }
        ranges[count] = FatSliceRange{ start: s_start, end: s_end };
        count += 1;

        const slice = data[(s_start as usize)..(s_end as usize)];
        validateThinMachoStrict(slice)?;
    }

    if (count != (nfat as usize)) return err.FatArchOutOfBounds;
}
