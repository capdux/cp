// Origin: Pure-CP Stdlib (Zero-Alloc RFC 4120 Kerberos v5 ASN.1 DER Wire Parser) | Created: 2026-11-22

// Standard Kerberos Port & Version
pub const KRB_PORT: u16 = 88;
pub const KRB_VERSION: u32 = 5;

// RFC 4120 Application Tags / Message Types
pub const MSG_AS_REQ: u8 = 10;
pub const MSG_AS_REP: u8 = 11;
pub const MSG_TGS_REQ: u8 = 12;
pub const MSG_TGS_REP: u8 = 13;
pub const MSG_AP_REQ: u8 = 14;
pub const MSG_AP_REP: u8 = 15;
pub const MSG_KRB_SAFE: u8 = 20;
pub const MSG_KRB_PRIV: u8 = 21;
pub const MSG_KRB_CRED: u8 = 22;
pub const MSG_KRB_ERROR: u8 = 30;

// Common Kerberos Encryption Types (RFC 3961 / RFC 3962 / RFC 4757)
pub const ETYPE_AES128_CTS_HMAC_SHA1: i32 = 17;
pub const ETYPE_AES256_CTS_HMAC_SHA1: i32 = 18;
pub const ETYPE_RC4_HMAC: i32 = 23;

// Kerberos Principal Name Types (RFC 4120 §6.2)
pub const NT_UNKNOWN: i32 = 0;
pub const NT_PRINCIPAL: i32 = 1;
pub const NT_SRV_INST: i32 = 2;
pub const NT_SRV_HST: i32 = 3;
pub const NT_ENTERPRISE: i32 = 10;

// Common Kerberos Error Codes (RFC 4120 §7.5.9)
pub const KDC_ERR_NONE: i32 = 0;
pub const KDC_ERR_C_PRINCIPAL_UNKNOWN: i32 = 6;
pub const KDC_ERR_S_PRINCIPAL_UNKNOWN: i32 = 7;
pub const KDC_ERR_PREAUTH_FAILED: i32 = 24;
pub const KDC_ERR_PREAUTH_REQUIRED: i32 = 25;
pub const KRB_AP_ERR_TKT_EXPIRED: i32 = 32;
pub const KRB_AP_ERR_SKEW: i32 = 37;

// ASN.1 DER Universal / Constructed Tag Constants
const TAG_INTEGER: u8 = 0x02;
const TAG_OCTET_STRING: u8 = 0x04;
const TAG_GENERALIZED_TIME: u8 = 0x18;
const TAG_GENERAL_STRING: u8 = 0x1B;
const TAG_SEQUENCE: u8 = 0x30;
const TAG_APP_TICKET: u8 = 0x61; // [APPLICATION 1] CONSTRUCTED

struct DerTlv {
    tag: u8,
    value: []const u8,
    next_off: usize,
}

fn readDerTlv(data: []const u8, off: usize): DerTlv? {
    if (off + 2 > data.len) {
        return null;
    }
    const tag = data[off];
    const len_b = data[off + 1];
    let val_len: usize = 0;
    let val_start: usize = off + 2;

    if ((len_b & 0x80) == 0) {
        val_len = len_b as usize;
    } else {
        const num_octets = (len_b & 0x7F) as usize;
        // Reject indefinite length (0x80) and > 4-byte lengths
        if (num_octets == 0 || num_octets > 4 || off + 2 + num_octets > data.len) {
            return null;
        }
        // Reject non-minimal DER length encoding
        if (data[off + 2] == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < num_octets) {
            val_len = (val_len << 8) | (data[off + 2 + i] as usize);
            i += 1;
        }
        if (val_len < 128) {
            return null;
        }
        val_start = off + 2 + num_octets;
    }

    if (val_len > data.len - val_start) {
        return null;
    }
    return DerTlv{
        .tag = tag,
        .value = data[val_start..(val_start + val_len)],
        .next_off = val_start + val_len,
    };
}

fn parseDerI32Bytes(v: []const u8): i32? {
    if (v.len == 0 || v.len > 4) {
        return null;
    }
    // Check minimal two's complement DER encoding
    if (v.len > 1) {
        const b0 = v[0];
        const b1 = v[1];
        if ((b0 == 0x00 && (b1 & 0x80) == 0) || (b0 == 0xFF && (b1 & 0x80) != 0)) {
            return null;
        }
    }
    let acc: i64 = if ((v[0] & 0x80) != 0) -1 else 0;
    let i: usize = 0;
    while (i < v.len) {
        acc = (acc << 8) | (v[i] as i64);
        i += 1;
    }
    if (acc < -2147483648 || acc > 2147483647) {
        return null;
    }
    return acc as i32;
}

