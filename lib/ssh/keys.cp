// Source: cp/lib/ssh/keys.cp (OpenSSH Public Key Wire Parser, SHA256 Fingerprint & authorized_keys Engine)
// Pure CP zero-C-dependency OpenSSH public key parser, RFC 4251 wire validator,
// SHA256 fingerprint calculator, and authorized_keys policy parser.

const std = @import("std");
import { sha256Raw } from "cap:crypto";
import { Base64 } from "cap:encoding";

/// Error set for SSH public key parsing, wire validation, and authorized_keys processing.
pub err SshError {
    InvalidPublicKey,
    InvalidBase64,
    WireTruncated,
    AlgorithmMismatch,
    UnsupportedKeyType,
    InvalidAuthorizedKeys,
    OutOfMemory,
}

/// Supported OpenSSH public key algorithm types.
pub enum SshKeyType {
    Ed25519,
    Rsa,
    EcdsaP256,
    SkEd25519,
    Unknown,

    /// Maps an OpenSSH algorithm identifier string to `SshKeyType`.
    pub static fn fromAlgoName(algo: string): SshKeyType {
        if (std.mem.eql(u8, algo, "ssh-ed25519")) return SshKeyType.Ed25519;
        if (std.mem.eql(u8, algo, "ssh-rsa")) return SshKeyType.Rsa;
        if (std.mem.eql(u8, algo, "ecdsa-sha2-nistp256")) return SshKeyType.EcdsaP256;
        if (std.mem.eql(u8, algo, "sk-ssh-ed25519@openssh.com")) return SshKeyType.SkEd25519;
        return SshKeyType.Unknown;
    }

    /// Returns the canonical OpenSSH algorithm identifier string.
    pub fn toAlgoName(): string {
        switch (_) {
            .Ed25519 => return "ssh-ed25519",
            .Rsa => return "ssh-rsa",
            .EcdsaP256 => return "ecdsa-sha2-nistp256",
            .SkEd25519 => return "sk-ssh-ed25519@openssh.com",
            .Unknown => return "unknown",
        }
    }
}

@inline fn isSpaceOrTab(b: u8): bool {
    return b == ' ' || b == '\t';
}

