// Origin: Pure-CP Stdlib (Native TLS 1.2/1.3 Config, PEM Validation, ALPN & ClientHello SNI: cap:web)
// ============================================================================
// Zero-C-Dependency TLS Configuration, PEM Certificate/Key Validation,
// ALPN Protocol Negotiation & Zero-Alloc TLS ClientHello / SNI Sniffing.
// ============================================================================

const std = @import("std");

const TLS_MAX_PATH_LEN: usize = 512;
const TLS_MAX_PEM_READ_LEN: usize = 16384;

pub enum TlsAlpnProto: u8 {
    Http11,
    Http2,
    Auto,
}

pub struct TlsConfig {
    enabled: bool = false;
    cert_path: string = "";
    key_path: string = "";
    min_version: u16 = 0x0303;
    alpn: TlsAlpnProto = TlsAlpnProto.Auto;
}

pub enum TlsConfigError: u8 {
    Ok,
    MissingCertPath,
    MissingKeyPath,
    InvalidPemHeader,
    FileNotFound,
    UnsupportedVersion,
}

fn tlsIndexOf(hay: string, needle: string, start: usize): usize? {
    if (needle.len == 0) return start;
    if (start >= hay.len || needle.len > hay.len - start) return null;
    let i = start;
    const limit = hay.len - needle.len;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) : (j += 1) {
            if (hay[i + j] != needle[j]) {
                matched = false;
                break;
            }
        }
        if (matched) return i;
    }
    return null;
}

fn tlsBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn tlsHasPemBlock(pem: string, begin_marker: string, end_marker: string): bool {
    const bpos = tlsIndexOf(pem, begin_marker, 0) ?? return false;
    const after_begin = bpos + begin_marker.len;
    const epos = tlsIndexOf(pem, end_marker, after_begin) ?? return false;
    return epos >= after_begin;
}

/// Validate PEM-encoded X.509 certificate header & footer markers.
pub fn validatePemCertBytes(pem: string): bool {
    return tlsHasPemBlock(pem, "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----");
}

/// Validate PEM-encoded private key (PKCS#8, RSA, or EC) header & footer markers.
pub fn validatePemKeyBytes(pem: string): bool {
    if (tlsHasPemBlock(pem, "-----BEGIN PRIVATE KEY-----", "-----END PRIVATE KEY-----")) {
        return true;
    }
    if (tlsHasPemBlock(pem, "-----BEGIN RSA PRIVATE KEY-----", "-----END RSA PRIVATE KEY-----")) {
        return true;
    }
    if (tlsHasPemBlock(pem, "-----BEGIN EC PRIVATE KEY-----", "-----END EC PRIVATE KEY-----")) {
        return true;
    }
    return false;
}

/// Detect standard TLS Record Header (`0x16, 0x03, 0x01..0x04`) with Handshake Type `0x01` (ClientHello).
pub fn isTlsClientHello(bytes: string): bool {
    if (bytes.len < 6) return false;
    if (bytes[0] != 0x16) return false;
    if (bytes[1] != 0x03) return false;
    if (bytes[2] < 0x01 || bytes[2] > 0x04) return false;
    if (bytes[5] != 0x01) return false;
    return true;
}

/// Zero-allocation extraction of SNI hostname (extension `0x0000`) from a TLS ClientHello record.
/// Returns hostname length written to `out_buf`, or `null` if absent, truncated, malformed, or `out_buf` is too small.
pub fn extractSniFromClientHello(record: string, out_buf: []u8): usize? {
    if (!isTlsClientHello(record)) return null;
    if (record.len < 9) return null;

    const record_len: usize = ((record[3] as usize) << 8) | (record[4] as usize);
    const rec_avail = record.len - 5;
    const rec_use: usize = if (record_len < rec_avail) record_len else rec_avail;
    const rec_end: usize = (5 as usize) + rec_use;
    if (rec_end < 9) return null;

    const hs_len: usize = ((record[6] as usize) << 16)
        | ((record[7] as usize) << 8)
        | (record[8] as usize);
    const hs_avail = rec_end - 9;
    const hs_use: usize = if (hs_len < hs_avail) hs_len else hs_avail;
    const hs_end: usize = (9 as usize) + hs_use;

    // ClientHello fixed fields: legacy_version (2) + random (32) = 34 bytes
    let pos: usize = 9;
    if (hs_end < pos + 34) return null;
    pos += 34;

    // legacy_session_id: 1-byte length + data
    if (pos + 1 > hs_end) return null;
    const sid_len = record[pos] as usize;
    pos += 1;
    if (sid_len > hs_end - pos) return null;
    pos += sid_len;

    // cipher_suites: 2-byte length + data
    if (pos + 2 > hs_end) return null;
    const cs_len: usize = ((record[pos] as usize) << 8) | (record[pos + 1] as usize);
    pos += 2;
    if (cs_len > hs_end - pos) return null;
    pos += cs_len;

    // legacy_compression_methods: 1-byte length + data
    if (pos + 1 > hs_end) return null;
    const comp_len = record[pos] as usize;
    pos += 1;
    if (comp_len > hs_end - pos) return null;
    pos += comp_len;

    // extensions: 2-byte total length + sequence of (type: u16, len: u16, body)
    if (pos + 2 > hs_end) return null;
    const exts_len: usize = ((record[pos] as usize) << 8) | (record[pos + 1] as usize);
    pos += 2;
    if (exts_len > hs_end - pos) return null;
    const exts_end = pos + exts_len;

    while (pos + 4 <= exts_end) {
        const ext_type: u16 = ((record[pos] as u16) << 8) | (record[pos + 1] as u16);
        const ext_len: usize = ((record[pos + 2] as usize) << 8) | (record[pos + 3] as usize);
        pos += 4;
        if (ext_len > exts_end - pos) return null;

        if (ext_type == 0x0000) {
            const sni_end = pos + ext_len;
            if (pos + 2 > sni_end) return null;
            const list_len: usize = ((record[pos] as usize) << 8) | (record[pos + 1] as usize);
            let spos = pos + 2;
            if (list_len > sni_end - spos) return null;
            const list_end = spos + list_len;

            while (spos + 3 <= list_end) {
                const name_type = record[spos];
                const name_len: usize = ((record[spos + 1] as usize) << 8) | (record[spos + 2] as usize);
                spos += 3;
                if (name_len > list_end - spos) return null;
                if (name_type == 0x00) {
                    if (name_len == 0 || name_len > out_buf.len) return null;
                    let k: usize = 0;
                    while (k < name_len) : (k += 1) {
                        out_buf[k] = record[spos + k];
                    }
                    return name_len;
                }
                spos += name_len;
            }
            return null;
        }
        pos += ext_len;
    }
    return null;
}

