// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T20:23:41Z
// lib/http/wire_http.cp
// Industrial HTTP/1.1 HttpRequestBuilder & HttpResponseParser:
// - Multi-line (obs-fold) & multi-value header parsing
// - 100-Continue intermediate response handling & Expect: 100-continue negotiation
// - Transfer-Encoding: chunked + trailers integration
// - Automatic Content-Encoding (gzip/deflate) decompression via cap:pack
// - Connection pool & RedirectPolicy lifecycle integration

const std = @import("std");
import { Method } from "./method";
import { Headers, HeaderEntry } from "./headers";
import { Encoding, ContentEncodingNegotiator } from "./encoding";
import { Decompressor } from "./decompressor";
import { ChunkedDecoder, ChunkTrailer, MAX_CHUNK_TRAILERS } from "./chunked";
import { HeaderBuilder } from "./header_builder";
import { HTTPStatus } from "./status";
import { RedirectPolicy, RedirectAction, RedirectDecision, ParsedHttpOrigin } from "./redirect";
import { HttpConnectionPool } from "./context";
import { FetchRedirect } from "./fetch_modes";

pub const MAX_WIRE_HEADERS: usize = 64;

pub struct HttpRequestBuilder {
    pub method: Method = Method.GET;
    pub url: []const u8 = "";
    pub scheme: []const u8 = "http";
    pub host: []const u8 = "";
    pub port: u16 = 80;
    pub path: []const u8 = "/";
    pub headers: Headers = undefined;
    pub body: []const u8 = "";
    pub keep_alive: bool = true;
    pub expect_100_continue: bool = false;
    pub accept_encoding: []const u8 = "gzip, deflate";
    pub user_agent: []const u8 = "CP-HTTP/1.1";

    pub static fn init(allocator: std.mem.Allocator, method: Method, url: []const u8): HttpRequestBuilder {
        let req = HttpRequestBuilder{
            method: method,
            url: url,
            headers: Headers.init(allocator),
        };
        if (orig := ParsedHttpOrigin.parse(url)) {
            req.scheme = orig.scheme;
            req.host = orig.host;
            req.port = orig.port;
            req.path = orig.path;
        }
        return req;
    }

    pub fn deinit(*_) {
        _.headers.deinit();
    }

    pub fn setHeader(*_, name: []const u8, value: []const u8) {
        _.headers.set(name, value);
    }

    pub fn appendHeader(*_, name: []const u8, value: []const u8) {
        _.headers.append(name, value);
    }

    pub fn setBody(*_, body: []const u8) {
        _.body = body;
    }

    pub fn enableExpectContinue(*_, enabled: bool) {
        _.expect_100_continue = enabled;
    }

    /// Applies redirect decision to this request builder (updates method, URL, and strips cross-origin auth / body headers).
    pub fn applyRedirect(*_, decision: RedirectDecision): bool {
        if (decision.action != RedirectAction.Follow) return false;
        _.method = decision.next_method;
        _.url = decision.next_url;
        if (orig := ParsedHttpOrigin.parse(decision.next_url)) {
            _.scheme = orig.scheme;
            _.host = orig.host;
            _.port = orig.port;
            _.path = orig.path;
        }
        if (!decision.keep_body) {
            _.body = "";
        }
        RedirectPolicy.applyHeaderPolicy(&_.headers, decision);
        return true;
    }

