// Origin: Pure-CP Stdlib (Zero-Alloc OpenSSH PROTOCOL.sshsig v1 Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation OpenSSH SSHSIG v1 binary & PEM armor parser:
// SSH wire-string decoding, nested publickey/signature extraction, strict Base64 PEM
// armor decoding, and PROTOCOL.sshsig signed-data preimage construction.

/// 6-byte OpenSSH SSHSIG binary magic header `"SSHSIG"`.
pub const SSHSIG_MAGIC: []const u8 = "SSHSIG";

/// OpenSSH PROTOCOL.sshsig v1 format version (`1`).
pub const SSHSIG_VERSION: u32 = 1;

/// PEM armor header banner for SSH signatures.
pub const SSHSIG_ARMOR_BEGIN: []const u8 = "-----BEGIN SSH SIGNATURE-----";

/// PEM armor footer banner for SSH signatures.
pub const SSHSIG_ARMOR_END: []const u8 = "-----END SSH SIGNATURE-----";

/// Zero-copy decoded OpenSSH `SSHSIG` v1 signature blob.
pub struct SshsigBlob {
    version: u32 = 0,
    publickey_blob: []const u8 = "",
    key_type: []const u8 = "",
    namespace: []const u8 = "",
    reserved: []const u8 = "",
    hash_algorithm: []const u8 = "",
    signature_blob: []const u8 = "",
    sig_type: []const u8 = "",
    raw_signature: []const u8 = "",
}

struct WireSlice {
    bytes: []const u8,
    next_pos: usize,
}

// ---------------------------------------------------------------------------
// Internal Big-Endian, Wire-String & Base64 Helpers
// ---------------------------------------------------------------------------

fn readU32Be(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

fn writeU32Be(out: []u8, off: usize, v: u32) {
    out[off] = ((v >> 24) & 0xFF) as u8;
    out[off + 1] = ((v >> 16) & 0xFF) as u8;
    out[off + 2] = ((v >> 8) & 0xFF) as u8;
    out[off + 3] = (v & 0xFF) as u8;
}

fn readSshString(data: []const u8, pos: usize): WireSlice? {
    if (pos + 4 > data.len) {
        return null;
    }
    const len = readU32Be(data, pos) as usize;
    const start = pos + 4;
    if (len > data.len - start) {
        return null;
    }
    return WireSlice {
        bytes: data[start..start + len],
        next_pos: start + len,
    };
}

fn isAsciiWs(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

fn findSubstring(haystack: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0 || start + needle.len > haystack.len) {
        return null;
    }
    let i = start;
    while (i + needle.len <= haystack.len) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
            j += 1;
        }
        if (matched) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn base64Val(ch: u8): u8? {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A';
    }
    if (ch >= 'a' && ch <= 'z') {
        return 26 + (ch - 'a');
    }
    if (ch >= '0' && ch <= '9') {
        return 52 + (ch - '0');
    }
    if (ch == '+') {
        return 62;
    }
    if (ch == '/') {
        return 63;
    }
    return null;
}

fn writeWireString(out: []u8, pos: usize, s: []const u8): usize? {
    if (s.len > 0xFFFFFFFF) {
        return null;
    }
    if (pos + 4 > out.len || s.len > out.len - (pos + 4)) {
        return null;
    }
    writeU32Be(out, pos, s.len as u32);
    let i: usize = 0;
    while (i < s.len) {
        out[pos + 4 + i] = s[i];
        i += 1;
    }
    return pos + 4 + s.len;
}

// ---------------------------------------------------------------------------
// Public Detection, PEM Armor Decoding & Binary Parsing API
// ---------------------------------------------------------------------------

