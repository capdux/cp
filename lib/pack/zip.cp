// ============================================================================
// lib/pack/zip.cp — PKZIP Local File Header, Central Directory, EOCD & Zero-Copy Reader
// Absorbed from Bun src/libarchive/
// ============================================================================

import { Crc32 } from "./checksum.cp";

pub const ZIP_LOCAL_HEADER_SIG: u32 = 0x04034b50;
pub const ZIP_CENTRAL_DIR_SIG: u32 = 0x02014b50;
pub const ZIP_EOCD_SIG: u32 = 0x06054b50;

pub const ZIP_LOCAL_HEADER_SIZE: usize = 30;
pub const ZIP_CENTRAL_DIR_SIZE: usize = 46;
pub const ZIP_EOCD_SIZE: usize = 22;

pub const ZIP_METHOD_STORE: u16 = 0;
pub const ZIP_METHOD_DEFLATE: u16 = 8;

pub struct ZipEndian {
    pub static fn readU16LE(buf: []const u8, offset: usize): u16 {
        return (buf[offset] as u16) | ((buf[offset + 1] as u16) << 8);
    }

    pub static fn readU32LE(buf: []const u8, offset: usize): u32 {
        return (buf[offset] as u32)
            | ((buf[offset + 1] as u32) << 8)
            | ((buf[offset + 2] as u32) << 16)
            | ((buf[offset + 3] as u32) << 24);
    }

    pub static fn writeU16LE(out: []u8, offset: usize, val: u16) {
        out[offset] = (val & 0xFF) as u8;
        out[offset + 1] = ((val >> 8) & 0xFF) as u8;
    }

    pub static fn writeU32LE(out: []u8, offset: usize, val: u32) {
        out[offset] = (val & 0xFF) as u8;
        out[offset + 1] = ((val >> 8) & 0xFF) as u8;
        out[offset + 2] = ((val >> 16) & 0xFF) as u8;
        out[offset + 3] = ((val >> 24) & 0xFF) as u8;
    }
}

/// PKZIP 30-byte Local File Header (0x04034b50)
pub struct ZipLocalHeader {
    version_needed: u16 = 20;
    flags: u16 = 0;
    compression_method: u16 = 0;
    mtime: u16 = 0;
    mdate: u16 = 0;
    crc32: u32 = 0;
    compressed_size: u32 = 0;
    uncompressed_size: u32 = 0;
    name_len: u16 = 0;
    extra_len: u16 = 0;

    pub fn write(out: []u8): usize {
        if (out.len < ZIP_LOCAL_HEADER_SIZE) {
            return 0;
        }
        ZipEndian.writeU32LE(out, 0, ZIP_LOCAL_HEADER_SIG);
        ZipEndian.writeU16LE(out, 4, _.version_needed);
        ZipEndian.writeU16LE(out, 6, _.flags);
        ZipEndian.writeU16LE(out, 8, _.compression_method);
        ZipEndian.writeU16LE(out, 10, _.mtime);
        ZipEndian.writeU16LE(out, 12, _.mdate);
        ZipEndian.writeU32LE(out, 14, _.crc32);
        ZipEndian.writeU32LE(out, 18, _.compressed_size);
        ZipEndian.writeU32LE(out, 22, _.uncompressed_size);
        ZipEndian.writeU16LE(out, 26, _.name_len);
        ZipEndian.writeU16LE(out, 28, _.extra_len);
        return ZIP_LOCAL_HEADER_SIZE;
    }

    pub static fn parse(buf: []const u8): ZipLocalHeader? {
        if (buf.len < ZIP_LOCAL_HEADER_SIZE) {
            return null;
        }
        if (ZipEndian.readU32LE(buf, 0) != ZIP_LOCAL_HEADER_SIG) {
            return null;
        }
        return ZipLocalHeader {
            version_needed: ZipEndian.readU16LE(buf, 4),
            flags: ZipEndian.readU16LE(buf, 6),
            compression_method: ZipEndian.readU16LE(buf, 8),
            mtime: ZipEndian.readU16LE(buf, 10),
            mdate: ZipEndian.readU16LE(buf, 12),
            crc32: ZipEndian.readU32LE(buf, 14),
            compressed_size: ZipEndian.readU32LE(buf, 18),
            uncompressed_size: ZipEndian.readU32LE(buf, 22),
            name_len: ZipEndian.readU16LE(buf, 26),
            extra_len: ZipEndian.readU16LE(buf, 28),
        };
    }

