// @cap/ssh-key — Zero-heap OpenSSH public key (authorized_keys) parser & validator
// Clean-room MIT implementation for CP (compatible with npm sshpk & Rust ssh-key).

import { strEq, trim, isAlnum } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const ALG_UNKNOWN: u8 = 0;
pub const ALG_ED25519: u8 = 1;
pub const ALG_RSA: u8 = 2;
pub const ALG_ECDSA_P256: u8 = 3;
pub const ALG_ECDSA_P384: u8 = 4;
pub const ALG_ECDSA_P521: u8 = 5;
pub const ALG_SK_ED25519: u8 = 6;
pub const ALG_SK_ECDSA_P256: u8 = 7;

pub struct SshPublicKey {
    pub alg: u8,
    pub key_type: string,
    pub key_b64: string,
    pub comment: string,
    pub valid: bool,
}

fn isHspace(c: u8): bool {
    return c == ' ' || c == '\t';
}

/// Identify OpenSSH public key algorithm identifier string.
pub fn identifyAlgorithm(key_type: string): u8 {
    if (strEq(key_type, "ssh-ed25519")) { return ALG_ED25519; }
    if (strEq(key_type, "ssh-rsa")) { return ALG_RSA; }
    if (strEq(key_type, "ecdsa-sha2-nistp256")) { return ALG_ECDSA_P256; }
    if (strEq(key_type, "ecdsa-sha2-nistp384")) { return ALG_ECDSA_P384; }
    if (strEq(key_type, "ecdsa-sha2-nistp521")) { return ALG_ECDSA_P521; }
    if (strEq(key_type, "sk-ssh-ed25519@openssh.com")) { return ALG_SK_ED25519; }
    if (strEq(key_type, "sk-ecdsa-sha2-nistp256@openssh.com")) { return ALG_SK_ECDSA_P256; }
    return ALG_UNKNOWN;
}

/// Validate that `b64` is well-formed non-empty RFC 4648 Base64 (length % 4 == 0, at most two trailing '=').
pub fn isValidBase64(b64: string): bool {
    if (b64.len < 4 || (b64.len % 4) != 0) {
        return false;
    }
    let pad_started: bool = false;
    let pad_count: usize = 0;
    let i: usize = 0;
    while (i < b64.len) {
        let c: u8 = b64[i];
        if (c == '=') {
            pad_started = true;
            pad_count += 1;
            if (pad_count > 2) { return false; }
        } else {
            if (pad_started) { return false; }
            if (!isAlnum(c) && c != '+' && c != '/') { return false; }
        }
        i += 1;
    }
    return true;
}

/// Parse a single OpenSSH public key line (`<type> <base64> [comment]`).
pub fn parsePublicKey(line: string): SshPublicKey {
    let invalid: SshPublicKey = SshPublicKey {
        alg: ALG_UNKNOWN,
        key_type: "",
        key_b64: "",
        comment: "",
        valid: false,
    };

    let s: string = trim(line);
    if (s.len == 0 || s[0] == '#') {
        return invalid;
    }

    let i: usize = 0;
    while (i < s.len && !isHspace(s[i])) { i += 1; }
    let key_type: string = s[0..i];
    let alg: u8 = identifyAlgorithm(key_type);
    if (alg == ALG_UNKNOWN) {
        return invalid;
    }

    while (i < s.len && isHspace(s[i])) { i += 1; }
    if (i >= s.len) { return invalid; }

    let b64_start: usize = i;
    while (i < s.len && !isHspace(s[i])) { i += 1; }
    let key_b64: string = s[b64_start..i];
    if (!isValidBase64(key_b64)) {
        return invalid;
    }

    while (i < s.len && isHspace(s[i])) { i += 1; }
    let comment: string = "";
    if (i < s.len) {
        comment = s[i..s.len];
    }

    return SshPublicKey {
        alg: alg,
        key_type: key_type,
        key_b64: key_b64,
        comment: comment,
        valid: true,
    };
}

/// Format an `SshPublicKey` back to canonical single-line OpenSSH format (`<type> <b64>[ <comment>]`).
pub fn formatPublicKey(key: SshPublicKey, out: []u8): usize {
    if (!key.valid || key.key_type.len == 0 || key.key_b64.len == 0) {
        return 0;
    }
    let need: usize = key.key_type.len + 1 + key.key_b64.len;
    if (key.comment.len > 0) {
        need += 1 + key.comment.len;
    }
    if (need > out.len) {
        return 0;
    }

    let pos: usize = writeStr(key.key_type, out, 0);
    out[pos] = ' ';
    pos += 1;
    pos = writeStr(key.key_b64, out, pos);
    if (key.comment.len > 0) {
        out[pos] = ' ';
        pos += 1;
        pos = writeStr(key.comment, out, pos);
    }
    return pos;
}

/// Count valid non-comment public keys in a multi-line `authorized_keys` text.
pub fn countAuthorizedKeys(text: string): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        let line_start: usize = i;
        while (i < text.len && text[i] != '\n') { i += 1; }
        let k: SshPublicKey = parsePublicKey(text[line_start..i]);
        if (k.valid) {
            count += 1;
        }
        if (i < text.len) { i += 1; }
    }
    return count;
}

/// Check if a key uses a modern recommended algorithm (Ed25519, ECDSA, or FIDO2 SK-*).
pub fn isModernAlgorithm(alg: u8): bool {
    return alg == ALG_ED25519 ||
        alg == ALG_ECDSA_P256 ||
        alg == ALG_ECDSA_P384 ||
        alg == ALG_ECDSA_P521 ||
        alg == ALG_SK_ED25519 ||
        alg == ALG_SK_ECDSA_P256;
}