/// Validates that `data` is at least 10 bytes, starts with `"SSHSIG"`, and has BE `version == 1`.
pub fn isSshsigBinary(data: []const u8): bool {
    if (data.len < 10) {
        return false;
    }
    if (data[0] != 'S' || data[1] != 'S' || data[2] != 'H' ||
        data[3] != 'S' || data[4] != 'I' || data[5] != 'G')
    {
        return false;
    }
    return readU32Be(data, 6) == SSHSIG_VERSION;
}

/// Validates that `data` (after optional leading whitespace) contains both
/// `SSHSIG_ARMOR_BEGIN` and `SSHSIG_ARMOR_END` in order.
pub fn isSshsigArmored(data: []const u8): bool {
    let start: usize = 0;
    while (start < data.len && isAsciiWs(data[start])) {
        start += 1;
    }
    if (b_pos := findSubstring(data, start, SSHSIG_ARMOR_BEGIN)) {
        const after_begin = b_pos + SSHSIG_ARMOR_BEGIN.len;
        if (e_pos := findSubstring(data, after_begin, SSHSIG_ARMOR_END)) {
            * = e_pos;
            return true;
        }
    }
    return false;
}

/// Strips PEM `-----BEGIN/END SSH SIGNATURE-----` armor banners and decodes the enclosed
/// standard Base64 payload into `out` (ignoring `\r`, `\n`, spaces, and tabs).
/// Returns decoded byte count, or `null` on malformed Base64, missing banners, or small `out`.
pub fn decodeSshsigArmor(armored: []const u8, out: []u8): usize? {
    let start: usize = 0;
    while (start < armored.len && isAsciiWs(armored[start])) {
        start += 1;
    }
    let b_pos: usize = 0;
    if (p := findSubstring(armored, start, SSHSIG_ARMOR_BEGIN)) {
        b_pos = p;
    } else {
        return null;
    }

    const body_start = b_pos + SSHSIG_ARMOR_BEGIN.len;
    let e_pos: usize = 0;
    if (p := findSubstring(armored, body_start, SSHSIG_ARMOR_END)) {
        e_pos = p;
    } else {
        return null;
    }

    const body = armored[body_start..e_pos];
    let quad: [4]u8 = [0; 4];
    let q_len: usize = 0;
    let out_pos: usize = 0;
    let saw_padding = false;

    let i: usize = 0;
    while (i < body.len) {
        const ch = body[i];
        i += 1;
        if (isAsciiWs(ch)) {
            continue;
        }
        if (saw_padding) {
            return null;
        }
        quad[q_len] = ch;
        q_len += 1;

        if (q_len == 4) {
            let v0: u8 = 0;
            let v1: u8 = 0;
            if (x := base64Val(quad[0])) { v0 = x; } else { return null; }
            if (x := base64Val(quad[1])) { v1 = x; } else { return null; }

            if (quad[2] == '=') {
                if (quad[3] != '=') {
                    return null;
                }
                if (out_pos + 1 > out.len) {
                    return null;
                }
                out[out_pos] = (v0 << 2) | (v1 >> 4);
                out_pos += 1;
                saw_padding = true;
            } else if (quad[3] == '=') {
                let v2: u8 = 0;
                if (x := base64Val(quad[2])) { v2 = x; } else { return null; }
                if (out_pos + 2 > out.len) {
                    return null;
                }
                out[out_pos] = (v0 << 2) | (v1 >> 4);
                out[out_pos + 1] = ((v1 & 0x0F) << 4) | (v2 >> 2);
                out_pos += 2;
                saw_padding = true;
            } else {
                let v2: u8 = 0;
                let v3: u8 = 0;
                if (x := base64Val(quad[2])) { v2 = x; } else { return null; }
                if (x := base64Val(quad[3])) { v3 = x; } else { return null; }
                if (out_pos + 3 > out.len) {
                    return null;
                }
                out[out_pos] = (v0 << 2) | (v1 >> 4);
                out[out_pos + 1] = ((v1 & 0x0F) << 4) | (v2 >> 2);
                out[out_pos + 2] = ((v2 & 0x03) << 6) | v3;
                out_pos += 3;
            }
            q_len = 0;
        }
    }

    if (q_len != 0 || out_pos == 0) {
        return null;
    }
    return out_pos;
}