fn parseDerU32Bytes(v: []const u8): u32? {
    if (v.len == 0 || v.len > 5) {
        return null;
    }
    if ((v[0] & 0x80) != 0) {
        return null;
    }
    if (v.len > 1 && v[0] == 0x00 && (v[1] & 0x80) == 0) {
        return null;
    }
    if (v.len == 5 && v[0] != 0x00) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < v.len) {
        acc = (acc << 8) | (v[i] as u64);
        i += 1;
    }
    if (acc > 4294967295) {
        return null;
    }
    return acc as u32;
}

fn parseTaggedIntU32(inner: []const u8): u32? {
    const tlv = readDerTlv(inner, 0) ?? return null;
    if (tlv.tag != TAG_INTEGER || tlv.next_off != inner.len) {
        return null;
    }
    return parseDerU32Bytes(tlv.value);
}

fn parseTaggedIntI32(inner: []const u8): i32? {
    const tlv = readDerTlv(inner, 0) ?? return null;
    if (tlv.tag != TAG_INTEGER || tlv.next_off != inner.len) {
        return null;
    }
    return parseDerI32Bytes(tlv.value);
}

fn parseTaggedSingle(inner: []const u8, expected_tag: u8): ([]const u8)? {
    const tlv = readDerTlv(inner, 0) ?? return null;
    if (tlv.tag != expected_tag || tlv.next_off != inner.len) {
        return null;
    }
    return tlv.value;
}

fn isKnownKrbAppTag(app_tag: u8): bool {
    if (app_tag == 1) {
        return true;
    }
    if (app_tag >= MSG_AS_REQ && app_tag <= MSG_AP_REP) {
        return true;
    }
    if (app_tag >= MSG_KRB_SAFE && app_tag <= MSG_KRB_CRED) {
        return true;
    }
    return app_tag == MSG_KRB_ERROR;
}

/// Unwraps an RFC 4120 §7.2.2 4-byte big-endian TCP Record Mark (MSB must be 0).
pub fn unwrapKrbTcpRecord(data: []const u8): ([]const u8)? {
    if (data.len < 4 || (data[0] & 0x80) != 0) {
        return null;
    }
    const rec_len = ((data[0] as usize) << 24)
        | ((data[1] as usize) << 16)
        | ((data[2] as usize) << 8)
        | (data[3] as usize);
    if (rec_len == 0 || rec_len > data.len - 4) {
        return null;
    }
    return data[4..(4 + rec_len)];
}

/// Peeks the Kerberos v5 ASN.1 `APPLICATION` tag (`1`, `10..15`, `20..22`, `30`).
pub fn peekKrbAppTag(data: []const u8): u8? {
    const tlv = readDerTlv(data, 0) ?? return null;
    if (tlv.tag < 0x60 || tlv.tag > 0x7E) {
        return null;
    }
    const app_tag = tlv.tag & 0x1F;
    if (!isKnownKrbAppTag(app_tag)) {
        return null;
    }
    return app_tag;
}

/// Returns `true` if `data` starts with a well-formed Kerberos v5 `APPLICATION` DER envelope.
pub fn isKerberos(data: []const u8): bool {
    return peekKrbAppTag(data) != null;
}

/// Zero-copy iterator over `SEQUENCE OF KerberosString` (`GeneralString` 0x1B) components.
pub struct KrbStringIter {
    data: []const u8,
    pos: usize,

    pub fn init(data: []const u8): KrbStringIter {
        return KrbStringIter{
            .data = data,
            .pos = 0,
        };
    }

    fn next(*_): ([]const u8)? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const tlv = readDerTlv(_.data, _.pos) ?? return null;
        if (tlv.tag != TAG_GENERAL_STRING) {
            return null;
        }
        _.pos = tlv.next_off;
        return tlv.value;
    }
}

/// RFC 4120 §5.2.2 `PrincipalName` (`name-type`, `SEQUENCE OF GeneralString`).
pub struct KrbPrincipalName {
    name_type: i32,
    names_der: []const u8,

    fn names(): KrbStringIter {
        return KrbStringIter.init(_.names_der);
    }
}

fn validateGeneralStringSeq(der: []const u8): bool {
    let pos: usize = 0;
    while (pos < der.len) {
        const tlv = readDerTlv(der, pos) ?? return false;
        if (tlv.tag != TAG_GENERAL_STRING) {
            return false;
        }
        pos = tlv.next_off;
    }
    return pos == der.len;
}

