// Source: Absorbed from /data/dev/bun/src/http/websocket.cp | Synced: 2026-11-20T05:41:53Z
// lib/ws/handshake.cp
// RFC 6455 WebSocket Opening Handshake (Sec-WebSocket-Accept SHA-1 + Base64, Upgrade Headers)

const std = @import("std");
import { sha1Raw, fillRandom } from "cap:crypto";
import { Base64 } from "cap:encoding";

pub const WS_MAGIC_GUID: []const u8 = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

pub struct WebSocketUpgradeRequest {
    path: []const u8 = "/",
    host: []const u8 = "",
    key: []const u8 = "",
    protocol: []const u8 = "",
    version: []const u8 = "",
}

pub struct WebSocketUpgradeResponse {
    accept: []const u8 = "",
    protocol: []const u8 = "",
}

pub struct WebSocketHandshake {
    static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize! {
        if (pos + s.len > out.len) return err.BufferTooSmall;
        if (s.len > 0) {
            @memcpy(out[pos .. pos + s.len], s);
        }
        return pos + s.len;
    }

    static fn containsIgnoreCaseAscii(haystack: []const u8, needle: []const u8): bool {
        if (needle.len == 0) return true;
        if (haystack.len < needle.len) return false;
        let i: usize = 0;
        while (i + needle.len <= haystack.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(haystack[i .. i + needle.len], needle)) {
                return true;
            }
        }
        return false;
    }

    static fn trimOws(s: []const u8): []const u8 {
        return std.mem.trim(u8, s, " \t\r");
    }

    static fn findHttpHeader(raw_headers: []const u8, target_name: []const u8): ([]const u8)? {
        let lines = std.mem.splitSequence(u8, raw_headers, "\r\n");
        // Skip request/status line
        * = lines.next();
        while (line := lines.next()) {
            if (line.len == 0) break;
            if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                const name = WebSocketHandshake.trimOws(line[0..colon]);
                if (std.ascii.eqlIgnoreCase(name, target_name)) {
                    return WebSocketHandshake.trimOws(line[colon + 1 ..]);
                }
            }
        }
        return null;
    }

    /// Computes RFC 6455 `Sec-WebSocket-Accept = Base64(SHA1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))`.
    /// `out_buf` must be at least 28 bytes. Returns the 28-byte slice.
    pub static fn computeAcceptKey(client_key: []const u8, out_buf: []u8): ([]const u8)! {
        const trimmed = WebSocketHandshake.trimOws(client_key);
        if (trimmed.len == 0) return err.InvalidHandshake;
        if (out_buf.len < 28) return err.BufferTooSmall;

        let combined: [160]u8 = undefined;
        if (trimmed.len + WS_MAGIC_GUID.len > combined.len) {
            return err.InvalidHandshake;
        }
        @memcpy(combined[0..trimmed.len], trimmed);
        @memcpy(combined[trimmed.len .. trimmed.len + WS_MAGIC_GUID.len], WS_MAGIC_GUID);

        let digest: [20]u8 = undefined;
        sha1Raw(&digest, combined[0 .. trimmed.len + WS_MAGIC_GUID.len]);
        const encoded_len = Base64.encode(out_buf, digest[0..20]);
        return out_buf[0..encoded_len];
    }

    /// Generates a cryptographically random 16-byte Base64-encoded `Sec-WebSocket-Key` (24 bytes).
    pub static fn generateClientKey(out_buf: []u8): ([]const u8)! {
        if (out_buf.len < 24) return err.BufferTooSmall;
        let nonce: [16]u8 = undefined;
        fillRandom(&nonce);
        const written = Base64.encode(out_buf, nonce[0..16]);
        return out_buf[0..written];
    }

    /// Formats a standard HTTP/1.1 WebSocket client Upgrade request.
    pub static fn buildClientUpgradeRequest(
        out_buf: []u8,
        host: []const u8,
        path: []const u8,
        client_key: []const u8,
        subprotocol: ([]const u8)?,
    ): ([]const u8)! {
        const req_path = if (path.len > 0) path else "/";
        let pos: usize = 0;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, "GET ")?;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, req_path)?;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, " HTTP/1.1\r\nHost: ")?;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, host)?;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ")?;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, client_key)?;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, "\r\nSec-WebSocket-Version: 13\r\n")?;
        if (proto := subprotocol) {
            if (proto.len > 0) {
                pos = WebSocketHandshake.appendSlice(out_buf, pos, "Sec-WebSocket-Protocol: ")?;
                pos = WebSocketHandshake.appendSlice(out_buf, pos, proto)?;
                pos = WebSocketHandshake.appendSlice(out_buf, pos, "\r\n")?;
            }
        }
        pos = WebSocketHandshake.appendSlice(out_buf, pos, "\r\n")?;
        return out_buf[0..pos];
    }

    /// Formats an HTTP/1.1 `101 Switching Protocols` server handshake response with computed `Sec-WebSocket-Accept`.
    pub static fn buildServerUpgradeResponse(
        out_buf: []u8,
        client_key: []const u8,
        subprotocol: ([]const u8)?,
    ): ([]const u8)! {
        let accept_buf: [32]u8 = undefined;
        const accept = WebSocketHandshake.computeAcceptKey(client_key, &accept_buf)?;

        let pos: usize = 0;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ")?;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, accept)?;
        pos = WebSocketHandshake.appendSlice(out_buf, pos, "\r\n")?;
        if (proto := subprotocol) {
            if (proto.len > 0) {
                pos = WebSocketHandshake.appendSlice(out_buf, pos, "Sec-WebSocket-Protocol: ")?;
                pos = WebSocketHandshake.appendSlice(out_buf, pos, proto)?;
                pos = WebSocketHandshake.appendSlice(out_buf, pos, "\r\n")?;
            }
        }
        pos = WebSocketHandshake.appendSlice(out_buf, pos, "\r\n")?;
        return out_buf[0..pos];
    }

    /// Validates an incoming HTTP/1.1 WebSocket Upgrade request per RFC 6455 Section 4.2.1.
    pub static fn validateClientUpgradeRequest(raw_http: []const u8): WebSocketUpgradeRequest! {
        const hdr_end = std.mem.indexOf(u8, raw_http, "\r\n\r\n") ?? return err.IncompleteHeader;
        const header_block = raw_http[0..hdr_end];

        const first_crlf = std.mem.indexOf(u8, header_block, "\r\n") ?? return err.InvalidHandshake;
        const req_line = header_block[0..first_crlf];
        if (!std.mem.startsWith(u8, req_line, "GET ")) return err.InvalidHandshake;
        const after_method = req_line[4..];
        const sp_idx = std.mem.indexOfScalar(u8, after_method, ' ') ?? return err.InvalidHandshake;
        const path = after_method[0..sp_idx];
        const http_ver = after_method[sp_idx + 1 ..];
        if (!std.mem.startsWith(u8, http_ver, "HTTP/1.1")) return err.InvalidHandshake;

        const upgrade = WebSocketHandshake.findHttpHeader(header_block, "Upgrade") ?? return err.InvalidHandshake;
        if (!std.ascii.eqlIgnoreCase(upgrade, "websocket")) return err.InvalidHandshake;

        const conn = WebSocketHandshake.findHttpHeader(header_block, "Connection") ?? return err.InvalidHandshake;
        if (!WebSocketHandshake.containsIgnoreCaseAscii(conn, "upgrade")) return err.InvalidHandshake;

        const version = WebSocketHandshake.findHttpHeader(header_block, "Sec-WebSocket-Version") ?? return err.InvalidHandshake;
        if (!std.mem.eql(u8, version, "13")) return err.InvalidHandshake;

        const key = WebSocketHandshake.findHttpHeader(header_block, "Sec-WebSocket-Key") ?? return err.InvalidHandshake;
        if (Base64.exactDecodedLen(key) != 16) return err.InvalidHandshake;

        const host = WebSocketHandshake.findHttpHeader(header_block, "Host") ?? "";
        const proto = WebSocketHandshake.findHttpHeader(header_block, "Sec-WebSocket-Protocol") ?? "";

        return WebSocketUpgradeRequest{
            path: path,
            host: host,
            key: key,
            protocol: proto,
            version: version,
        };
    }

    /// Validates an HTTP/1.1 `101 Switching Protocols` server response against `expected_client_key`.
    pub static fn validateServerUpgradeResponse(
        raw_http: []const u8,
        expected_client_key: []const u8,
    ): WebSocketUpgradeResponse! {
        const hdr_end = std.mem.indexOf(u8, raw_http, "\r\n\r\n") ?? return err.IncompleteHeader;
        const header_block = raw_http[0..hdr_end];

        const first_crlf = std.mem.indexOf(u8, header_block, "\r\n") ?? return err.InvalidHandshake;
        const status_line = header_block[0..first_crlf];
        if (!std.mem.startsWith(u8, status_line, "HTTP/1.1 101")) return err.InvalidHandshake;

        const upgrade = WebSocketHandshake.findHttpHeader(header_block, "Upgrade") ?? return err.InvalidHandshake;
        if (!std.ascii.eqlIgnoreCase(upgrade, "websocket")) return err.InvalidHandshake;

        const conn = WebSocketHandshake.findHttpHeader(header_block, "Connection") ?? return err.InvalidHandshake;
        if (!WebSocketHandshake.containsIgnoreCaseAscii(conn, "upgrade")) return err.InvalidHandshake;

        const actual_accept = WebSocketHandshake.findHttpHeader(header_block, "Sec-WebSocket-Accept") ?? return err.InvalidHandshake;
        let expected_buf: [32]u8 = undefined;
        const expected_accept = WebSocketHandshake.computeAcceptKey(expected_client_key, &expected_buf)?;
        if (!std.mem.eql(u8, actual_accept, expected_accept)) {
            return err.InvalidHandshake;
        }

        const proto = WebSocketHandshake.findHttpHeader(header_block, "Sec-WebSocket-Protocol") ?? "";
        return WebSocketUpgradeResponse{
            accept: actual_accept,
            protocol: proto,
        };
    }
}
