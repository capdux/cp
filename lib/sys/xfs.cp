// Origin: Pure-CP Stdlib (Zero-Alloc Linux XFS v4/v5 Filesystem Parser) | Created: 2026-11-22

pub const XFS_SB_MAGIC: u32 = 0x58465342;      // "XFSB" big-endian
pub const XFS_SB_MIN_SIZE: usize = 264;
pub const XFS_SB_SECTOR_SIZE: usize = 512;
pub const XFS_INODE_MAGIC: u16 = 0x494E;       // "IN" big-endian
pub const XFS_INODE_CORE_SIZE: usize = 96;
pub const XFS_AGF_MAGIC: u32 = 0x58414746;     // "XAGF" big-endian
pub const XFS_AGI_MAGIC: u32 = 0x58414749;     // "XAGI" big-endian
pub const XFS_SB_CRC_OFFSET: usize = 224;
pub const XFS_BMBT_REC_SIZE: usize = 16;

// Superblock Version (versionnum & 0x000F)
pub const XFS_SB_VERSION_4: u8 = 4;
pub const XFS_SB_VERSION_5: u8 = 5;

// Inode Data Fork Format
pub const FMT_DEV: u8 = 0;
pub const FMT_LOCAL: u8 = 1;
pub const FMT_EXTENTS: u8 = 2;
pub const FMT_BTREE: u8 = 3;

// v5 RO-Compat / Incompat Feature Flags
pub const RO_COMPAT_FINOBT: u32 = 0x01;
pub const RO_COMPAT_RMAPBT: u32 = 0x02;
pub const RO_COMPAT_REFLINK: u32 = 0x04;
pub const INCOMPAT_FTYPE: u32 = 0x01;
pub const INCOMPAT_BIGTIME: u32 = 0x08;

// POSIX Mode Bits for XfsInodeCore
pub const S_IFMT: u16 = 0xF000;
pub const S_IFREG: u16 = 0x8000;
pub const S_IFDIR: u16 = 0x4000;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readU64Be(buf: []const u8, off: usize): u64 {
    const hi = readU32Be(buf, off) as u64;
    const lo = readU32Be(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn writeU64Be(out: *[]u8, off: usize, v: u64) {
    out.*[off + 0] = ((v >> 56) & 0xFF) as u8;
    out.*[off + 1] = ((v >> 48) & 0xFF) as u8;
    out.*[off + 2] = ((v >> 40) & 0xFF) as u8;
    out.*[off + 3] = ((v >> 32) & 0xFF) as u8;
    out.*[off + 4] = ((v >> 24) & 0xFF) as u8;
    out.*[off + 5] = ((v >> 16) & 0xFF) as u8;
    out.*[off + 6] = ((v >> 8) & 0xFF) as u8;
    out.*[off + 7] = (v & 0xFF) as u8;
}

fn copy12(src: []const u8, off: usize): [12]u8 {
    let out: [12]u8 = undefined;
    let i: usize = 0;
    while (i < 12) {
        out[i] = src[off + i];
        i += 1;
    }
    return out;
}

fn copy16(src: []const u8, off: usize): [16]u8 {
    let out: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 16) {
        out[i] = src[off + i];
        i += 1;
    }
    return out;
}

fn crc32cStep(crc_in: u32, byte: u8): u32 {
    let crc = crc_in ^ (byte as u32);
    let bit: u8 = 0;
    while (bit < 8) {
        if ((crc & 1) != 0) {
            crc = (crc >> 1) ^ 0x82F63B78;
        } else {
            crc = crc >> 1;
        }
        bit += 1;
    }
    return crc;
}

/// Computes standard iSCSI / XFS CRC32C (reflected Castagnoli 0x82F63B78, init 0xFFFFFFFF, final XOR 0xFFFFFFFF).
pub fn xfsCrc32c(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc32cStep(crc, data[i]);
        i += 1;
    }
    return ~crc;
}

/// Computes XFS CRC32C over `data`, treating the 4-byte checksum field at `[zero_offset..zero_offset+4]` as zeroes.
pub fn xfsCrc32cWithZeroField(data: []const u8, zero_offset: usize): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        let b = data[i];
        if (i >= zero_offset && i < zero_offset + 4) {
            b = 0;
        }
        crc = crc32cStep(crc, b);
        i += 1;
    }
    return ~crc;
}

