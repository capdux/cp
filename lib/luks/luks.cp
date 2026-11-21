// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux LUKS1 & LUKS2 Disk Encryption Header Parser)

// LUKS Core Protocol Constants
pub const LUKS_MAGIC: [6]u8 = [0x4C, 0x55, 0x4B, 0x53, 0xBA, 0xBE]; // "LUKS\xBA\xBE"
pub const LUKS2_MAGIC_SECONDARY: [6]u8 = [0x53, 0x4B, 0x55, 0x4C, 0xBA, 0xBE]; // "SKUL\xBA\xBE"

pub const LUKS_V1: u16 = 1;
pub const LUKS_V2: u16 = 2;

pub const LUKS1_HEADER_SIZE: usize = 592;
pub const LUKS1_NUM_KEYSLOTS: usize = 8;
pub const LUKS1_KEYSLOT_SIZE: usize = 48;
pub const LUKS1_KEY_ENABLED: u32 = 0x00AC71F3;
pub const LUKS1_KEY_DISABLED: u32 = 0x0000DEAD;

pub const LUKS2_BIN_HEADER_SIZE: usize = 4096;
pub const LUKS2_MIN_HDR_SIZE: u64 = 16384;

fn readU16Be(data: []const u8, off: usize): u16 {
    const b0 = (data[off] as u16) << 8;
    const b1 = data[off + 1] as u16;
    return b0 | b1;
}

fn readU32Be(data: []const u8, off: usize): u32 {
    const b0 = (data[off] as u32) << 24;
    const b1 = (data[off + 1] as u32) << 16;
    const b2 = (data[off + 2] as u32) << 8;
    const b3 = data[off + 3] as u32;
    return b0 | b1 | b2 | b3;
}

fn readU64Be(data: []const u8, off: usize): u64 {
    const hi = (readU32Be(data, off) as u64) << 32;
    const lo = readU32Be(data, off + 4) as u64;
    return hi | lo;
}

