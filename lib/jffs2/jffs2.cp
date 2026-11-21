// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux JFFS2 Flash Node Stream, Inode, Dirent & CRC32 Parser)

// ---------------------------------------------------------------------------
// JFFS2 Protocol Constants (Linux include/uapi/linux/jffs2.h)
// ---------------------------------------------------------------------------

pub const JFFS2_MAGIC_BITMASK: u16 = 0x1985;
pub const JFFS2_OLD_MAGIC_BITMASK: u16 = 0x1984;
pub const JFFS2_EMPTY_BITMASK: u16 = 0xFFFF;

pub const JFFS2_COMMON_HEADER_SIZE: usize = 12;
pub const JFFS2_INODE_HEADER_SIZE: usize = 68;
pub const JFFS2_DIRENT_HEADER_SIZE: usize = 40;

// Node Types
pub const NODETYPE_DIRENT: u16 = 0xE001;
pub const NODETYPE_INODE: u16 = 0xE002;
pub const NODETYPE_CLEANMARKER: u16 = 0x2003;
pub const NODETYPE_PADDING: u16 = 0x2004;
pub const NODETYPE_SUMMARY: u16 = 0xE006;
pub const NODETYPE_XATTR: u16 = 0xE008;
pub const NODETYPE_XREF: u16 = 0xE009;

// Compression Types
pub const COMPR_NONE: u8 = 0;
pub const COMPR_ZERO: u8 = 1;
pub const COMPR_RTIME: u8 = 2;
pub const COMPR_RUBINMIPS: u8 = 3;
pub const COMPR_COPY: u8 = 4;
pub const COMPR_DYNRUBIN: u8 = 5;
pub const COMPR_ZLIB: u8 = 6;
pub const COMPR_LZO: u8 = 7;

// Dirent Types
pub const DT_UNKNOWN: u8 = 0;
pub const DT_FIFO: u8 = 1;
pub const DT_CHR: u8 = 2;
pub const DT_DIR: u8 = 4;
pub const DT_BLK: u8 = 6;
pub const DT_REG: u8 = 8;
pub const DT_LNK: u8 = 10;
pub const DT_SOCK: u8 = 12;

// ---------------------------------------------------------------------------
// Internal Little-Endian & Alignment Helpers
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

fn align4(n: usize): usize {
    const rem = n % 4;
    if (rem == 0) {
        return n;
    }
    return n + (4 - rem);
}

// ---------------------------------------------------------------------------
// JFFS2 CRC32 (IEEE 802.3 poly 0xEDB88320, initial 0, no final XOR)
// ---------------------------------------------------------------------------

const CRC32_IEEE_POLY: u32 = 0xEDB88320;

