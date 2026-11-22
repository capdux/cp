// Origin: Pure-CP Stdlib (Zero-Alloc Linux CramFS Superblock, CRC32, Packed Inode & Dir Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// CramFS Protocol Constants (Linux include/uapi/linux/cramfs_fs.h)
// ---------------------------------------------------------------------------

pub const CRAMFS_MAGIC: u32 = 0x28CD3D45;
pub const CRAMFS_SIGNATURE: []const u8 = "Compressed ROMFS";
pub const CRAMFS_SUPER_SIZE: usize = 76;
pub const CRAMFS_INODE_SIZE: usize = 12;
pub const CRAMFS_CRC_OFFSET: usize = 32;

// Superblock Feature Flags
pub const FLAG_FSID_VERSION_2: u32 = 0x00000001;
pub const FLAG_SORTED_DIRS: u32 = 0x00000002;
pub const FLAG_HOLES: u32 = 0x00000100;
pub const FLAG_WRONG_SIGNATURE: u32 = 0x00000200;
pub const FLAG_SHIFTED_ROOT_OFFSET: u32 = 0x00000400;
pub const FLAG_EXT_BLOCK_POINTERS: u32 = 0x00000800;

// POSIX Mode Bits
pub const S_IFMT: u16 = 0xF000;
pub const S_IFDIR: u16 = 0x4000;
pub const S_IFREG: u16 = 0x8000;
pub const S_IFLNK: u16 = 0xA000;

// ---------------------------------------------------------------------------
// Internal Little-Endian & Byte Helpers
// ---------------------------------------------------------------------------

fn readU16Le(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return b0 | (b1 << 8);
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn readBytes16(buf: []const u8, off: usize): [16]u8 {
    let out: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        out[i] = buf[off + i];
        i += 1;
    }
    return out;
}

