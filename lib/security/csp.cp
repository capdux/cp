// Source: cp/lib/security/csp.cp (Content-Security-Policy & Security Headers)
// lib/security/csp.cp
// Chainable Content-Security-Policy Builder with CRLF Injection Guard & OWASP Security Header Presets

const std = @import("std");

pub const MAX_CSP_DIRECTIVES: usize = 16;
pub const MAX_CSP_VALUE_LEN: usize = 512;

pub struct SecurityHeader {
    name: []const u8 = "";
    value: []const u8 = "";
}

pub struct SecurityHeaderSet {
    items: [8]SecurityHeader = undefined;
    len: usize = 0;

    fn add(*_, name: []const u8, value: []const u8) {
        if (_.len >= 8) return;
        _.items[_.len] = SecurityHeader{ name: name, value: value };
        _.len += 1;
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): SecurityHeader? {
        if (idx >= _.len) return null;
        return _.items[idx];
    }

    fn get(name: []const u8): ([]const u8)? {
        for (i of 0.._.len) {
            if (std.ascii.eqlIgnoreCase(_.items[i].name, name)) {
                return _.items[i].value;
            }
        }
        return null;
    }
}

pub struct SecurityHeadersPreset {
    /// Strict production API response headers (JSON/REST/gRPC-Web APIs).
    static fn strictApi(): SecurityHeaderSet {
        let set = SecurityHeaderSet{};
        set.add("Strict-Transport-Security", "max-age=63072000; includeSubDomains; preload");
        set.add("X-Content-Type-Options", "nosniff");
        set.add("X-Frame-Options", "DENY");
        set.add("Referrer-Policy", "no-referrer");
        set.add("Permissions-Policy", "geolocation=(), microphone=(), camera=(), payment=(), usb=()");
        set.add("Content-Security-Policy", "default-src 'none'; frame-ancestors 'none'");
        return set;
    }

    /// Strict production Web HTML response headers.
    static fn strictWeb(): SecurityHeaderSet {
        let set = SecurityHeaderSet{};
        set.add("Strict-Transport-Security", "max-age=63072000; includeSubDomains; preload");
        set.add("X-Content-Type-Options", "nosniff");
        set.add("X-Frame-Options", "DENY");
        set.add("Referrer-Policy", "strict-origin-when-cross-origin");
        set.add("Permissions-Policy", "geolocation=(), microphone=(), camera=(), payment=()");
        set.add("Content-Security-Policy", "default-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none'");
        return set;
    }
}

struct CspDirective {
    name_buf: [64]u8 = undefined;
    name_len: usize = 0;
    val_buf: [MAX_CSP_VALUE_LEN]u8 = undefined;
    val_len: usize = 0;
}

fn isValidDirectiveName(name: []const u8): bool {
    if (name.len == 0 || name.len > 64) return false;
    for (i of 0..name.len) {
        const c = name[i];
        const is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const is_digit = (c >= '0' && c <= '9');
        if (!is_alpha && !is_digit && c != '-') return false;
    }
    return true;
}

fn isSafeCspValue(val: []const u8): bool {
    if (val.len == 0) return false;
    for (i of 0..val.len) {
        const c = val[i];
        // Reject CR, LF, NUL, and ';' (prevents header/directive smuggling)
        if (c == '\r' || c == '\n' || c == 0 || c == ';') return false;
    }
    return true;
}

fn isSafeTokenAtom(atom: []const u8): bool {
    if (atom.len == 0) return false;
    for (i of 0..atom.len) {
        const c = atom[i];
        const is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const is_digit = (c >= '0' && c <= '9');
        const is_b64 = c == '+' || c == '/' || c == '=' || c == '-' || c == '_';
        if (!is_alpha && !is_digit && !is_b64) return false;
    }
    return true;
}

pub struct CspBuilder {
    directives: [MAX_CSP_DIRECTIVES]CspDirective = undefined;
    count: usize = 0;
    has_invalid_input: bool = false;

    static fn init(): CspBuilder {
        return CspBuilder{
            directives: undefined,
            count: 0,
            has_invalid_input: false,
        };
    }

    fn isValid(): bool {
        return !_.has_invalid_input;
    }