/// Parsed Linux XFS v4/v5 Big-Endian Superblock (`xfs_dsb`).
pub struct XfsSuperblock {
    blocksize: u32,
    dblocks: u64,
    rblocks: u64,
    rextents: u64,
    uuid: [16]u8,
    logstart: u64,
    rootino: u64,
    agblocks: u32,
    agcount: u32,
    sectsize: u16,
    inodesize: u16,
    fname: [12]u8,
    blocklog: u8,
    sectlog: u8,
    inodelog: u8,
    agblklog: u8,
    versionnum: u16,
    icount: u64,
    ifree: u64,
    fdblocks: u64,
    features_compat: u32,
    features_ro_compat: u32,
    features_incompat: u32,
    crc: u32,

    /// Returns the superblock format version (`versionnum & 0x000F`, typically 4 or 5).
    fn version(): u8 {
        return (_.versionnum & 0x000F) as u8;
    }

    /// Returns true if this is an XFS v5 superblock.
    fn isV5(): bool {
        return _.version() == XFS_SB_VERSION_5;
    }

    /// Returns true if v5 Reflink (`RO_COMPAT_REFLINK`) is enabled.
    fn hasReflink(): bool {
        return _.isV5() && (_.features_ro_compat & RO_COMPAT_REFLINK) != 0;
    }

    /// Returns true if v5 directory entry file-type (`INCOMPAT_FTYPE`) is enabled.
    fn hasFtype(): bool {
        return _.isV5() && (_.features_incompat & INCOMPAT_FTYPE) != 0;
    }

    /// Returns total filesystem capacity in bytes (`dblocks * blocksize`).
    fn totalBytes(): u128 {
        return (_.dblocks as u128) * (_.blocksize as u128);
    }

    /// Returns the NUL-terminated filesystem label slice from `fname[0..12]`.
    fn labelSlice(): []const u8 {
        let end: usize = 0;
        while (end < 12 && _.fname[end] != 0) {
            end += 1;
        }
        return _.fname[0..end];
    }
}

/// Parsed Linux XFS Inode Core (`xfs_dinode_core`).
pub struct XfsInodeCore {
    mode: u16,
    version: u8,
    format: u8,
    uid: u32,
    gid: u32,
    nlink: u32,
    size: u64,
    nblocks: u64,
    nextents: u32,

    fn isV3(): bool {
        return _.version == 3;
    }

    fn isRegular(): bool {
        return (_.mode & S_IFMT) == S_IFREG;
    }

    fn isDirectory(): bool {
        return (_.mode & S_IFMT) == S_IFDIR;
    }
}

/// Decoded 128-bit packed XFS BMBT Extent Record (`xfs_bmbt_rec`).
pub struct XfsBmbtRec {
    is_unwritten: bool,
    startoff: u64,
    startblock: u64,
    blockcount: u32,
}

/// Fast magic and minimum-length check for an XFS superblock (`"XFSB"` at offset 0).
pub fn isXfsSuperblock(data: []const u8): bool {
    if (data.len < XFS_SB_MIN_SIZE) {
        return false;
    }
    return readU32Be(data, 0) == XFS_SB_MAGIC;
}

