// Source: Absorbed from /data/dev/bun/src/exe_format/pe.cp

pub err Error {
    OutOfBounds,
    BadAlignment,
    Overflow,
    InvalidPEFile,
    InvalidDOSSignature,
    InvalidPESignature,
    UnsupportedPEFormat,
    InsufficientHeaderSpace,
    TooManySections,
    SectionExists,
    SectionNotFound,
    InvalidSectionName,
    InputIsSigned,
    InvalidSecurityDirectory,
    SecurityDirInsideImage,
    UnexpectedOverlayPresent,
    InvalidSectionData,
    InsufficientSpace,
    SizeOfImageMismatch,
}

pub enum StripMode: u8 {
    none = 0,
    strip_if_signed = 1,
    strip_always = 2,
}

pub struct StripOpts {
    require_overlay: bool = true,
    recompute_checksum: bool = true,
}

pub @c struct DOSHeader {
    e_magic: u16,
    e_cblp: u16,
    e_cp: u16,
    e_crlc: u16,
    e_cparhdr: u16,
    e_minalloc: u16,
    e_maxalloc: u16,
    e_ss: u16,
    e_sp: u16,
    e_csum: u16,
    e_ip: u16,
    e_cs: u16,
    e_lfarlc: u16,
    e_ovno: u16,
    e_res: [4]u16,
    e_oemid: u16,
    e_oeminfo: u16,
    e_res2: [10]u16,
    e_lfanew: u32,
}

pub @c struct PEHeader {
    signature: u32,
    machine: u16,
    number_of_sections: u16,
    time_date_stamp: u32,
    pointer_to_symbol_table: u32,
    number_of_symbols: u32,
    size_of_optional_header: u16,
    characteristics: u16,
}

pub @c struct DataDirectory {
    virtual_address: u32,
    size: u32,
}

pub @c struct OptionalHeader64 {
    magic: u16,
    major_linker_version: u8,
    minor_linker_version: u8,
    size_of_code: u32,
    size_of_initialized_data: u32,
    size_of_uninitialized_data: u32,
    address_of_entry_point: u32,
    base_of_code: u32,
    image_base: u64,
    section_alignment: u32,
    file_alignment: u32,
    major_operating_system_version: u16,
    minor_operating_system_version: u16,
    major_image_version: u16,
    minor_image_version: u16,
    major_subsystem_version: u16,
    minor_subsystem_version: u16,
    win32_version_value: u32,
    size_of_image: u32,
    size_of_headers: u32,
    checksum: u32,
    subsystem: u16,
    dll_characteristics: u16,
    size_of_stack_reserve: u64,
    size_of_stack_commit: u64,
    size_of_heap_reserve: u64,
    size_of_heap_commit: u64,
    loader_flags: u32,
    number_of_rva_and_sizes: u32,
    data_directories: [16]DataDirectory,
}

pub @c struct SectionHeader {
    name: [8]u8,
    virtual_size: u32,
    virtual_address: u32,
    size_of_raw_data: u32,
    pointer_to_raw_data: u32,
    pointer_to_relocations: u32,
    pointer_to_line_numbers: u32,
    number_of_relocations: u16,
    number_of_line_numbers: u16,
    characteristics: u32,
}

pub const DOS_SIGNATURE: u16 = 0x5A4D;
pub const PE_SIGNATURE: u32 = 0x00004550;
pub const OPTIONAL_HEADER_MAGIC_64: u16 = 0x020B;

pub const IMAGE_FILE_MACHINE_AMD64: u16 = 0x8664;
pub const IMAGE_FILE_MACHINE_ARM64: u16 = 0xAA64;
pub const IMAGE_FILE_EXECUTABLE_IMAGE: u16 = 0x0002;
pub const IMAGE_FILE_LARGE_ADDRESS_AWARE: u16 = 0x0020;

pub const IMAGE_SUBSYSTEM_WINDOWS_CUI: u16 = 3;

