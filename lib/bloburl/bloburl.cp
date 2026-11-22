// Source: Absorbed from /data/dev/bun/src/runtime/webcore/ObjectURLRegistry.cp | Synced: 2026-11-22T00:25:01Z

const SPLITMIX_INC: u64 = 0x9E3779B97F4A7C15;
const HEX_LOWER: []const u8 = "0123456789abcdef";

/// W3C File API Blob URL scheme prefix.
pub const BLOB_PREFIX: []const u8 = "blob:";

/// Canonical RFC 4122 hyphenated UUID string length (`8-4-4-4-12`).
pub const UUID_STR_LEN: usize = 36;

/// Minimum length of a valid `blob:<uuid>` specifier (absorbed from Bun `specifier_len`).
pub const BLOB_URL_MIN_LEN: usize = 41;

/// 128-bit UUID value for `blob:` Object URLs.
pub struct BlobUuid {
    bytes: [16]u8,

    /// Returns true if two UUIDs have identical 16-byte payloads.
    fn eql(other: BlobUuid): bool {
        let i: usize = 0;
        while (i < 16) {
            if (_.bytes[i] != other.bytes[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// Parses a 36-character canonical hyphenated UUID (`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).
    fn parse(s: []const u8): BlobUuid? {
        if (s.len != UUID_STR_LEN) {
            return null;
        }
        if (s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-') {
            return null;
        }
        let out: [16]u8 = [0; 16];
        let byte_idx: usize = 0;
        let i: usize = 0;
        while (i < UUID_STR_LEN) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                i += 1;
                continue;
            }
            const hi = hexNibble(s[i]) ?? return null;
            const lo = hexNibble(s[i + 1]) ?? return null;
            out[byte_idx] = (hi << 4) | lo;
            byte_idx += 1;
            i += 2;
        }
        return BlobUuid { bytes: out };
    }

    /// Formats the UUID as lowercase 36-byte canonical ASCII into `out_buf[0..36]`.
    fn format(out_buf: *[]u8): usize? {
        if (out_buf.*.len < UUID_STR_LEN) {
            return null;
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 16) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                out_buf.*[pos] = '-';
                pos += 1;
            }
            const b = _.bytes[i];
            out_buf.*[pos] = HEX_LOWER[(b >> 4) as usize];
            out_buf.*[pos + 1] = HEX_LOWER[(b & 0x0F) as usize];
            pos += 2;
            i += 1;
        }
        return UUID_STR_LEN;
    }
}

/// Parsed components of a `blob:<uuid>` or `blob:<origin>/<uuid>` URL.
pub struct ParsedBlobUrl {
    origin: []const u8,
    uuid: BlobUuid,
}

/// Caller-owned registry slot stored in `BlobUrlRegistry`.
pub struct BlobUrlEntry {
    occupied: bool,
    uuid: BlobUuid,
    payload_id: u64,
    content_type: []const u8,
    size: u64,
}

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn hasBlobPrefix(url: []const u8): bool {
    return url.len >= 5 &&
        url[0] == 'b' &&
        url[1] == 'l' &&
        url[2] == 'o' &&
        url[3] == 'b' &&
        url[4] == ':';
}

