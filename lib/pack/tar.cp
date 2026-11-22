// Source: Absorbed from /data/dev/bun/src/libarchive/libarchive.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pack/tar.cp — POSIX.1-1988 USTAR Tar Archive Header & Zero-Copy Reader
// Absorbed from Bun src/libarchive/libarchive.cp
// ============================================================================

const std = @import("std");

pub const TAR_BLOCK_SIZE: usize = 512;

pub enum TarTypeFlag: u8 {
    Regular = '0',
    HardLink = '1',
    Symlink = '2',
    CharDevice = '3',
    BlockDevice = '4',
    Directory = '5',
    Fifo = '6',
    PaxExtendedHeader = 'x',
    PaxGlobalHeader = 'g',
    GnuLongName = 'L',
    GnuLongLink = 'K',
}

pub struct TarEntryView {
    name: []const u8 = "";
    size: u64 = 0;
    mode: u32 = 0o644;
    mtime: u64 = 0;
    typeflag: u8 = '0';
    data: []const u8 = "";

    pub fn isDirectory(): bool {
        return _.typeflag == '5' || (_.name.len > 0 && _.name[_.name.len - 1] == '/');
    }

    pub fn isRegularFile(): bool {
        return _.typeflag == '0' || _.typeflag == 0;
    }

    pub fn isSymlink(): bool {
        return _.typeflag == '2';
    }
}

pub struct TarHeader {
    /// Parses an octal ASCII field from a 512-byte USTAR header block.
    pub static fn parseOctal(field: []const u8): u64 {
        let result: u64 = 0;
        let i: usize = 0;
        while (i < field.len) {
            const c = field[i];
            if (c == 0 || c == ' ') {
                if (result > 0) {
                    break;
                }
                i += 1;
                continue;
            }
            if (c >= '0' && c <= '7') {
                result = (result << 3) | ((c - '0') as u64);
            } else {
                break;
            }
            i += 1;
        }
        return result;
    }

    /// Writes a fixed-width octal ASCII string (null-terminated) into `out`.
    pub static fn writeOctal(out: []u8, val: u64) {
        if (out.len == 0) {
            return;
        }
        let i: usize = 0;
        while (i < out.len) {
            out[i] = '0';
            i += 1;
        }
        out[out.len - 1] = 0;
        let v = val;
        let pos: usize = out.len - 1;
        while (pos > 0 && v > 0) {
            pos -= 1;
            out[pos] = (48 as u8) + ((v & 7) as u8);
            v >>= 3;
        }
    }

    /// Computes the unsigned 512-byte POSIX tar header checksum (treating bytes 148..156 as ASCII spaces).
    pub static fn computeChecksum(block: []const u8): u32 {
        if (block.len < TAR_BLOCK_SIZE) {
            return 0;
        }
        let sum: u32 = 0;
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) {
            if (i >= 148 && i < 156) {
                sum += 32; // ' '
            } else {
                sum += (block[i] as u32);
            }
            i += 1;
        }
        return sum;
    }

    /// Checks whether a 512-byte block is an all-zero End-Of-Archive marker.
    pub static fn isZeroBlock(block: []const u8): bool {
        if (block.len < TAR_BLOCK_SIZE) {
            return false;
        }
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) {
            if (block[i] != 0) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// Encodes a 512-byte USTAR header block for a file with `name`, `size`, `mode`, `mtime`, and `typeflag`.
    pub static fn encodeHeader(
        out_block: []u8,
        name: []const u8,
        size: u64,
        mode: u32,
        mtime: u64,
        typeflag: u8
    ): bool {
        if (out_block.len < TAR_BLOCK_SIZE || name.len > 100) {
            return false;
        }
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) {
            out_block[i] = 0;
            i += 1;
        }

        // Name (0..100)
        let n_idx: usize = 0;
        while (n_idx < name.len) {
            out_block[n_idx] = name[n_idx];
            n_idx += 1;
        }

        // Mode (100..108), Size (124..136), Mtime (136..148)
        TarHeader.writeOctal(out_block[100..108], mode as u64);
        TarHeader.writeOctal(out_block[108..116], 0);
        TarHeader.writeOctal(out_block[116..124], 0);
        TarHeader.writeOctal(out_block[124..136], size);
        TarHeader.writeOctal(out_block[136..148], mtime);

        // Typeflag (156)
        out_block[156] = if (typeflag == 0) ('0' as u8) else typeflag;

        // USTAR magic "ustar\x0000" (257..265)
        out_block[257] = 'u';
        out_block[258] = 's';
        out_block[259] = 't';
        out_block[260] = 'a';
        out_block[261] = 'r';
        out_block[262] = 0;
        out_block[263] = '0';
        out_block[264] = '0';

        // Checksum (148..156): 6 octal digits + '\0' + ' '
        const chksum = TarHeader.computeChecksum(out_block[0..TAR_BLOCK_SIZE]);
        TarHeader.writeOctal(out_block[148..155], chksum as u64);
        out_block[155] = ' ';
        return true;
    }
}

/// Zero-copy in-memory Tar archive iterator (BufferReadStream equivalent)
pub struct TarReader {
    buf: []const u8;
    pos: usize = 0;

    pub static fn init(buf: []const u8): TarReader {
        return TarReader {
            buf: buf,
            pos: 0,
        };
    }

    pub fn next(*_): TarEntryView? {
        if (_.pos + TAR_BLOCK_SIZE > _.buf.len) {
            return null;
        }
        const hdr = _.buf[_.pos .. _.pos + TAR_BLOCK_SIZE];
        if (TarHeader.isZeroBlock(hdr)) {
            return null;
        }

        // Verify checksum
        const expected_sum = TarHeader.parseOctal(hdr[148..156]) as u32;
        const actual_sum = TarHeader.computeChecksum(hdr);
        if (expected_sum != actual_sum) {
            return null;
        }

        // Extract null-terminated name (0..100)
        let name_end: usize = 0;
        while (name_end < 100 && hdr[name_end] != 0) {
            name_end += 1;
        }
        const name = hdr[0..name_end];
        const mode = TarHeader.parseOctal(hdr[100..108]) as u32;
        const size = TarHeader.parseOctal(hdr[124..136]);
        const mtime = TarHeader.parseOctal(hdr[136..148]);
        const typeflag = if (hdr[156] == 0) ('0' as u8) else hdr[156];

        _.pos += TAR_BLOCK_SIZE;
        const data_size = size as usize;
        if (_.pos + data_size > _.buf.len) {
            return null;
        }
        const data = _.buf[_.pos .. _.pos + data_size];

        // Advance pos rounded up to 512-byte boundary
        const padded_size = (data_size + TAR_BLOCK_SIZE - 1) & ~(TAR_BLOCK_SIZE - 1);
        _.pos += padded_size;

        return TarEntryView {
            name: name,
            size: size,
            mode: mode,
            mtime: mtime,
            typeflag: typeflag,
            data: data,
        };
    }
}
