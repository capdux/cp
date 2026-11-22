// Origin: Pure-CP Stdlib (Zero-Alloc ISO 9660 Volume Descriptor & Directory Parser) | Created: 2026-11-22

/// ISO 9660 (ECMA-119) 标准逻辑扇区大小与卷描述符常量
pub const ISO_SECTOR_SIZE: usize = 2048;
pub const ISO_SYSTEM_AREA_SECTORS: usize = 16;
pub const ISO_PVD_OFFSET: usize = 32768;
pub const ISO_MAGIC: []const u8 = "CD001";

pub const VD_TYPE_BOOT: u8 = 0;
pub const VD_TYPE_PRIMARY: u8 = 1;
pub const VD_TYPE_SUPPLEMENTARY: u8 = 2;
pub const VD_TYPE_TERMINATOR: u8 = 255;

/// ISO 9660 目录记录标志位
pub const ISO_FLAG_HIDDEN: u8 = 0x01;
pub const ISO_FLAG_DIRECTORY: u8 = 0x02;

/// ISO 9660 主卷描述符（Primary Volume Descriptor, PVD）零拷贝视图
pub struct IsoPrimaryVolume {
    system_id: []const u8,
    volume_id: []const u8,
    volume_space_size: u32,
    logical_block_size: u16,
    path_table_size: u32,
    root_extent_lba: u32,
    root_data_len: u32,
    publisher_id: []const u8,
    application_id: []const u8,
}

/// ISO 9660 目录记录（Directory Record）零拷贝视图
pub struct IsoDirEntry {
    extent_lba: u32,
    data_len: u32,
    flags: u8,
    name: []const u8,

    /// 是否为目录项（flags bit 1）
    fn isDir(): bool {
        return (_.flags & ISO_FLAG_DIRECTORY) != 0;
    }

    /// 是否为隐藏项（flags bit 0）
    fn isHidden(): bool {
        return (_.flags & ISO_FLAG_HIDDEN) != 0;
    }

    /// 是否为当前目录 `.`（单字节 0x00）
    fn isDot(): bool {
        return _.name.len == 1 && _.name[0] == 0x00;
    }

    /// 是否为上级目录 `..`（单字节 0x01）
    fn isDotDot(): bool {
        return _.name.len == 1 && _.name[0] == 0x01;
    }

