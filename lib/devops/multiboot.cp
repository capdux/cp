// Origin: Pure-CP Stdlib (Zero-Alloc GNU Multiboot1 & Multiboot2 Header, Boot Info & Mmap Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Multiboot1 & Multiboot2 Protocol Constants (Little-Endian)
// ---------------------------------------------------------------------------

pub const MB1_HEADER_MAGIC: u32 = 0x1BADB002;
pub const MB1_BOOTLOADER_MAGIC: u32 = 0x2BADB002;
pub const MB2_HEADER_MAGIC: u32 = 0xE85250D6;
pub const MB2_BOOTLOADER_MAGIC: u32 = 0x36D76289;

pub const MB2_ARCH_I386: u32 = 0;
pub const MB2_ARCH_MIPS32: u32 = 4;

// Multiboot2 Boot Information Tag Types
pub const TAG_END: u32 = 0;
pub const TAG_CMDLINE: u32 = 1;
pub const TAG_BOOT_LOADER_NAME: u32 = 2;
pub const TAG_MODULE: u32 = 3;
pub const TAG_BASIC_MEMINFO: u32 = 4;
pub const TAG_BOOTDEV: u32 = 5;
pub const TAG_MMAP: u32 = 6;
pub const TAG_VBE: u32 = 7;
pub const TAG_FRAMEBUFFER: u32 = 8;
pub const TAG_ELF_SECTIONS: u32 = 9;
pub const TAG_APM: u32 = 10;
pub const TAG_ACPI_OLD: u32 = 14;
pub const TAG_ACPI_NEW: u32 = 15;

// Multiboot2 Memory Map Entry Types
pub const MMAP_AVAILABLE: u32 = 1;
pub const MMAP_RESERVED: u32 = 2;
pub const MMAP_ACPI_RECLAIMABLE: u32 = 3;
pub const MMAP_NVS: u32 = 4;
pub const MMAP_BADRAM: u32 = 5;

// ---------------------------------------------------------------------------
// Internal Little-Endian & Alignment Helpers
// ---------------------------------------------------------------------------

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn readU64Le(buf: []const u8, off: usize): u64 {
    const lo = readU32Le(buf, off) as u64;
    const hi = readU32Le(buf, off + 4) as u64;
    return lo | (hi << 32);
}

fn align8(n: usize): usize {
    const rem = n % 8;
    if (rem == 0) {
        return n;
    }
    return n + (8 - rem);
}

