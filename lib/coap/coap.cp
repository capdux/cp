// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 7252 CoAP Wire Codec)
// Pure CP zero-heap-allocation RFC 7252 Constrained Application Protocol (CoAP) codec:
// 4-byte header + 0..8B token, Delta/Length extended option encoding (0..12, 13, 14),
// strict payload marker (0xFF) validation, CoapOptIter, and zero-allocation CoapWriter.

/// Standard CoAP UDP port (`5683`, RFC 7252).
pub const COAP_UDP_PORT: u16 = 5683;

/// Standard CoAPS (DTLS) UDP port (`5684`, RFC 7252).
pub const COAPS_UDP_PORT: u16 = 5684;

/// CoAP protocol version (`1`).
pub const COAP_VERSION: u8 = 1;

/// Fixed CoAP header size in bytes (`4`).
pub const COAP_HEADER_SIZE: usize = 4;

/// CoAP Payload Marker byte (`0xFF`).
pub const COAP_PAYLOAD_MARKER: u8 = 0xFF;

// CoAP Message Types (2-bit T field)
pub const COAP_CON: u8 = 0;
pub const COAP_NON: u8 = 1;
pub const COAP_ACK: u8 = 2;
pub const COAP_RST: u8 = 3;

// Common CoAP Method & Response Codes (class.detail)
pub const CODE_EMPTY: u8 = 0x00;
pub const CODE_GET: u8 = 0x01;
pub const CODE_POST: u8 = 0x02;
pub const CODE_PUT: u8 = 0x03;
pub const CODE_DELETE: u8 = 0x04;
pub const CODE_CREATED: u8 = 0x41;        // 2.01
pub const CODE_DELETED: u8 = 0x42;        // 2.02
pub const CODE_VALID: u8 = 0x43;          // 2.03
pub const CODE_CHANGED: u8 = 0x44;        // 2.04
pub const CODE_CONTENT: u8 = 0x45;        // 2.05
pub const CODE_BAD_REQUEST: u8 = 0x80;    // 4.00
pub const CODE_NOT_FOUND: u8 = 0x84;      // 4.04
pub const CODE_INTERNAL_ERROR: u8 = 0xA0; // 5.00

// Common CoAP Option Numbers (RFC 7252 / RFC 7641 / RFC 7959)
pub const OPT_IF_MATCH: u16 = 1;
pub const OPT_URI_HOST: u16 = 3;
pub const OPT_ETAG: u16 = 4;
pub const OPT_OBSERVE: u16 = 6;
pub const OPT_URI_PORT: u16 = 7;
pub const OPT_URI_PATH: u16 = 11;
pub const OPT_CONTENT_FORMAT: u16 = 12;
pub const OPT_MAX_AGE: u16 = 14;
pub const OPT_URI_QUERY: u16 = 15;
pub const OPT_ACCEPT: u16 = 17;
pub const OPT_BLOCK2: u16 = 23;
pub const OPT_BLOCK1: u16 = 27;
pub const OPT_SIZE1: u16 = 60;

/// Constructs a CoAP 8-bit message code from 3-bit `class` (`0..7`) and 5-bit `detail` (`0..31`).
pub fn coapCode(code_class: u8, detail: u8): u8 {
    return ((code_class & 0x07) << 5) | (detail & 0x1F);
}

/// Zero-copy decoded CoAP Option element.
pub struct CoapOption {
    number: u16 = 0,
    value: []const u8 = "",
}

/// Zero-allocation iterator over RFC 7252 Delta-encoded CoAP Options.
pub struct CoapOptIter {
    data: []const u8 = "",
    pos: usize = 0,
    last_num: u16 = 0,
    failed: bool = false,