pub const IMAGE_SCN_CNT_CODE: u32 = 0x00000020;
pub const IMAGE_SCN_CNT_INITIALIZED_DATA: u32 = 0x00000040;
pub const IMAGE_SCN_MEM_EXECUTE: u32 = 0x20000000;
pub const IMAGE_SCN_MEM_READ: u32 = 0x40000000;
pub const IMAGE_SCN_MEM_WRITE: u32 = 0x80000000;

pub const IMAGE_DIRECTORY_ENTRY_SECURITY: usize = 4;
pub const IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY: u16 = 0x0080;

pub const MAX_SECTIONS: u16 = 96;

/// 判断 u32 是否为 2 的幂次
pub fn isPow2(x: u32): bool {
    return x != 0 && (x & (x - 1)) == 0;
}

/// 按 2 的幂次向上对齐 u32（带溢出与对齐校验）
pub fn alignUpU32(v: u32, a: u32): u32! {
    if (a == 0) {
        return v;
    }
    if (!isPow2(a)) {
        return err.BadAlignment;
    }
    const add = a - 1;
    if (v > std.math.maxInt(u32) - add) {
        return err.Overflow;
    }
    return (v + add) & ~add;
}

/// 按 2 的幂次向上对齐 usize（带溢出与对齐校验）
pub fn alignUpUsize(v: usize, a: usize): usize! {
    if (a == 0) {
        return v;
    }
    if ((a & (a - 1)) != 0) {
        return err.BadAlignment;
    }
    const add = a - 1;
    if (v > std.math.maxInt(usize) - add) {
        return err.Overflow;
    }
    return (v + add) & ~add;
}

/// 将不超过 8 字节的节名编码为定长 [8]u8（零填充）
pub fn encodeSectionName(name: string): [8]u8! {
    if (name.len == 0 || name.len > 8) {
        return err.InvalidSectionName;
    }
    let out: [8]u8 = [0; 8];
    let i: usize = 0;
    while (i < name.len) {
        out[i] = name[i];
        i += 1;
    }
    return out;
}

/// 计算标准 Windows PE 镜像校验和（跳过 checksum 字段的 4 字节）
pub fn computePEChecksum(raw_bytes: []const u8, checksum_off: usize): u32 {
    let sum: u64 = 0;
    let i: usize = 0;
    while (i + 1 < raw_bytes.len) {
        let b0: u8 = raw_bytes[i];
        let b1: u8 = raw_bytes[i + 1];
        if (i >= checksum_off && i < checksum_off + 4) {
            b0 = 0;
        }
        if (i + 1 >= checksum_off && i + 1 < checksum_off + 4) {
            b1 = 0;
        }
        const w: u16 = (b0 as u16) | ((b1 as u16) << 8);
        sum += w;
        sum = (sum & 0xffff) + (sum >> 16);
        i += 2;
    }
    if ((raw_bytes.len & 1) != 0) {
        const last_idx = raw_bytes.len - 1;
        if (last_idx < checksum_off || last_idx >= checksum_off + 4) {
            sum += raw_bytes[last_idx];
        }
    }
    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);
    sum += raw_bytes.len as u64;
    sum = (sum & 0xffff) + (sum >> 16);
    return ((sum & 0xffff) + (sum >> 16)) as u32;
}

fn readDosAt(buf: []const u8, off: usize): DOSHeader! {
    const sz: usize = @sizeOf(DOSHeader);
    if (off > buf.len || buf.len - off < sz) {
        return err.OutOfBounds;
    }
    let hdr: DOSHeader = std.mem.zeroes(DOSHeader);
    @memcpy(std.mem.asBytes(&hdr), buf[off .. off + sz]);
    return hdr;
}

