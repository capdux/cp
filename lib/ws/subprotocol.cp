// Source: cp/lib/ws/subprotocol.cp (RFC 6455 Subprotocol, Extensions & Close Validation)
// lib/ws/subprotocol.cp
// RFC 6455 Subprotocol Negotiation, Sec-WebSocket-Extensions Parser/Serializer & Close Code Validation

const std = @import("std");
import { Utf8 } from "cap:string";
import { ClosePayload } from "./frame";

pub struct SubprotocolList {
    items: [32][]const u8 = undefined,
    len: usize = 0,

    pub fn count(): usize {
        return _.len;
    }

    pub fn asSlice(*_): []const []const u8 {
        return _.items[0.._.len];
    }

    pub fn get(index: usize): ([]const u8)? {
        if (index >= _.len) return null;
        return _.items[index];
    }
}

pub struct SubprotocolNegotiator {
    static fn trimOws(s: []const u8): []const u8 {
        return std.mem.trim(u8, s, " \t\r\n");
    }

    /// Validates an HTTP token per RFC 7230 §3.2.6 / RFC 6455 §4.1.
    pub static fn isValidToken(token: []const u8): bool {
        if (token.len == 0) return false;
        let i: usize = 0;
        while (i < token.len) : (i += 1) {
            const c = token[i];
            const is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
            const is_digit = c >= '0' && c <= '9';
            const is_sym = c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
                || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
                || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
            if (!is_alpha && !is_digit && !is_sym) {
                return false;
            }
        }
        return true;
    }

    /// Parses a comma-separated `Sec-WebSocket-Protocol` header value into a validated list.
    /// Rejects empty tokens, invalid HTTP token characters, and duplicate entries per RFC 6455 §4.1.
    pub static fn parseOffered(header_value: []const u8): SubprotocolList! {
        let result = SubprotocolList{};
        const trimmed_all = SubprotocolNegotiator.trimOws(header_value);
        if (trimmed_all.len == 0) {
            return result;
        }

        let iter = std.mem.splitScalar(u8, trimmed_all, ',');
        while (raw_part := iter.next()) {
            const token = SubprotocolNegotiator.trimOws(raw_part);
            if (!SubprotocolNegotiator.isValidToken(token)) {
                return err.InvalidSubprotocol;
            }
            let j: usize = 0;
            while (j < result.len) : (j += 1) {
                if (std.mem.eql(u8, result.items[j], token)) {
                    return err.DuplicateSubprotocol;
                }
            }
            if (result.len >= 32) {
                return err.TooManySubprotocols;
            }
            result.items[result.len] = token;
            result.len += 1;
        }
        return result;
    }

    /// Selects the first offered subprotocol that matches a supported subprotocol.
    /// Uses exact case-sensitive matching per RFC 6455 §11.3.4.
    pub static fn select(offered: []const []const u8, supported: []const []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < offered.len) : (i += 1) {
            const candidate = offered[i];
            if (!SubprotocolNegotiator.isValidToken(candidate)) {
                continue;
            }
            let j: usize = 0;
            while (j < supported.len) : (j += 1) {
                if (std.mem.eql(u8, candidate, supported[j])) {
                    return supported[j];
                }
            }
        }
        return null;
    }

    /// Parses `Sec-WebSocket-Protocol` header and selects the highest-priority supported protocol.
    pub static fn selectFromHeader(header_value: []const u8, supported: []const []const u8): (([]const u8)?)! {
        let list = SubprotocolNegotiator.parseOffered(header_value)?;
        return SubprotocolNegotiator.select(list.asSlice(), supported);
    }
}

pub struct WsExtensionParam {
    name: []const u8 = "",
    value: ([]const u8)? = null,
}

pub struct WsExtensionHeader {
    name: []const u8 = "",
    params: [16]WsExtensionParam = undefined,
    param_count: usize = 0,

    static fn trimOws(s: []const u8): []const u8 {
        return std.mem.trim(u8, s, " \t\r\n");
    }

