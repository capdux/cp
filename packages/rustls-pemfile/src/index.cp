// packages/@cap/rustls-pemfile/src/index.cp
// Zero-allocation streaming TLS PEM bundle reader in pure CP.
// Reuses @cap/pem-rfc7468 for RFC 7468 PEM framing, label validation, and Base64 decoding.
// Clean-room MIT implementation (rustls-pemfile v2 parity).

import {
    PEM_OK,
    PEM_ERR_MISSING_BEGIN,
    locatePem,
    decodePem,
    decodeLabel,
    isValidLabel,
} from "../../pem-rfc7468/src/index.cp";
import { bytesEq } from "cap:bin";

pub const PEM_ITEM_NONE: u8 = 0;
pub const PEM_ITEM_X509_CERT: u8 = 1;
pub const PEM_ITEM_PKCS8_KEY: u8 = 2;
pub const PEM_ITEM_PKCS1_KEY: u8 = 3;
pub const PEM_ITEM_SEC1_KEY: u8 = 4;
pub const PEM_ITEM_SPKI_PUBKEY: u8 = 5;
pub const PEM_ITEM_CRL: u8 = 6;
pub const PEM_ITEM_CSR: u8 = 7;
pub const PEM_ITEM_UNKNOWN: u8 = 8;

fn rpfStrEq(a: string, b: string): bool {
    return bytesEq(a, b);
}

/// Returns true if `kind` represents a supported private key item
/// (PKCS#8, PKCS#1 RSA, or SEC1 EC).
pub fn isPrivateKeyKind(kind: u8): bool {
    return kind == PEM_ITEM_PKCS8_KEY
        || kind == PEM_ITEM_PKCS1_KEY
        || kind == PEM_ITEM_SEC1_KEY;
}

/// Maps an RFC 7468 PEM label string to its corresponding `PEM_ITEM_*` constant.
/// Returns `PEM_ITEM_NONE` for empty or invalid RFC 7468 labels, and
/// `PEM_ITEM_UNKNOWN` for valid labels outside the standard TLS/PKI set.
pub fn classifyPemLabel(label: string): u8 {
    if (!isValidLabel(label)) {
        return PEM_ITEM_NONE;
    }
    if (rpfStrEq(label, "CERTIFICATE")) {
        return PEM_ITEM_X509_CERT;
    }
    if (rpfStrEq(label, "PRIVATE KEY")) {
        return PEM_ITEM_PKCS8_KEY;
    }
    if (rpfStrEq(label, "RSA PRIVATE KEY")) {
        return PEM_ITEM_PKCS1_KEY;
    }
    if (rpfStrEq(label, "EC PRIVATE KEY")) {
        return PEM_ITEM_SEC1_KEY;
    }
    if (rpfStrEq(label, "PUBLIC KEY")) {
        return PEM_ITEM_SPKI_PUBKEY;
    }
    if (rpfStrEq(label, "X509 CRL")) {
        return PEM_ITEM_CRL;
    }
    if (rpfStrEq(label, "CERTIFICATE REQUEST")) {
        return PEM_ITEM_CSR;
    }
    return PEM_ITEM_UNKNOWN;
}

pub struct PemItem {
    pub kind: u8,
    pub der_len: usize,
    pub ok: bool,
    pub eof: bool,

    pub static fn makeEof(): PemItem {
        return PemItem {
            kind: PEM_ITEM_NONE,
            der_len: 0,
            ok: true,
            eof: true,
        };
    }

    pub static fn makeErr(): PemItem {
        return PemItem {
            kind: PEM_ITEM_NONE,
            der_len: 0,
            ok: false,
            eof: false,
        };
    }

    pub fn isCert(): bool {
        return _.ok && !_.eof && _.kind == PEM_ITEM_X509_CERT;
    }

    pub fn isPrivateKey(): bool {
        return _.ok && !_.eof && isPrivateKeyKind(_.kind);
    }
}

pub struct PemBundleReader {
    pub input: string,
    pub offset: usize,

    pub static fn new(input: string): PemBundleReader {
        return PemBundleReader {
            input: input,
            offset: 0,
        };
    }

    /// Returns the number of unread bytes remaining in the PEM bundle.
    pub fn remaining(): usize {
        if (_.offset >= _.input.len) {
            return 0;
        }
        return _.input.len - _.offset;
    }

    /// Returns true if the cursor has reached the end of the input buffer.
    pub fn isEof(): bool {
        return _.offset >= _.input.len;
    }

    /// Resets the cursor back to the beginning of the PEM bundle.
    pub fn reset(*_) {
        _.offset = 0;
    }