fn splitmixMix(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

/// Fast prefix + minimum-length check matching Bun's `isBlobURL` (`ObjectURLRegistry.cp`).
pub fn isBlobUrl(url: []const u8): bool {
    return url.len >= BLOB_URL_MIN_LEN && hasBlobPrefix(url);
}

/// Parses either Bun's `blob:<uuid>` or W3C `blob:<origin>/<uuid>` format (also rejects fragments/trailing junk).
pub fn parseBlobUrl(url: []const u8): ParsedBlobUrl? {
    if (!isBlobUrl(url)) {
        return null;
    }
    const rest = url[5..];
    if (rest.len == UUID_STR_LEN) {
        const uuid = BlobUuid.parse(rest) ?? return null;
        return ParsedBlobUrl { origin: "", uuid: uuid };
    }

    // Find last '/' separating origin and UUID.
    let last_slash: usize = rest.len;
    let i: usize = rest.len;
    while (i > 0) {
        i -= 1;
        if (rest[i] == '/') {
            last_slash = i;
            break;
        }
    }
    if (last_slash == rest.len || last_slash == 0) {
        return null;
    }
    const origin = rest[0..last_slash];
    const uuid_part = rest[last_slash + 1..];
    const uuid = BlobUuid.parse(uuid_part) ?? return null;
    return ParsedBlobUrl { origin: origin, uuid: uuid };
}

/// Formats `blob:<uuid>` (when `origin.len == 0`) or `blob:<origin>/<uuid>` into `out_buf`.
pub fn formatBlobUrl(origin: []const u8, uuid: BlobUuid, out_buf: *[]u8): usize? {
    const needed = BLOB_PREFIX.len + (if (origin.len > 0) (origin.len + 1) else 0) + UUID_STR_LEN;
    if (out_buf.*.len < needed) {
        return null;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < BLOB_PREFIX.len) {
        out_buf.*[pos] = BLOB_PREFIX[i];
        pos += 1;
        i += 1;
    }
    if (origin.len > 0) {
        i = 0;
        while (i < origin.len) {
            out_buf.*[pos] = origin[i];
            pos += 1;
            i += 1;
        }
        out_buf.*[pos] = '/';
        pos += 1;
    }
    let uuid_slice: []u8 = out_buf.*[pos..pos + UUID_STR_LEN];
    * = uuid.format(&uuid_slice) ?? return null;
    return pos + UUID_STR_LEN;
}

/// Zero-alloc fixed-capacity Object URL registry (absorbed from Bun `ObjectURLRegistry.cp`).
/// Generates deterministic RFC 4122 UUID v4 identifiers using internal SplitMix64 state.
pub struct BlobUrlRegistry {
    entries: []BlobUrlEntry,
    active: usize,
    rng_state: u64,

    /// Initializes a zero-alloc registry backed by caller-provided `entries_buf` and deterministic `seed`.
    fn init(entries_buf: *[]BlobUrlEntry, seed: u64): BlobUrlRegistry? {
        if (entries_buf.*.len == 0) {
            return null;
        }
        const zero_uuid = BlobUuid { bytes: [0; 16] };
        let i: usize = 0;
        while (i < entries_buf.*.len) {
            entries_buf.*[i] = BlobUrlEntry {
                occupied: false,
                uuid: zero_uuid,
                payload_id: 0,
                content_type: "",
                size: 0,
            };
            i += 1;
        }
        return BlobUrlRegistry {
            entries: entries_buf.*,
            active: 0,
            rng_state: seed,
        };
    }

    /// Returns the number of currently registered (non-revoked) Object URLs.
    fn activeCount(): usize {
        return _.active;
    }

    fn nextUuidV4(*_): BlobUuid {
        _.rng_state = _.rng_state +% SPLITMIX_INC;
        const hi = splitmixMix(_.rng_state);
        _.rng_state = _.rng_state +% SPLITMIX_INC;
        const lo = splitmixMix(_.rng_state);

        let b: [16]u8 = [0; 16];
        b[0] = ((hi >> 56) & 0xFF) as u8;
        b[1] = ((hi >> 48) & 0xFF) as u8;
        b[2] = ((hi >> 40) & 0xFF) as u8;
        b[3] = ((hi >> 32) & 0xFF) as u8;
        b[4] = ((hi >> 24) & 0xFF) as u8;
        b[5] = ((hi >> 16) & 0xFF) as u8;
        b[6] = ((((hi >> 8) & 0x0F) as u8) | 0x40); // Version 4
        b[7] = (hi & 0xFF) as u8;
        b[8] = ((((lo >> 56) & 0x3F) as u8) | 0x80); // RFC 4122 variant 10xx
        b[9] = ((lo >> 48) & 0xFF) as u8;
        b[10] = ((lo >> 40) & 0xFF) as u8;
        b[11] = ((lo >> 32) & 0xFF) as u8;
        b[12] = ((lo >> 24) & 0xFF) as u8;
        b[13] = ((lo >> 16) & 0xFF) as u8;
        b[14] = ((lo >> 8) & 0xFF) as u8;
        b[15] = (lo & 0xFF) as u8;
        return BlobUuid { bytes: b };
    }

    /// Registers a new object payload and formats its `blob:` URL into `url_out`.
    /// Returns URL length in bytes, or `null` if registry is full or `url_out` is too small.
    fn createObjectUrl(
        *_,
        payload_id: u64,
        content_type: []const u8,
        size: u64,
        origin: []const u8,
        url_out: *[]u8,
    ): usize? {
        if (_.active >= _.entries.len) {
            return null;
        }
        const needed = BLOB_PREFIX.len + (if (origin.len > 0) (origin.len + 1) else 0) + UUID_STR_LEN;
        if (url_out.*.len < needed) {
            return null;
        }

        let slot: usize = 0;
        while (slot < _.entries.len && _.entries[slot].occupied) {
            slot += 1;
        }
        if (slot >= _.entries.len) {
            return null;
        }

        const uuid = _.nextUuidV4();
        const url_len = formatBlobUrl(origin, uuid, url_out) ?? return null;
        _.entries[slot] = BlobUrlEntry {
            occupied: true,
            uuid: uuid,
            payload_id: payload_id,
            content_type: content_type,
            size: size,
        };
        _.active += 1;
        return url_len;
    }

    /// Resolves a `blob:` URL to its registered `BlobUrlEntry`, or `null` if invalid/not found.
    fn resolve(url: []const u8): BlobUrlEntry? {
        const parsed = parseBlobUrl(url) ?? return null;
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].occupied && _.entries[i].uuid.eql(parsed.uuid)) {
                return _.entries[i];
            }
            i += 1;
        }
        return null;
    }

    /// Returns true if `url` is a valid currently registered `blob:` URL.
    fn has(url: []const u8): bool {
        return _.resolve(url) != null;
    }

    /// Revokes a registered `blob:` URL (idempotent; returns `true` if removed, `false` otherwise).
    fn revoke(*_, url: []const u8): bool {
        const parsed = parseBlobUrl(url) ?? return false;
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].occupied && _.entries[i].uuid.eql(parsed.uuid)) {
                _.entries[i].occupied = false;
                _.active -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }
}
