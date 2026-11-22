// Source: Absorbed from /data/dev/bun/src/runtime/api/bun/x509.cp | Synced: 2026-11-22T20:16:34Z (X.509 Subject Alternative Name safety validator isSafeAltName, RFC 5280 SAN entry formatter/parser & quoted escape serializer)
// Pure-CP zero-allocation X.509 Subject Alternative Name (SAN) validator, parser, and injection-safe serializer.

/// Maximum SAN entries recommended in fixed-size batch helpers.
pub const X509_ALT_NAME_MAX_ENTRIES: usize = 32;

/// RFC 5280 / OpenSSL Subject Alternative Name entry kind.
pub enum X509AltNameKind: u8 {
    Dns = 0,
    Ip = 1,
    Email = 2,
    Uri = 3,
    DirName = 4,
    RegisteredId = 5,

    /// Returns the OpenSSL / Node.js `subjectaltname` prefix string for this kind.
    fn prefix(): []const u8 {
        return switch (_) {
            .Dns => "DNS:",
            .Ip => "IP Address:",
            .Email => "email:",
            .Uri => "URI:",
            .DirName => "DirName:",
            .RegisteredId => "Registered ID:",
        };
    }
}

/// Error set for X.509 SAN validation, parsing, and formatting.
pub err X509AltNameError {
    BufferTooSmall,
    UnsafeAltName,
    InvalidEntry,
    UnknownPrefix,
}

/// Validates whether an X.509 Subject Alternative Name value is safe to emit unquoted
/// in a comma-separated `subjectaltname` string (exact match with Bun `x509.cp:isSafeAltName`).
/// Rejects `"`, `\`, `,`, `'`, ASCII control chars `< 32` and `127`; when `utf8 == false`, also rejects `> 126`.
pub fn isSafeAltName(name: []const u8, utf8: bool): bool {
    let i: usize = 0;
    while (i < name.len) {
        const c = name[i];
        if (c == '"' || c == '\\' || c == ',' || c == '\'') {
            return false;
        }
        if (utf8) {
            if (c < 32 || c == 127) return false;
        } else {
            if (c < 32 || c > 126) return false;
        }
        i += 1;
    }
    return true;
}

fn hasPrefix(s: []const u8, p: []const u8): bool {
    if (s.len < p.len) return false;
    let i: usize = 0;
    while (i < p.len) {
        if (s[i] != p[i]) return false;
        i += 1;
    }
    return true;
}

fn appendSlice(out: []u8, pos: *usize, src: []const u8): usize ! X509AltNameError {
    if (pos.* + src.len > out.len) return X509AltNameError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
    return src.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): usize ! X509AltNameError {
    if (pos.* + 1 > out.len) return X509AltNameError.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
    return 1;
}

fn hexDigit(n: u8): u8 {
    const nib = n & 0x0F;
    if (nib < 10) return '0' + nib;
    return 'A' + (nib - 10);
}

/// A single X.509 Subject Alternative Name entry.
pub struct X509AltNameEntry {
    kind: X509AltNameKind = X509AltNameKind.Dns,
    value: []const u8 = "",
    utf8: bool = false,

    static fn init(kind: X509AltNameKind, value: []const u8): X509AltNameEntry {
        return X509AltNameEntry{
            kind: kind,
            value: value,
            utf8: false,
        };
    }

    static fn initUtf8(kind: X509AltNameKind, value: []const u8): X509AltNameEntry {
        return X509AltNameEntry{
            kind: kind,
            value: value,
            utf8: true,
        };
    }

    fn isSafe(): bool {
        return isSafeAltName(_.value, _.utf8);
    }

    /// Formats this entry strictly: rejects unsafe values with `UnsafeAltName`.
    fn formatStrictInto(out: []u8): usize ! X509AltNameError {
        if (!_.isSafe()) return X509AltNameError.UnsafeAltName;
        let pos: usize = 0;
        * = appendSlice(out, &pos, _.kind.prefix())?;
        * = appendSlice(out, &pos, _.value)?;
        return pos;
    }

    /// Formats this entry into `out`. Safe values are emitted verbatim (`DNS:example.com`);
    /// values containing delimiter/quote/control characters are JSON-style quoted & escaped
    /// to prevent comma-separated SAN spoofing (`DNS:"bad,DNS:evil.com"`).
    fn formatInto(out: []u8): usize ! X509AltNameError {
        let pos: usize = 0;
        * = appendSlice(out, &pos, _.kind.prefix())?;
        if (_.isSafe()) {
            * = appendSlice(out, &pos, _.value)?;
            return pos;
        }
        * = appendByte(out, &pos, '"')?;
        let i: usize = 0;
        while (i < _.value.len) {
            const c = _.value[i];
            if (c == '"' || c == '\\') {
                * = appendByte(out, &pos, '\\')?;
                * = appendByte(out, &pos, c)?;
            } else if (c < 32 || c == 127 || (!_.utf8 && c > 126)) {
                * = appendSlice(out, &pos, "\\x")?;
                * = appendByte(out, &pos, hexDigit(c >> 4))?;
                * = appendByte(out, &pos, hexDigit(c))?;
            } else {
                * = appendByte(out, &pos, c)?;
            }
            i += 1;
        }
        * = appendByte(out, &pos, '"')?;
        return pos;
    }
}

/// Parses a single OpenSSL/Node `subjectaltname` entry (e.g. `"DNS:api.example.com"`, `"IP Address:10.0.0.1"`).
pub fn parseX509AltNameEntry(raw: []const u8): X509AltNameEntry ! X509AltNameError {
    if (raw.len == 0) return X509AltNameError.InvalidEntry;
    if (hasPrefix(raw, "DNS:")) {
        return X509AltNameEntry.init(X509AltNameKind.Dns, raw[4..]);
    }
    if (hasPrefix(raw, "IP Address:")) {
        return X509AltNameEntry.init(X509AltNameKind.Ip, raw[11..]);
    }
    if (hasPrefix(raw, "email:")) {
        return X509AltNameEntry.init(X509AltNameKind.Email, raw[6..]);
    }
    if (hasPrefix(raw, "URI:")) {
        return X509AltNameEntry.init(X509AltNameKind.Uri, raw[4..]);
    }
    if (hasPrefix(raw, "DirName:")) {
        return X509AltNameEntry.init(X509AltNameKind.DirName, raw[8..]);
    }
    if (hasPrefix(raw, "Registered ID:")) {
        return X509AltNameEntry.init(X509AltNameKind.RegisteredId, raw[14..]);
    }
    return X509AltNameError.UnknownPrefix;
}

/// Formats a list of `X509AltNameEntry` items into a `", "`-separated `subjectaltname` string.
pub fn formatX509AltNameList(entries: []const X509AltNameEntry, out: []u8): usize ! X509AltNameError {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < entries.len) {
        if (i > 0) {
            * = appendSlice(out, &pos, ", ")?;
        }
        const n = entries[i].formatInto(out[pos..])?;
        pos += n;
        i += 1;
    }
    return pos;
}

/// Counts how many entries in `entries` pass `isSafeAltName`.
pub fn countSafeX509AltNames(entries: []const X509AltNameEntry): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < entries.len) {
        if (entries[i].isSafe()) {
            count += 1;
        }
        i += 1;
    }
    return count;
}
