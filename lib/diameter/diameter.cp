// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 6733 Diameter AAA Base Protocol Codec)
// Pure CP zero-heap-allocation RFC 6733 Diameter Base Protocol codec:
// 20-byte big-endian header, 24-bit Command-Code & Message-Length, 4-byte aligned
// AVP iterator (8B standard / 12B Vendor-Specific), typed AVP accessors, and
// zero-allocation DiameterWriter with automatic 4B padding and length backpatching.

/// Diameter Base Protocol header size in bytes (RFC 6733 Section 3).
pub const DIAMETER_HEADER_SIZE: usize = 20;

/// Diameter protocol version (`1`).
pub const DIAMETER_VERSION: u8 = 1;

/// Standard IANA Diameter TCP/SCTP port (`3868`).
pub const DIAMETER_PORT: u16 = 3868;

// Header Command Flags (RFC 6733 Section 3)
pub const DIAM_FLAG_REQUEST: u8 = 0x80;
pub const DIAM_FLAG_PROXIABLE: u8 = 0x40;
pub const DIAM_FLAG_ERROR: u8 = 0x20;
pub const DIAM_FLAG_RETRANSMIT: u8 = 0x10;

// AVP Flags (RFC 6733 Section 4.1)
pub const AVP_FLAG_VENDOR: u8 = 0x80;
pub const AVP_FLAG_MANDATORY: u8 = 0x40;
pub const AVP_FLAG_PROTECTED: u8 = 0x20;

// Common Diameter Command Codes (RFC 6733 / RFC 4006 / 3GPP TS 29.272)
pub const CMD_CER_CEA: u32 = 257;
pub const CMD_ACR_ACA: u32 = 271;
pub const CMD_CCR_CCA: u32 = 272;
pub const CMD_DWR_DWA: u32 = 280;
pub const CMD_DPR_DPA: u32 = 282;
pub const CMD_AIR_AIA: u32 = 318;

// Common Diameter AVP Codes (RFC 6733)
pub const AVP_AUTH_APP_ID: u32 = 258;
pub const AVP_SESSION_ID: u32 = 263;
pub const AVP_ORIGIN_HOST: u32 = 264;
pub const AVP_VENDOR_ID: u32 = 266;
pub const AVP_RESULT_CODE: u32 = 268;
pub const AVP_DEST_REALM: u32 = 283;
pub const AVP_DEST_HOST: u32 = 293;
pub const AVP_ORIGIN_REALM: u32 = 296;

/// Decoded 20-byte RFC 6733 Diameter message header.
pub struct DiameterHeader {
    version: u8 = DIAMETER_VERSION,
    msg_len: u32 = 20,
    flags: u8 = 0,
    command_code: u32 = 0,
    application_id: u32 = 0,
    hop_by_hop_id: u32 = 0,
    end_to_end_id: u32 = 0,

    fn isRequest(): bool {
        return (_.flags & DIAM_FLAG_REQUEST) != 0;
    }

    fn isProxiable(): bool {
        return (_.flags & DIAM_FLAG_PROXIABLE) != 0;
    }

    fn isError(): bool {
        return (_.flags & DIAM_FLAG_ERROR) != 0;
    }

    fn isRetransmit(): bool {
        return (_.flags & DIAM_FLAG_RETRANSMIT) != 0;
    }
}

/// Zero-copy decoded RFC 6733 Diameter AVP (Attribute-Value Pair).
pub struct DiameterAvp {
    code: u32 = 0,
    flags: u8 = 0,
    avp_len: u32 = 0,
    vendor_id: u32 = 0,
    data: []const u8 = "",

    fn isVendor(): bool {
        return (_.flags & AVP_FLAG_VENDOR) != 0;
    }

    fn isMandatory(): bool {
        return (_.flags & AVP_FLAG_MANDATORY) != 0;
    }

    fn isProtected(): bool {
        return (_.flags & AVP_FLAG_PROTECTED) != 0;
    }

    /// Decodes a 4-byte Unsigned32 AVP payload in big-endian order.
    fn asU32(): u32? {
        if (_.data.len != 4) {
            return null;
        }
        return readBeU32(_.data, 0);
    }

    /// Decodes a 4-byte Integer32 AVP payload in big-endian two's complement order.
    fn asI32(): i32? {
        if (_.data.len != 4) {
            return null;
        }
        const u = readBeU32(_.data, 0);
        if ((u & 0x80000000) == 0) {
            return u as i32;
        }
        const mag = ((~u) + 1) as i64;
        return (-mag) as i32;
    }