fn stripNul(bytes: []const u8): []const u8 {
    let i: usize = 0;
    while (i < bytes.len) {
        if (bytes[i] == 0) {
            return bytes[0..i];
        }
        i += 1;
    }
    return bytes;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// Parsed GNU Multiboot1 kernel header (12 bytes).
pub struct Mb1Header {
    flags: u32,
    checksum: u32,
}

/// Parsed GNU Multiboot2 kernel header (16+ bytes).
pub struct Mb2Header {
    arch: u32,
    header_length: u32,
    checksum: u32,
    tags_bytes: []const u8,
}

/// A single zero-copy Multiboot2 Boot Information Tag.
pub struct Mb2Tag {
    tag_type: u32,
    size: u32,
    body: []const u8,

    /// Returns the tag body stripped at the first NUL terminator (for cmdline/bootloader name).
    fn asString(): []const u8 {
        return stripNul(_.body);
    }
}

/// Zero-copy 8-byte-aligned iterator over Multiboot2 Boot Information tags until `TAG_END`.
pub struct Mb2TagIter {
    bytes: []const u8,
    pos: usize,
    done: bool,
    err: bool,

    fn hasError(): bool {
        return _.err;
    }

    fn next(*_): Mb2Tag? {
        if (_.done || _.err) {
            return null;
        }
        if (_.pos + 8 > _.bytes.len) {
            _.err = true;
            _.done = true;
            return null;
        }
        const tag_type = readU32Le(_.bytes, _.pos);
        const size = readU32Le(_.bytes, _.pos + 4);
        if (size < 8) {
            _.err = true;
            _.done = true;
            return null;
        }
        const sz: usize = size as usize;
        if (sz > _.bytes.len - _.pos) {
            _.err = true;
            _.done = true;
            return null;
        }
        if (tag_type == TAG_END) {
            _.done = true;
            return null;
        }
        const body_start = _.pos + 8;
        const body_end = _.pos + sz;
        const step = align8(sz);
        if (step > _.bytes.len - _.pos) {
            _.pos = _.bytes.len;
        } else {
            _.pos += step;
        }
        return Mb2Tag {
            tag_type: tag_type,
            size: size,
            body: _.bytes[body_start..body_end],
        };
    }
}

/// Validated Multiboot2 Boot Information structure.
pub struct Mb2BootInfo {
    total_size: u32,
    tags_bytes: []const u8,

    fn tags(): Mb2TagIter {
        return Mb2TagIter {
            bytes: _.tags_bytes,
            pos: 0,
            done: false,
            err: false,
        };
    }

    fn findTag(tag_type: u32): Mb2Tag? {
        let iter = _.tags();
        while (tag := iter.next()) {
            if (tag.tag_type == tag_type) {
                return tag;
            }
        }
        return null;
    }

    fn cmdline(): ([]const u8)? {
        if (tag := _.findTag(TAG_CMDLINE)) {
            return tag.asString();
        }
        return null;
    }

    fn bootloaderName(): ([]const u8)? {
        if (tag := _.findTag(TAG_BOOT_LOADER_NAME)) {
            return tag.asString();
        }
        return null;
    }
}

/// Multiboot2 Basic Memory Information (`TAG_BASIC_MEMINFO`).
pub struct Mb2BasicMemInfo {
    mem_lower_kb: u32,
    mem_upper_kb: u32,
}

/// A single entry in the Multiboot2 Memory Map (`TAG_MMAP`).
pub struct Mb2MmapEntry {
    base_addr: u64,
    length: u64,
    entry_type: u32,

    fn isAvailable(): bool {
        return _.entry_type == MMAP_AVAILABLE;
    }
}

/// Zero-copy iterator over `Mb2MmapEntry` records inside a `TAG_MMAP` tag.
pub struct Mb2MmapIter {
    entries_bytes: []const u8,
    entry_size: usize,
    pos: usize,

    fn next(*_): Mb2MmapEntry? {
        if (_.entry_size < 24 || _.pos + _.entry_size > _.entries_bytes.len) {
            return null;
        }
        const off = _.pos;
        _.pos += _.entry_size;
        return Mb2MmapEntry {
            base_addr: readU64Le(_.entries_bytes, off),
            length: readU64Le(_.entries_bytes, off + 8),
            entry_type: readU32Le(_.entries_bytes, off + 16),
        };
    }
}

/// Multiboot2 Framebuffer information (`TAG_FRAMEBUFFER`).
pub struct Mb2Framebuffer {
    addr: u64,
    pitch: u32,
    width: u32,
    height: u32,
    bpp: u8,
    fb_type: u8,
}

/// Multiboot2 loaded boot module (`TAG_MODULE`).
pub struct Mb2Module {
    mod_start: u32,
    mod_end: u32,
    cmdline: []const u8,
}

// ---------------------------------------------------------------------------
// Parsing Functions
// ---------------------------------------------------------------------------

/// Parses and validates a Multiboot1 kernel header (`magic == 0x1BADB002` and 32-bit wrapping sum == 0).
pub fn parseMb1Header(data: []const u8): Mb1Header? {
    if (data.len < 12) {
        return null;
    }
    const magic = readU32Le(data, 0);
    if (magic != MB1_HEADER_MAGIC) {
        return null;
    }
    const flags = readU32Le(data, 4);
    const checksum = readU32Le(data, 8);
    const sum32: u32 = (((magic as u64) + (flags as u64) + (checksum as u64)) & 0xFFFFFFFF) as u32;
    if (sum32 != 0) {
        return null;
    }
    return Mb1Header {
        flags: flags,
        checksum: checksum,
    };
}

/// Parses and validates a Multiboot2 kernel header (`magic == 0xE85250D6`, 4-word 32-bit wrapping sum == 0,
/// and `16 <= header_length <= data.len`).
pub fn parseMb2Header(data: []const u8): Mb2Header? {
    if (data.len < 16) {
        return null;
    }
    const magic = readU32Le(data, 0);
    if (magic != MB2_HEADER_MAGIC) {
        return null;
    }
    const arch = readU32Le(data, 4);
    const header_length = readU32Le(data, 8);
    const checksum = readU32Le(data, 12);
    if (header_length < 16 || (header_length as usize) > data.len) {
        return null;
    }
    const sum32: u32 = (((magic as u64) + (arch as u64) + (header_length as u64) + (checksum as u64)) & 0xFFFFFFFF) as u32;
    if (sum32 != 0) {
        return null;
    }
    return Mb2Header {
        arch: arch,
        header_length: header_length,
        checksum: checksum,
        tags_bytes: data[16 .. header_length as usize],
    };
}

/// Parses and validates the Multiboot2 Boot Information header (`total_size >= 8`, `reserved == 0`).
pub fn parseMb2BootInfo(data: []const u8): Mb2BootInfo? {
    if (data.len < 8) {
        return null;
    }
    const total_size = readU32Le(data, 0);
    const reserved = readU32Le(data, 4);
    if (reserved != 0) {
        return null;
    }
    if (total_size < 8 || (total_size as usize) > data.len) {
        return null;
    }
    return Mb2BootInfo {
        total_size: total_size,
        tags_bytes: data[8 .. total_size as usize],
    };
}

/// Decodes a `TAG_BASIC_MEMINFO` tag into `Mb2BasicMemInfo`.
pub fn parseMb2BasicMemInfo(tag: Mb2Tag): Mb2BasicMemInfo? {
    if (tag.tag_type != TAG_BASIC_MEMINFO || tag.body.len < 8) {
        return null;
    }
    return Mb2BasicMemInfo {
        mem_lower_kb: readU32Le(tag.body, 0),
        mem_upper_kb: readU32Le(tag.body, 4),
    };
}

/// Creates a zero-copy `Mb2MmapIter` over a `TAG_MMAP` tag.
pub fn iterMb2Mmap(tag: Mb2Tag): Mb2MmapIter? {
    if (tag.tag_type != TAG_MMAP || tag.body.len < 8) {
        return null;
    }
    const entry_size = readU32Le(tag.body, 0) as usize;
    if (entry_size < 24) {
        return null;
    }
    return Mb2MmapIter {
        entries_bytes: tag.body[8..tag.body.len],
        entry_size: entry_size,
        pos: 0,
    };
}

/// Decodes a `TAG_FRAMEBUFFER` tag into `Mb2Framebuffer`.
pub fn parseMb2Framebuffer(tag: Mb2Tag): Mb2Framebuffer? {
    if (tag.tag_type != TAG_FRAMEBUFFER || tag.body.len < 24) {
        return null;
    }
    return Mb2Framebuffer {
        addr: readU64Le(tag.body, 0),
        pitch: readU32Le(tag.body, 8),
        width: readU32Le(tag.body, 12),
        height: readU32Le(tag.body, 16),
        bpp: tag.body[20],
        fb_type: tag.body[21],
    };
}

/// Decodes a `TAG_MODULE` tag into `Mb2Module`.
pub fn parseMb2Module(tag: Mb2Tag): Mb2Module? {
    if (tag.tag_type != TAG_MODULE || tag.body.len < 8) {
        return null;
    }
    return Mb2Module {
        mod_start: readU32Le(tag.body, 0),
        mod_end: readU32Le(tag.body, 4),
        cmdline: stripNul(tag.body[8..tag.body.len]),
    };
}
