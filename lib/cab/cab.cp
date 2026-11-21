// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Microsoft Cabinet .cab Archive Parser)

/// Microsoft Cabinet (MS-CAB) 归档常量与标志位
pub const CAB_MAGIC: []const u8 = "MSCF";
pub const CAB_HEADER_SIZE: usize = 36;
pub const CAB_FOLDER_SIZE: usize = 8;
pub const CAB_FILE_FIXED_SIZE: usize = 16;

pub const CAB_FLAG_PREV: u16 = 0x0001;
pub const CAB_FLAG_NEXT: u16 = 0x0002;
pub const CAB_FLAG_RESERVE: u16 = 0x0004;

/// CFFILE 文件属性标志位
pub const CAB_ATTR_RDONLY: u16 = 0x0001;
pub const CAB_ATTR_HIDDEN: u16 = 0x0002;
pub const CAB_ATTR_SYSTEM: u16 = 0x0004;
pub const CAB_ATTR_ARCH: u16 = 0x0020;
pub const CAB_ATTR_EXEC: u16 = 0x0040;
pub const CAB_ATTR_NAME_IS_UTF: u16 = 0x0080;

/// CAB 文件夹压缩算法枚举（按 `type_compress & 0x000F` 解码）
pub enum CabCompression: u8 {
    none = 0,
    mszip = 1,
    quantum = 2,
    lzx = 3,
    unknown = 255,
}

/// 将 `CFFOLDER.type_compress` 低 4 位解码为 `CabCompression` 枚举
pub fn decodeCabCompression(type_compress: u16): CabCompression {
    const method = type_compress & 0x000F;
    if (method == 0) return CabCompression.none;
    if (method == 1) return CabCompression.mszip;
    if (method == 2) return CabCompression.quantum;
    if (method == 3) return CabCompression.lzx;
    return CabCompression.unknown;
}

/// `CFHEADER` 归档头解析结果（包含可选保留区与前/后卷字符串之后精确的 `folders_offset`）
pub struct CabHeader {
    cb_cabinet: u32,
    coff_files: u32,
    version_minor: u8,
    version_major: u8,
    c_folders: u16,
    c_files: u16,
    flags: u16,
    set_id: u16,
    i_cabinet: u16,
    cb_cf_header: u16,
    cb_cf_folder: u8,
    cb_cf_data: u8,
    folders_offset: usize,
}

/// `CFFOLDER` 文件夹条目视图
pub struct CabFolder {
    coff_cab_start: u32,
    c_cf_data: u16,
    type_compress: u16,
    compression: CabCompression,
}

/// `CFFILE` 文件条目零拷贝视图
pub struct CabFile {
    cb_file: u32,
    uoff_folder_start: u32,
    i_folder: u16,
    date: u16,
    time: u16,
    attribs: u16,
    name: []const u8,

    /// 文件名是否采用 UTF-8 编码（`_A_NAME_IS_UTF`, 0x0080）
    fn isUtf8Name(): bool {
        return (_.attribs & CAB_ATTR_NAME_IS_UTF) != 0;
    }

    /// 文件是否为只读属性（`_A_RDONLY`, 0x0001）
    fn isReadOnly(): bool {
        return (_.attribs & CAB_ATTR_RDONLY) != 0;
    }

    /// 文件是否为隐藏属性（`_A_HIDDEN`, 0x0002）
    fn isHidden(): bool {
        return (_.attribs & CAB_ATTR_HIDDEN) != 0;
    }
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn readU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

/// 跳过一个以 `\0` 结尾的字符串，返回 `\0` 之后的偏移；未闭合则返回 `null`
fn skipCString(data: []const u8, start: usize): usize? {
    let i = start;
    while (i < data.len) {
        if (data[i] == 0) {
            return i + 1;
        }
        i += 1;
    }
    return null;
}

/// 校验字节切片是否以合法 MS-CAB 头部开头：
/// `data.len >= 36`、首 4B 为 `"MSCF"`、`reserved1 == 0`、`version_major == 1 && version_minor == 3`
pub fn isCab(data: []const u8): bool {
    if (data.len < CAB_HEADER_SIZE) return false;
    if (!strEq(data[0..4], CAB_MAGIC)) return false;
    if (readU32LE(data, 4) != 0) return false;
    if (data[24] != 3 || data[25] != 1) return false;
    return true;
}

/// 解析 36B `CFHEADER` 及其可选扩展区域（`CAB_FLAG_RESERVE` / `CAB_FLAG_PREV` / `CAB_FLAG_NEXT`），
/// 并计算精确的 `folders_offset`
pub fn parseCabHeader(data: []const u8): CabHeader? {
    if (!isCab(data)) return null;

    const cb_cabinet = readU32LE(data, 8);
    const coff_files = readU32LE(data, 16);
    const version_minor = data[24];
    const version_major = data[25];
    const c_folders = readU16LE(data, 26);
    const c_files = readU16LE(data, 28);
    const flags = readU16LE(data, 30);
    const set_id = readU16LE(data, 32);
    const i_cabinet = readU16LE(data, 34);

    if (c_files > 0 && (coff_files as u64) > (data.len as u64)) {
        return null;
    }

    let pos: usize = CAB_HEADER_SIZE;
    let cb_cf_header: u16 = 0;
    let cb_cf_folder: u8 = 0;
    let cb_cf_data: u8 = 0;

    if ((flags & CAB_FLAG_RESERVE) != 0) {
        if (data.len - pos < 4) return null;
        cb_cf_header = readU16LE(data, pos);
        cb_cf_folder = data[pos + 2];
        cb_cf_data = data[pos + 3];
        pos += 4;
        const reserve_len = cb_cf_header as usize;
        if (reserve_len > data.len - pos) return null;
        pos += reserve_len;
    }

    if ((flags & CAB_FLAG_PREV) != 0) {
        if (next_pos := skipCString(data, pos)) {
            pos = next_pos;
        } else {
            return null;
        }
        if (next_pos := skipCString(data, pos)) {
            pos = next_pos;
        } else {
            return null;
        }
    }

    if ((flags & CAB_FLAG_NEXT) != 0) {
        if (next_pos := skipCString(data, pos)) {
            pos = next_pos;
        } else {
            return null;
        }
        if (next_pos := skipCString(data, pos)) {
            pos = next_pos;
        } else {
            return null;
        }
    }

    return CabHeader {
        cb_cabinet: cb_cabinet,
        coff_files: coff_files,
        version_minor: version_minor,
        version_major: version_major,
        c_folders: c_folders,
        c_files: c_files,
        flags: flags,
        set_id: set_id,
        i_cabinet: i_cabinet,
        cb_cf_header: cb_cf_header,
        cb_cf_folder: cb_cf_folder,
        cb_cf_data: cb_cf_data,
        folders_offset: pos,
    };
}

/// 零堆分配 `CFFOLDER` 迭代器（自动按 `8 + cb_cf_folder` 步进）
pub struct CabFolderIter {
    data: []const u8,
    pos: usize,
    index: u16 = 0,
    count: u16,
    entry_size: usize,
    had_error: bool = false,