    /// Decodes an 8-byte Unsigned64 AVP payload in big-endian order.
    fn asU64(): u64? {
        if (_.data.len != 8) {
            return null;
        }
        const hi = readBeU32(_.data, 0) as u64;
        const lo = readBeU32(_.data, 4) as u64;
        return (hi << 32) | lo;
    }
}

/// Zero-copy decoded RFC 6733 Diameter message.
pub struct DiameterMessage {
    header: DiameterHeader = DiameterHeader {},
    avps_slice: []const u8 = "",

    fn isRequest(): bool {
        return _.header.isRequest();
    }

    fn isProxiable(): bool {
        return _.header.isProxiable();
    }

    fn isError(): bool {
        return _.header.isError();
    }

    fn isRetransmit(): bool {
        return _.header.isRetransmit();
    }

    /// Returns a zero-allocation iterator over the message's 4-byte aligned AVPs.
    fn avps(): DiameterAvpIter {
        return DiameterAvpIter.init(_.avps_slice);
    }

    /// Linearly searches for the first AVP matching `code`.
    fn findAvp(code: u32): DiameterAvp? {
        let it = _.avps();
        while (avp := it.next()) {
            if (avp.code == code) {
                return avp;
            }
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// Internal Big-Endian Helpers
// ---------------------------------------------------------------------------

fn readBeU24(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 16)
        | ((data[off + 1] as u32) << 8)
        | (data[off + 2] as u32);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn writeBeU24(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 16) & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
    buf[off + 2] = (val & 0xFF) as u8;
}

fn writeBeU32(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

// ---------------------------------------------------------------------------
// Public Diameter Message Parsing API
// ---------------------------------------------------------------------------

/// Strictly parses a Diameter message from `data` per RFC 6733 Section 3.
/// Validates `data.len >= 20`, `version == 1`, `20 <= msg_len <= data.len`,
/// `(msg_len % 4) == 0`, and reserved header flag bits (`flags & 0x0F == 0`).
pub fn parseDiameterMessage(data: []const u8): DiameterMessage? {
    if (data.len < DIAMETER_HEADER_SIZE) {
        return null;
    }
    const version = data[0];
    if (version != DIAMETER_VERSION) {
        return null;
    }
    const msg_len = readBeU24(data, 1);
    const msg_len_usize = msg_len as usize;
    if (msg_len_usize < DIAMETER_HEADER_SIZE || msg_len_usize > data.len) {
        return null;
    }
    if ((msg_len_usize % 4) != 0) {
        return null;
    }
    const flags = data[4];
    if ((flags & 0x0F) != 0) {
        return null;
    }
    const command_code = readBeU24(data, 5);
    const application_id = readBeU32(data, 8);
    const hop_by_hop_id = readBeU32(data, 12);
    const end_to_end_id = readBeU32(data, 16);

    return DiameterMessage {
        header: DiameterHeader {
            version: version,
            msg_len: msg_len,
            flags: flags,
            command_code: command_code,
            application_id: application_id,
            hop_by_hop_id: hop_by_hop_id,
            end_to_end_id: end_to_end_id,
        },
        avps_slice: data[DIAMETER_HEADER_SIZE..msg_len_usize],
    };
}

/// Returns `true` if `data` starts with a structurally valid RFC 6733 Diameter message.
pub fn isDiameter(data: []const u8): bool {
    if (msg := parseDiameterMessage(data)) {
        * = msg;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Zero-Alloc 4-Byte Aligned AVP Iterator
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over 4-byte aligned RFC 6733 Diameter AVPs.
pub struct DiameterAvpIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(avps_slice: []const u8): DiameterAvpIter {
        return DiameterAvpIter {
            data: avps_slice,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    /// Yields the next `DiameterAvp` and advances to the next 4-byte aligned boundary.
    /// Returns `null` at EOF or on malformed/truncated AVP (setting `hasError() == true`).
    fn next(*_): DiameterAvp? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        const rem = _.data.len - _.pos;
        if (rem < 8) {
            _.failed = true;
            return null;
        }
        const code = readBeU32(_.data, _.pos);
        const flags = _.data[_.pos + 4];
        if ((flags & 0x1F) != 0) {
            _.failed = true;
            return null;
        }
        const avp_len = readBeU24(_.data, _.pos + 5);
        const avp_len_usize = avp_len as usize;

        let hdr_len: usize = 8;
        let vendor_id: u32 = 0;
        if ((flags & AVP_FLAG_VENDOR) != 0) {
            hdr_len = 12;
            if (rem < 12) {
                _.failed = true;
                return null;
            }
            vendor_id = readBeU32(_.data, _.pos + 8);
        }

        if (avp_len_usize < hdr_len || avp_len_usize > rem) {
            _.failed = true;
            return null;
        }

        // RFC 6733 Section 4.1: AVPs are padded to a multiple of 4 octets
        let pad: usize = 0;
        const mod4: usize = avp_len_usize % 4;
        if (mod4 != 0) {
            pad = (4 as usize) - mod4;
        }
        const padded_len = avp_len_usize + pad;
        if (padded_len > rem) {
            _.failed = true;
            return null;
        }

        const payload = _.data[(_.pos + hdr_len)..(_.pos + avp_len_usize)];
        _.pos += padded_len;

        return DiameterAvp {
            code: code,
            flags: flags,
            avp_len: avp_len,
            vendor_id: vendor_id,
            data: payload,
        };
    }
}

// ---------------------------------------------------------------------------
// Zero-Alloc Diameter Message Writer
// ---------------------------------------------------------------------------

/// Zero-allocation RFC 6733 Diameter message encoder writing directly into a caller-supplied buffer.
pub struct DiameterWriter {
    buf: []u8,
    pos: usize = 0,
    failed: bool = false,

    /// Initializes the writer and encodes the 20-byte Diameter header with placeholder length.
    pub fn init(
        buf: []u8,
        flags: u8,
        command_code: u32,
        app_id: u32,
        hbh_id: u32,
        e2e_id: u32,
    ): DiameterWriter {
        if (buf.len < DIAMETER_HEADER_SIZE || (flags & 0x0F) != 0 || command_code > 0x00FFFFFF) {
            return DiameterWriter {
                buf: buf,
                pos: 0,
                failed: true,
            };
        }
        buf[0] = DIAMETER_VERSION;
        writeBeU24(buf, 1, DIAMETER_HEADER_SIZE as u32);
        buf[4] = flags;
        writeBeU24(buf, 5, command_code);
        writeBeU32(buf, 8, app_id);
        writeBeU32(buf, 12, hbh_id);
        writeBeU32(buf, 16, e2e_id);

        return DiameterWriter {
            buf: buf,
            pos: DIAMETER_HEADER_SIZE,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    /// Appends an AVP (8B header or 12B Vendor-Specific header when `AVP_FLAG_VENDOR` is set)
    /// and zero-pads to a 4-byte boundary.
    fn writeAvp(*_, code: u32, flags: u8, vendor_id: u32, payload: []const u8): bool {
        if (_.failed || (flags & 0x1F) != 0) {
            _.failed = true;
            return false;
        }
        const has_vendor = (flags & AVP_FLAG_VENDOR) != 0;
        let hdr_len: usize = 8;
        if (has_vendor) {
            hdr_len = 12;
        }
        const avp_len = hdr_len + payload.len;
        if (avp_len > 0x00FFFFFF) {
            _.failed = true;
            return false;
        }
        let pad: usize = 0;
        const mod4: usize = avp_len % 4;
        if (mod4 != 0) {
            pad = (4 as usize) - mod4;
        }
        const padded_len = avp_len + pad;
        if (_.buf.len - _.pos < padded_len || _.pos + padded_len > 0x00FFFFFF) {
            _.failed = true;
            return false;
        }

        writeBeU32(_.buf, _.pos, code);
        _.buf[_.pos + 4] = flags;
        writeBeU24(_.buf, _.pos + 5, avp_len as u32);
        let cur = _.pos + 8;
        if (has_vendor) {
            writeBeU32(_.buf, cur, vendor_id);
            cur += 4;
        }
        let i: usize = 0;
        while (i < payload.len) {
            _.buf[cur + i] = payload[i];
            i += 1;
        }
        cur += payload.len;
        let p: usize = 0;
        while (p < pad) {
            _.buf[cur + p] = 0;
            p += 1;
        }
        _.pos += padded_len;
        return true;
    }

    /// Convenience helper to append a 4-byte Unsigned32 AVP.
    fn writeU32Avp(*_, code: u32, mandatory: bool, val: u32): bool {
        let tmp: [4]u8 = [0; 4];
        writeBeU32(tmp[0..4], 0, val);
        let flags: u8 = 0;
        if (mandatory) {
            flags = AVP_FLAG_MANDATORY;
        }
        return _.writeAvp(code, flags, 0, tmp[0..4]);
    }

    /// Backpatches the 24-bit total Message Length in the header and returns the finalized packet slice.
    fn finish(*_): ([]const u8)? {
        if (_.failed || _.pos < DIAMETER_HEADER_SIZE || (_.pos % 4) != 0 || _.pos > 0x00FFFFFF) {
            return null;
        }
        writeBeU24(_.buf, 1, _.pos as u32);
        return _.buf[0.._.pos];
    }
}