    /// Encodes a complete stored local file entry (30-byte header + file name + payload) into `out`.
    pub static fn encodeEntry(out: []u8, name: []const u8, payload: []const u8): usize {
        const total_len = ZIP_LOCAL_HEADER_SIZE + name.len + payload.len;
        if (out.len < total_len) {
            return 0;
        }
        const crc = Crc32.checksum(payload);
        const hdr = ZipLocalHeader {
            version_needed: 20,
            flags: 0,
            compression_method: ZIP_METHOD_STORE,
            mtime: 0,
            mdate: 0,
            crc32: crc,
            compressed_size: payload.len as u32,
            uncompressed_size: payload.len as u32,
            name_len: name.len as u16,
            extra_len: 0,
        };
        * = hdr.write(out[0..ZIP_LOCAL_HEADER_SIZE]);
        let i: usize = 0;
        while (i < name.len) {
            out[ZIP_LOCAL_HEADER_SIZE + i] = name[i];
            i += 1;
        }
        let j: usize = 0;
        while (j < payload.len) {
            out[ZIP_LOCAL_HEADER_SIZE + name.len + j] = payload[j];
            j += 1;
        }
        return total_len;
    }
}

/// PKZIP 46-byte Central Directory File Header (0x02014b50)
pub struct ZipCentralHeader {
    version_made_by: u16 = 20;
    version_needed: u16 = 20;
    flags: u16 = 0;
    compression_method: u16 = 0;
    mtime: u16 = 0;
    mdate: u16 = 0;
    crc32: u32 = 0;
    compressed_size: u32 = 0;
    uncompressed_size: u32 = 0;
    name_len: u16 = 0;
    extra_len: u16 = 0;
    comment_len: u16 = 0;
    disk_num_start: u16 = 0;
    internal_attrs: u16 = 0;
    external_attrs: u32 = 0;
    local_header_offset: u32 = 0;

    pub fn write(out: []u8): usize {
        if (out.len < ZIP_CENTRAL_DIR_SIZE) {
            return 0;
        }
        ZipEndian.writeU32LE(out, 0, ZIP_CENTRAL_DIR_SIG);
        ZipEndian.writeU16LE(out, 4, _.version_made_by);
        ZipEndian.writeU16LE(out, 6, _.version_needed);
        ZipEndian.writeU16LE(out, 8, _.flags);
        ZipEndian.writeU16LE(out, 10, _.compression_method);
        ZipEndian.writeU16LE(out, 12, _.mtime);
        ZipEndian.writeU16LE(out, 14, _.mdate);
        ZipEndian.writeU32LE(out, 16, _.crc32);
        ZipEndian.writeU32LE(out, 20, _.compressed_size);
        ZipEndian.writeU32LE(out, 24, _.uncompressed_size);
        ZipEndian.writeU16LE(out, 28, _.name_len);
        ZipEndian.writeU16LE(out, 30, _.extra_len);
        ZipEndian.writeU16LE(out, 32, _.comment_len);
        ZipEndian.writeU16LE(out, 34, _.disk_num_start);
        ZipEndian.writeU16LE(out, 36, _.internal_attrs);
        ZipEndian.writeU32LE(out, 38, _.external_attrs);
        ZipEndian.writeU32LE(out, 42, _.local_header_offset);
        return ZIP_CENTRAL_DIR_SIZE;
    }

    pub static fn parse(buf: []const u8): ZipCentralHeader? {
        if (buf.len < ZIP_CENTRAL_DIR_SIZE) {
            return null;
        }
        if (ZipEndian.readU32LE(buf, 0) != ZIP_CENTRAL_DIR_SIG) {
            return null;
        }
        return ZipCentralHeader {
            version_made_by: ZipEndian.readU16LE(buf, 4),
            version_needed: ZipEndian.readU16LE(buf, 6),
            flags: ZipEndian.readU16LE(buf, 8),
            compression_method: ZipEndian.readU16LE(buf, 10),
            mtime: ZipEndian.readU16LE(buf, 12),
            mdate: ZipEndian.readU16LE(buf, 14),
            crc32: ZipEndian.readU32LE(buf, 16),
            compressed_size: ZipEndian.readU32LE(buf, 20),
            uncompressed_size: ZipEndian.readU32LE(buf, 24),
            name_len: ZipEndian.readU16LE(buf, 28),
            extra_len: ZipEndian.readU16LE(buf, 30),
            comment_len: ZipEndian.readU16LE(buf, 32),
            disk_num_start: ZipEndian.readU16LE(buf, 34),
            internal_attrs: ZipEndian.readU16LE(buf, 36),
            external_attrs: ZipEndian.readU32LE(buf, 38),
            local_header_offset: ZipEndian.readU32LE(buf, 42),
        };
    }
}

