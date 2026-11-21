// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux Kernel romfs Big-Endian Read-Only Filesystem Parser)

// romfs Core Protocol Constants
pub const ROMFS_MAGIC: []const u8 = "-rom1fs-";
pub const ROMFS_ALIGN: usize = 16;
pub const ROMFS_MAX_CHECKSUM_LEN: usize = 512;
pub const ROMFS_TYPE_MASK: u32 = 0x07;
pub const ROMFS_EXEC_BIT: u32 = 0x08;
pub const ROMFS_NEXT_MASK: u32 = 0xFFFFFFF0;
pub const ROMFS_MAX_DIR_STEPS: usize = 4096;

// romfs File Types (low 3 bits of nextfh)
pub const TYPE_HARD_LINK: u8 = 0;
pub const TYPE_DIRECTORY: u8 = 1;
pub const TYPE_REGULAR: u8 = 2;
pub const TYPE_SYMLINK: u8 = 3;
pub const TYPE_BLOCK_DEV: u8 = 4;
pub const TYPE_CHAR_DEV: u8 = 5;
pub const TYPE_SOCKET: u8 = 6;
pub const TYPE_FIFO: u8 = 7;

fn readU32Be(data: []const u8, off: usize): u32 {
    const b0 = (data[off] as u32) << 24;
    const b1 = (data[off + 1] as u32) << 16;
    const b2 = (data[off + 2] as u32) << 8;
    const b3 = data[off + 3] as u32;
    return b0 | b1 | b2 | b3;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn align16(v: usize): usize {
    return ((v + 15) / 16) * 16;
}

fn findNul(data: []const u8, start: usize, limit: usize): usize? {
    let i = start;
    while (i < limit) {
        if (data[i] == 0) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Computes the Linux romfs 32-bit big-endian wrapping word sum over `data`.
/// A valid romfs superblock (first `min(512, full_size)` bytes) or file header sums to `0`.
pub fn romfsChecksum(data: []const u8): u32 {
    let sum: u64 = 0;
    let off: usize = 0;
    while (off + 4 <= data.len) {
        sum = (sum + (readU32Be(data, off) as u64)) & 0xFFFFFFFF;
        off += 4;
    }
    return sum as u32;
}

/// Parsed romfs superblock (16-byte aligned volume name followed by root directory entries).
pub struct RomfsSuperblock {
    full_size: u32,
    checksum: u32,
    volume_name: []const u8,
    first_file_offset: usize,
}

/// Parsed 16-byte aligned romfs directory/file entry (zero-copy `name` and `data` slices).
pub struct RomfsEntry {
    offset: usize,
    next_offset: u32,
    file_type: u8,
    is_executable: bool,
    spec_info: u32,
    size: u32,
    checksum: u32,
    name: []const u8,
    data: []const u8,

    fn isDir(): bool {
        return _.file_type == TYPE_DIRECTORY;
    }

    fn isRegular(): bool {
        return _.file_type == TYPE_REGULAR;
    }

    fn isSymlink(): bool {
        return _.file_type == TYPE_SYMLINK;
    }

    /// Returns the first child entry offset (`spec_info`) if this entry is a directory with children.
    fn firstChildOffset(): u32? {
        if (!_.isDir() || _.spec_info == 0) {
            return null;
        }
        return _.spec_info;
    }
}

fn parseEntryAt(raw: []const u8, full_size: usize, offset: usize): RomfsEntry? {
    if (offset == 0 || (offset & 15) != 0 || offset >= full_size || full_size - offset < 16) {
        return null;
    }
    const nextfh = readU32Be(raw, offset);
    const next_offset = nextfh & ROMFS_NEXT_MASK;
    const file_type = (nextfh & ROMFS_TYPE_MASK) as u8;
    const is_executable = (nextfh & ROMFS_EXEC_BIT) != 0;
    const spec_info = readU32Be(raw, offset + 4);
    const size = readU32Be(raw, offset + 8);
    const checksum = readU32Be(raw, offset + 12);

    const nul_pos = findNul(raw, offset + 16, full_size) ?? return null;
    const name = raw[(offset + 16)..nul_pos];
    const data_offset = align16(nul_pos + 1);
    if (data_offset > full_size) {
        return null;
    }

    // Verify file header 32-bit big-endian checksum == 0
    if (romfsChecksum(raw[offset..data_offset]) != 0) {
        return null;
    }

    let payload: []const u8 = raw[0..0];
    if (file_type == TYPE_REGULAR || file_type == TYPE_SYMLINK) {
        const sz = size as usize;
        if (sz > full_size - data_offset) {
            return null;
        }
        payload = raw[data_offset..(data_offset + sz)];
    }

    return RomfsEntry{
        .offset = offset,
        .next_offset = next_offset,
        .file_type = file_type,
        .is_executable = is_executable,
        .spec_info = spec_info,
        .size = size,
        .checksum = checksum,
        .name = name,
        .data = payload,
    };
}

/// Zero-copy directory entry iterator following `next_offset` with cycle & bounds protection.
pub struct RomfsDirIter {
    raw: []const u8,
    full_size: usize,
    cur_offset: usize,
    steps: usize,

    pub fn init(raw: []const u8, full_size: usize, start_offset: usize): RomfsDirIter {
        return RomfsDirIter{
            .raw = raw,
            .full_size = full_size,
            .cur_offset = start_offset,
            .steps = 0,
        };
    }

    fn next(*_): RomfsEntry? {
        if (_.cur_offset == 0 || _.steps >= ROMFS_MAX_DIR_STEPS) {
            return null;
        }
        if (entry := parseEntryAt(_.raw, _.full_size, _.cur_offset)) {
            _.steps += 1;

            const next_off = entry.next_offset as usize;
            // Cycle protection: require strictly increasing sibling offsets and within bounds
            if (next_off == 0 || next_off <= _.cur_offset || next_off >= _.full_size) {
                _.cur_offset = 0;
            } else {
                _.cur_offset = next_off;
            }
            return entry;
        }
        _.cur_offset = 0;
        return null;
    }
}

/// Zero-copy parsed romfs image handle.
pub struct RomfsImage {
    raw: []const u8,
    superblock: RomfsSuperblock,

    /// Iterates entries in the root directory starting at `superblock.first_file_offset`.
    fn rootEntries(): RomfsDirIter {
        return RomfsDirIter.init(_.raw, _.superblock.full_size as usize, _.superblock.first_file_offset);
    }

    /// Parses a single entry at a 16-byte aligned byte offset.
    fn entryAt(offset: usize): RomfsEntry? {
        return parseEntryAt(_.raw, _.superblock.full_size as usize, offset);
    }

    /// Iterates child entries of a directory entry (`dir_entry.firstChildOffset()`).
    fn dirEntries(dir_entry: RomfsEntry): RomfsDirIter {
        const start = (dir_entry.firstChildOffset() ?? 0) as usize;
        return RomfsDirIter.init(_.raw, _.superblock.full_size as usize, start);
    }

    /// Finds an entry named `name` inside the directory whose first child starts at `dir_offset`.
    fn findInDir(dir_offset: usize, name: []const u8): RomfsEntry? {
        let it = RomfsDirIter.init(_.raw, _.superblock.full_size as usize, dir_offset);
        while (true) {
            if (ent := it.next()) {
                if (bytesEq(ent.name, name)) {
                    return ent;
                }
            } else {
                break;
            }
        }
        return null;
    }
}

/// Returns `true` if `data` starts with the 8-byte `"-rom1fs-"` magic.
pub fn isRomfs(data: []const u8): bool {
    return data.len >= 8 && bytesEq(data[0..8], ROMFS_MAGIC);
}

/// Parses and strictly validates a romfs image:
/// - `"-rom1fs-"` magic
/// - `16 <= full_size <= data.len`
/// - 32-bit big-endian checksum over first `min(512, full_size)` bytes equals `0`
/// - NUL-terminated `volume_name` and 16-byte aligned `first_file_offset <= full_size`
pub fn parseRomfs(data: []const u8): RomfsImage? {
    if (data.len < 32 || !isRomfs(data)) {
        return null;
    }
    const full_size = readU32Be(data, 8);
    const checksum = readU32Be(data, 12);
    const full_sz = full_size as usize;
    if (full_sz < 32 || full_sz > data.len || (full_sz & 3) != 0) {
        return null;
    }

    let check_len = full_sz;
    if (check_len > ROMFS_MAX_CHECKSUM_LEN) {
        check_len = ROMFS_MAX_CHECKSUM_LEN;
    }
    if (romfsChecksum(data[0..check_len]) != 0) {
        return null;
    }

    const nul_pos = findNul(data, 16, full_sz) ?? return null;
    const volume_name = data[16..nul_pos];
    const first_file_offset = align16(nul_pos + 1);
    if (first_file_offset > full_sz) {
        return null;
    }

    return RomfsImage{
        .raw = data,
        .superblock = RomfsSuperblock{
            .full_size = full_size,
            .checksum = checksum,
            .volume_name = volume_name,
            .first_file_offset = first_file_offset,
        },
    };
}