fn readPEAt(buf: []const u8, off: usize): PEHeader! {
    const sz: usize = @sizeOf(PEHeader);
    if (off > buf.len || buf.len - off < sz) {
        return err.OutOfBounds;
    }
    let hdr: PEHeader = std.mem.zeroes(PEHeader);
    @memcpy(std.mem.asBytes(&hdr), buf[off .. off + sz]);
    return hdr;
}

fn readOptAt(buf: []const u8, off: usize): OptionalHeader64! {
    const sz: usize = @sizeOf(OptionalHeader64);
    if (off > buf.len || buf.len - off < sz) {
        return err.OutOfBounds;
    }
    let hdr: OptionalHeader64 = std.mem.zeroes(OptionalHeader64);
    @memcpy(std.mem.asBytes(&hdr), buf[off .. off + sz]);
    return hdr;
}

fn readSectionAt(buf: []const u8, off: usize): SectionHeader! {
    const sz: usize = @sizeOf(SectionHeader);
    if (off > buf.len || buf.len - off < sz) {
        return err.OutOfBounds;
    }
    let hdr: SectionHeader = std.mem.zeroes(SectionHeader);
    @memcpy(std.mem.asBytes(&hdr), buf[off .. off + sz]);
    return hdr;
}

/// 快速魔数检测：判断字节流是否具有合法的 DOS + PE 签名
pub fn isPE(data: []const u8): bool {
    const dos = readDosAt(data, 0) catch return false;
    if (dos.e_magic != DOS_SIGNATURE) {
        return false;
    }
    const pe_off = dos.e_lfanew as usize;
    if (pe_off < @sizeOf(DOSHeader)) {
        return false;
    }
    const pe = readPEAt(data, pe_off) catch return false;
    return pe.signature == PE_SIGNATURE;
}

/// Windows PE64 镜像解析、校验与节表修改器（RAII 自动释放底层缓冲）
pub struct PEFile {
    data: std.array_list.Managed(u8),
    dos_header_offset: usize,
    pe_header_offset: usize,
    optional_header_offset: usize,
    section_headers_offset: usize,
    num_sections: u16,
    first_raw: u32,
    last_file_end: u32,
    last_va_end: u32,

    ~init() {
        _.data.deinit();
    }