    static fn unquoteParamValue(s: []const u8): ([]const u8)! {
        if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
            const inner = s[1 .. s.len - 1];
            if (inner.len == 0) return err.InvalidExtensionHeader;
            return inner;
        }
        if (!SubprotocolNegotiator.isValidToken(s)) {
            return err.InvalidExtensionHeader;
        }
        return s;
    }

    static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize! {
        if (pos + s.len > out.len) return err.BufferTooSmall;
        if (s.len > 0) {
            @memcpy(out[pos .. pos + s.len], s);
        }
        return pos + s.len;
    }

    pub static fn init(name: []const u8): WsExtensionHeader {
        return WsExtensionHeader{
            name: name,
            params: undefined,
            param_count: 0,
        };
    }

    pub fn addParam(*_, name: []const u8, value: ([]const u8)?): ! {
        if (!SubprotocolNegotiator.isValidToken(name)) {
            return err.InvalidExtensionParam;
        }
        if (_.param_count >= 16) {
            return err.TooManyExtensionParams;
        }
        _.params[_.param_count] = WsExtensionParam{
            name: name,
            value: value,
        };
        _.param_count += 1;
    }

    pub fn hasParam(name: []const u8): bool {
        let i: usize = 0;
        while (i < _.param_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.params[i].name, name)) {
                return true;
            }
        }
        return false;
    }

    pub fn getParam(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.param_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.params[i].name, name)) {
                return _.params[i].value;
            }
        }
        return null;
    }

    pub fn getParamU8(name: []const u8): u8? {
        const val = _.getParam(name) ?? return null;
        if (val.len == 0) return null;
        return std.fmt.parseInt(u8, val, 10) catch null;
    }

    pub fn paramSlice(*_): []const WsExtensionParam {
        return _.params[0.._.param_count];
    }

    /// Parses a single extension element such as
    /// `"permessage-deflate; client_max_window_bits=15; server_no_context_takeover"`.
    pub static fn parse(single_ext: []const u8): WsExtensionHeader! {
        const trimmed = WsExtensionHeader.trimOws(single_ext);
        if (trimmed.len == 0) return err.InvalidExtensionHeader;

        let segs = std.mem.splitScalar(u8, trimmed, ';');
        const raw_name = segs.next() ?? return err.InvalidExtensionHeader;
        const ext_name = WsExtensionHeader.trimOws(raw_name);
        if (!SubprotocolNegotiator.isValidToken(ext_name)) {
            return err.InvalidExtensionHeader;
        }

        let header = WsExtensionHeader.init(ext_name);
        while (raw_param := segs.next()) {
            const p = WsExtensionHeader.trimOws(raw_param);
            if (p.len == 0) return err.InvalidExtensionHeader;
            if (eq_idx := std.mem.indexOfScalar(u8, p, '=')) {
                const pname = WsExtensionHeader.trimOws(p[0..eq_idx]);
                const raw_pval = WsExtensionHeader.trimOws(p[eq_idx + 1 ..]);
                if (!SubprotocolNegotiator.isValidToken(pname) || raw_pval.len == 0) {
                    return err.InvalidExtensionHeader;
                }
                const pval = WsExtensionHeader.unquoteParamValue(raw_pval)?;
                header.addParam(pname, pval)?;
            } else {
                if (!SubprotocolNegotiator.isValidToken(p)) {
                    return err.InvalidExtensionHeader;
                }
                header.addParam(p, null)?;
            }
        }
        return header;
    }

    /// Parses a comma-separated `Sec-WebSocket-Extensions` header list (up to 16 extensions).
    pub static fn parseAll(header_value: []const u8): WsExtensionList! {
        let list = WsExtensionList{};
        const trimmed = WsExtensionHeader.trimOws(header_value);
        if (trimmed.len == 0) return list;

        let start: usize = 0;
        let i: usize = 0;
        let in_quotes = false;
        while (i <= trimmed.len) : (i += 1) {
            if (i < trimmed.len && trimmed[i] == '"') {
                in_quotes = !in_quotes;
                continue;
            }
            if (i == trimmed.len || (!in_quotes && trimmed[i] == ',')) {
                const segment = WsExtensionHeader.trimOws(trimmed[start..i]);
                const parsed = WsExtensionHeader.parse(segment)?;
                if (list.len >= 16) return err.TooManyExtensions;
                list.items[list.len] = parsed;
                list.len += 1;
                start = i + 1;
            }
        }
        if (in_quotes) return err.InvalidExtensionHeader;
        return list;
    }

    /// Serializes the extension header and its parameters into `out`.
    pub fn serialize(*_, out: []u8): ([]const u8)! {
        if (!SubprotocolNegotiator.isValidToken(_.name)) {
            return err.InvalidExtensionHeader;
        }
        let pos: usize = 0;
        pos = WsExtensionHeader.appendSlice(out, pos, _.name)?;
        let i: usize = 0;
        while (i < _.param_count) : (i += 1) {
            const p = _.params[i];
            pos = WsExtensionHeader.appendSlice(out, pos, "; ")?;
            pos = WsExtensionHeader.appendSlice(out, pos, p.name)?;
            if (v := p.value) {
                pos = WsExtensionHeader.appendSlice(out, pos, "=")?;
                pos = WsExtensionHeader.appendSlice(out, pos, v)?;
            }
        }
        return out[0..pos];
    }
}