fn tlsCsvContainsProto(protos: string, target: string): bool {
    let i: usize = 0;
    while (i < protos.len) {
        while (i < protos.len && (protos[i] == ' ' || protos[i] == '\t' || protos[i] == ',')) : (i += 1) {}
        const start = i;
        while (i < protos.len && protos[i] != ',') : (i += 1) {}
        let end = i;
        while (end > start && (protos[end - 1] == ' ' || protos[end - 1] == '\t')) : (end -= 1) {}
        if (end > start && tlsBytesEql(protos[start..end], target)) {
            return true;
        }
        if (i < protos.len && protos[i] == ',') {
            i += 1;
        }
    }
    return false;
}

/// Negotiate ALPN application protocol (`"h2"` or `"http/1.1"`) from comma-separated client list.
pub fn negotiateAlpn(client_protos: string, pref: TlsAlpnProto): string {
    const has_h2 = tlsCsvContainsProto(client_protos, "h2");
    const has_h11 = tlsCsvContainsProto(client_protos, "http/1.1");
    if (pref == TlsAlpnProto.Http2) {
        return if (has_h2) "h2" else "";
    }
    if (pref == TlsAlpnProto.Http11) {
        return if (has_h11) "http/1.1" else "";
    }
    if (has_h2) return "h2";
    if (has_h11) return "http/1.1";
    return "";
}

/// Parse a server ALPN config string (e.g. `"h2,http/1.1"`, `"h2"`, `"http/1.1"`) into `TlsAlpnProto`.
pub fn parseAlpnPreference(alpn: string): TlsAlpnProto {
    const has_h2 = tlsCsvContainsProto(alpn, "h2");
    const has_h11 = tlsCsvContainsProto(alpn, "http/1.1");
    if (has_h2 && has_h11) return TlsAlpnProto.Auto;
    if (has_h2) return TlsAlpnProto.Http2;
    if (has_h11) return TlsAlpnProto.Http11;
    return TlsAlpnProto.Auto;
}

/// Build a standard 7-byte TLS 1.2/1.3 Fatal Alert record (e.g. `handshake_failure` = 0x28, `protocol_version` = 0x46).
pub fn tlsFatalAlertRecord(alert_desc: u8): [7]u8 {
    let out: [7]u8 = [0x15, 0x03, 0x03, 0x00, 0x02, 0x02, alert_desc];
    return out;
}

fn tlsReadFileStack(path: string, out_buf: []u8): usize? {
    if (path.len == 0 || path.len >= TLS_MAX_PATH_LEN) return null;
    let cpath: [TLS_MAX_PATH_LEN]u8 = [0; TLS_MAX_PATH_LEN];
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        if (path[i] == 0) return null;
        cpath[i] = path[i];
    }
    cpath[path.len] = 0;

    let total: usize = 0;
    unsafe {
        const path_z: [*:0]const u8 = @ptrCast(&cpath[0]);
        const fd_raw = std.os.linux.open(path_z, .{}, 0);
        const fd_i: isize = @bitCast(fd_raw);
        if (fd_i < 0) return null;
        const fd: i32 = @intCast(fd_i);

        while (total < out_buf.len) {
            const n_raw = std.os.linux.read(fd, out_buf.ptr + total, out_buf.len - total);
            const n_i: isize = @bitCast(n_raw);
            if (n_i <= 0) break;
            total += n_i as usize;
        }
        * = std.os.linux.close(fd);
    }
    return total;
}

/// Validate TLS configuration version and verify on-disk PEM certificate & private key files exist and have valid PEM headers.
pub fn validateTlsFiles(cfg: TlsConfig): TlsConfigError {
    if (cfg.min_version != 0x0303 && cfg.min_version != 0x0304) {
        return TlsConfigError.UnsupportedVersion;
    }
    if (cfg.cert_path.len == 0) {
        return TlsConfigError.MissingCertPath;
    }
    if (cfg.key_path.len == 0) {
        return TlsConfigError.MissingKeyPath;
    }

    let pem_buf: [TLS_MAX_PEM_READ_LEN]u8 = undefined;
    const cert_len = tlsReadFileStack(cfg.cert_path, pem_buf[0..]) ?? return TlsConfigError.FileNotFound;
    if (!validatePemCertBytes(pem_buf[0..cert_len])) {
        return TlsConfigError.InvalidPemHeader;
    }

    const key_len = tlsReadFileStack(cfg.key_path, pem_buf[0..]) ?? return TlsConfigError.FileNotFound;
    if (!validatePemKeyBytes(pem_buf[0..key_len])) {
        return TlsConfigError.InvalidPemHeader;
    }

    return TlsConfigError.Ok;
}