    /// Formats the head (request line + headers + `\r\n`). If `expect_100_continue` is enabled and body is non-empty,
    /// emits `Expect: 100-continue` and withholds the body until `100 Continue` is received.
    pub fn buildHead(out_buf: []u8): ([]const u8)! {
        let hb = HeaderBuilder.init(out_buf);
        if (!hb.writeRequestLine(_.method.toString(), _.path)) return error.BufferTooSmall;

        const default_p: u16 = if (std.ascii.eqlIgnoreCase(_.scheme, "https")) 443 else 80;
        let host_buf: [256]u8 = undefined;
        const host_val = if (_.port == default_p)
            _.host
        else
            (std.fmt.bufPrint(&host_buf, "{s}:{d}", .{ _.host, _.port }) catch return error.BufferTooSmall);

        if (!_.headers.has("Host") && host_val.len > 0) {
            if (!hb.append("Host", host_val)) return error.BufferTooSmall;
        }
        if (!_.headers.has("User-Agent") && _.user_agent.len > 0) {
            if (!hb.append("User-Agent", _.user_agent)) return error.BufferTooSmall;
        }
        if (!_.headers.has("Accept-Encoding") && _.accept_encoding.len > 0) {
            if (!hb.append("Accept-Encoding", _.accept_encoding)) return error.BufferTooSmall;
        }
        if (!_.headers.has("Connection")) {
            const conn_val = if (_.keep_alive) "keep-alive" else "close";
            if (!hb.append("Connection", conn_val)) return error.BufferTooSmall;
        }

        if (!hb.appendHeaders(_.headers)) return error.BufferTooSmall;

        if (_.body.len > 0 || _.method.hasRequestBody()) {
            if (!_.headers.has("Content-Length") && !_.headers.has("Transfer-Encoding")) {
                if (!hb.appendInt("Content-Length", _.body.len as i64)) return error.BufferTooSmall;
            }
        }
        if (_.expect_100_continue && _.body.len > 0 && !_.headers.has("Expect")) {
            if (!hb.append("Expect", "100-continue")) return error.BufferTooSmall;
        }

        return try hb.finish();
    }

    /// Formats the complete HTTP/1.1 request wire bytes (head + body when not awaiting `100-continue`).
    pub fn buildWire(out_buf: []u8): ([]const u8)! {
        const head = try _.buildHead(out_buf);
        if (_.expect_100_continue || _.body.len == 0) {
            return head;
        }
        if (head.len + _.body.len > out_buf.len) return error.BufferTooSmall;
        @memcpy(out_buf[head.len .. head.len + _.body.len], _.body);
        return out_buf[0 .. head.len + _.body.len];
    }
}

pub enum HttpResponseStage: u8 {
    StatusLine = 0,
    Informational100 = 1,
    Headers = 2,
    Body = 3,
    Done = 4,
    Error = 5,
}

pub struct ParsedHttpResponse {
    pub status: u16 = 0;
    pub reason: []const u8 = "";
    pub minor_version: u8 = 1;
    pub headers: Headers = undefined;
    pub raw_body: []const u8 = "";
    pub body: []const u8 = "";
    pub trailers: [MAX_CHUNK_TRAILERS]ChunkTrailer = undefined;
    pub trailer_count: usize = 0;
    pub is_chunked: bool = false;
    pub content_encoding: Encoding = Encoding.Identity;
    pub content_length: usize? = null;
    pub keep_alive: bool = true;
    pub saw_100_continue: bool = false;
    pub allocator: std.mem.Allocator = undefined;
    pub owns_body: bool = false;

    pub fn deinit(*_) {
        _.headers.deinit();
        if (_.owns_body && _.body.len > 0) {
            _.allocator.free(_.body);
            _.body = "";
            _.owns_body = false;
        }
    }

    pub fn getHeader(name: []const u8): ([]const u8)? {
        return _.headers.get(name);
    }

    pub fn getHeaderAll(name: []const u8, out: [][]const u8): usize {
        return _.headers.getAll(name, out);
    }

    pub fn getTrailer(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.trailer_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.trailers[i].name, name)) {
                return _.trailers[i].value;
            }
        }
        return null;
    }

    pub fn isRedirect(): bool {
        return RedirectPolicy.isRedirectStatus(_.status);
    }
}

fn asciiContainsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