    /// 剥离 ISO 9660 版本号后缀（如 `"README.TXT;1"` -> `"README.TXT"`）
    fn baseName(): []const u8 {
        let i: usize = 0;
        while (i < _.name.len) {
            if (_.name[i] == ';') {
                return _.name[0..i];
            }
            i += 1;
        }
        return _.name;
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

fn trimTrailingSpaces(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && s[end - 1] == ' ') {
        end -= 1;
    }
    return s[0..end];
}

fn readU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBothU16(data: []const u8, off: usize): u16? {
    const le = readU16LE(data, off);
    const be = readU16BE(data, off + 2);
    if (le != be) return null;
    return le;
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readBothU32(data: []const u8, off: usize): u32? {
    const le = readU32LE(data, off);
    const be = readU32BE(data, off + 4);
    if (le != be) return null;
    return le;
}

fn hasValidVdHeader(sector: []const u8): bool {
    if (sector.len < ISO_SECTOR_SIZE) return false;
    return strEq(sector[1..6], ISO_MAGIC) && sector[6] == 1;
}

/// 校验字节切片是否为合法 ISO 9660 镜像：
/// `data.len >= 32768 + 2048` 且在偏移 `32768 + 1..6` 为 `"CD001"` 且版本号为 `1`
pub fn isIso9660(data: []const u8): bool {
    if (data.len < ISO_PVD_OFFSET + ISO_SECTOR_SIZE) return false;
    return hasValidVdHeader(data[ISO_PVD_OFFSET .. ISO_PVD_OFFSET + ISO_SECTOR_SIZE]);
}

/// 遍历从扇区 16 开始的 2048B 卷描述符序列（遇 `VD_TYPE_TERMINATOR` 停止），
/// 定位并解析 `VD_TYPE_PRIMARY` 主卷描述符（同时校验所有 both-endian 字段一致性）
pub fn parseIsoPvd(data: []const u8): IsoPrimaryVolume? {
    if (data.len < ISO_PVD_OFFSET + ISO_SECTOR_SIZE) return null;

    let off: usize = ISO_PVD_OFFSET;
    while (off + ISO_SECTOR_SIZE <= data.len) {
        const sector = data[off .. off + ISO_SECTOR_SIZE];
        if (!hasValidVdHeader(sector)) return null;

        const vd_type = sector[0];
        if (vd_type == VD_TYPE_TERMINATOR) {
            return null;
        }

        if (vd_type == VD_TYPE_PRIMARY) {
            let volume_space_size: u32 = 0;
            if (v := readBothU32(sector, 80)) {
                volume_space_size = v;
            } else {
                return null;
            }

            let logical_block_size: u16 = 0;
            if (v := readBothU16(sector, 128)) {
                if (v == 0) return null;
                logical_block_size = v;
            } else {
                return null;
            }

            let path_table_size: u32 = 0;
            if (v := readBothU32(sector, 132)) {
                path_table_size = v;
            } else {
                return null;
            }

            // Root Directory Record 位于 PVD 偏移 156..190（34 字节）
            const root_rec_len = sector[156];
            if (root_rec_len < 34) return null;

            let root_extent_lba: u32 = 0;
            if (v := readBothU32(sector, 158)) {
                root_extent_lba = v;
            } else {
                return null;
            }

            let root_data_len: u32 = 0;
            if (v := readBothU32(sector, 166)) {
                root_data_len = v;
            } else {
                return null;
            }

            return IsoPrimaryVolume {
                system_id: trimTrailingSpaces(sector[8..40]),
                volume_id: trimTrailingSpaces(sector[40..72]),
                volume_space_size: volume_space_size,
                logical_block_size: logical_block_size,
                path_table_size: path_table_size,
                root_extent_lba: root_extent_lba,
                root_data_len: root_data_len,
                publisher_id: trimTrailingSpaces(sector[318..446]),
                application_id: trimTrailingSpaces(sector[574..702]),
            };
        }

        off += ISO_SECTOR_SIZE;
    }

    return null;
}

/// 零堆分配 ISO 9660 目录记录迭代器（支持跨扇区 `record_len == 0` 填充自动对齐跳过）
pub struct IsoDirIter {
    data: []const u8,
    start: usize,
    pos: usize,
    end: usize,
    sector_size: usize,
    had_error: bool = false,

    /// 从指定逻辑块号（LBA）、数据长度与逻辑块大小构造目录迭代器
    static fn init(data: []const u8, lba: u32, data_len: u32, block_size: u16): IsoDirIter {
        if (block_size == 0) {
            return IsoDirIter {
                data: data,
                start: 0,
                pos: 0,
                end: 0,
                sector_size: ISO_SECTOR_SIZE,
                had_error: true,
            };
        }

        const start_u64: u64 = (lba as u64) * (block_size as u64);
        const end_u64: u64 = start_u64 + (data_len as u64);
        const data_len_u64: u64 = data.len as u64;
        if (end_u64 < start_u64 || start_u64 > data_len_u64 || end_u64 > data_len_u64) {
            return IsoDirIter {
                data: data,
                start: 0,
                pos: 0,
                end: 0,
                sector_size: ISO_SECTOR_SIZE,
                had_error: true,
            };
        }

        const start_usize: usize = start_u64 as usize;
        const end_usize: usize = end_u64 as usize;
        return IsoDirIter {
            data: data,
            start: start_usize,
            pos: start_usize,
            end: end_usize,
            sector_size: ISO_SECTOR_SIZE,
            had_error: false,
        };
    }

    /// 从 PVD 根目录构造目录迭代器
    static fn fromRoot(data: []const u8, pvd: IsoPrimaryVolume): IsoDirIter {
        return IsoDirIter.init(data, pvd.root_extent_lba, pvd.root_data_len, pvd.logical_block_size);
    }

    /// 从目录项 `IsoDirEntry` 构造子目录迭代器（非目录项时置 `had_error = true` 返回空迭代器）
    static fn fromEntry(data: []const u8, pvd: IsoPrimaryVolume, entry: IsoDirEntry): IsoDirIter {
        if (!entry.isDir()) {
            return IsoDirIter {
                data: data,
                start: 0,
                pos: 0,
                end: 0,
                sector_size: ISO_SECTOR_SIZE,
                had_error: true,
            };
        }
        return IsoDirIter.init(data, entry.extent_lba, entry.data_len, pvd.logical_block_size);
    }

    /// 读取下一条目录记录；自动跳过扇区末尾 `record_len == 0` 填充对齐至下一 2048B 扇区
    fn next(*_): IsoDirEntry? {
        if (_.had_error) return null;

        while (_.pos < _.end) {
            const rec_len = _.data[_.pos] as usize;
            if (rec_len == 0) {
                const rel = _.pos - _.start;
                const rem = rel % _.sector_size;
                const step = if (rem == 0) _.sector_size else (_.sector_size - rem);
                if (step > _.end - _.pos) {
                    _.pos = _.end;
                    return null;
                }
                _.pos += step;
                continue;
            }

            if (rec_len < 33 || rec_len > _.end - _.pos) {
                _.had_error = true;
                _.pos = _.end;
                return null;
            }

            const rec = _.data[_.pos .. _.pos + rec_len];
            let extent_lba: u32 = 0;
            if (v := readBothU32(rec, 2)) {
                extent_lba = v;
            } else {
                _.had_error = true;
                _.pos = _.end;
                return null;
            }

            let entry_len: u32 = 0;
            if (v := readBothU32(rec, 10)) {
                entry_len = v;
            } else {
                _.had_error = true;
                _.pos = _.end;
                return null;
            }

            const flags = rec[25];
            const name_len = rec[32] as usize;
            if (33 + name_len > rec_len) {
                _.had_error = true;
                _.pos = _.end;
                return null;
            }

            const name = rec[33 .. 33 + name_len];
            _.pos += rec_len;

            return IsoDirEntry {
                extent_lba: extent_lba,
                data_len: entry_len,
                flags: flags,
                name: name,
            };
        }

        return null;
    }
}

/// 在根目录中按名称查找目录项（同时匹配完整 `name` 与剥离 `;1` 后的 `baseName()`）
pub fn findIsoEntry(data: []const u8, pvd: IsoPrimaryVolume, name: []const u8): IsoDirEntry? {
    let it = IsoDirIter.fromRoot(data, pvd);
    while (entry := it.next()) {
        if (strEq(entry.name, name) || strEq(entry.baseName(), name)) {
            return entry;
        }
    }
    return null;
}

/// 零拷贝读取普通文件的数据切片，防乘法/加法溢出及镜像越界（目录项或越界时返回 `null`）
pub fn readIsoFileData(data: []const u8, pvd: IsoPrimaryVolume, entry: IsoDirEntry): ([]const u8)? {
    if (entry.isDir()) return null;
    if (pvd.logical_block_size == 0) return null;

    const start_u64: u64 = (entry.extent_lba as u64) * (pvd.logical_block_size as u64);
    const end_u64: u64 = start_u64 + (entry.data_len as u64);
    const data_len_u64: u64 = data.len as u64;
    if (end_u64 < start_u64 || start_u64 > data_len_u64 || end_u64 > data_len_u64) {
        return null;
    }

    return data[(start_u64 as usize)..(end_u64 as usize)];
}