pub struct WsExtensionList {
    items: [16]WsExtensionHeader = undefined,
    len: usize = 0,

    pub fn count(): usize {
        return _.len;
    }

    pub fn asSlice(*_): []const WsExtensionHeader {
        return _.items[0.._.len];
    }

    pub fn get(index: usize): WsExtensionHeader? {
        if (index >= _.len) return null;
        return _.items[index];
    }
}

pub struct CloseCodeValidator {
    /// Strictly validates RFC 6455 §7.4.1 wire-transmissible WebSocket close status codes.
    /// Rejects 0-999, 1004, 1005, 1006, 1015, 1016-2999, and >= 5000.
    /// Accepts 1000-1003, 1007-1014, and 3000-4999.
    pub static fn isValidWireCloseCode(code: u16): bool {
        if (code >= 1000 && code <= 1003) return true;
        if (code >= 1007 && code <= 1014) return true;
        if (code >= 3000 && code <= 4999) return true;
        return false;
    }

    /// Validates that close reason bytes are well-formed UTF-8 per RFC 6455 §5.5.1.
    pub static fn isValidUtf8(bytes: []const u8): bool {
        return Utf8.isValid(bytes);
    }

    /// Validates an entire Close frame payload (empty or 2-byte big-endian wire code + <=123-byte UTF-8 reason).
    pub static fn isValidClosePayload(payload: []const u8): bool {
        if (payload.len > 125) return false;
        if (payload.len == 0) return true;
        if (payload.len == 1) return false;
        const code = std.mem.readInt(u16, payload[0..2], .big);
        if (!CloseCodeValidator.isValidWireCloseCode(code)) return false;
        return CloseCodeValidator.isValidUtf8(payload[2..]);
    }

    /// Parses and validates an RFC 6455 Close frame payload using `CloseCodeValidator`.
    pub static fn parseClosePayload(payload: []const u8): ClosePayload! {
        if (payload.len > 125) {
            return err.ControlFrameTooLarge;
        }
        if (payload.len == 0) {
            return ClosePayload{ code: 1005, reason: "" };
        }
        if (payload.len == 1) {
            return err.InvalidClosePayload;
        }
        const code = std.mem.readInt(u16, payload[0..2], .big);
        if (!CloseCodeValidator.isValidWireCloseCode(code)) {
            return err.InvalidClosePayload;
        }
        const reason = payload[2..];
        if (reason.len > 0 && !CloseCodeValidator.isValidUtf8(reason)) {
            return err.InvalidUtf8;
        }
        return ClosePayload{ code: code, reason: reason };
    }
}