    /// 解析并校验 PE64 字节流，返回拥有独立缓冲区的 PEFile 实例
    static fn parse(allocator: std.mem.Allocator, pe_data: []const u8): PEFile! {
        if (pe_data.len < @sizeOf(DOSHeader)) {
            return err.InvalidPEFile;
        }

        const dos_header = readDosAt(pe_data, 0)?;
        if (dos_header.e_magic != DOS_SIGNATURE) {
            return err.InvalidDOSSignature;
        }

        if (dos_header.e_lfanew < @sizeOf(DOSHeader)) {
            return err.InvalidPEFile;
        }
        const pe_off = dos_header.e_lfanew as usize;
        if (pe_off > pe_data.len || pe_data.len - pe_off < @sizeOf(PEHeader)) {
            return err.InvalidPEFile;
        }

        const pe_header = readPEAt(pe_data, pe_off)?;
        if (pe_header.signature != PE_SIGNATURE) {
            return err.InvalidPESignature;
        }

        const optional_header_offset = pe_off + @sizeOf(PEHeader);
        const opt_sz = pe_header.size_of_optional_header as usize;
        if (opt_sz < @sizeOf(OptionalHeader64)) {
            return err.InvalidPEFile;
        }
        if (optional_header_offset > pe_data.len || pe_data.len - optional_header_offset < opt_sz) {
            return err.InvalidPEFile;
        }

        const optional_header = readOptAt(pe_data, optional_header_offset)?;
        if (optional_header.magic != OPTIONAL_HEADER_MAGIC_64) {
            return err.UnsupportedPEFormat;
        }

        if (!isPow2(optional_header.file_alignment) || !isPow2(optional_header.section_alignment)) {
            return err.BadAlignment;
        }
        if (optional_header.section_alignment < 4096) {
            if (optional_header.file_alignment != optional_header.section_alignment) {
                return err.InvalidPEFile;
            }
        }

        const section_headers_offset = optional_header_offset + opt_sz;
        const num_sections = pe_header.number_of_sections;
        if (num_sections > MAX_SECTIONS) {
            return err.TooManySections;
        }
        const section_headers_size = @sizeOf(SectionHeader) * (num_sections as usize);
        if (section_headers_offset > pe_data.len || pe_data.len - section_headers_offset < section_headers_size) {
            return err.InvalidPEFile;
        }

        let first_raw = pe_data.len as u32;
        let last_file_end: u32 = 0;
        let last_va_end: u32 = 0;

        const sh_sz: usize = @sizeOf(SectionHeader);
        let s_idx: usize = 0;
        while (s_idx < (num_sections as usize)) {
            const section = readSectionAt(pe_data, section_headers_offset + s_idx * sh_sz)?;
            if (section.size_of_raw_data > 0) {
                if (section.pointer_to_raw_data < first_raw) {
                    first_raw = section.pointer_to_raw_data;
                }
                const file_end = section.pointer_to_raw_data + section.size_of_raw_data;
                if (file_end > last_file_end) {
                    last_file_end = file_end;
                }
            }
            const vs_effective = if (section.virtual_size > section.size_of_raw_data) section.virtual_size else section.size_of_raw_data;
            const aligned_vs = alignUpU32(vs_effective, optional_header.section_alignment)?;
            const va_end = section.virtual_address + aligned_vs;
            if (va_end > last_va_end) {
                last_va_end = va_end;
            }
            s_idx += 1;
        }

        let buf = std.array_list.Managed(u8).initCapacity(allocator, pe_data.len + 4096)?;
        errdefer buf.deinit();
        buf.appendSlice(pe_data)?;

        return PEFile{
            data: buf,
            dos_header_offset: 0,
            pe_header_offset: pe_off,
            optional_header_offset: optional_header_offset,
            section_headers_offset: section_headers_offset,
            num_sections: num_sections,
            first_raw: first_raw,
            last_file_end: last_file_end,
            last_va_end: last_va_end,
        };
    }