    fn findDirectiveIndex(name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.count) {
            const d_name = _.directives[i].name_buf[0.._.directives[i].name_len];
            if (std.ascii.eqlIgnoreCase(d_name, name)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn setOrAppend(*_, name: []const u8, value: []const u8, append_mode: bool): *CspBuilder {
        if (!isValidDirectiveName(name) || !isSafeCspValue(value)) {
            _.has_invalid_input = true;
            return _;
        }

        if (idx := _.findDirectiveIndex(name)) {
            if (!append_mode) {
                if (value.len > MAX_CSP_VALUE_LEN) {
                    _.has_invalid_input = true;
                    return _;
                }
                @memcpy(_.directives[idx].val_buf[0..value.len], value);
                _.directives[idx].val_len = value.len;
                return _;
            }

            const cur_len = _.directives[idx].val_len;
            const sep_len: usize = if (cur_len > 0) 1 else 0;
            if (cur_len + sep_len + value.len > MAX_CSP_VALUE_LEN) {
                _.has_invalid_input = true;
                return _;
            }
            if (sep_len == 1) {
                _.directives[idx].val_buf[cur_len] = ' ';
            }
            @memcpy(_.directives[idx].val_buf[cur_len + sep_len .. cur_len + sep_len + value.len], value);
            _.directives[idx].val_len = cur_len + sep_len + value.len;
            return _;
        }

        if (_.count >= MAX_CSP_DIRECTIVES || value.len > MAX_CSP_VALUE_LEN) {
            _.has_invalid_input = true;
            return _;
        }

        let entry = CspDirective{};
        for (i of 0..name.len) {
            entry.name_buf[i] = std.ascii.toLower(name[i]);
        }
        entry.name_len = name.len;
        @memcpy(entry.val_buf[0..value.len], value);
        entry.val_len = value.len;

        _.directives[_.count] = entry;
        _.count += 1;
        return _;
    }

    fn directive(*_, name: []const u8, value: []const u8): *CspBuilder {
        return _.setOrAppend(name, value, false);
    }

    fn defaultSrc(*_, value: []const u8): *CspBuilder {
        return _.setOrAppend("default-src", value, false);
    }

    fn scriptSrc(*_, value: []const u8): *CspBuilder {
        return _.setOrAppend("script-src", value, false);
    }

    fn styleSrc(*_, value: []const u8): *CspBuilder {
        return _.setOrAppend("style-src", value, false);
    }

    fn imgSrc(*_, value: []const u8): *CspBuilder {
        return _.setOrAppend("img-src", value, false);
    }

    fn connectSrc(*_, value: []const u8): *CspBuilder {
        return _.setOrAppend("connect-src", value, false);
    }

    fn frameAncestors(*_, value: []const u8): *CspBuilder {
        return _.setOrAppend("frame-ancestors", value, false);
    }

    fn addNonce(*_, directive_name: []const u8, nonce: []const u8): *CspBuilder {
        if (!isSafeTokenAtom(nonce)) {
            _.has_invalid_input = true;
            return _;
        }
        let token_buf: [256]u8 = undefined;
        const prefix = "'nonce-";
        if (prefix.len + nonce.len + 1 > token_buf.len) {
            _.has_invalid_input = true;
            return _;
        }
        @memcpy(token_buf[0..prefix.len], prefix);
        @memcpy(token_buf[prefix.len .. prefix.len + nonce.len], nonce);
        token_buf[prefix.len + nonce.len] = '\'';
        const full_token = token_buf[0 .. prefix.len + nonce.len + 1];
        return _.setOrAppend(directive_name, full_token, true);
    }

    fn addSha256(*_, directive_name: []const u8, b64_hash: []const u8): *CspBuilder {
        if (!isSafeTokenAtom(b64_hash)) {
            _.has_invalid_input = true;
            return _;
        }
        let token_buf: [256]u8 = undefined;
        const prefix = "'sha256-";
        if (prefix.len + b64_hash.len + 1 > token_buf.len) {
            _.has_invalid_input = true;
            return _;
        }
        @memcpy(token_buf[0..prefix.len], prefix);
        @memcpy(token_buf[prefix.len .. prefix.len + b64_hash.len], b64_hash);
        token_buf[prefix.len + b64_hash.len] = '\'';
        const full_token = token_buf[0 .. prefix.len + b64_hash.len + 1];
        return _.setOrAppend(directive_name, full_token, true);
    }

    /// Serializes the configured directives into a standard `; `-delimited CSP header value.
    fn build(alloc: std.mem.Allocator): ([]u8)! {
        if (_.has_invalid_input) {
            return err.UnsafeCspDirective;
        }
        if (_.count == 0) {
            return alloc.alloc(u8, 0)?;
        }

        let total_len: usize = 0;
        for (i of 0.._.count) {
            if (i > 0) total_len += 2; // "; "
            total_len += _.directives[i].name_len + 1 + _.directives[i].val_len;
        }

        let out = alloc.alloc(u8, total_len)?;
        let pos: usize = 0;
        for (i of 0.._.count) {
            if (i > 0) {
                out[pos] = ';';
                out[pos + 1] = ' ';
                pos += 2;
            }
            const d_name = _.directives[i].name_buf[0.._.directives[i].name_len];
            const d_val = _.directives[i].val_buf[0.._.directives[i].val_len];
            @memcpy(out[pos .. pos + d_name.len], d_name);
            pos += d_name.len;
            out[pos] = ' ';
            pos += 1;
            @memcpy(out[pos .. pos + d_val.len], d_val);
            pos += d_val.len;
        }

        return out;
    }
}