/// Parses and validates an XFS v4 or v5 superblock.
/// Validates `"XFSB"` magic, version in 4..5, `blocksize == 1<<blocklog`,
/// `sectsize == 1<<sectlog`, `inodesize == 1<<inodelog`, and for v5 verifies
/// the big-endian CRC32C at offset 224 over the sector.
pub fn parseXfsSuperblock(sector0: []const u8): XfsSuperblock? {
    if (!isXfsSuperblock(sector0)) {
        return null;
    }

    const blocksize = readU32Be(sector0, 4);
    const versionnum = readU16Be(sector0, 100);
    const sectsize = readU16Be(sector0, 102);
    const inodesize = readU16Be(sector0, 104);
    const blocklog = sector0[120];
    const sectlog = sector0[121];
    const inodelog = sector0[122];

    const ver = (versionnum & 0x000F) as u8;
    if (ver != XFS_SB_VERSION_4 && ver != XFS_SB_VERSION_5) {
        return null;
    }
    if (blocklog > 31 || sectlog > 15 || inodelog > 15) {
        return null;
    }
    if (blocksize != ((1 as u32) << (blocklog as u5))) {
        return null;
    }
    if (sectsize != ((1 as u16) << (sectlog as u4))) {
        return null;
    }
    if (inodesize != ((1 as u16) << (inodelog as u4))) {
        return null;
    }

    let features_compat: u32 = 0;
    let features_ro_compat: u32 = 0;
    let features_incompat: u32 = 0;
    let crc: u32 = 0;

    if (ver == XFS_SB_VERSION_5) {
        const crc_len = sectsize as usize;
        if (crc_len < XFS_SB_MIN_SIZE || sector0.len < crc_len) {
            return null;
        }
        features_compat = readU32Be(sector0, 208);
        features_ro_compat = readU32Be(sector0, 212);
        features_incompat = readU32Be(sector0, 216);
        crc = readU32Be(sector0, XFS_SB_CRC_OFFSET);

        const actual_crc = xfsCrc32cWithZeroField(sector0[0..crc_len], XFS_SB_CRC_OFFSET);
        if (actual_crc != crc) {
            return null;
        }
    }

    return XfsSuperblock {
        blocksize: blocksize,
        dblocks: readU64Be(sector0, 8),
        rblocks: readU64Be(sector0, 16),
        rextents: readU64Be(sector0, 24),
        uuid: copy16(sector0, 32),
        logstart: readU64Be(sector0, 48),
        rootino: readU64Be(sector0, 56),
        agblocks: readU32Be(sector0, 84),
        agcount: readU32Be(sector0, 88),
        sectsize: sectsize,
        inodesize: inodesize,
        fname: copy12(sector0, 108),
        blocklog: blocklog,
        sectlog: sectlog,
        inodelog: inodelog,
        agblklog: sector0[124],
        versionnum: versionnum,
        icount: readU64Be(sector0, 128),
        ifree: readU64Be(sector0, 136),
        fdblocks: readU64Be(sector0, 144),
        features_compat: features_compat,
        features_ro_compat: features_ro_compat,
        features_incompat: features_incompat,
        crc: crc,
    };
}

/// Parses an XFS big-endian Inode Core (`"IN"` magic, 96-byte minimum header).
pub fn parseXfsInodeCore(data: []const u8): XfsInodeCore? {
    if (data.len < XFS_INODE_CORE_SIZE) {
        return null;
    }
    if (readU16Be(data, 0) != XFS_INODE_MAGIC) {
        return null;
    }
    const version = data[4];
    let nlink = readU32Be(data, 16);
    if (version == 1) {
        nlink = readU16Be(data, 6) as u32;
    }
    return XfsInodeCore {
        mode: readU16Be(data, 2),
        version: version,
        format: data[5],
        uid: readU32Be(data, 8),
        gid: readU32Be(data, 12),
        nlink: nlink,
        size: readU64Be(data, 56),
        nblocks: readU64Be(data, 64),
        nextents: readU32Be(data, 76),
    };
}

/// Decodes a 16-byte big-endian 128-bit packed XFS BMBT extent record.
pub fn decodeXfsBmbtRec(rec16: []const u8): XfsBmbtRec? {
    if (rec16.len < XFS_BMBT_REC_SIZE) {
        return null;
    }
    const l0 = readU64Be(rec16, 0);
    const l1 = readU64Be(rec16, 8);

    const is_unwritten = (l0 >> 63) != 0;
    const startoff = (l0 >> 9) & 0x003FFFFFFFFFFFFF;
    const startblock = ((l0 & 0x1FF) << 43) | (l1 >> 21);
    const blockcount = (l1 & 0x1FFFFF) as u32;

    return XfsBmbtRec {
        is_unwritten: is_unwritten,
        startoff: startoff,
        startblock: startblock,
        blockcount: blockcount,
    };
}

/// Encodes an `XfsBmbtRec` into a 16-byte big-endian packed BMBT extent record.
/// Returns `16` on success, or `null` if `out` is too small or any bitfield overflows.
pub fn encodeXfsBmbtRec(rec: XfsBmbtRec, out: *[]u8): usize? {
    if (out.*.len < XFS_BMBT_REC_SIZE) {
        return null;
    }
    if (rec.startoff > 0x003FFFFFFFFFFFFF || rec.startblock > 0x000FFFFFFFFFFFFF || rec.blockcount > 0x1FFFFF) {
        return null;
    }

    let flag_bit: u64 = 0;
    if (rec.is_unwritten) {
        flag_bit = (1 as u64) << 63;
    }
    const l0 = flag_bit | (rec.startoff << 9) | (rec.startblock >> 43);
    const l1 = ((rec.startblock & 0x000007FFFFFFFFFF) << 21) | (rec.blockcount as u64);

    writeU64Be(out, 0, l0);
    writeU64Be(out, 8, l1);
    return XFS_BMBT_REC_SIZE;
}