    /// 合成一个最小合法 PE64 镜像（含 `.text` 节，满足 `validate()` 与正确校验和）
    static fn buildMinimalPE64(allocator: std.mem.Allocator, code: []const u8): PEFile! {
        const file_align: u32 = 512;
        const sect_align: u32 = 4096;
        const pe_off: u32 = 0x80;
        const headers_size: u32 = 0x400; // 1024 字节头区，预留充足节表空间

        const code_len: u32 = if (code.len == 0) 1 else (code.len as u32);
        const raw_size = alignUpU32(code_len, file_align)?;
        const text_va: u32 = sect_align;
        const text_va_aligned = alignUpU32(if (code_len > raw_size) code_len else raw_size, sect_align)?;
        const size_of_image = alignUpU32(text_va + text_va_aligned, sect_align)?;
        const total_size = (headers_size as usize) + (raw_size as usize);

        let raw = std.array_list.Managed(u8).initCapacity(allocator, total_size)?;
        defer raw.deinit();
        raw.resize(total_size)?;
        @memset(raw.items, 0);

        let dos: DOSHeader = std.mem.zeroes(DOSHeader);
        dos.e_magic = DOS_SIGNATURE;
        dos.e_lfanew = pe_off;
        @memcpy(raw.items[0..@sizeOf(DOSHeader)], std.mem.asBytes(&dos));

        let pe: PEHeader = std.mem.zeroes(PEHeader);
        pe.signature = PE_SIGNATURE;
        pe.machine = IMAGE_FILE_MACHINE_AMD64;
        pe.number_of_sections = 1;
        pe.size_of_optional_header = @sizeOf(OptionalHeader64) as u16;
        pe.characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE;
        const pe_start = pe_off as usize;
        @memcpy(raw.items[pe_start .. pe_start + @sizeOf(PEHeader)], std.mem.asBytes(&pe));

        let opt: OptionalHeader64 = std.mem.zeroes(OptionalHeader64);
        opt.magic = OPTIONAL_HEADER_MAGIC_64;
        opt.size_of_code = raw_size;
        opt.address_of_entry_point = text_va;
        opt.base_of_code = text_va;
        opt.image_base = 0x0000000140000000;
        opt.section_alignment = sect_align;
        opt.file_alignment = file_align;
        opt.major_operating_system_version = 6;
        opt.major_subsystem_version = 6;
        opt.size_of_image = size_of_image;
        opt.size_of_headers = headers_size;
        opt.subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
        opt.number_of_rva_and_sizes = 16;
        const opt_start = pe_start + @sizeOf(PEHeader);
        @memcpy(raw.items[opt_start .. opt_start + @sizeOf(OptionalHeader64)], std.mem.asBytes(&opt));

        let sh: SectionHeader = std.mem.zeroes(SectionHeader);
        sh.name = encodeSectionName(".text")?;
        sh.virtual_size = code_len;
        sh.virtual_address = text_va;
        sh.size_of_raw_data = raw_size;
        sh.pointer_to_raw_data = headers_size;
        sh.characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
        const sh_start = opt_start + @sizeOf(OptionalHeader64);
        @memcpy(raw.items[sh_start .. sh_start + @sizeOf(SectionHeader)], std.mem.asBytes(&sh));

        if (code.len > 0) {
            @memcpy(raw.items[(headers_size as usize) .. (headers_size as usize) + code.len], code);
        } else {
            raw.items[headers_size as usize] = 0xC3; // x86_64 RET
        }

        let pe_file = PEFile.parse(allocator, raw.items)?;
        errdefer pe_file.~init();
        pe_file.recomputeChecksum()?;
        return pe_file;
    }

    fn bytes(): []const u8 {
        return _.data.items;
    }

    fn sectionCount(): u16 {
        return _.num_sections;
    }

    fn getDosHeader(): DOSHeader! {
        return readDosAt(_.data.items, _.dos_header_offset);
    }

    fn getPEHeader(): PEHeader! {
        return readPEAt(_.data.items, _.pe_header_offset);
    }

    fn writePEHeader(*_, hdr: PEHeader)! {
        const sz: usize = @sizeOf(PEHeader);
        if (_.pe_header_offset + sz > _.data.items.len) {
            return err.OutOfBounds;
        }
        @memcpy(_.data.items[_.pe_header_offset .. _.pe_header_offset + sz], std.mem.asBytes(&hdr));
    }

    fn getOptionalHeader(): OptionalHeader64! {
        return readOptAt(_.data.items, _.optional_header_offset);
    }

    fn writeOptionalHeader(*_, hdr: OptionalHeader64)! {
        const sz: usize = @sizeOf(OptionalHeader64);
        if (_.optional_header_offset + sz > _.data.items.len) {
            return err.OutOfBounds;
        }
        @memcpy(_.data.items[_.optional_header_offset .. _.optional_header_offset + sz], std.mem.asBytes(&hdr));
    }

    fn checkSectionHeadersBounds()! {
        const size = @sizeOf(SectionHeader) * (_.num_sections as usize);
        if (_.section_headers_offset + size > _.data.items.len) {
            return err.OutOfBounds;
        }
    }

    fn getSectionHeader(idx: usize): SectionHeader! {
        if (idx >= (_.num_sections as usize)) {
            return err.OutOfBounds;
        }
        const off = _.section_headers_offset + idx * @sizeOf(SectionHeader);
        return readSectionAt(_.data.items, off);
    }