fn parsePrincipalNameDer(wrapper: []const u8): KrbPrincipalName? {
    const seq = parseTaggedSingle(wrapper, TAG_SEQUENCE) ?? return null;
    const f0 = readDerTlv(seq, 0) ?? return null;
    if (f0.tag != 0xA0) {
        return null;
    }
    const name_type = parseTaggedIntI32(f0.value) ?? return null;

    const f1 = readDerTlv(seq, f0.next_off) ?? return null;
    if (f1.tag != 0xA1 || f1.next_off != seq.len) {
        return null;
    }
    const names_der = parseTaggedSingle(f1.value, TAG_SEQUENCE) ?? return null;
    if (!validateGeneralStringSeq(names_der)) {
        return null;
    }
    return KrbPrincipalName{
        .name_type = name_type,
        .names_der = names_der,
    };
}

/// RFC 4120 §5.2.9 `EncryptedData` (`etype`, optional `kvno`, `cipher`).
pub struct KrbEncryptedData {
    etype: i32,
    kvno: u32?,
    cipher: []const u8,
}

fn parseEncryptedDataDer(wrapper: []const u8): KrbEncryptedData? {
    const seq = parseTaggedSingle(wrapper, TAG_SEQUENCE) ?? return null;
    const f0 = readDerTlv(seq, 0) ?? return null;
    if (f0.tag != 0xA0) {
        return null;
    }
    const etype = parseTaggedIntI32(f0.value) ?? return null;

    let pos = f0.next_off;
    let kvno: u32? = null;
    let next_tlv = readDerTlv(seq, pos) ?? return null;
    if (next_tlv.tag == 0xA1) {
        kvno = parseTaggedIntU32(next_tlv.value) ?? return null;
        pos = next_tlv.next_off;
        next_tlv = readDerTlv(seq, pos) ?? return null;
    }

    if (next_tlv.tag != 0xA2 || next_tlv.next_off != seq.len) {
        return null;
    }
    const cipher = parseTaggedSingle(next_tlv.value, TAG_OCTET_STRING) ?? return null;

    return KrbEncryptedData{
        .etype = etype,
        .kvno = kvno,
        .cipher = cipher,
    };
}

/// RFC 4120 §5.3 `[APPLICATION 1]` `Ticket` (`tkt-vno == 5`, `realm`, `sname`, `enc-part`).
pub struct KrbTicket {
    tkt_vno: u32,
    realm: []const u8,
    sname: KrbPrincipalName,
    enc_part: KrbEncryptedData,
}

/// Parses a Kerberos v5 `APPLICATION [1]` `Ticket` DER structure.
pub fn parseKrbTicket(data: []const u8): KrbTicket? {
    const outer = readDerTlv(data, 0) ?? return null;
    if (outer.tag != TAG_APP_TICKET) {
        return null;
    }
    const seq = parseTaggedSingle(outer.value, TAG_SEQUENCE) ?? return null;

    const f0 = readDerTlv(seq, 0) ?? return null;
    if (f0.tag != 0xA0) {
        return null;
    }
    const tkt_vno = parseTaggedIntU32(f0.value) ?? return null;
    if (tkt_vno != KRB_VERSION) {
        return null;
    }

    const f1 = readDerTlv(seq, f0.next_off) ?? return null;
    if (f1.tag != 0xA1) {
        return null;
    }
    const realm = parseTaggedSingle(f1.value, TAG_GENERAL_STRING) ?? return null;

    const f2 = readDerTlv(seq, f1.next_off) ?? return null;
    if (f2.tag != 0xA2) {
        return null;
    }
    const sname = parsePrincipalNameDer(f2.value) ?? return null;

    const f3 = readDerTlv(seq, f2.next_off) ?? return null;
    if (f3.tag != 0xA3 || f3.next_off != seq.len) {
        return null;
    }
    const enc_part = parseEncryptedDataDer(f3.value) ?? return null;

    return KrbTicket{
        .tkt_vno = tkt_vno,
        .realm = realm,
        .sname = sname,
        .enc_part = enc_part,
    };
}

/// Common header metadata for RFC 4120 `APPLICATION [10..30]` Kerberos v5 messages.
pub struct KrbMessageInfo {
    app_tag: u8,
    pvno: u32,
    msg_type: u32,
    seq_body: []const u8,
}