    /// Peeks the `PEM_ITEM_*` kind of the next PEM block without advancing the cursor
    /// or decoding the Base64 payload. Returns `PEM_ITEM_NONE` if no valid block follows.
    pub fn peekNextKind(): u8 {
        if (_.offset >= _.input.len) {
            return PEM_ITEM_NONE;
        }
        let label_buf: [64]u8 = undefined;
        const n = decodeLabel(_.input[_.offset.._.input.len], label_buf[0..64]);
        if (n <= 0) {
            return PEM_ITEM_NONE;
        }
        return classifyPemLabel(label_buf[0..(n as usize)]);
    }

    /// Advances the cursor to the next PEM block, skips arbitrary inter-block comments
    /// and whitespace, decodes its DER payload into `der_out` via `@cap/pem-rfc7468::decodePem`,
    /// and classifies its PEM label.
    pub fn nextItem(*_, der_out: []u8): PemItem {
        if (_.offset >= _.input.len) {
            return PemItem.makeEof();
        }
        const rest = _.input[_.offset.._.input.len];
        const loc = locatePem(rest);
        if (loc.err == PEM_ERR_MISSING_BEGIN) {
            _.offset = _.input.len;
            return PemItem.makeEof();
        }
        if (loc.err != PEM_OK) {
            _.offset = _.input.len;
            return PemItem.makeErr();
        }

        const block = rest[0..loc.end_pos];
        let label_buf: [64]u8 = undefined;
        let label_len: usize = 0;
        const dec_len = decodePem(block, label_buf[0..64], &label_len, der_out);
        _.offset = _.offset + loc.end_pos;
        if (dec_len < 0) {
            return PemItem.makeErr();
        }
        const kind = classifyPemLabel(label_buf[0..label_len]);
        return PemItem {
            kind: kind,
            der_len: dec_len as usize,
            ok: true,
            eof: false,
        };
    }

    /// Counts the total number of well-framed PEM items from the current cursor position
    /// to the end of the bundle without advancing the cursor or decoding Base64.
    pub fn countItems(): usize {
        let pos: usize = _.offset;
        let count: usize = 0;
        while (pos < _.input.len) {
            const rest = _.input[pos.._.input.len];
            const loc = locatePem(rest);
            if (loc.err != PEM_OK || loc.end_pos == 0) {
                break;
            }
            count = count + 1;
            pos = pos + loc.end_pos;
        }
        return count;
    }

    /// Counts the number of `CERTIFICATE` PEM items from the current cursor position
    /// to the end of the bundle without advancing the cursor or decoding Base64.
    pub fn countCerts(): usize {
        let pos: usize = _.offset;
        let count: usize = 0;
        while (pos < _.input.len) {
            const rest = _.input[pos.._.input.len];
            const loc = locatePem(rest);
            if (loc.err != PEM_OK || loc.end_pos == 0) {
                break;
            }
            const label = rest[loc.label_start..loc.label_start + loc.label_len];
            if (classifyPemLabel(label) == PEM_ITEM_X509_CERT) {
                count = count + 1;
            }
            pos = pos + loc.end_pos;
        }
        return count;
    }
}

/// Quickly scans a mixed PEM bundle (e.g. certificate chain + private key) and decodes
/// the first private key (`PRIVATE KEY`, `RSA PRIVATE KEY`, or `EC PRIVATE KEY`) into `der_out`.
/// Returns `eof = true, ok = true` if no private key is present, or `ok = false` on malformed PEM.
pub fn readFirstPrivateKey(input: string, der_out: []u8): PemItem {
    let pos: usize = 0;
    let label_buf: [64]u8 = undefined;
    while (pos < input.len) {
        const rest = input[pos..input.len];
        const loc = locatePem(rest);
        if (loc.err == PEM_ERR_MISSING_BEGIN) {
            return PemItem.makeEof();
        }
        if (loc.err != PEM_OK || loc.end_pos == 0) {
            return PemItem.makeErr();
        }
        const label = rest[loc.label_start..loc.label_start + loc.label_len];
        const kind = classifyPemLabel(label);
        if (isPrivateKeyKind(kind)) {
            let label_len: usize = 0;
            const dec_len = decodePem(rest[0..loc.end_pos], label_buf[0..64], &label_len, der_out);
            if (dec_len < 0) {
                return PemItem.makeErr();
            }
            return PemItem {
                kind: kind,
                der_len: dec_len as usize,
                ok: true,
                eof: false,
            };
        }
        pos = pos + loc.end_pos;
    }
    return PemItem.makeEof();
}