    /// 按节名查找节表项（最多 8 字节）
    fn findSection(name: string): SectionHeader? {
        const target = encodeSectionName(name) catch return null;
        let idx: usize = 0;
        while (idx < (_.num_sections as usize)) {
            const sh = _.getSectionHeader(idx) catch return null;
            if (std.mem.eql(u8, sh.name[0..8], target[0..8])) {
                return sh;
            }
            idx += 1;
        }
        return null;
    }

    /// 读取指定节名的原始文件内容切片
    fn getSectionRawData(name: string): ([]const u8)! {
        if (sh := _.findSection(name)) {
            const start = sh.pointer_to_raw_data as usize;
            const len = sh.size_of_raw_data as usize;
            if (start > _.data.items.len || _.data.items.len - start < len) {
                return err.InvalidSectionData;
            }
            return _.data.items[start .. start + len];
        }
        return err.SectionNotFound;
    }

    /// 重新计算并写回 PE64 镜像校验和
    fn recomputeChecksum(*_)! {
        const checksum_off = _.optional_header_offset + 64;
        if (checksum_off + 4 > _.data.items.len) {
            return err.OutOfBounds;
        }
        const final_sum = computePEChecksum(_.data.items, checksum_off);
        let opt = _.getOptionalHeader()?;
        opt.checksum = final_sum;
        _.writeOptionalHeader(opt)?;
    }

    /// 剥离 Authenticode 安全目录签名表
    fn stripAuthenticode(*_, opts: StripOpts)! {
        const opt = _.getOptionalHeader()?;
        const dd = opt.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY];
        const sec_off_u32 = dd.virtual_address;
        const sec_size_u32 = dd.size;

        if (sec_off_u32 == 0 || sec_size_u32 == 0) {
            return;
        }

        _.checkSectionHeadersBounds()?;
        let last_raw_end: u32 = 0;
        let s_idx: usize = 0;
        while (s_idx < (_.num_sections as usize)) {
            const s = _.getSectionHeader(s_idx)?;
            const end = s.pointer_to_raw_data + s.size_of_raw_data;
            if (end > last_raw_end) {
                last_raw_end = end;
            }
            s_idx += 1;
        }

        const file_len = _.data.items.len;
        const sec_off = sec_off_u32 as usize;
        const sec_size = sec_size_u32 as usize;

        if (sec_off >= file_len || sec_size == 0) {
            return err.InvalidSecurityDirectory;
        }
        if (opts.require_overlay && sec_off < (last_raw_end as usize)) {
            return err.SecurityDirInsideImage;
        }

        const end_raw = alignUpUsize(sec_off + sec_size, 8)?;
        if (end_raw > file_len) {
            return err.InvalidSecurityDirectory;
        }

        if (end_raw == file_len) {
            _.data.resize(sec_off)?;
        } else {
            const tail_len = file_len - end_raw;
            std.mem.copyBackwards(u8, _.data.items[sec_off .. sec_off + tail_len], _.data.items[end_raw..file_len]);
            _.data.resize(sec_off + tail_len)?;
        }