/// Parses the outer `APPLICATION [10..30]` envelope, inner `SEQUENCE`, `pvno == 5`, and `msg-type`.
/// Supports both RFC 4120 `KDC-REQ` (`[1]` pvno, `[2]` msg-type) and standard (`[0]` pvno, `[1]` msg-type) layouts.
pub fn parseKrbMessageInfo(data: []const u8): KrbMessageInfo? {
    const app_tag = peekKrbAppTag(data) ?? return null;
    if (app_tag < MSG_AS_REQ) {
        return null;
    }
    const outer = readDerTlv(data, 0) ?? return null;
    const seq = parseTaggedSingle(outer.value, TAG_SEQUENCE) ?? return null;

    const f0 = readDerTlv(seq, 0) ?? return null;
    const f1 = readDerTlv(seq, f0.next_off) ?? return null;

    const is_kdc_req = app_tag == MSG_AS_REQ || app_tag == MSG_TGS_REQ;
    let valid_tags = false;
    if (f0.tag == 0xA0 && f1.tag == 0xA1) {
        valid_tags = true;
    } else if (is_kdc_req && f0.tag == 0xA1 && f1.tag == 0xA2) {
        valid_tags = true;
    }
    if (!valid_tags) {
        return null;
    }

    const pvno = parseTaggedIntU32(f0.value) ?? return null;
    if (pvno != KRB_VERSION) {
        return null;
    }
    const msg_type = parseTaggedIntU32(f1.value) ?? return null;
    if (msg_type != (app_tag as u32)) {
        return null;
    }

    return KrbMessageInfo{
        .app_tag = app_tag,
        .pvno = pvno,
        .msg_type = msg_type,
        .seq_body = seq,
    };
}

/// RFC 4120 §5.9.1 `[APPLICATION 30]` `KRB-ERROR` message.
pub struct KrbError {
    pvno: u32,
    msg_type: u32,
    stime: []const u8,
    susec: u32,
    error_code: i32,
    realm: []const u8,
    sname: KrbPrincipalName,
    e_text: []const u8,
    e_data: []const u8,
}

/// Parses an RFC 4120 `[APPLICATION 30]` `KRB-ERROR` message, handling all optional fields.
pub fn parseKrbError(data: []const u8): KrbError? {
    const info = parseKrbMessageInfo(data) ?? return null;
    if (info.app_tag != MSG_KRB_ERROR || info.msg_type != (MSG_KRB_ERROR as u32)) {
        return null;
    }
    const seq = info.seq_body;

    // Skip already-validated [0] pvno and [1] msg-type
    const f0 = readDerTlv(seq, 0) ?? return null;
    const f1 = readDerTlv(seq, f0.next_off) ?? return null;
    let pos = f1.next_off;

    let has_stime = false;
    let has_susec = false;
    let has_err = false;
    let has_realm = false;
    let has_sname = false;

    let stime: []const u8 = seq[0..0];
    let susec: u32 = 0;
    let error_code: i32 = 0;
    let realm: []const u8 = seq[0..0];
    let sname = KrbPrincipalName{ .name_type = 0, .names_der = seq[0..0] };
    let e_text: []const u8 = seq[0..0];
    let e_data: []const u8 = seq[0..0];

    let last_ctx: u8 = 1;
    while (pos < seq.len) {
        const tlv = readDerTlv(seq, pos) ?? return null;
        if (tlv.tag < 0xA2 || tlv.tag > 0xAC) {
            return null;
        }
        const ctx = tlv.tag & 0x1F;
        if (ctx <= last_ctx) {
            return null;
        }
        last_ctx = ctx;

        if (ctx == 2) {
            * = parseTaggedSingle(tlv.value, TAG_GENERALIZED_TIME) ?? return null;
        } else if (ctx == 3) {
            * = parseTaggedIntU32(tlv.value) ?? return null;
        } else if (ctx == 4) {
            stime = parseTaggedSingle(tlv.value, TAG_GENERALIZED_TIME) ?? return null;
            has_stime = true;
        } else if (ctx == 5) {
            susec = parseTaggedIntU32(tlv.value) ?? return null;
            has_susec = true;
        } else if (ctx == 6) {
            error_code = parseTaggedIntI32(tlv.value) ?? return null;
            has_err = true;
        } else if (ctx == 7) {
            * = parseTaggedSingle(tlv.value, TAG_GENERAL_STRING) ?? return null;
        } else if (ctx == 8) {
            * = parsePrincipalNameDer(tlv.value) ?? return null;
        } else if (ctx == 9) {
            realm = parseTaggedSingle(tlv.value, TAG_GENERAL_STRING) ?? return null;
            has_realm = true;
        } else if (ctx == 10) {
            sname = parsePrincipalNameDer(tlv.value) ?? return null;
            has_sname = true;
        } else if (ctx == 11) {
            e_text = parseTaggedSingle(tlv.value, TAG_GENERAL_STRING) ?? return null;
        } else if (ctx == 12) {
            e_data = parseTaggedSingle(tlv.value, TAG_OCTET_STRING) ?? return null;
        }
        pos = tlv.next_off;
    }

    if (!has_stime || !has_susec || !has_err || !has_realm || !has_sname) {
        return null;
    }

    return KrbError{
        .pvno = info.pvno,
        .msg_type = info.msg_type,
        .stime = stime,
        .susec = susec,
        .error_code = error_code,
        .realm = realm,
        .sname = sname,
        .e_text = e_text,
        .e_data = e_data,
    };
}