    pub fn init(options_bytes: []const u8): CoapOptIter {
        return CoapOptIter {
            data: options_bytes,
            pos: 0,
            last_num: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): CoapOption? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        const b = _.data[_.pos];
        if (b == COAP_PAYLOAD_MARKER) {
            return null;
        }
        const delta_nib = (b >> 4) & 0x0F;
        const len_nib = b & 0x0F;
        if (delta_nib == 15 || len_nib == 15) {
            _.failed = true;
            return null;
        }
        let cur = _.pos + 1;

        let delta: usize = 0;
        if (delta_nib < 13) {
            delta = delta_nib as usize;
        } else if (delta_nib == 13) {
            if (_.data.len - cur < 1) { _.failed = true; return null; }
            delta = (_.data[cur] as usize) + 13;
            cur += 1;
        } else {
            if (_.data.len - cur < 2) { _.failed = true; return null; }
            delta = (readBeU16(_.data, cur) as usize) + 269;
            cur += 2;
        }

        let opt_len: usize = 0;
        if (len_nib < 13) {
            opt_len = len_nib as usize;
        } else if (len_nib == 13) {
            if (_.data.len - cur < 1) { _.failed = true; return null; }
            opt_len = (_.data[cur] as usize) + 13;
            cur += 1;
        } else {
            if (_.data.len - cur < 2) { _.failed = true; return null; }
            opt_len = (readBeU16(_.data, cur) as usize) + 269;
            cur += 2;
        }

        const next_num = (_.last_num as usize) + delta;
        if (next_num > 65535 || _.data.len - cur < opt_len) {
            _.failed = true;
            return null;
        }

        const val = _.data[cur..(cur + opt_len)];
        _.last_num = next_num as u16;
        _.pos = cur + opt_len;

        return CoapOption {
            number: _.last_num,
            value: val,
        };
    }
}

/// Zero-copy decoded RFC 7252 CoAP message.
pub struct CoapMessage {
    version: u8 = COAP_VERSION,
    msg_type: u8 = COAP_CON,
    code: u8 = CODE_EMPTY,
    message_id: u16 = 0,
    token: []const u8 = "",
    options_bytes: []const u8 = "",
    payload: []const u8 = "",

    fn codeClass(): u8 {
        return (_.code >> 5) & 0x07;
    }

    fn codeDetail(): u8 {
        return _.code & 0x1F;
    }

    fn isEmpty(): bool {
        return _.code == CODE_EMPTY;
    }

    fn isRequest(): bool {
        return _.codeClass() == 0 && _.codeDetail() != 0;
    }

    fn isResponse(): bool {
        const cls = _.codeClass();
        return cls == 2 || cls == 4 || cls == 5;
    }

    fn options(): CoapOptIter {
        return CoapOptIter.init(_.options_bytes);
    }

    fn findOption(num: u16): CoapOption? {
        let it = _.options();
        while (opt := it.next()) {
            if (opt.number == num) {
                return opt;
            }
        }
        return null;
    }

