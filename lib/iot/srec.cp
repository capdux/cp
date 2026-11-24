// Origin: Pure-CP Stdlib (Zero-Alloc Motorola S-Record S0-S9 Firmware Codec) | Created: 2026-11-22
// Pure CP zero-heap-allocation Motorola S-Record (.srec/.s19/.s28/.s37) parser, encoder,
// one's-complement checksum verifier, multi-line iterator, and firmware summarizer.

const std = @import("std");

/// Motorola S-Record type tag (S0..S9; S4 is reserved and rejected).
pub enum SrecType: u8 {
    s0_header = 0,
    s1_data16 = 1,
    s2_data24 = 2,
    s3_data32 = 3,
    s5_count16 = 5,
    s6_count24 = 6,
    s7_start32 = 7,
    s8_start24 = 8,
    s9_start16 = 9,
    unknown = 255,
}

/// Decoded Motorola S-Record line stored completely on the stack (zero heap allocation).
pub struct SrecRecord {
    rec_type: SrecType = SrecType.unknown,
    address: u32 = 0,
    data_len: u8 = 0,
    data: [255]u8 = [0; 255],
    checksum: u8 = 0,

    /// Returns a read-only slice over the decoded payload bytes.
    fn dataSlice(): []const u8 {
        return _.data[0 .. (_.data_len as usize)];
    }

    /// Returns `true` for data records (S1, S2, S3).
    fn isData(): bool {
        return _.rec_type == SrecType.s1_data16 ||
            _.rec_type == SrecType.s2_data24 ||
            _.rec_type == SrecType.s3_data32;
    }

    /// Returns `true` for start-address termination records (S7, S8, S9).
    fn isTermination(): bool {
        return _.rec_type == SrecType.s7_start32 ||
            _.rec_type == SrecType.s8_start24 ||
            _.rec_type == SrecType.s9_start16;
    }
}

/// Aggregated single-pass summary of a Motorola S-Record firmware image.
pub struct SrecStats {
    header_text_len: usize = 0,
    data_records: u32 = 0,
    total_bytes: usize = 0,
    min_addr: u32 = 0,
    max_addr: u32 = 0,
    entry_addr: u32? = null,
    declared_count: u32? = null,
    count_matches: bool = true,
}

// ---------------------------------------------------------------------------
// Internal Hex Helpers
// ---------------------------------------------------------------------------

const HEX_UPPER: []const u8 = "0123456789ABCDEF";

fn hexNibble(b: u8): u8? {
    if (b >= '0' && b <= '9') {
        return b - '0';
    }
    if (b >= 'A' && b <= 'F') {
        return b - 'A' + 10;
    }
    if (b >= 'a' && b <= 'f') {
        return b - 'a' + 10;
    }
    return null;
}

fn parseHexU8(hi: u8, lo: u8): u8? {
    const h = hexNibble(hi) ?? return null;
    const l = hexNibble(lo) ?? return null;
    return (h << 4) | l;
}

fn writeHexU8(out: []u8, pos: usize, val: u8) {
    out[pos] = HEX_UPPER[(val >> 4) as usize];
    out[pos + 1] = HEX_UPPER[(val & 0x0F) as usize];
}

fn trimLineEnd(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\n' || line[end - 1] == '\r')) {
        end -= 1;
    }
    return line[0..end];
}

