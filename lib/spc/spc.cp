// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc SNES SPC700 .spc Audio Parser)
// Pure CP zero-heap-allocation SNES SPC700 v0.30 parser: 256B header, CPU registers,
// automatic Text vs Binary ID666 tag decoding, 64KB RAM / 128B DSP / 64B Extra RAM slices,
// and 4-byte aligned Extended ID666 (xid6) sub-chunk iterator.

const std = @import("std");

/// 33-byte SNES SPC700 v0.30 file signature.
pub const SPC_MAGIC: []const u8 = "SNES-SPC700 Sound File Data v0.30";

/// Fixed 256-byte SPC header size.
pub const SPC_HEADER_SIZE: usize = 256;

/// 64KB SPC700 RAM snapshot offset and size.
pub const SPC_RAM_OFFSET: usize = 256;
pub const SPC_RAM_SIZE: usize = 65536;

/// 128B S-DSP register snapshot offset and size.
pub const SPC_DSP_OFFSET: usize = 65792;
pub const SPC_DSP_SIZE: usize = 128;

/// 64B Extra RAM (IPL ROM area) offset and size.
pub const SPC_EXTRA_RAM_OFFSET: usize = 65984;
pub const SPC_EXTRA_RAM_SIZE: usize = 64;

/// Minimum complete SPC file size (256B header + 64KB RAM + 128B DSP + 64B pad + 64B Extra RAM).
pub const SPC_MIN_FILE_SIZE: usize = 66048;

/// 4-byte Extended ID666 chunk magic `"xid6"`.
pub const SPC_XID6_MAGIC: []const u8 = "xid6";

/// Detected ID666 tag encoding format.
pub enum SpcId666Format: u8 {
    none = 0,
    text = 1,
    binary = 2,
}

/// Initial SPC700 CPU register state (header offsets 37..44).
pub struct SpcRegs {
    pc: u16 = 0,
    a: u8 = 0,
    x: u8 = 0,
    y: u8 = 0,
    psw: u8 = 0,
    sp: u8 = 0,
}

/// Zero-copy parsed SNES SPC700 header & ID666 metadata.
pub struct SpcHeader {
    has_id666: bool = false,
    version_minor: u8 = 0,
    regs: SpcRegs = SpcRegs {},
    id666_format: SpcId666Format = SpcId666Format.none,

    song_title: []const u8 = "",
    game_title: []const u8 = "",
    dumper_name: []const u8 = "",
    comments: []const u8 = "",
    artist: []const u8 = "",
    play_seconds: u32 = 0,
    fade_ms: u32 = 0,
    emulator_id: u8 = 0,

    /// Returns a zero-copy 64KB slice of SPC700 RAM if `data` contains the full snapshot.
    fn ramSlice(data: []const u8): ([]const u8)? {
        if (data.len < SPC_RAM_OFFSET + SPC_RAM_SIZE) {
            return null;
        }
        return data[SPC_RAM_OFFSET .. SPC_RAM_OFFSET + SPC_RAM_SIZE];
    }

    /// Returns a zero-copy 128B slice of S-DSP registers if `data` contains them.
    fn dspSlice(data: []const u8): ([]const u8)? {
        if (data.len < SPC_DSP_OFFSET + SPC_DSP_SIZE) {
            return null;
        }
        return data[SPC_DSP_OFFSET .. SPC_DSP_OFFSET + SPC_DSP_SIZE];
    }

    /// Returns a zero-copy 64B slice of Extra RAM if `data` contains the full 66048B snapshot.
    fn extraRamSlice(data: []const u8): ([]const u8)? {
        if (data.len < SPC_MIN_FILE_SIZE) {
            return null;
        }
        return data[SPC_EXTRA_RAM_OFFSET .. SPC_EXTRA_RAM_OFFSET + SPC_EXTRA_RAM_SIZE];
    }
}

/// Decoded Extended ID666 (`xid6`) sub-chunk item.
pub struct SpcXid6Item {
    id: u8 = 0,
    data_type: u8 = 0,
    int_val: u32 = 0,
    str_val: []const u8 = "",
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU24Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn trimCString(slice: []const u8): []const u8 {
    let end: usize = 0;
    while (end < slice.len && slice[end] != 0) {
        end += 1;
    }
    return slice[0..end];
}

fn isAsciiDigitOrNullOrSpace(b: u8): bool {
    return (b >= '0' && b <= '9') || b == 0 || b == ' ';
}

fn isAsciiDateChar(b: u8): bool {
    return (b >= '0' && b <= '9') || b == '/' || b == '-' || b == 0 || b == ' ';
}

fn parseAsciiDigitsU32(slice: []const u8): u32 {
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < slice.len && slice[i] != 0) {
        const b = slice[i];
        if (b >= '0' && b <= '9') {
            acc = acc * 10 + ((b - '0') as u32);
        }
        i += 1;
    }
    return acc;
}

/// Distinguishes Text vs Binary ID666 format from date/duration/fade fields (offsets 158..177).
fn detectId666Format(data: []const u8): SpcId666Format {
    // Check if play_seconds (169..172) or fade_ms (172..177) contain non-ASCII-digit bytes
    let i: usize = 169;
    let has_ascii_digit = false;
    while (i < 177) {
        const b = data[i];
        if (!isAsciiDigitOrNullOrSpace(b)) {
            return SpcId666Format.binary;
        }
        if (b >= '0' && b <= '9') {
            has_ascii_digit = true;
        }
        i += 1;
    }

    // Check date field (158..169)
    let has_slash = false;
    let j: usize = 158;
    while (j < 169) {
        const b = data[j];
        if (b == '/') {
            has_slash = true;
        }
        if (!isAsciiDateChar(b)) {
            return SpcId666Format.binary;
        }
        j += 1;
    }

    if (has_slash || has_ascii_digit) {
        return SpcId666Format.text;
    }

    // Check if binary unused region (162..169) is non-zero (then text), else check date bytes
    let k: usize = 162;
    while (k < 169) {
        if (data[k] != 0) {
            return SpcId666Format.text;
        }
        k += 1;
    }
    return SpcId666Format.text;
}