    /// Decodes Option 12 (`Content-Format`, 0..2 byte big-endian uint), or `null` if absent/invalid.
    fn contentFormat(): u16? {
        if (opt := _.findOption(OPT_CONTENT_FORMAT)) {
            if (opt.value.len == 0) {
                return 0;
            }
            if (opt.value.len == 1) {
                return opt.value[0] as u16;
            }
            if (opt.value.len == 2) {
                return readBeU16(opt.value, 0);
            }
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn writeBeU16(buf: []u8, off: usize, val: u16) {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
}

// ---------------------------------------------------------------------------
// Public CoAP Parser
// ---------------------------------------------------------------------------

/// Strictly parses an RFC 7252 CoAP message.
/// Validates `version == 1`, `tkl <= 8`, all Delta/Length option headers, and enforces
/// at least 1 payload byte after `COAP_PAYLOAD_MARKER (0xFF)`.
pub fn parseCoap(data: []const u8): CoapMessage? {
    if (data.len < COAP_HEADER_SIZE) {
        return null;
    }
    const b0 = data[0];
    const version: u8 = (b0 >> 6) & 0x03;
    if (version != COAP_VERSION) {
        return null;
    }
    const msg_type: u8 = (b0 >> 4) & 0x03;
    const tkl = (b0 & 0x0F) as usize;
    if (tkl > 8 || data.len - COAP_HEADER_SIZE < tkl) {
        return null;
    }
    const code = data[1];
    const message_id = readBeU16(data, 2);
    const token = data[COAP_HEADER_SIZE..(COAP_HEADER_SIZE + tkl)];
    const opts_start = COAP_HEADER_SIZE + tkl;

    // Pre-scan all options to validate framing and locate payload boundary
    let pos = opts_start;
    let last_num: usize = 0;
    let opts_end = data.len;
    let payload: []const u8 = "";

    while (pos < data.len) {
        const b = data[pos];
        if (b == COAP_PAYLOAD_MARKER) {
            // RFC 7252 Section 3: presence of marker followed by zero-length payload is a format error
            if (pos + 1 >= data.len) {
                return null;
            }
            opts_end = pos;
            payload = data[(pos + 1)..data.len];
            break;
        }
        const delta_nib = (b >> 4) & 0x0F;
        const len_nib = b & 0x0F;
        if (delta_nib == 15 || len_nib == 15) {
            return null;
        }
        pos += 1;

        let delta: usize = 0;
        if (delta_nib < 13) {
            delta = delta_nib as usize;
        } else if (delta_nib == 13) {
            if (data.len - pos < 1) { return null; }
            delta = (data[pos] as usize) + 13;
            pos += 1;
        } else {
            if (data.len - pos < 2) { return null; }
            delta = (readBeU16(data, pos) as usize) + 269;
            pos += 2;
        }

        let opt_len: usize = 0;
        if (len_nib < 13) {
            opt_len = len_nib as usize;
        } else if (len_nib == 13) {
            if (data.len - pos < 1) { return null; }
            opt_len = (data[pos] as usize) + 13;
            pos += 1;
        } else {
            if (data.len - pos < 2) { return null; }
            opt_len = (readBeU16(data, pos) as usize) + 269;
            pos += 2;
        }

        last_num += delta;
        if (last_num > 65535 || data.len - pos < opt_len) {
            return null;
        }
        pos += opt_len;
    }

    return CoapMessage {
        version: version,
        msg_type: msg_type,
        code: code,
        message_id: message_id,
        token: token,
        options_bytes: data[opts_start..opts_end],
        payload: payload,
    };
}

/// Returns `true` if `data` is a structurally valid RFC 7252 CoAP message.
pub fn isCoap(data: []const u8): bool {
    if (m := parseCoap(data)) {
        * = m;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Zero-Alloc CoAP Writer
// ---------------------------------------------------------------------------

/// Zero-allocation RFC 7252 CoAP message encoder writing directly into a caller-supplied buffer.
pub struct CoapWriter {
    buf: []u8,
    pos: usize = 0,
    last_opt_num: u16 = 0,
    payload_written: bool = false,
    failed: bool = false,

    /// Initializes the 4-byte CoAP header and optional 0..8-byte Token.
    pub fn init(
        buf: []u8,
        msg_type: u8,
        code: u8,
        message_id: u16,
        token: []const u8,
    ): CoapWriter {
        if (msg_type > 3 || token.len > 8 || buf.len < COAP_HEADER_SIZE + token.len) {
            return CoapWriter {
                buf: buf,
                pos: 0,
                last_opt_num: 0,
                payload_written: false,
                failed: true,
            };
        }
        buf[0] = (COAP_VERSION << 6) | ((msg_type & 0x03) << 4) | (token.len as u8);
        buf[1] = code;
        writeBeU16(buf, 2, message_id);
        let i: usize = 0;
        while (i < token.len) {
            buf[COAP_HEADER_SIZE + i] = token[i];
            i += 1;
        }
        return CoapWriter {
            buf: buf,
            pos: COAP_HEADER_SIZE + token.len,
            last_opt_num: 0,
            payload_written: false,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    /// Appends a Delta-encoded Option (must be called in non-decreasing `number` order before payload).
    fn writeOption(*_, number: u16, value: []const u8): bool {
        if (_.failed || _.payload_written || number < _.last_opt_num || value.len > 65804) {
            _.failed = true;
            return false;
        }
        const delta = (number - _.last_opt_num) as usize;
        const vlen = value.len;

        let d_nib: u8 = 0;
        let d_ext: usize = 0;
        if (delta < 13) {
            d_nib = delta as u8;
        } else if (delta < 269) {
            d_nib = 13;
            d_ext = 1;
        } else {
            d_nib = 14;
            d_ext = 2;
        }

        let l_nib: u8 = 0;
        let l_ext: usize = 0;
        if (vlen < 13) {
            l_nib = vlen as u8;
        } else if (vlen < 269) {
            l_nib = 13;
            l_ext = 1;
        } else {
            l_nib = 14;
            l_ext = 2;
        }

        const need = 1 + d_ext + l_ext + vlen;
        if (_.buf.len - _.pos < need) {
            _.failed = true;
            return false;
        }

        _.buf[_.pos] = (d_nib << 4) | l_nib;
        _.pos += 1;

        if (d_ext == 1) {
            _.buf[_.pos] = (delta - 13) as u8;
            _.pos += 1;
        } else if (d_ext == 2) {
            writeBeU16(_.buf, _.pos, (delta - 269) as u16);
            _.pos += 2;
        }

        if (l_ext == 1) {
            _.buf[_.pos] = (vlen - 13) as u8;
            _.pos += 1;
        } else if (l_ext == 2) {
            writeBeU16(_.buf, _.pos, (vlen - 269) as u16);
            _.pos += 2;
        }

        let i: usize = 0;
        while (i < vlen) {
            _.buf[_.pos + i] = value[i];
            i += 1;
        }
        _.pos += vlen;
        _.last_opt_num = number;
        return true;
    }

    /// Appends a minimal big-endian unsigned integer Option per RFC 7252 Section 3.2 (`0` -> 0 bytes).
    fn writeUintOption(*_, number: u16, val: u32): bool {
        if (val == 0) {
            return _.writeOption(number, "");
        }
        let tmp: [4]u8 = [0; 4];
        if (val <= 0xFF) {
            tmp[0] = val as u8;
            return _.writeOption(number, tmp[0..1]);
        }
        if (val <= 0xFFFF) {
            tmp[0] = ((val >> 8) & 0xFF) as u8;
            tmp[1] = (val & 0xFF) as u8;
            return _.writeOption(number, tmp[0..2]);
        }
        if (val <= 0xFFFFFF) {
            tmp[0] = ((val >> 16) & 0xFF) as u8;
            tmp[1] = ((val >> 8) & 0xFF) as u8;
            tmp[2] = (val & 0xFF) as u8;
            return _.writeOption(number, tmp[0..3]);
        }
        tmp[0] = ((val >> 24) & 0xFF) as u8;
        tmp[1] = ((val >> 16) & 0xFF) as u8;
        tmp[2] = ((val >> 8) & 0xFF) as u8;
        tmp[3] = (val & 0xFF) as u8;
        return _.writeOption(number, tmp[0..4]);
    }

    /// Appends `0xFF` Payload Marker followed by `payload` (no-op if `payload` is empty).
    fn writePayload(*_, payload: []const u8): bool {
        if (_.failed || _.payload_written) {
            _.failed = true;
            return false;
        }
        if (payload.len == 0) {
            return true;
        }
        if (_.buf.len - _.pos < 1 + payload.len) {
            _.failed = true;
            return false;
        }
        _.buf[_.pos] = COAP_PAYLOAD_MARKER;
        _.pos += 1;
        let i: usize = 0;
        while (i < payload.len) {
            _.buf[_.pos + i] = payload[i];
            i += 1;
        }
        _.pos += payload.len;
        _.payload_written = true;
        return true;
    }

    /// Returns the finalized CoAP packet slice, or `null` if any write failed.
    fn finish(*_): ([]const u8)? {
        if (_.failed || _.pos < COAP_HEADER_SIZE) {
            return null;
        }
        return _.buf[0.._.pos];
    }
}