/// PKZIP 22-byte End of Central Directory Record (0x06054b50)
pub struct ZipEocd {
    disk_num: u16 = 0;
    central_dir_disk: u16 = 0;
    entries_on_disk: u16 = 0;
    total_entries: u16 = 0;
    central_dir_size: u32 = 0;
    central_dir_offset: u32 = 0;
    comment_len: u16 = 0;

    pub fn write(out: []u8): usize {
        if (out.len < ZIP_EOCD_SIZE) {
            return 0;
        }
        ZipEndian.writeU32LE(out, 0, ZIP_EOCD_SIG);
        ZipEndian.writeU16LE(out, 4, _.disk_num);
        ZipEndian.writeU16LE(out, 6, _.central_dir_disk);
        ZipEndian.writeU16LE(out, 8, _.entries_on_disk);
        ZipEndian.writeU16LE(out, 10, _.total_entries);
        ZipEndian.writeU32LE(out, 12, _.central_dir_size);
        ZipEndian.writeU32LE(out, 16, _.central_dir_offset);
        ZipEndian.writeU16LE(out, 20, _.comment_len);
        return ZIP_EOCD_SIZE;
    }

    pub static fn parse(buf: []const u8): ZipEocd? {
        if (buf.len < ZIP_EOCD_SIZE) {
            return null;
        }
        if (ZipEndian.readU32LE(buf, 0) != ZIP_EOCD_SIG) {
            return null;
        }
        return ZipEocd {
            disk_num: ZipEndian.readU16LE(buf, 4),
            central_dir_disk: ZipEndian.readU16LE(buf, 6),
            entries_on_disk: ZipEndian.readU16LE(buf, 8),
            total_entries: ZipEndian.readU16LE(buf, 10),
            central_dir_size: ZipEndian.readU32LE(buf, 12),
            central_dir_offset: ZipEndian.readU32LE(buf, 16),
            comment_len: ZipEndian.readU16LE(buf, 20),
        };
    }

    /// Scans backward from the end of `buf` to locate and parse the End of Central Directory record.
    pub static fn findInBuffer(buf: []const u8): ZipEocd? {
        if (buf.len < ZIP_EOCD_SIZE) {
            return null;
        }
        let pos: usize = buf.len - ZIP_EOCD_SIZE;
        while (true) {
            if (ZipEndian.readU32LE(buf, pos) == ZIP_EOCD_SIG) {
                return ZipEocd.parse(buf[pos..]);
            }
            if (pos == 0) {
                break;
            }
            pos -= 1;
        }
        return null;
    }
}

pub struct ZipEntryView {
    name: []const u8 = "";
    compression_method: u16 = 0;
    crc32: u32 = 0;
    compressed_size: u32 = 0;
    uncompressed_size: u32 = 0;
    data: []const u8 = "";

    pub fn isStored(): bool {
        return _.compression_method == ZIP_METHOD_STORE;
    }

    pub fn verifyCrc(): bool {
        return Crc32.checksum(_.data) == _.crc32;
    }
}

/// Zero-copy sequential scanner over PKZIP Local File Entries in memory
pub struct ZipReader {
    buf: []const u8;
    pos: usize = 0;

    pub static fn init(buf: []const u8): ZipReader {
        return ZipReader {
            buf: buf,
            pos: 0,
        };
    }

    pub fn next(*_): ZipEntryView? {
        if (_.pos + ZIP_LOCAL_HEADER_SIZE > _.buf.len) {
            return null;
        }
        if (hdr := ZipLocalHeader.parse(_.buf[_.pos..])) {
            const name_start = _.pos + ZIP_LOCAL_HEADER_SIZE;
            const name_end = name_start + (hdr.name_len as usize);
            const data_start = name_end + (hdr.extra_len as usize);
            const data_end = data_start + (hdr.compressed_size as usize);
            if (data_end > _.buf.len) {
                return null;
            }
            _.pos = data_end;
            return ZipEntryView {
                name: _.buf[name_start..name_end],
                compression_method: hdr.compression_method,
                crc32: hdr.crc32,
                compressed_size: hdr.compressed_size,
                uncompressed_size: hdr.uncompressed_size,
                data: _.buf[data_start..data_end],
            };
        }
        return null;
    }
}