fn matchMagic6(data: []const u8, magic: [6]u8): bool {
    if (data.len < 6) {
        return false;
    }
    let i: usize = 0;
    while (i < 6) {
        if (data[i] != magic[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimNul(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) {
            return s[0..i];
        }
        i += 1;
    }
    return s;
}

fn copySlice(dst: []u8, src: []const u8, src_off: usize) {
    let i: usize = 0;
    while (i < dst.len) {
        dst[i] = src[src_off + i];
        i += 1;
    }
}

/// Peeks the LUKS format version (`LUKS_V1` or `LUKS_V2`) from the 8-byte magic + version prefix.
/// Recognizes primary `"LUKS\xBA\xBE"` (v1/v2) and secondary `"SKUL\xBA\xBE"` (v2 only).
pub fn peekLuksVersion(data: []const u8): u16? {
    if (data.len < 8) {
        return null;
    }
    if (matchMagic6(data, LUKS_MAGIC)) {
        const ver = readU16Be(data, 6);
        if (ver == LUKS_V1 || ver == LUKS_V2) {
            return ver;
        }
        return null;
    }
    if (matchMagic6(data, LUKS2_MAGIC_SECONDARY)) {
        const ver = readU16Be(data, 6);
        if (ver == LUKS_V2) {
            return ver;
        }
        return null;
    }
    return null;
}

/// Returns `true` if `data` starts with a valid LUKS1 or LUKS2 (primary/secondary) magic and version.
pub fn isLuks(data: []const u8): bool {
    return peekLuksVersion(data) != null;
}

/// Single 48-byte LUKS1 keyslot descriptor.
pub struct Luks1Keyslot {
    active: u32,
    iterations: u32,
    salt: [32]u8,
    key_material_offset: u32,
    stripes: u32,

    /// Returns `true` if the keyslot is enabled (`LUKS1_KEY_ENABLED == 0x00AC71F3`).
    fn isEnabled(): bool {
        return _.active == LUKS1_KEY_ENABLED;
    }

    /// Returns `true` if the keyslot is explicitly disabled (`LUKS1_KEY_DISABLED == 0x0000DEAD`).
    fn isDisabled(): bool {
        return _.active == LUKS1_KEY_DISABLED;
    }
}

/// Parsed 592-byte LUKS1 on-disk header (big-endian) with 8 keyslots.
pub struct Luks1Header {
    version: u16,
    cipher_name: [32]u8,
    cipher_mode: [32]u8,
    hash_spec: [32]u8,
    payload_offset: u32,
    key_bytes: u32,
    mk_digest: [20]u8,
    mk_digest_salt: [32]u8,
    mk_digest_iter: u32,
    uuid: [40]u8,
    keyslots: [8]Luks1Keyslot,

    fn cipherNameSlice(): []const u8 {
        return trimNul(_.cipher_name[0..]);
    }

    fn cipherModeSlice(): []const u8 {
        return trimNul(_.cipher_mode[0..]);
    }

    fn hashSpecSlice(): []const u8 {
        return trimNul(_.hash_spec[0..]);
    }

    fn uuidSlice(): []const u8 {
        return trimNul(_.uuid[0..]);
    }

    /// Counts how many of the 8 keyslots have `active == LUKS1_KEY_ENABLED`.
    fn activeKeyslotCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < LUKS1_NUM_KEYSLOTS) {
            if (_.keyslots[i].isEnabled()) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }
}

/// Parses a 592-byte LUKS1 header with strict validation of magic, `version == 1`,
/// `data.len >= 592`, and `key_bytes > 0`.
pub fn parseLuks1Header(data: []const u8): Luks1Header? {
    if (data.len < LUKS1_HEADER_SIZE || !matchMagic6(data, LUKS_MAGIC)) {
        return null;
    }
    const version = readU16Be(data, 6);
    if (version != LUKS_V1) {
        return null;
    }
    const payload_offset = readU32Be(data, 104);
    const key_bytes = readU32Be(data, 108);
    if (key_bytes == 0) {
        return null;
    }

    let cipher_name: [32]u8 = [0; 32];
    let cipher_mode: [32]u8 = [0; 32];
    let hash_spec: [32]u8 = [0; 32];
    let mk_digest: [20]u8 = [0; 20];
    let mk_digest_salt: [32]u8 = [0; 32];
    let uuid: [40]u8 = [0; 40];

    copySlice(cipher_name[0..], data, 8);
    copySlice(cipher_mode[0..], data, 40);
    copySlice(hash_spec[0..], data, 72);
    copySlice(mk_digest[0..], data, 112);
    copySlice(mk_digest_salt[0..], data, 132);
    const mk_digest_iter = readU32Be(data, 164);
    copySlice(uuid[0..], data, 168);

    const empty_slot = Luks1Keyslot{
        .active = 0,
        .iterations = 0,
        .salt = [0; 32],
        .key_material_offset = 0,
        .stripes = 0,
    };
    let keyslots: [8]Luks1Keyslot = [empty_slot; 8];

    let i: usize = 0;
    while (i < LUKS1_NUM_KEYSLOTS) {
        const off = 208 + i * LUKS1_KEYSLOT_SIZE;
        let salt: [32]u8 = [0; 32];
        copySlice(salt[0..], data, off + 8);
        keyslots[i] = Luks1Keyslot{
            .active = readU32Be(data, off),
            .iterations = readU32Be(data, off + 4),
            .salt = salt,
            .key_material_offset = readU32Be(data, off + 40),
            .stripes = readU32Be(data, off + 44),
        };
        i += 1;
    }

    return Luks1Header{
        .version = version,
        .cipher_name = cipher_name,
        .cipher_mode = cipher_mode,
        .hash_spec = hash_spec,
        .payload_offset = payload_offset,
        .key_bytes = key_bytes,
        .mk_digest = mk_digest,
        .mk_digest_salt = mk_digest_salt,
        .mk_digest_iter = mk_digest_iter,
        .uuid = uuid,
        .keyslots = keyslots,
    };
}

/// Parsed 4096-byte LUKS2 primary (`"LUKS\xBA\xBE"`) or secondary (`"SKUL\xBA\xBE"`) binary header.
pub struct Luks2Header {
    is_secondary: bool,
    version: u16,
    hdr_size: u64,
    seqid: u64,
    label: [48]u8,
    csum_alg: [32]u8,
    salt: [64]u8,
    uuid: [40]u8,
    subsystem: [48]u8,
    hdr_offset: u64,
    csum: [64]u8,

    fn labelSlice(): []const u8 {
        return trimNul(_.label[0..]);
    }

    fn csumAlgSlice(): []const u8 {
        return trimNul(_.csum_alg[0..]);
    }

    fn uuidSlice(): []const u8 {
        return trimNul(_.uuid[0..]);
    }

    fn subsystemSlice(): []const u8 {
        return trimNul(_.subsystem[0..]);
    }

    /// Size of the JSON metadata area in bytes (`hdr_size - 4096`).
    fn jsonAreaBytes(): u64 {
        return _.hdr_size - (LUKS2_BIN_HEADER_SIZE as u64);
    }

    /// Zero-copy extraction of the JSON metadata from `[4096..hdr_size]` of `full_hdr`,
    /// stripping trailing NUL padding bytes.
    fn extractJson(full_hdr: []const u8): ([]const u8)? {
        if (_.hdr_size < LUKS2_MIN_HDR_SIZE || _.hdr_size > (full_hdr.len as u64)) {
            return null;
        }
        const end = _.hdr_size as usize;
        return trimNul(full_hdr[LUKS2_BIN_HEADER_SIZE..end]);
    }
}

/// Parses a 4096-byte LUKS2 primary or secondary binary header with strict validation:
/// - `data.len >= 4096`
/// - Magic `"LUKS\xBA\xBE"` (primary) or `"SKUL\xBA\xBE"` (secondary)
/// - `version == 2`
/// - `hdr_size >= 16384` and a power of 2
pub fn parseLuks2Header(data: []const u8): Luks2Header? {
    if (data.len < LUKS2_BIN_HEADER_SIZE) {
        return null;
    }
    let is_secondary = false;
    if (matchMagic6(data, LUKS_MAGIC)) {
        is_secondary = false;
    } else if (matchMagic6(data, LUKS2_MAGIC_SECONDARY)) {
        is_secondary = true;
    } else {
        return null;
    }

    const version = readU16Be(data, 6);
    if (version != LUKS_V2) {
        return null;
    }

    const hdr_size = readU64Be(data, 8);
    if (hdr_size < LUKS2_MIN_HDR_SIZE || (hdr_size & (hdr_size - 1)) != 0) {
        return null;
    }

    const seqid = readU64Be(data, 16);

    let label: [48]u8 = [0; 48];
    let csum_alg: [32]u8 = [0; 32];
    let salt: [64]u8 = [0; 64];
    let uuid: [40]u8 = [0; 40];
    let subsystem: [48]u8 = [0; 48];
    let csum: [64]u8 = [0; 64];

    copySlice(label[0..], data, 24);
    copySlice(csum_alg[0..], data, 72);
    copySlice(salt[0..], data, 104);
    copySlice(uuid[0..], data, 168);
    copySlice(subsystem[0..], data, 208);
    const hdr_offset = readU64Be(data, 256);
    copySlice(csum[0..], data, 448);

    return Luks2Header{
        .is_secondary = is_secondary,
        .version = version,
        .hdr_size = hdr_size,
        .seqid = seqid,
        .label = label,
        .csum_alg = csum_alg,
        .salt = salt,
        .uuid = uuid,
        .subsystem = subsystem,
        .hdr_offset = hdr_offset,
        .csum = csum,
    };
}

/// Selects the active LUKS2 header between primary (`h1`) and secondary (`h2`) by choosing the higher `seqid`.
pub fn selectActiveLuks2(h1: Luks2Header?, h2: Luks2Header?): Luks2Header? {
    if (a := h1) {
        if (b := h2) {
            if (b.seqid > a.seqid) {
                return b;
            }
            return a;
        }
        return a;
    }
    return h2;
}