pub struct HttpResponseParser {
    /// Parses a raw HTTP/1.1 response (including optional leading `HTTP/1.1 100 Continue`, multi-line obs-fold headers,
    /// multi-value headers, `Transfer-Encoding: chunked` + trailers, and automatic `Content-Encoding: gzip/deflate` decompression).
    pub static fn parse(allocator: std.mem.Allocator, wire: []const u8, auto_decompress: bool = true): ParsedHttpResponse! {
        let pos: usize = 0;
        let saw_100 = false;

        // Loop past any 1xx informational responses (e.g. HTTP/1.1 100 Continue\r\n\r\n)
        while (true) {
            if (pos >= wire.len) return error.IncompleteHttpResponse;
            const head_end_rel = std.mem.indexOf(u8, wire[pos..], "\r\n\r\n") orelse return error.IncompleteHttpResponse;
            const head_slice = wire[pos .. pos + head_end_rel];
            const first_crlf = std.mem.indexOf(u8, head_slice, "\r\n") orelse head_slice.len;
            const status_line = head_slice[0..first_crlf];

            if (!std.mem.startsWith(u8, status_line, "HTTP/1.")) {
                return error.InvalidHttpStatusLine;
            }
            const minor: u8 = if (status_line.len >= 8 && status_line[7] == '0') 0 else 1;
            const sp1 = std.mem.indexOfScalar(u8, status_line, ' ') orelse return error.InvalidHttpStatusLine;
            const after_ver = std.mem.trimStart(u8, status_line[sp1 + 1 ..], " ");
            const sp2 = std.mem.indexOfScalar(u8, after_ver, ' ');
            const code_str = if (sp2 != null) after_ver[0..sp2.?] else after_ver;
            const status_code = std.fmt.parseInt(u16, code_str, 10) catch return error.InvalidHttpStatusCode;
            const reason_phrase = if (sp2 != null) std.mem.trim(u8, after_ver[sp2.? + 1 ..], " \t") else HTTPStatus.reasonPhrase(status_code);

            pos += head_end_rel + 4;

            if (status_code >= 100 && status_code < 200) {
                if (status_code == 100) saw_100 = true;
                continue;
            }

            let headers = Headers.init(allocator);
            errdefer headers.deinit();

            // Parse headers with obs-fold (SP / HTAB continuation lines) support
            if (first_crlf < head_slice.len) {
                let hdr_pos: usize = first_crlf + 2;
                let cur_name: []const u8 = "";
                let cur_val_buf: [2048]u8 = undefined;
                let cur_val_len: usize = 0;
                let has_cur = false;

                while (hdr_pos <= head_slice.len) {
                    const next_crlf = std.mem.indexOfPos(u8, head_slice, hdr_pos, "\r\n") orelse head_slice.len;
                    const line = head_slice[hdr_pos..next_crlf];
                    if (line.len > 0 && (line[0] == ' ' || line[0] == '\t') && has_cur) {
                        // obs-fold continuation: collapse leading LWS into a single space
                        const folded = std.mem.trim(u8, line, " \t");
                        if (folded.len > 0 && cur_val_len + 1 + folded.len <= cur_val_buf.len) {
                            cur_val_buf[cur_val_len] = ' ';
                            cur_val_len += 1;
                            @memcpy(cur_val_buf[cur_val_len .. cur_val_len + folded.len], folded);
                            cur_val_len += folded.len;
                        }
                    } else {
                        if (has_cur) {
                            const v_copy = try allocator.dupe(u8, cur_val_buf[0..cur_val_len]);
                            headers.append(cur_name, v_copy);
                            has_cur = false;
                            cur_val_len = 0;
                        }
                        if (line.len > 0) {
                            const colon = std.mem.indexOfScalar(u8, line, ':') orelse return error.InvalidHttpHeader;
                            cur_name = std.mem.trim(u8, line[0..colon], " \t");
                            const val = std.mem.trim(u8, line[colon + 1 ..], " \t");
                            if (val.len <= cur_val_buf.len) {
                                @memcpy(cur_val_buf[0..val.len], val);
                                cur_val_len = val.len;
                                has_cur = true;
                            }
                        }
                    }
                    if (next_crlf >= head_slice.len) break;
                    hdr_pos = next_crlf + 2;
                }
                if (has_cur) {
                    const v_copy = try allocator.dupe(u8, cur_val_buf[0..cur_val_len]);
                    headers.append(cur_name, v_copy);
                }
            }

            // Determine Connection keep-alive semantics
            let keep_alive = (minor == 1);
            if (conn := headers.get("Connection")) {
                if (asciiContainsIgnoreCase(conn, "close")) {
                    keep_alive = false;
                } else if (asciiContainsIgnoreCase(conn, "keep-alive")) {
                    keep_alive = true;
                }
            }

            // Determine Content-Encoding & Transfer-Encoding
            let is_chunked = false;
            if (te := headers.get("Transfer-Encoding")) {
                if (asciiContainsIgnoreCase(te, "chunked")) {
                    is_chunked = true;
                }
            }

            let content_enc = Encoding.Identity;
            if (ce := headers.get("Content-Encoding")) {
                if (parsed_ce := Encoding.fromString(ce)) {
                    content_enc = parsed_ce;
                }
            }

            let content_len: usize? = null;
            if (cl := headers.get("Content-Length")) {
                content_len = std.fmt.parseInt(usize, std.mem.trim(u8, cl, " \t"), 10) catch null;
            }

            const remaining_body = wire[pos..];
            let resp = ParsedHttpResponse{
                status: status_code,
                reason: reason_phrase,
                minor_version: minor,
                headers: headers,
                raw_body: remaining_body,
                body: remaining_body,
                trailer_count: 0,
                is_chunked: is_chunked,
                content_encoding: content_enc,
                content_length: content_len,
                keep_alive: keep_alive,
                saw_100_continue: saw_100,
                allocator: allocator,
                owns_body: false,
            };

            if (!HTTPStatus.allowsBody(status_code)) {
                resp.body = "";
                return resp;
            }

            if (is_chunked) {
                let dec_buf = try allocator.alloc(u8, remaining_body.len + 64);
                let cdec = ChunkedDecoder.init(true);
                const cres = cdec.feed(remaining_body, dec_buf) catch |e| {
                    allocator.free(dec_buf);
                    return e;
                };
                if (!cres.is_done) {
                    allocator.free(dec_buf);
                    return error.IncompleteChunkedBody;
                }
                let ti: usize = 0;
                while (ti < cdec.trailer_count && ti < MAX_CHUNK_TRAILERS) : (ti += 1) {
                    if (tr := cdec.trailerAt(ti)) {
                        resp.trailers[ti] = ChunkTrailer{
                            name: try allocator.dupe(u8, tr.name),
                            value: try allocator.dupe(u8, tr.value),
                        };
                    }
                }
                resp.trailer_count = cdec.trailer_count;
                const final_chunked = try allocator.dupe(u8, dec_buf[0..cres.bytes_written]);
                allocator.free(dec_buf);
                resp.body = final_chunked;
                resp.owns_body = true;
            } else if (cl_val := content_len) {
                if (remaining_body.len < cl_val) {
                    return error.IncompleteHttpBody;
                }
                resp.body = remaining_body[0..cl_val];
            }

            // Automatic Content-Encoding decompression bridged to cap:pack
            if (auto_decompress && content_enc.canUseLibDeflate() && resp.body.len > 0) {
                let decomp = Decompressor.init(content_enc);
                const plain = try decomp.decompressStrict(resp.body, allocator);
                if (resp.owns_body) {
                    allocator.free(resp.body);
                }
                resp.body = plain;
                resp.owns_body = true;
            }

            return resp;
        }
    }

    /// Pool + Redirect lifecycle helper: updates pool health / keep-alive release and evaluates redirect policy.
    pub static fn completeTurn(
        pool: *HttpConnectionPool,
        fd: i32,
        host: []const u8,
        port: u16,
        is_ssl: bool,
        now_ms: i64,
        resp: ParsedHttpResponse,
        policy: *RedirectPolicy,
        current_method: Method,
        current_url: []const u8,
        out_url_buf: []u8,
    ): RedirectDecision {
        if (resp.status >= 500) {
            * = pool.healthCheck(fd, false);
        } else {
            * = pool.releaseConnection(fd, host, port, is_ssl, now_ms, resp.keep_alive);
        }
        if (resp.isRedirect()) {
            if (loc := resp.getHeader("Location")) {
                return policy.evaluate(resp.status, current_method, current_url, loc, out_url_buf);
            }
        }
        return RedirectDecision{
            action: RedirectAction.Stop,
            next_method: current_method,
            next_url: current_url,
        };
    }
}