fn matchBytes(buf: []const u8, off: usize, sig: []const u8): bool {
    if (buf.len < off + sig.len) {
        return false;
    }
    let i: usize = 0;
    while (i < sig.len) {
        if (buf[off + i] != sig[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn stripTrailingNul(bytes: []const u8): []const u8 {
    let end: usize = bytes.len;
    while (end > 0 && bytes[end - 1] == 0) {
        end -= 1;
    }
    return bytes[0..end];
}

// ---------------------------------------------------------------------------
// IEEE 802.3 CRC-32 (Zero-Copy Zeroing of [32..36))
// ---------------------------------------------------------------------------

const CRC32_IEEE_POLY: u32 = 0xEDB88320;

/// Computes the IEEE 802.3 CRC-32 of a CramFS image while treating the 4-byte CRC
/// field at `[32..36)` as zeros, without mutating `image`.
pub fn cramfsCrc32(image: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < image.len) {
        const b: u8 = if (i >= CRAMFS_CRC_OFFSET && i < CRAMFS_CRC_OFFSET + 4) 0 else image[i];
        crc = crc ^ (b as u32);
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ CRC32_IEEE_POLY;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// Decoded 12-byte bit-packed CramFS Inode (`struct cramfs_inode`).
pub struct CramfsInode {
    mode: u16,
    uid: u16,
    size: u32,
    gid: u8,
    namelen_words: u8,
    offset_words: u32,

    /// Byte length of the padded filename field (`namelen_words * 4`).
    fn nameBytesLen(): usize {
        return (_.namelen_words as usize) * 4;
    }

    /// Byte offset of the inode's data or directory entries (`offset_words * 4`).
    fn dataOffset(): usize {
        return (_.offset_words as usize) * 4;
    }

    fn isDir(): bool {
        return (_.mode & S_IFMT) == S_IFDIR;
    }

    fn isRegular(): bool {
        return (_.mode & S_IFMT) == S_IFREG;
    }

    fn isSymlink(): bool {
        return (_.mode & S_IFMT) == S_IFLNK;
    }

    fn permissions(): u16 {
        return _.mode & 0x0FFF;
    }
}

/// Parsed 76-byte CramFS Superblock (`struct cramfs_super`).
pub struct CramfsSuperblock {
    size: u32,
    flags: u32,
    future: u32,
    crc: u32,
    edition: u32,
    blocks: u32,
    files: u32,
    name: [16]u8,
    root: CramfsInode,

    fn nameSlice(): []const u8 {
        let end: usize = 0;
        while (end < 16 && _.name[end] != 0) {
            end += 1;
        }
        return _.name[0..end];
    }

    fn isVersion2(): bool {
        return (_.flags & FLAG_FSID_VERSION_2) != 0;
    }

    fn hasSortedDirs(): bool {
        return (_.flags & FLAG_SORTED_DIRS) != 0;
    }

    fn hasHoles(): bool {
        return (_.flags & FLAG_HOLES) != 0;
    }
}

/// A single zero-copy directory entry yielded by `CramfsDirIter`.
pub struct CramfsDirEntry {
    inode: CramfsInode,
    name: []const u8,
}

/// Zero-copy iterator over directory entries within `[dir.dataOffset() .. dir.dataOffset() + dir.size]`.
pub struct CramfsDirIter {
    bytes: []const u8,
    pos: usize,

    fn next(*_): CramfsDirEntry? {
        if (_.pos + CRAMFS_INODE_SIZE > _.bytes.len) {
            return null;
        }
        const inode = parseCramfsInode(_.bytes[_.pos .. _.pos + CRAMFS_INODE_SIZE]) ?? return null;
        const name_len = inode.nameBytesLen();
        if (_.pos + CRAMFS_INODE_SIZE + name_len > _.bytes.len) {
            return null;
        }
        const raw_name = _.bytes[_.pos + CRAMFS_INODE_SIZE .. _.pos + CRAMFS_INODE_SIZE + name_len];
        _.pos += CRAMFS_INODE_SIZE + name_len;
        return CramfsDirEntry {
            inode: inode,
            name: stripTrailingNul(raw_name),
        };
    }
}

// ---------------------------------------------------------------------------
// Parsing Functions
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with `CRAMFS_MAGIC` (`0x28CD3D45`) and `"Compressed ROMFS"` at offset 16.
pub fn isCramfs(data: []const u8): bool {
    if (data.len < 32) {
        return false;
    }
    return readU32Le(data, 0) == CRAMFS_MAGIC && matchBytes(data, 16, CRAMFS_SIGNATURE);
}

/// Decodes a 12-byte bit-packed CramFS Inode.
pub fn parseCramfsInode(bytes12: []const u8): CramfsInode? {
    if (bytes12.len < CRAMFS_INODE_SIZE) {
        return null;
    }
    const word1 = readU32Le(bytes12, 4);
    const word2 = readU32Le(bytes12, 8);
    return CramfsInode {
        mode: readU16Le(bytes12, 0),
        uid: readU16Le(bytes12, 2),
        size: word1 & 0x00FFFFFF,
        gid: ((word1 >> 24) & 0xFF) as u8,
        namelen_words: (word2 & 0x3F) as u8,
        offset_words: word2 >> 6,
    };
}

/// Parses and validates a CramFS Superblock.
/// Validates `CRAMFS_MAGIC`, `"Compressed ROMFS"` signature, `76 <= size <= data.len`,
/// and IEEE CRC-32 when `FLAG_FSID_VERSION_2` is set.
pub fn parseCramfsSuperblock(data: []const u8): CramfsSuperblock? {
    if (data.len < CRAMFS_SUPER_SIZE) {
        return null;
    }
    if (!isCramfs(data)) {
        return null;
    }
    const size = readU32Le(data, 4);
    if ((size as usize) < CRAMFS_SUPER_SIZE || (size as usize) > data.len) {
        return null;
    }
    const flags = readU32Le(data, 8);
    const crc = readU32Le(data, CRAMFS_CRC_OFFSET);
    if ((flags & FLAG_FSID_VERSION_2) != 0) {
        const computed = cramfsCrc32(data[0 .. size as usize]);
        if (computed != crc) {
            return null;
        }
    }
    const root = parseCramfsInode(data[64..76]) ?? return null;
    return CramfsSuperblock {
        size: size,
        flags: flags,
        future: readU32Le(data, 12),
        crc: crc,
        edition: readU32Le(data, 36),
        blocks: readU32Le(data, 40),
        files: readU32Le(data, 44),
        name: readBytes16(data, 48),
        root: root,
    };
}

/// Creates a zero-copy `CramfsDirIter` for `dir_inode` inside `image`.
/// Returns `null` if `dir_inode` is not a directory or its data range exceeds `image`.
pub fn iterCramfsDir(image: []const u8, dir_inode: CramfsInode): CramfsDirIter? {
    if (!dir_inode.isDir()) {
        return null;
    }
    const start = dir_inode.dataOffset();
    const len = dir_inode.size as usize;
    if (start > image.len || len > image.len - start) {
        return null;
    }
    return CramfsDirIter {
        bytes: image[start .. start + len],
        pos: 0,
    };
}