// ---------------------------------------------------------------------------
// 1. Detection & Header Parser
// ---------------------------------------------------------------------------

/// Returns `true` if `data` has >= 256 bytes, the 33-byte SPC700 magic, and `[0x1A, 0x1A]` at 33..35.
pub fn isSpc(data: []const u8): bool {
    if (data.len < SPC_HEADER_SIZE) {
        return false;
    }
    if (!std.mem.eql(u8, data[0..33], SPC_MAGIC)) {
        return false;
    }
    return data[33] == 0x1A && data[34] == 0x1A;
}

/// Parses the 256-byte SNES SPC700 header, CPU registers, and Text/Binary ID666 tags.
pub fn parseSpcHeader(data: []const u8): SpcHeader? {
    if (!isSpc(data)) {
        return null;
    }

    const has_id666 = data[35] == 26;
    let hdr = SpcHeader {
        has_id666: has_id666,
        version_minor: data[36],
        regs: SpcRegs {
            pc: readU16Le(data, 37),
            a: data[39],
            x: data[40],
            y: data[41],
            psw: data[42],
            sp: data[43],
        },
        id666_format: SpcId666Format.none,
    };

    if (!has_id666) {
        return hdr;
    }

    // Common ID666 string fields (offsets 46..158)
    hdr.song_title = trimCString(data[46..78]);
    hdr.game_title = trimCString(data[78..110]);
    hdr.dumper_name = trimCString(data[110..126]);
    hdr.comments = trimCString(data[126..158]);

    const fmt = detectId666Format(data);
    hdr.id666_format = fmt;

    if (fmt == SpcId666Format.text) {
        hdr.play_seconds = parseAsciiDigitsU32(data[169..172]);
        hdr.fade_ms = parseAsciiDigitsU32(data[172..177]);
        hdr.artist = trimCString(data[177..209]);
        const emu = data[210];
        hdr.emulator_id = if (emu >= '0' && emu <= '9') (emu - '0') else emu;
    } else {
        hdr.play_seconds = readU24Le(data, 169);
        hdr.fade_ms = readU32Le(data, 172);
        hdr.artist = trimCString(data[176..208]);
        hdr.emulator_id = data[209];
    }

    return hdr;
}

// ---------------------------------------------------------------------------
// 2. Extended ID666 (`xid6`) 4-Byte Aligned Sub-Chunk Iterator
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over Extended ID666 (`xid6`) sub-chunks appended after offset 66048.
pub struct SpcXid6Iter {
    src: []const u8 = "",
    pos: usize = 0,
    end: usize = 0,
    done: bool = true,

    /// Initializes iteration from a full SPC file buffer (locates `"xid6"` at offset 66048).
    fn init(file_data: []const u8): SpcXid6Iter {
        if (file_data.len < SPC_MIN_FILE_SIZE + 8) {
            return SpcXid6Iter { done: true };
        }
        const xid6_off = SPC_MIN_FILE_SIZE;
        if (!std.mem.eql(u8, file_data[xid6_off .. xid6_off + 4], SPC_XID6_MAGIC)) {
            return SpcXid6Iter { done: true };
        }
        const chunk_size = readU32Le(file_data, xid6_off + 4) as usize;
        const body_start = xid6_off + 8;
        if (chunk_size > file_data.len - body_start) {
            return SpcXid6Iter { done: true };
        }
        return SpcXid6Iter {
            src: file_data,
            pos: body_start,
            end: body_start + chunk_size,
            done: chunk_size < 4,
        };
    }

    fn next(*_): SpcXid6Item? {
        if (_.done || _.pos + 4 > _.end) {
            _.done = true;
            return null;
        }

        const sub_id = _.src[_.pos];
        const dtype = _.src[_.pos + 1];
        const header_val = readU16Le(_.src, _.pos + 2);
        _.pos += 4;

        if (dtype == 0) {
            // Inline 16-bit length/data value (total 4 bytes)
            return SpcXid6Item {
                id: sub_id,
                data_type: dtype,
                int_val: header_val as u32,
                str_val: "",
            };
        }

        if (dtype == 1) {
            // String payload of `header_val` bytes, padded to 4-byte boundary
            const slen = header_val as usize;
            const padded = (slen + 3) & (~usize(3));
            if (slen > _.end - _.pos) {
                _.done = true;
                return null;
            }
            const raw_str = trimCString(_.src[_.pos .. _.pos + slen]);
            if (padded <= _.end - _.pos) {
                _.pos += padded;
            } else {
                _.pos = _.end;
            }
            return SpcXid6Item {
                id: sub_id,
                data_type: dtype,
                int_val: 0,
                str_val: raw_str,
            };
        }

        if (dtype == 4) {
            // 32-bit LE integer payload (4 bytes)
            if (_.pos + 4 > _.end) {
                _.done = true;
                return null;
            }
            const val = readU32Le(_.src, _.pos);
            _.pos += 4;
            return SpcXid6Item {
                id: sub_id,
                data_type: dtype,
                int_val: val,
                str_val: "",
            };
        }

        // Unknown data_type -> terminate safely
        _.done = true;
        return null;
    }
}
