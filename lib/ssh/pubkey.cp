// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T16:05:00Z (OpenSSH authorized_keys Public Key Line Parser)
// Pure-CP zero-allocation OpenSSH public key & authorized_keys parser, option splitter, and key-type classifier.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

export enum SshKeyType {
    ed25519,
    rsa,
    ecdsa_nistp256,
    ecdsa_nistp384,
    ecdsa_nistp521,
    sk_ed25519,
    sk_ecdsa_nistp256,
    unknown,
}

export fn parseKeyType(alg: []const u8): SshKeyType {
    if (strEq(alg, "ssh-ed25519")) return SshKeyType.ed25519;
    if (strEq(alg, "ssh-rsa")) return SshKeyType.rsa;
    if (strEq(alg, "ecdsa-sha2-nistp256")) return SshKeyType.ecdsa_nistp256;
    if (strEq(alg, "ecdsa-sha2-nistp384")) return SshKeyType.ecdsa_nistp384;
    if (strEq(alg, "ecdsa-sha2-nistp521")) return SshKeyType.ecdsa_nistp521;
    if (strEq(alg, "sk-ssh-ed25519@openssh.com")) return SshKeyType.sk_ed25519;
    if (strEq(alg, "sk-ecdsa-sha2-nistp256@openssh.com")) return SshKeyType.sk_ecdsa_nistp256;
    return SshKeyType.unknown;
}

fn isValidBase64Blob(s: []const u8): bool {
    if (s.len < 4 || (s.len % 4) != 0) return false;
    let i: usize = 0;
    let pad_started = false;
    let pad_count: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '=') {
            pad_started = true;
            pad_count += 1;
            if (pad_count > 2) return false;
        } else {
            if (pad_started) return false;
            const ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                c == '+' || c == '/';
            if (!ok) return false;
        }
        i += 1;
    }
    return true;
}

fn nextUnquotedToken(s: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < s.len && isWs(s[pos.*])) pos.* += 1;
    if (pos.* >= s.len) return null;
    const start = pos.*;
    while (pos.* < s.len && !isWs(s[pos.*])) pos.* += 1;
    return s[start..pos.*];
}

/// Scan an authorized_keys options field (which may contain double-quoted strings with spaces).
fn scanOptionsToken(s: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < s.len && isWs(s[pos.*])) pos.* += 1;
    if (pos.* >= s.len) return null;
    const start = pos.*;
    let in_quote = false;
    let escaped = false;
    while (pos.* < s.len) {
        const c = s[pos.*];
        if (in_quote) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_quote = false;
            }
        } else {
            if (c == '"') {
                in_quote = true;
            } else if (isWs(c)) {
                break;
            }
        }
        pos.* += 1;
    }
    if (in_quote) return null;
    return s[start..pos.*];
}

export struct SshPubKey {
    options: ([]const u8)?,
    key_type_raw: []const u8,
    key_type: SshKeyType,
    key_base64: []const u8,
    comment: ([]const u8)?,

    /// Parse a single OpenSSH public key or `authorized_keys` line.
    /// Supports optional leading options (e.g. `no-pty,command="/bin/echo hi" ssh-ed25519 AAAA... user@host`).
    pub fn parseLine(raw_line: []const u8): SshPubKey? {
        let end = raw_line.len;
        while (end > 0 && (raw_line[end - 1] == '\r' || raw_line[end - 1] == '\n' || isWs(raw_line[end - 1]))) {
            end -= 1;
        }
        let pos: usize = 0;
        while (pos < end && isWs(raw_line[pos])) pos += 1;
        if (pos >= end || raw_line[pos] == '#') return null;

        const line = raw_line[pos..end];
        let p: usize = 0;
        const first = scanOptionsToken(line, &p) ?? return null;

        let options: ([]const u8)? = null;
        let alg = first;
        let kt = parseKeyType(alg);

        if (kt == SshKeyType.unknown) {
            // Treat first token as authorized_keys options field
            options = first;
            alg = nextUnquotedToken(line, &p) ?? return null;
            kt = parseKeyType(alg);
            if (kt == SshKeyType.unknown) return null;
        }

        const blob = nextUnquotedToken(line, &p) ?? return null;
        if (!isValidBase64Blob(blob)) return null;

        while (p < line.len && isWs(line[p])) p += 1;
        let comment: ([]const u8)? = null;
        if (p < line.len) {
            comment = line[p..];
        }

        return SshPubKey {
            options: options,
            key_type_raw: alg,
            key_type: kt,
            key_base64: blob,
            comment: comment,
        };
    }

    /// Check if this key is a hardware-backed FIDO2/SK key.
    pub fn isSecurityKey(): bool {
        return _.key_type == SshKeyType.sk_ed25519 || _.key_type == SshKeyType.sk_ecdsa_nistp256;
    }
}

export struct AuthorizedKeysIter {
    src: []const u8,
    pos: usize,

    pub fn init(content: []const u8): AuthorizedKeysIter {
        return AuthorizedKeysIter { src: content, pos: 0 };
    }

    pub fn next(*_): SshPubKey? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            if (key := SshPubKey.parseLine(_.src[start..end])) {
                return key;
            }
        }
        return null;
    }
}