    static fn init(data: []const u8, header: CabHeader): CabFolderIter {
        const step = CAB_FOLDER_SIZE + (header.cb_cf_folder as usize);
        if (header.folders_offset > data.len) {
            return CabFolderIter {
                data: data,
                pos: data.len,
                index: 0,
                count: 0,
                entry_size: step,
                had_error: true,
            };
        }
        return CabFolderIter {
            data: data,
            pos: header.folders_offset,
            index: 0,
            count: header.c_folders,
            entry_size: step,
            had_error: false,
        };
    }

    static fn fromData(data: []const u8): CabFolderIter {
        if (hdr := parseCabHeader(data)) {
            return CabFolderIter.init(data, hdr);
        }
        return CabFolderIter {
            data: data,
            pos: 0,
            index: 0,
            count: 0,
            entry_size: CAB_FOLDER_SIZE,
            had_error: true,
        };
    }

    fn next(*_): CabFolder? {
        if (_.had_error || _.index >= _.count) return null;
        if (_.pos > _.data.len || _.entry_size > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const off = _.pos;
        const coff_cab_start = readU32LE(_.data, off);
        const c_cf_data = readU16LE(_.data, off + 4);
        const type_compress = readU16LE(_.data, off + 6);

        _.pos += _.entry_size;
        _.index += 1;

        return CabFolder {
            coff_cab_start: coff_cab_start,
            c_cf_data: c_cf_data,
            type_compress: type_compress,
            compression: decodeCabCompression(type_compress),
        };
    }
}

/// 零堆分配 `CFFILE` 迭代器（从 `coff_files` 起迭代 `c_files` 个文件条目并解析 `\0` 结尾文件名）
pub struct CabFileIter {
    data: []const u8,
    pos: usize,
    index: u16 = 0,
    count: u16,
    had_error: bool = false,

    static fn init(data: []const u8, header: CabHeader): CabFileIter {
        const start_u64 = header.coff_files as u64;
        if (header.c_files > 0 && start_u64 > (data.len as u64)) {
            return CabFileIter {
                data: data,
                pos: data.len,
                index: 0,
                count: 0,
                had_error: true,
            };
        }
        const start_pos: usize = if (start_u64 <= (data.len as u64)) (start_u64 as usize) else data.len;
        return CabFileIter {
            data: data,
            pos: start_pos,
            index: 0,
            count: header.c_files,
            had_error: false,
        };
    }

    static fn fromData(data: []const u8): CabFileIter {
        if (hdr := parseCabHeader(data)) {
            return CabFileIter.init(data, hdr);
        }
        return CabFileIter {
            data: data,
            pos: 0,
            index: 0,
            count: 0,
            had_error: true,
        };
    }

    fn next(*_): CabFile? {
        if (_.had_error || _.index >= _.count) return null;
        if (_.pos > _.data.len || CAB_FILE_FIXED_SIZE > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const off = _.pos;
        const cb_file = readU32LE(_.data, off);
        const uoff_folder_start = readU32LE(_.data, off + 4);
        const i_folder = readU16LE(_.data, off + 8);
        const date = readU16LE(_.data, off + 10);
        const time = readU16LE(_.data, off + 12);
        const attribs = readU16LE(_.data, off + 14);

        const name_start = off + CAB_FILE_FIXED_SIZE;
        let name_end: usize = name_start;
        let found_nul = false;
        while (name_end < _.data.len) {
            if (_.data[name_end] == 0) {
                found_nul = true;
                break;
            }
            name_end += 1;
        }
        if (!found_nul) {
            _.had_error = true;
            return null;
        }

        const name = _.data[name_start..name_end];
        _.pos = name_end + 1;
        _.index += 1;

        return CabFile {
            cb_file: cb_file,
            uoff_folder_start: uoff_folder_start,
            i_folder: i_folder,
            date: date,
            time: time,
            attribs: attribs,
            name: name,
        };
    }
}

/// 按精确文件名在 `.cab` 归档中查找首个匹配的 `CFFILE` 条目
pub fn findCabFile(data: []const u8, name: []const u8): CabFile? {
    if (hdr := parseCabHeader(data)) {
        let it = CabFileIter.init(data, hdr);
        while (file := it.next()) {
            if (strEq(file.name, name)) return file;
        }
    }
    return null;
}