@inline fn isWhitespace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWhitespace(s: string): string {
    let start: usize = 0;
    while (start < s.len && isWhitespace(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && isWhitespace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

@inline fn isStandardBase64Char(b: u8): bool {
    return (b >= 'A' && b <= 'Z') ||
        (b >= 'a' && b <= 'z') ||
        (b >= '0' && b <= '9') ||
        b == '+' ||
        b == '/';
}

fn decodeStrictBase64Alloc(alloc: std.mem.Allocator, b64: string): []u8 ! SshError {
    if (b64.len == 0 || (b64.len % 4) == 1) {
        return SshError.InvalidBase64;
    }

    let pad_seen = false;
    let pad_count: usize = 0;
    let i: usize = 0;
    while (i < b64.len) : (i += 1) {
        const c = b64[i];
        if (c == '=') {
            pad_seen = true;
            pad_count += 1;
            if (pad_count > 2) {
                return SshError.InvalidBase64;
            }
        } else {
            if (pad_seen || !isStandardBase64Char(c)) {
                return SshError.InvalidBase64;
            }
        }
    }

    if (pad_seen && (b64.len % 4) != 0) {
        return SshError.InvalidBase64;
    }

    const max_len = Base64.decodeLen(b64.len);
    let tmp = alloc.alloc(u8, max_len) catch return SshError.OutOfMemory;
    defer alloc.free(tmp);

    const actual_len = Base64.decode(tmp, b64) catch return SshError.InvalidBase64;
    if (actual_len == 0) {
        return SshError.InvalidBase64;
    }

    let out = alloc.alloc(u8, actual_len) catch return SshError.OutOfMemory;
    @memcpy(out, tmp[0..actual_len]);
    return out;
}

fn readWireString(blob: []const u8, offset: *usize): ([]const u8) ! SshError {
    if (offset.* + 4 > blob.len) {
        return SshError.WireTruncated;
    }
    const pos = offset.*;
    const len_u32: u32 = ((blob[pos] as u32) << 24) |
        ((blob[pos + 1] as u32) << 16) |
        ((blob[pos + 2] as u32) << 8) |
        (blob[pos + 3] as u32);
    offset.* = pos + 4;

    const len = usize(len_u32);
    if (len > blob.len - offset.*) {
        return SshError.WireTruncated;
    }
    const slice = blob[offset.* .. offset.* + len];
    offset.* += len;
    return slice;
}

fn validateWireBlob(outer_algo: string, wire_blob: []const u8): SshKeyType ! SshError {
    let offset: usize = 0;
    const inner_algo = readWireString(wire_blob, &offset)?;
    if (!std.mem.eql(u8, inner_algo, outer_algo)) {
        return SshError.AlgorithmMismatch;
    }

    const key_type = SshKeyType.fromAlgoName(outer_algo);
    switch (key_type) {
        .Ed25519 => {
            const pk = readWireString(wire_blob, &offset)?;
            if (pk.len != 32 || offset != wire_blob.len) {
                return SshError.InvalidPublicKey;
            }
        },
        .Rsa => {
            const e = readWireString(wire_blob, &offset)?;
            const n = readWireString(wire_blob, &offset)?;
            if (e.len == 0 || n.len == 0 || offset != wire_blob.len) {
                return SshError.InvalidPublicKey;
            }
        },
        .EcdsaP256 => {
            const curve = readWireString(wire_blob, &offset)?;
            if (!std.mem.eql(u8, curve, "nistp256")) {
                return SshError.InvalidPublicKey;
            }
            const q = readWireString(wire_blob, &offset)?;
            const valid_uncompressed = q.len == 65 && q[0] == 0x04;
            const valid_compressed = q.len == 33 && (q[0] == 0x02 || q[0] == 0x03);
            if ((!valid_uncompressed && !valid_compressed) || offset != wire_blob.len) {
                return SshError.InvalidPublicKey;
            }
        },
        .SkEd25519 => {
            const pk = readWireString(wire_blob, &offset)?;
            const app = readWireString(wire_blob, &offset)?;
            if (pk.len != 32 || app.len == 0 || offset != wire_blob.len) {
                return SshError.InvalidPublicKey;
            }
        },
        .Unknown => {
            return SshError.UnsupportedKeyType;
        },
    }

    return key_type;
}

/// Parsed OpenSSH public key with validated RFC 4251 wire blob.
pub struct SshPublicKey {
    alloc: std.mem.Allocator;
    key_type: SshKeyType;
    algo_name: string;
    wire_blob: []u8;
    comment: string;

    /// Parses a single OpenSSH public key line (`<algo> <base64> [comment]`).
    /// Validates strict Base64, RFC 4251 length-prefixed wire encoding, and algorithm consistency.
    pub static fn parse(alloc: std.mem.Allocator, line: string): SshPublicKey ! SshError {
        const trimmed = trimWhitespace(line);
        if (trimmed.len == 0) {
            return SshError.InvalidPublicKey;
        }

        // 1. Extract outer algorithm name
        let i: usize = 0;
        while (i < trimmed.len && !isSpaceOrTab(trimmed[i])) : (i += 1) {}
        const outer_algo = trimmed[0..i];
        if (outer_algo.len == 0) {
            return SshError.InvalidPublicKey;
        }

        while (i < trimmed.len && isSpaceOrTab(trimmed[i])) : (i += 1) {}
        if (i >= trimmed.len) {
            return SshError.InvalidPublicKey;
        }

        // 2. Extract Base64 payload
        const b64_start = i;
        while (i < trimmed.len && !isSpaceOrTab(trimmed[i])) : (i += 1) {}
        const b64_str = trimmed[b64_start..i];
        if (b64_str.len == 0) {
            return SshError.InvalidPublicKey;
        }

        // 3. Extract optional comment
        while (i < trimmed.len && isSpaceOrTab(trimmed[i])) : (i += 1) {}
        const comment_slice = trimmed[i..];

        // 4. Decode Base64 and validate RFC 4251 wire blob
        const wire_blob = decodeStrictBase64Alloc(alloc, b64_str)?;
        errdefer alloc.free(wire_blob);

        const key_type = validateWireBlob(outer_algo, wire_blob)?;

        const owned_algo = alloc.dupe(u8, outer_algo) catch return SshError.OutOfMemory;
        errdefer alloc.free(owned_algo);

        let owned_comment: string = "";
        if (comment_slice.len > 0) {
            owned_comment = alloc.dupe(u8, comment_slice) catch return SshError.OutOfMemory;
        }

        return SshPublicKey{
            alloc: alloc,
            key_type: key_type,
            algo_name: owned_algo,
            wire_blob: wire_blob,
            comment: owned_comment,
        };
    }

    /// Releases all heap memory owned by this `SshPublicKey`.
    pub fn deinit(*_) {
        if (_.algo_name.len > 0) {
            _.alloc.free(_.algo_name);
        }
        if (_.wire_blob.len > 0) {
            _.alloc.free(_.wire_blob);
        }
        if (_.comment.len > 0) {
            _.alloc.free(_.comment);
        }
    }

    /// Computes the standard OpenSSH SHA-256 fingerprint (`"SHA256:<base64_no_pad>"`),
    /// matching `ssh-keygen -lf` output byte-for-byte.
    pub fn fingerprintSha256(alloc: std.mem.Allocator): []u8 ! SshError {
        let digest: [32]u8 = undefined;
        sha256Raw(&digest, _.wire_blob);

        // 32 bytes in unpadded standard Base64 is always 43 characters; "SHA256:" is 7 bytes.
        let out = alloc.alloc(u8, 50) catch return SshError.OutOfMemory;
        @memcpy(out[0..7], "SHA256:");
        const b64_len = Base64.encodeNoPad(out[7..50], &digest);
        if (b64_len != 43) {
            let exact = alloc.alloc(u8, 7 + b64_len) catch {
                alloc.free(out);
                return SshError.OutOfMemory;
            };
            @memcpy(exact, out[0 .. 7 + b64_len]);
            alloc.free(out);
            return exact;
        }
        return out;
    }

    /// Formats this key in canonical OpenSSH public key format (`<algo> <base64>[ <comment>]`).
    pub fn format(alloc: std.mem.Allocator): []u8 ! SshError {
        const b64_len = Base64.encodeLen(_.wire_blob.len);
        let comment_extra: usize = 0;
        if (_.comment.len > 0) {
            comment_extra = _.comment.len + 1;
        }
        const total_len = _.algo_name.len + 1 + b64_len + comment_extra;

        let out = alloc.alloc(u8, total_len) catch return SshError.OutOfMemory;
        let pos: usize = 0;

        @memcpy(out[pos .. pos + _.algo_name.len], _.algo_name);
        pos += _.algo_name.len;

        out[pos] = ' ';
        pos += 1;

        const actual_b64 = Base64.encode(out[pos .. pos + b64_len], _.wire_blob);
        pos += actual_b64;

        if (_.comment.len > 0) {
            out[pos] = ' ';
            pos += 1;
            @memcpy(out[pos .. pos + _.comment.len], _.comment);
            pos += _.comment.len;
        }

        if (pos != total_len) {
            let exact = alloc.alloc(u8, pos) catch {
                alloc.free(out);
                return SshError.OutOfMemory;
            };
            @memcpy(exact, out[0..pos]);
            alloc.free(out);
            return exact;
        }
        return out;
    }

    /// Compares two public keys by exact byte-for-byte equality of their wire blobs.
    pub fn equals(other: *const SshPublicKey): bool {
        return std.mem.eql(u8, _.wire_blob, other.wire_blob);
    }
}

/// Parsed single entry from an OpenSSH `authorized_keys` file.
pub struct AuthorizedKeyEntry {
    alloc: std.mem.Allocator;
    options_raw: string;
    command: string?;
    from_pattern: string?;
    principals: string?;
    no_port_forwarding: bool;
    no_pty: bool;
    cert_authority: bool;
    public_key: SshPublicKey;

    /// Releases all heap memory owned by this entry and its embedded `SshPublicKey`.
    pub fn deinit(*_) {
        if (_.options_raw.len > 0) {
            _.alloc.free(_.options_raw);
        }
        if (cmd := _.command) {
            _.alloc.free(cmd);
        }
        if (fp := _.from_pattern) {
            _.alloc.free(fp);
        }
        if (pr := _.principals) {
            _.alloc.free(pr);
        }
        _.public_key.deinit();
    }
}

fn unquoteOptionValueAlloc(alloc: std.mem.Allocator, raw: string): string ! SshError {
    if (raw.len >= 2 && raw[0] == '"' && raw[raw.len - 1] == '"') {
        const inner = raw[1 .. raw.len - 1];
        let buf = alloc.alloc(u8, inner.len) catch return SshError.OutOfMemory;
        let out_len: usize = 0;
        let i: usize = 0;
        while (i < inner.len) : (i += 1) {
            if (inner[i] == '\\' && i + 1 < inner.len && (inner[i + 1] == '"' || inner[i + 1] == '\\')) {
                i += 1;
            }
            buf[out_len] = inner[i];
            out_len += 1;
        }
        if (out_len == buf.len) {
            return buf;
        }
        let exact = alloc.alloc(u8, out_len) catch {
            alloc.free(buf);
            return SshError.OutOfMemory;
        };
        @memcpy(exact, buf[0..out_len]);
        alloc.free(buf);
        return exact;
    }
    return alloc.dupe(u8, raw) catch return SshError.OutOfMemory;
}

fn parseAuthorizedOptions(alloc: std.mem.Allocator, entry: *AuthorizedKeyEntry, options_str: string): void ! SshError {
    if (options_str.len == 0) return;

    let start: usize = 0;
    let i: usize = 0;
    let in_quotes = false;
    let escaped = false;

    while (i <= options_str.len) : (i += 1) {
        const at_end = i == options_str.len;
        let is_comma = false;

        if (!at_end) {
            const c = options_str[i];
            if (in_quotes) {
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    in_quotes = false;
                }
            } else {
                if (c == '"') {
                    in_quotes = true;
                } else if (c == ',') {
                    is_comma = true;
                }
            }
        }

        if (at_end || is_comma) {
            const tok = trimWhitespace(options_str[start..i]);
            if (tok.len == 0) {
                return SshError.InvalidAuthorizedKeys;
            }

            let eq_pos: usize? = null;
            let k: usize = 0;
            while (k < tok.len) : (k += 1) {
                if (tok[k] == '=') {
                    eq_pos = k;
                    break;
                }
            }

            if (eq := eq_pos) {
                const opt_name = trimWhitespace(tok[0..eq]);
                const opt_val_raw = trimWhitespace(tok[eq + 1 ..]);
                if (opt_name.len == 0 || opt_val_raw.len == 0) {
                    return SshError.InvalidAuthorizedKeys;
                }
                if (std.mem.eql(u8, opt_name, "command")) {
                    if (prev := entry.command) alloc.free(prev);
                    entry.command = unquoteOptionValueAlloc(alloc, opt_val_raw)?;
                } else if (std.mem.eql(u8, opt_name, "from")) {
                    if (prev := entry.from_pattern) alloc.free(prev);
                    entry.from_pattern = unquoteOptionValueAlloc(alloc, opt_val_raw)?;
                } else if (std.mem.eql(u8, opt_name, "principals")) {
                    if (prev := entry.principals) alloc.free(prev);
                    entry.principals = unquoteOptionValueAlloc(alloc, opt_val_raw)?;
                }
            } else {
                if (std.mem.eql(u8, tok, "no-port-forwarding")) {
                    entry.no_port_forwarding = true;
                } else if (std.mem.eql(u8, tok, "no-pty")) {
                    entry.no_pty = true;
                } else if (std.mem.eql(u8, tok, "cert-authority")) {
                    entry.cert_authority = true;
                } else if (std.mem.eql(u8, tok, "restrict")) {
                    entry.no_port_forwarding = true;
                    entry.no_pty = true;
                }
            }

            start = i + 1;
        }
    }

    if (in_quotes) {
        return SshError.InvalidAuthorizedKeys;
    }
}

fn splitOptionsAndKey(line: string): { options: string, key_part: string } ! SshError {
    let i: usize = 0;
    let in_quotes = false;
    let escaped = false;

    while (i < line.len) : (i += 1) {
        const c = line[i];
        if (in_quotes) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_quotes = false;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (isSpaceOrTab(c)) {
                break;
            }
        }
    }

    if (in_quotes) {
        return SshError.InvalidAuthorizedKeys;
    }

    const first_tok = line[0..i];
    if (SshKeyType.fromAlgoName(first_tok) != SshKeyType.Unknown) {
        return { options: "", key_part: line };
    }

    let j = i;
    while (j < line.len && isSpaceOrTab(line[j])) : (j += 1) {}
    if (j >= line.len) {
        return SshError.InvalidAuthorizedKeys;
    }
    return { options: first_tok, key_part: line[j..] };
}

/// Parser and matcher for OpenSSH `authorized_keys` policy files.
pub static class AuthorizedKeys {
    /// Parses a multi-line `authorized_keys` file, skipping `#` comments and blank lines,
    /// and extracting quoted/escaped options (`command="..."`, `from="..."`, `principals="..."`,
    /// `no-port-forwarding`, `no-pty`, `cert-authority`).
    pub static fn parseFile(alloc: std.mem.Allocator, content: string): []AuthorizedKeyEntry ! SshError {
        let entries: std.ArrayList(AuthorizedKeyEntry) = .empty;
        errdefer {
            let ei: usize = 0;
            while (ei < entries.items.len) : (ei += 1) {
                entries.items[ei].deinit();
            }
            entries.deinit(alloc);
        }

        let line_start: usize = 0;
        let idx: usize = 0;
        while (idx <= content.len) : (idx += 1) {
            if (idx == content.len || content[idx] == '\n') {
                const raw_line = content[line_start..idx];
                line_start = idx + 1;

                const line = trimWhitespace(raw_line);
                if (line.len == 0 || line[0] == '#') {
                    continue;
                }

                const parts = splitOptionsAndKey(line)?;

                let owned_opts: string = "";
                if (parts.options.len > 0) {
                    owned_opts = alloc.dupe(u8, parts.options) catch return SshError.OutOfMemory;
                }
                errdefer if (owned_opts.len > 0) alloc.free(owned_opts);

                let pub_key = SshPublicKey.parse(alloc, parts.key_part)?;

                let entry = AuthorizedKeyEntry{
                    alloc: alloc,
                    options_raw: owned_opts,
                    command: null,
                    from_pattern: null,
                    principals: null,
                    no_port_forwarding: false,
                    no_pty: false,
                    cert_authority: false,
                    public_key: pub_key,
                };

                if (parts.options.len > 0) {
                    parseAuthorizedOptions(alloc, &entry, parts.options) catch e => {
                        entry.deinit();
                        return e;
                    };
                }

                entries.append(alloc, entry) catch {
                    entry.deinit();
                    return SshError.OutOfMemory;
                };
            }
        }

        return entries.toOwnedSlice(alloc) catch return SshError.OutOfMemory;
    }

    /// Releases a slice of `AuthorizedKeyEntry` returned by `parseFile`.
    pub static fn freeEntries(alloc: std.mem.Allocator, entries: []AuthorizedKeyEntry) {
        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            entries[i].deinit();
        }
        alloc.free(entries);
    }

    /// Searches `entries` for the first entry matching `candidate` by wire blob equality.
    pub static fn matchKey(entries: []const AuthorizedKeyEntry, candidate: *const SshPublicKey): usize? {
        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            if (entries[i].public_key.equals(candidate)) {
                return i;
            }
        }
        return null;
    }
}