/// Parses a binary OpenSSH `SSHSIG` v1 blob in strict zero-copy wire order.
/// Enforces `version == 1`, non-empty `namespace`, valid nested `key_type`/`sig_type`/`raw_signature`,
/// and zero trailing garbage bytes.
pub fn parseSshsigBinary(data: []const u8): SshsigBlob? {
    if (!isSshsigBinary(data)) {
        return null;
    }

    let pos: usize = 10;
    let pk_blob: []const u8 = "";
    let ns: []const u8 = "";
    let reserved: []const u8 = "";
    let hash_alg: []const u8 = "";
    let sig_blob: []const u8 = "";

    if (w := readSshString(data, pos)) { pk_blob = w.bytes; pos = w.next_pos; } else { return null; }
    if (w := readSshString(data, pos)) { ns = w.bytes; pos = w.next_pos; } else { return null; }
    if (w := readSshString(data, pos)) { reserved = w.bytes; pos = w.next_pos; } else { return null; }
    if (w := readSshString(data, pos)) { hash_alg = w.bytes; pos = w.next_pos; } else { return null; }
    if (w := readSshString(data, pos)) { sig_blob = w.bytes; pos = w.next_pos; } else { return null; }

    // Strict check: no trailing garbage bytes and namespace must be non-empty
    if (pos != data.len || ns.len == 0 || hash_alg.len == 0) {
        return null;
    }

    // Parse nested key_type from publickey_blob
    let key_type: []const u8 = "";
    if (kt := readSshString(pk_blob, 0)) {
        key_type = kt.bytes;
    } else {
        return null;
    }
    if (key_type.len == 0) {
        return null;
    }

    // Parse nested sig_type & raw_signature from signature_blob
    let sig_type: []const u8 = "";
    let raw_sig: []const u8 = "";
    if (st := readSshString(sig_blob, 0)) {
        sig_type = st.bytes;
        if (rs := readSshString(sig_blob, st.next_pos)) {
            raw_sig = rs.bytes;
            if (rs.next_pos != sig_blob.len) {
                return null;
            }
        } else {
            return null;
        }
    } else {
        return null;
    }
    if (sig_type.len == 0) {
        return null;
    }

    return SshsigBlob {
        version: SSHSIG_VERSION,
        publickey_blob: pk_blob,
        key_type: key_type,
        namespace: ns,
        reserved: reserved,
        hash_algorithm: hash_alg,
        signature_blob: sig_blob,
        sig_type: sig_type,
        raw_signature: raw_sig,
    };
}

/// Constructs the OpenSSH `PROTOCOL.sshsig` message-to-be-signed preimage into `out`:
/// `"SSHSIG" || string(namespace) || string(reserved) || string(hash_alg) || string(hash_digest)`.
/// Returns bytes written, or `null` if `namespace` is empty or `out` is too small.
pub fn buildSshsigSignedData(
    namespace: []const u8,
    reserved: []const u8,
    hash_alg: []const u8,
    hash_digest: []const u8,
    out: []u8,
): usize? {
    if (namespace.len == 0 || hash_alg.len == 0) {
        return null;
    }
    if (out.len < 6) {
        return null;
    }
    let i: usize = 0;
    while (i < 6) {
        out[i] = SSHSIG_MAGIC[i];
        i += 1;
    }
    let pos: usize = 6;
    if (p := writeWireString(out, pos, namespace)) { pos = p; } else { return null; }
    if (p := writeWireString(out, pos, reserved)) { pos = p; } else { return null; }
    if (p := writeWireString(out, pos, hash_alg)) { pos = p; } else { return null; }
    if (p := writeWireString(out, pos, hash_digest)) { pos = p; } else { return null; }
    return pos;
}