fn isBlankLine(line: []const u8): bool {
    let i: usize = 0;
    while (i < line.len) {
        const b = line[i];
        if (b != ' ' && b != '\t' && b != '\r' && b != '\n') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn recTypeFromDigit(b: u8): SrecType? {
    if (b == '0') return SrecType.s0_header;
    if (b == '1') return SrecType.s1_data16;
    if (b == '2') return SrecType.s2_data24;
    if (b == '3') return SrecType.s3_data32;
    if (b == '5') return SrecType.s5_count16;
    if (b == '6') return SrecType.s6_count24;
    if (b == '7') return SrecType.s7_start32;
    if (b == '8') return SrecType.s8_start24;
    if (b == '9') return SrecType.s9_start16;
    return null;
}

fn recTypeToDigit(t: SrecType): u8? {
    if (t == SrecType.s0_header) return '0';
    if (t == SrecType.s1_data16) return '1';
    if (t == SrecType.s2_data24) return '2';
    if (t == SrecType.s3_data32) return '3';
    if (t == SrecType.s5_count16) return '5';
    if (t == SrecType.s6_count24) return '6';
    if (t == SrecType.s7_start32) return '7';
    if (t == SrecType.s8_start24) return '8';
    if (t == SrecType.s9_start16) return '9';
    return null;
}

fn requiresZeroPayload(t: SrecType): bool {
    return t == SrecType.s5_count16 ||
        t == SrecType.s6_count24 ||
        t == SrecType.s7_start32 ||
        t == SrecType.s8_start24 ||
        t == SrecType.s9_start16;
}

// ---------------------------------------------------------------------------
// Public API: Address Width & One's-Complement Checksum
// ---------------------------------------------------------------------------

/// Returns the address field width in bytes for an S-Record type:
/// - S0, S1, S5, S9 -> 2 bytes (16-bit)
/// - S2, S6, S8     -> 3 bytes (24-bit)
/// - S3, S7         -> 4 bytes (32-bit)
/// - unknown        -> 0
pub fn srecAddrBytes(t: SrecType): u8 {
    if (t == SrecType.s0_header || t == SrecType.s1_data16 || t == SrecType.s5_count16 || t == SrecType.s9_start16) {
        return 2;
    }
    if (t == SrecType.s2_data24 || t == SrecType.s6_count24 || t == SrecType.s8_start24) {
        return 3;
    }
    if (t == SrecType.s3_data32 || t == SrecType.s7_start32) {
        return 4;
    }
    return 0;
}

/// Computes the Motorola S-Record one's-complement checksum `(~sum) & 0xFF`
/// over `byte_count`, all big-endian address bytes (`addr_bytes`), and `payload`.
pub fn srecChecksum(byte_count: u8, addr_bytes: u8, address: u32, payload: []const u8): u8 {
    let sum: u32 = byte_count as u32;
    if (addr_bytes >= 4) {
        sum += (address >> 24) & 0xFF;
    }
    if (addr_bytes >= 3) {
        sum += (address >> 16) & 0xFF;
    }
    if (addr_bytes >= 2) {
        sum += (address >> 8) & 0xFF;
    }
    if (addr_bytes >= 1) {
        sum += address & 0xFF;
    }
    let i: usize = 0;
    while (i < payload.len) {
        sum += payload[i] as u32;
        i += 1;
    }
    return (~(sum as u8)) & 0xFF;
}

// ---------------------------------------------------------------------------
// Public API: Line Parser & Encoder
// ---------------------------------------------------------------------------

/// Parses and strictly validates a single Motorola S-Record line (ignores trailing `\r\n`).
/// Rejects reserved S4, invalid hex, wrong line length, non-empty payload on S5-S9, and bad checksums.
pub fn parseSrecLine(line: []const u8): SrecRecord? {
    const trimmed = trimLineEnd(line);
    if (trimmed.len < 10 || trimmed[0] != 'S') {
        return null;
    }
    const rec_type = recTypeFromDigit(trimmed[1]) ?? return null;
    const addr_bytes = srecAddrBytes(rec_type);
    const byte_count = parseHexU8(trimmed[2], trimmed[3]) ?? return null;

    if (byte_count < addr_bytes + 1) {
        return null;
    }
    const bc_usize: usize = byte_count as usize;
    const expected_len: usize = bc_usize * 2 + 4;
    if (trimmed.len != expected_len) {
        return null;
    }

    const data_len: u8 = byte_count - addr_bytes - 1;
    if (requiresZeroPayload(rec_type) && data_len != 0) {
        return null;
    }

    // Parse big-endian address
    let pos: usize = 4;
    let addr: u32 = 0;
    let ab: u8 = 0;
    while (ab < addr_bytes) {
        const b = parseHexU8(trimmed[pos], trimmed[pos + 1]) ?? return null;
        addr = (addr << 8) | (b as u32);
        pos += 2;
        ab += 1;
    }

    // Parse payload bytes
    let rec = SrecRecord {
        rec_type: rec_type,
        address: addr,
        data_len: data_len,
        data: [0; 255],
        checksum: 0,
    };

    let di: usize = 0;
    const dlen = data_len as usize;
    while (di < dlen) {
        const b = parseHexU8(trimmed[pos], trimmed[pos + 1]) ?? return null;
        rec.data[di] = b;
        pos += 2;
        di += 1;
    }

    // Parse and verify one's-complement checksum
    const cs = parseHexU8(trimmed[pos], trimmed[pos + 1]) ?? return null;
    const expected_cs = srecChecksum(byte_count, addr_bytes, addr, rec.data[0..dlen]);
    if (cs != expected_cs) {
        return null;
    }
    rec.checksum = cs;
    return rec;
}

/// Encodes a Motorola S-Record line in uppercase format `S<n><LL><ADDR><DATA><CC>\n` into `out`.
/// Returns total bytes written, or `null` if arguments are invalid or `out` is too small.
pub fn encodeSrecLine(rec_type: SrecType, address: u32, payload: []const u8, out: []u8): usize? {
    const digit = recTypeToDigit(rec_type) ?? return null;
    const addr_bytes = srecAddrBytes(rec_type);
    if (addr_bytes == 0) {
        return null;
    }
    if (requiresZeroPayload(rec_type) && payload.len != 0) {
        return null;
    }
    if (addr_bytes == 2 && address > 0xFFFF) {
        return null;
    }
    if (addr_bytes == 3 && address > 0xFFFFFF) {
        return null;
    }

    const total_count: usize = (addr_bytes as usize) + payload.len + 1;
    if (total_count > 255) {
        return null;
    }
    const byte_count: u8 = total_count as u8;
    const needed: usize = total_count * 2 + 5;
    if (out.len < needed) {
        return null;
    }

    out[0] = 'S';
    out[1] = digit;
    writeHexU8(out, 2, byte_count);

    let pos: usize = 4;
    if (addr_bytes >= 4) {
        writeHexU8(out, pos, ((address >> 24) & 0xFF) as u8);
        pos += 2;
    }
    if (addr_bytes >= 3) {
        writeHexU8(out, pos, ((address >> 16) & 0xFF) as u8);
        pos += 2;
    }
    if (addr_bytes >= 2) {
        writeHexU8(out, pos, ((address >> 8) & 0xFF) as u8);
        pos += 2;
    }
    writeHexU8(out, pos, (address & 0xFF) as u8);
    pos += 2;

    let i: usize = 0;
    while (i < payload.len) {
        writeHexU8(out, pos, payload[i]);
        pos += 2;
        i += 1;
    }

    const cs = srecChecksum(byte_count, addr_bytes, address, payload);
    writeHexU8(out, pos, cs);
    pos += 2;
    out[pos] = '\n';
    pos += 1;

    return pos;
}

// ---------------------------------------------------------------------------
// Public API: Multi-Line Iterator & Single-Pass Summarizer
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over multi-line Motorola S-Record firmware text.
/// Ignores blank lines and handles both `\r\n` and `\n`. Sets `had_error = true` on invalid lines.
pub struct SrecIter {
    src: []const u8 = "",
    pos: usize = 0,
    had_error: bool = false,

    static fn init(text: []const u8): SrecIter {
        return SrecIter {
            src: text,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): SrecRecord? {
        if (_.had_error) {
            return null;
        }
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            let i = line_start;
            while (i < _.src.len && _.src[i] != '\n') {
                i += 1;
            }
            const line = _.src[line_start..i];
            _.pos = if (i < _.src.len) i + 1 else _.src.len;

            if (isBlankLine(line)) {
                continue;
            }
            if (rec := parseSrecLine(line)) {
                return rec;
            }
            _.had_error = true;
            return null;
        }
        return null;
    }
}

/// Single-pass firmware summarizer. Computes address bounds (`min_addr`..`max_addr` inclusive),
/// data record count, total payload bytes, optional entry address (S7/S8/S9), and verifies
/// optional S5/S6 declared record count against actual data records.
/// Returns `null` if any non-blank line fails S-Record validation.
pub fn summarizeSrec(text: []const u8): SrecStats? {
    let stats = SrecStats {};
    let has_addr = false;
    let it = SrecIter.init(text);

    while (rec := it.next()) {
        if (rec.rec_type == SrecType.s0_header) {
            stats.header_text_len = rec.data_len as usize;
        } else if (rec.isData()) {
            stats.data_records += 1;
            const dlen = rec.data_len as usize;
            stats.total_bytes += dlen;
            if (dlen > 0) {
                const start_a = rec.address;
                const end_a: u32 = start_a + ((dlen - 1) as u32);
                if (!has_addr) {
                    stats.min_addr = start_a;
                    stats.max_addr = end_a;
                    has_addr = true;
                } else {
                    if (start_a < stats.min_addr) {
                        stats.min_addr = start_a;
                    }
                    if (end_a > stats.max_addr) {
                        stats.max_addr = end_a;
                    }
                }
            }
        } else if (rec.rec_type == SrecType.s5_count16 || rec.rec_type == SrecType.s6_count24) {
            stats.declared_count = rec.address;
        } else if (rec.isTermination()) {
            stats.entry_addr = rec.address;
        }
    }

    if (it.had_error) {
        return null;
    }
    if (decl := stats.declared_count) {
        stats.count_matches = (decl == stats.data_records);
    } else {
        stats.count_matches = true;
    }
    return stats;
}