/// Computes Linux JFFS2 CRC32 (reflected IEEE 802.3 `0xEDB88320`, initial `0x00000000`, no final XOR).
pub fn jffs2Crc32(data: []const u8): u32 {
    let crc: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ (data[i] as u32);
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
    return crc;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// Validated 12-byte JFFS2 Common Node Header plus zero-copy slice of the full node.
pub struct Jffs2NodeHeader {
    magic: u16,
    nodetype: u16,
    totlen: u32,
    hdr_crc: u32,
    offset: usize,
    raw: []const u8,
}

/// Validated JFFS2 Raw Inode (`NODETYPE_INODE`, 68B header + `csize` payload).
pub struct Jffs2RawInode {
    ino: u32,
    version: u32,
    mode: u32,
    uid: u16,
    gid: u16,
    isize: u32,
    atime: u32,
    mtime: u32,
    ctime: u32,
    offset: u32,
    csize: u32,
    dsize: u32,
    compr: u8,
    usercompr: u8,
    flags: u16,
    data_crc: u32,
    node_crc: u32,
    data: []const u8,
}

/// Validated JFFS2 Raw Dirent (`NODETYPE_DIRENT`, 40B header + `nsize` name).
pub struct Jffs2RawDirent {
    pino: u32,
    version: u32,
    ino: u32,
    mctime: u32,
    nsize: u8,
    dirent_type: u8,
    node_crc: u32,
    name_crc: u32,
    name: []const u8,

    /// Returns `true` if this dirent is a deletion tombstone (`ino == 0`).
    fn isDeletion(): bool {
        return _.ino == 0;
    }
}

/// Zero-copy 4-byte-aligned iterator over JFFS2 nodes in a flash eraseblock.
/// Automatically skips `0xFFFF` erased regions and validates each node's `hdr_crc`.
pub struct Jffs2NodeIter {
    flash_bytes: []const u8,
    pos: usize,
    err: bool,

    fn hasError(): bool {
        return _.err;
    }

    fn next(*_): Jffs2NodeHeader? {
        if (_.err) {
            return null;
        }
        // Skip 0xFFFF erased words on 4-byte boundaries
        while (_.pos + 4 <= _.flash_bytes.len && readU16Le(_.flash_bytes, _.pos) == JFFS2_EMPTY_BITMASK) {
            _.pos += 4;
        }
        if (_.pos + JFFS2_COMMON_HEADER_SIZE > _.flash_bytes.len) {
            return null;
        }

        const start = _.pos;
        if (hdr := parseJffs2NodeHeader(_.flash_bytes[start.._.flash_bytes.len])) {
            const step = align4(hdr.totlen as usize);
            if (step > _.flash_bytes.len - start) {
                _.pos = _.flash_bytes.len;
            } else {
                _.pos = start + step;
            }
            return Jffs2NodeHeader {
                magic: hdr.magic,
                nodetype: hdr.nodetype,
                totlen: hdr.totlen,
                hdr_crc: hdr.hdr_crc,
                offset: start,
                raw: hdr.raw,
            };
        } else {
            _.err = true;
            return null;
        }
    }
}

// ---------------------------------------------------------------------------
// Parsing Functions
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with `JFFS2_MAGIC_BITMASK` (`0x1985`).
pub fn isJffs2(data: []const u8): bool {
    if (data.len < 2) {
        return false;
    }
    return readU16Le(data, 0) == JFFS2_MAGIC_BITMASK;
}

/// Parses and validates a 12-byte JFFS2 Common Node Header (`magic == 0x1985`,
/// `12 <= totlen <= data.len`, and `hdr_crc == jffs2Crc32(data[0..8])`).
pub fn parseJffs2NodeHeader(data: []const u8): Jffs2NodeHeader? {
    if (data.len < JFFS2_COMMON_HEADER_SIZE) {
        return null;
    }
    const magic = readU16Le(data, 0);
    if (magic != JFFS2_MAGIC_BITMASK) {
        return null;
    }
    const nodetype = readU16Le(data, 2);
    const totlen = readU32Le(data, 4);
    const hdr_crc = readU32Le(data, 8);
    if ((totlen as usize) < JFFS2_COMMON_HEADER_SIZE || (totlen as usize) > data.len) {
        return null;
    }
    if (jffs2Crc32(data[0..8]) != hdr_crc) {
        return null;
    }
    return Jffs2NodeHeader {
        magic: magic,
        nodetype: nodetype,
        totlen: totlen,
        hdr_crc: hdr_crc,
        offset: 0,
        raw: data[0 .. totlen as usize],
    };
}

/// Parses and validates a `NODETYPE_INODE` node (`node_crc == jffs2Crc32(raw[0..60])`
/// and `data_crc == jffs2Crc32(data)`).
pub fn parseJffs2Inode(node: Jffs2NodeHeader): Jffs2RawInode? {
    if (node.nodetype != NODETYPE_INODE || node.raw.len < JFFS2_INODE_HEADER_SIZE) {
        return null;
    }
    const csize = readU32Le(node.raw, 48);
    const data_end: usize = JFFS2_INODE_HEADER_SIZE + (csize as usize);
    if (data_end > node.raw.len) {
        return null;
    }
    const data_crc = readU32Le(node.raw, 60);
    const node_crc = readU32Le(node.raw, 64);
    if (jffs2Crc32(node.raw[0..60]) != node_crc) {
        return null;
    }
    const payload = node.raw[JFFS2_INODE_HEADER_SIZE..data_end];
    if (jffs2Crc32(payload) != data_crc) {
        return null;
    }

    return Jffs2RawInode {
        ino: readU32Le(node.raw, 12),
        version: readU32Le(node.raw, 16),
        mode: readU32Le(node.raw, 20),
        uid: readU16Le(node.raw, 24),
        gid: readU16Le(node.raw, 26),
        isize: readU32Le(node.raw, 28),
        atime: readU32Le(node.raw, 32),
        mtime: readU32Le(node.raw, 36),
        ctime: readU32Le(node.raw, 40),
        offset: readU32Le(node.raw, 44),
        csize: csize,
        dsize: readU32Le(node.raw, 52),
        compr: node.raw[56],
        usercompr: node.raw[57],
        flags: readU16Le(node.raw, 58),
        data_crc: data_crc,
        node_crc: node_crc,
        data: payload,
    };
}

/// Parses and validates a `NODETYPE_DIRENT` node (`node_crc == jffs2Crc32(raw[0..32])`
/// and `name_crc == jffs2Crc32(name)`).
pub fn parseJffs2Dirent(node: Jffs2NodeHeader): Jffs2RawDirent? {
    if (node.nodetype != NODETYPE_DIRENT || node.raw.len < JFFS2_DIRENT_HEADER_SIZE) {
        return null;
    }
    const nsize = node.raw[28];
    const name_end: usize = JFFS2_DIRENT_HEADER_SIZE + (nsize as usize);
    if (name_end > node.raw.len) {
        return null;
    }
    const node_crc = readU32Le(node.raw, 32);
    const name_crc = readU32Le(node.raw, 36);
    if (jffs2Crc32(node.raw[0..32]) != node_crc) {
        return null;
    }
    const name_slice = node.raw[JFFS2_DIRENT_HEADER_SIZE..name_end];
    if (jffs2Crc32(name_slice) != name_crc) {
        return null;
    }

    return Jffs2RawDirent {
        pino: readU32Le(node.raw, 12),
        version: readU32Le(node.raw, 16),
        ino: readU32Le(node.raw, 20),
        mctime: readU32Le(node.raw, 24),
        nsize: nsize,
        dirent_type: node.raw[29],
        node_crc: node_crc,
        name_crc: name_crc,
        name: name_slice,
    };
}

/// Creates a zero-copy `Jffs2NodeIter` over a JFFS2 flash eraseblock buffer.
pub fn iterJffs2Nodes(flash_bytes: []const u8): Jffs2NodeIter {
    return Jffs2NodeIter {
        flash_bytes: flash_bytes,
        pos: 0,
        err: false,
    };
}