        let opt_after = _.getOptionalHeader()?;
        opt_after.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].virtual_address = 0;
        opt_after.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].size = 0;

        if ((opt_after.dll_characteristics & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY) != 0) {
            opt_after.dll_characteristics &= ~IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY;
        }

        _.writeOptionalHeader(opt_after)?;
        if (opts.recompute_checksum) {
            _.recomputeChecksum()?;
        }

        const after_strip_len = _.data.items.len;
        if ((last_raw_end as usize) < after_strip_len) {
            return err.UnexpectedOverlayPresent;
        }
    }

    /// 向 PE64 镜像追加新节（自动对齐虚拟地址/文件偏移、更新 SizeOfImage 并重算校验和）
    fn addSection(*_, name: string, payload: []const u8, characteristics: u32, strip: StripMode)! {
        const target_name = encodeSectionName(name)?;

        if (strip == .strip_always) {
            _.stripAuthenticode(StripOpts{ require_overlay: true, recompute_checksum: true })?;
        } else if (strip == .strip_if_signed) {
            const opt_check = _.getOptionalHeader()?;
            const dd = opt_check.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY];
            if (dd.virtual_address != 0 || dd.size != 0) {
                _.stripAuthenticode(StripOpts{ require_overlay: true, recompute_checksum: true })?;
            }
        }

        const opt = _.getOptionalHeader()?;
        _.checkSectionHeadersBounds()?;

        let s_idx: usize = 0;
        while (s_idx < (_.num_sections as usize)) {
            const section = _.getSectionHeader(s_idx)?;
            if (std.mem.eql(u8, section.name[0..8], target_name[0..8])) {
                return err.SectionExists;
            }
            s_idx += 1;
        }

        if (_.num_sections >= MAX_SECTIONS) {
            return err.TooManySections;
        }

        const new_headers_end = _.section_headers_offset + @sizeOf(SectionHeader) * ((_.num_sections as usize) + 1);
        const new_size_of_headers = alignUpU32(new_headers_end as u32, opt.file_alignment)?;

        let first_raw = _.data.items.len as u32;
        let last_file_end: u32 = 0;
        let last_va_end: u32 = 0;
        s_idx = 0;
        while (s_idx < (_.num_sections as usize)) {
            const section = _.getSectionHeader(s_idx)?;
            if (section.size_of_raw_data > 0) {
                if (section.pointer_to_raw_data < first_raw) {
                    first_raw = section.pointer_to_raw_data;
                }
                const file_end = section.pointer_to_raw_data + section.size_of_raw_data;
                if (file_end > last_file_end) {
                    last_file_end = file_end;
                }
            }
            const vs_effective = if (section.virtual_size > section.size_of_raw_data) section.virtual_size else section.size_of_raw_data;
            const aligned_vs = alignUpU32(vs_effective, opt.section_alignment)?;
            const va_end = section.virtual_address + aligned_vs;
            if (va_end > last_va_end) {
                last_va_end = va_end;
            }
            s_idx += 1;
        }

        if (new_size_of_headers > first_raw) {
            return err.InsufficientHeaderSpace;
        }

        if (payload.len > std.math.maxInt(u32)) {
            return err.Overflow;
        }
        const payload_len = payload.len as u32;
        const raw_size = alignUpU32(payload_len, opt.file_alignment)?;
        const new_va = alignUpU32(last_va_end, opt.section_alignment)?;
        const new_raw = alignUpU32(last_file_end, opt.file_alignment)?;

        const new_file_size = (new_raw as usize) + (raw_size as usize);
        _.data.resize(new_file_size)?;
        @memset(_.data.items[(new_raw as usize)..new_file_size], 0);

        const sh = SectionHeader{
            name: target_name,
            virtual_size: payload_len,
            virtual_address: new_va,
            size_of_raw_data: raw_size,
            pointer_to_raw_data: new_raw,
            pointer_to_relocations: 0,
            pointer_to_line_numbers: 0,
            number_of_relocations: 0,
            number_of_line_numbers: 0,
            characteristics: characteristics,
        };

        const new_sh_off = _.section_headers_offset + @sizeOf(SectionHeader) * (_.num_sections as usize);
        if (new_sh_off + @sizeOf(SectionHeader) > (first_raw as usize)) {
            return err.InsufficientHeaderSpace;
        }
        @memcpy(_.data.items[new_sh_off .. new_sh_off + @sizeOf(SectionHeader)], std.mem.asBytes(&sh));

        if (payload.len > 0) {
            @memcpy(_.data.items[(new_raw as usize) .. (new_raw as usize) + payload.len], payload);
        }

        let pe_after = _.getPEHeader()?;
        pe_after.number_of_sections += 1;
        _.writePEHeader(pe_after)?;
        _.num_sections += 1;
        _.last_file_end = new_raw + raw_size;

        let opt_after = _.getOptionalHeader()?;
        if (opt_after.size_of_headers < new_size_of_headers) {
            opt_after.size_of_headers = new_size_of_headers;
        }
        const vs_for_img = if (sh.virtual_size > sh.size_of_raw_data) sh.virtual_size else sh.size_of_raw_data;
        const section_va_end = new_va + alignUpU32(vs_for_img, opt_after.section_alignment)?;
        opt_after.size_of_image = alignUpU32(section_va_end, opt_after.section_alignment)?;
        _.last_va_end = opt_after.size_of_image;

        if (opt_after.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].virtual_address != 0 ||
            opt_after.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].size != 0)
        {
            opt_after.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].virtual_address = 0;
            opt_after.data_directories[IMAGE_DIRECTORY_ENTRY_SECURITY].size = 0;
        }
        _.writeOptionalHeader(opt_after)?;
        _.recomputeChecksum()?;
    }

    /// 全量结构合法性校验（签名、对齐、节表越界、节数据重叠与 SizeOfImage 一致性）
    fn validate()! {
        const dos_header = _.getDosHeader()?;
        if (dos_header.e_magic != DOS_SIGNATURE) {
            return err.InvalidDOSSignature;
        }

        const pe_header = _.getPEHeader()?;
        if (pe_header.signature != PE_SIGNATURE) {
            return err.InvalidPESignature;
        }

        const optional_header = _.getOptionalHeader()?;
        if (optional_header.magic != OPTIONAL_HEADER_MAGIC_64) {
            return err.UnsupportedPEFormat;
        }

        if (!isPow2(optional_header.file_alignment) || !isPow2(optional_header.section_alignment)) {
            return err.BadAlignment;
        }
        if (optional_header.section_alignment < 4096) {
            if (optional_header.file_alignment != optional_header.section_alignment) {
                return err.InvalidPEFile;
            }
        }

        const section_headers_end = _.section_headers_offset + @sizeOf(SectionHeader) * (_.num_sections as usize);
        if (section_headers_end > (optional_header.size_of_headers as usize) ||
            section_headers_end > _.data.items.len)
        {
            return err.InvalidPEFile;
        }

        _.checkSectionHeadersBounds()?;
        let max_va_end: u32 = 0;

        let i: usize = 0;
        while (i < (_.num_sections as usize)) {
            const section = _.getSectionHeader(i)?;
            if (section.size_of_raw_data > 0) {
                if (section.pointer_to_raw_data < optional_header.size_of_headers ||
                    (section.pointer_to_raw_data as usize) + (section.size_of_raw_data as usize) > _.data.items.len)
                {
                    return err.InvalidSectionData;
                }

                let j: usize = i + 1;
                while (j < (_.num_sections as usize)) {
                    const other = _.getSectionHeader(j)?;
                    if (other.size_of_raw_data > 0) {
                        const section_start = section.pointer_to_raw_data;
                        const section_end = section_start + section.size_of_raw_data;
                        const other_start = other.pointer_to_raw_data;
                        const other_end = other_start + other.size_of_raw_data;
                        const max_start = if (section_start > other_start) section_start else other_start;
                        const min_end = if (section_end < other_end) section_end else other_end;
                        if (max_start < min_end) {
                            return err.InvalidPEFile;
                        }
                    }
                    j += 1;
                }
            }

            const vs_effective = if (section.virtual_size > section.size_of_raw_data) section.virtual_size else section.size_of_raw_data;
            const aligned_vs = alignUpU32(vs_effective, optional_header.section_alignment)?;
            const va_end = section.virtual_address + aligned_vs;
            if (va_end > max_va_end) {
                max_va_end = va_end;
            }
            i += 1;
        }

        const expected_size_of_image = alignUpU32(max_va_end, optional_header.section_alignment)?;
        if (optional_header.size_of_image != expected_size_of_image) {
            return err.SizeOfImageMismatch;
        }
    }
}
