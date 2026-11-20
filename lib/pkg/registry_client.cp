// Source: /data/dev/bun/src/install/NetworkTask.cp
// ============================================================================
// lib/pkg/registry_client.cp — Industrial NPM/Cap Registry HTTP Client & Disk Cache
// Absorbed from Bun src/install/{NetworkTask.cp,PackageManifestMap.cp}
// ============================================================================

const std = @import("std");
import { Hash } from "cap:hash";
import { gunzip } from "cap:pack";
import { Method } from "../http/method.cp";
import { Headers } from "../http/headers.cp";
import { HeaderBuilder } from "../http/header_builder.cp";
import { HTTPStatus } from "../http/status.cp";
import { exists, mkdirAll, readFile, writeFile, realtimeMs } from "cap:sys";
import { PkgError } from "./types.cp";
import { RegistryConfig, RegistryScope, NpmPackageManifest } from "./npm.cp";

pub const NPM_INSTALL_ACCEPT_HEADER: []const u8 = "application/vnd.npm.install-v1+json; q=1.0, application/json; q=0.8, */*";
pub const NPM_FULL_ACCEPT_HEADER: []const u8 = "application/json, */*";
pub const DEFAULT_MANIFEST_TTL_SECS: u64 = 300;

pub err RegistryError {
    InvalidUrl,
    HttpFailed,
    Unauthorized,
    NotFound,
    RetryExhausted,
    NoTransportConfigured,
    CacheCorrupt,
}

/// On-disk & in-memory HTTP metadata cache entry for a package packument.
pub struct RegistryCacheEntry {
    pkg_name: []const u8 = "",
    etag: []const u8 = "",
    last_modified: []const u8 = "",
    fetched_at: i64 = 0,
    ttl_secs: u64 = DEFAULT_MANIFEST_TTL_SECS,
    body: []const u8 = "",

    pub fn isFresh(now_secs: i64): bool {
        if (_.body.len == 0 || _.ttl_secs == 0) return false;
        if (now_secs < _.fetched_at) return false;
        return ((now_secs - _.fetched_at) as u64) < _.ttl_secs;
    }

    /// Parses `max-age=<seconds>` from an HTTP `Cache-Control` header value.
    pub static fn parseMaxAge(cache_control: []const u8): u64? {
        let iter = std.mem.splitScalar(u8, cache_control, ',');
        while (part := iter.next()) {
            const trimmed = std.mem.trim(u8, part, " \t\r\n");
            if (trimmed.len >= 8 && std.ascii.eqlIgnoreCase(trimmed[0..8], "max-age=")) {
                const num_s = std.mem.trim(u8, trimmed[8..], " \t\"");
                return std.fmt.parseInt(u64, num_s, 10) catch null;
            }
            if (std.ascii.eqlIgnoreCase(trimmed, "no-cache") || std.ascii.eqlIgnoreCase(trimmed, "no-store")) {
                return 0;
            }
        }
        return null;
    }
}

/// Exponential backoff & `Retry-After` policy for transient registry errors (429 / 5xx).
pub struct RetryPolicy {
    max_retries: u32 = 3,
    base_delay_ms: u64 = 250,
    max_delay_ms: u64 = 10000,
    respect_retry_after: bool = true,

    pub static fn isRetryableStatus(status: u16): bool {
        return status == 429 || status == 500 || status == 502 || status == 503 || status == 504;
    }

    pub static fn parseRetryAfterMs(header_val: []const u8): u64? {
        const trimmed = std.mem.trim(u8, header_val, " \t\r\n");
        if (trimmed.len == 0) return null;
        if (secs := std.fmt.parseInt(u64, trimmed, 10) catch null) {
            return secs * 1000;
        }
        return null;
    }

    pub fn computeDelayMs(attempt: u32, retry_after_header: ([]const u8)?): u64 {
        if (_.respect_retry_after) {
            if (ra := retry_after_header) {
                if (ms := RetryPolicy.parseRetryAfterMs(ra)) {
                    return if (ms > _.max_delay_ms) _.max_delay_ms else ms;
                }
            }
        }
        const shift: u6 = if (attempt >= 16) 16 else @intCast(attempt);
        const factor: u64 = (@as(u64, 1) << shift);
        const raw_delay = _.base_delay_ms * factor;
        return if (raw_delay > _.max_delay_ms) _.max_delay_ms else raw_delay;
    }
}

pub struct MockHttpReply {
    url_match: []const u8 = "",
    raw_wire: []const u8 = "",
    consumed: bool = false,
    reusable: bool = false,
}

pub struct RecordedHttpExchange {
    url: []const u8 = "",
    wire_request: []const u8 = "",
    status: u16 = 0,
}

/// Pluggable HTTP transport for RegistryHttpClient (supports deterministic offline Wire mocks + real HTTP/1.1 wire serialization).
pub struct MockRegistryTransport {
    replies: std.ArrayListUnmanaged(MockHttpReply) = .empty,
    recorded_requests: std.ArrayListUnmanaged(RecordedHttpExchange) = .empty,
    owned_strings: std.ArrayListUnmanaged([]u8) = .empty,

    pub static fn init(): MockRegistryTransport {
        return MockRegistryTransport{};
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.replies.deinit(allocator);
        _.recorded_requests.deinit(allocator);
        for (s of _.owned_strings.items) allocator.free(s);
        _.owned_strings.deinit(allocator);
    }

    fn internStr(*_, allocator: std.mem.Allocator, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        const copy = try allocator.dupe(u8, s);
        errdefer allocator.free(copy);
        try _.owned_strings.append(allocator, copy);
        return copy;
    }

    /// Enqueues a raw HTTP/1.1 wire response (e.g. `HTTP/1.1 200 OK\r\nETag: ...\r\n\r\n...`).
    pub fn pushWireReply(*_, allocator: std.mem.Allocator, url_match: []const u8, raw_wire: []const u8, reusable: bool = false): ! {
        try _.replies.append(allocator, MockHttpReply{
            url_match: try _.internStr(allocator, url_match),
            raw_wire: try _.internStr(allocator, raw_wire),
            consumed: false,
            reusable: reusable,
        });
    }

    /// Enqueues a structured HTTP/1.1 response by building wire bytes automatically.
    pub fn pushResponse(
        *_,
        allocator: std.mem.Allocator,
        url_match: []const u8,
        status: u16,
        etag: ([]const u8)?,
        cache_control: ([]const u8)?,
        retry_after: ([]const u8)?,
        body: []const u8,
        reusable: bool = false,
    ): ! {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        defer out.deinit(allocator);

        const phrase = HTTPStatus.reasonPhrase(status);
        const status_line = try std.fmt.allocPrint(allocator, "HTTP/1.1 {d} {s}\r\n", .{ status, phrase });
        defer allocator.free(status_line);
        try out.appendSlice(allocator, status_line);

        if (e := etag) {
            if (e.len > 0) {
                const h = try std.fmt.allocPrint(allocator, "ETag: {s}\r\n", .{e});
                defer allocator.free(h);
                try out.appendSlice(allocator, h);
            }
        }
        if (cc := cache_control) {
            if (cc.len > 0) {
                const h = try std.fmt.allocPrint(allocator, "Cache-Control: {s}\r\n", .{cc});
                defer allocator.free(h);
                try out.appendSlice(allocator, h);
            }
        }
        if (ra := retry_after) {
            if (ra.len > 0) {
                const h = try std.fmt.allocPrint(allocator, "Retry-After: {s}\r\n", .{ra});
                defer allocator.free(h);
                try out.appendSlice(allocator, h);
            }
        }
        if (HTTPStatus.allowsBody(status)) {
            const cl = try std.fmt.allocPrint(allocator, "Content-Length: {d}\r\n", .{body.len});
            defer allocator.free(cl);
            try out.appendSlice(allocator, cl);
        }
        try out.appendSlice(allocator, "\r\n");
        if (HTTPStatus.allowsBody(status) && body.len > 0) {
            try out.appendSlice(allocator, body);
        }
        try _.pushWireReply(allocator, url_match, out.items, reusable);
    }

    pub fn exchange(*_, allocator: std.mem.Allocator, url: []const u8, wire_req: []const u8): ([]u8)! {
        for (*rep of _.replies.items) {
            if (rep.consumed && !rep.reusable) continue;
            if (rep.url_match.len == 0 || std.mem.indexOf(u8, url, rep.url_match) != null || std.mem.eql(u8, rep.url_match, "*")) {
                if (!rep.reusable) rep.consumed = true;
                try _.recorded_requests.append(allocator, RecordedHttpExchange{
                    url: try _.internStr(allocator, url),
                    wire_request: try _.internStr(allocator, wire_req),
                    status: 0,
                });
                return allocator.dupe(u8, rep.raw_wire);
            }
        }
        return error.NotFound;
    }
}

pub struct PackumentFetchResult {
    manifest: NpmPackageManifest = NpmPackageManifest{},
    raw_json: []u8 = [],
    etag: []u8 = [],
    status: u16 = 200,
    from_cache: bool = false,
    not_modified_304: bool = false,
    attempts: u32 = 1,
    total_backoff_ms: u64 = 0,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.manifest.deinit(allocator);
        if (_.raw_json.len > 0) allocator.free(_.raw_json);
        if (_.etag.len > 0) allocator.free(_.etag);
        _.raw_json = [];
        _.etag = [];
    }
}

pub struct RegistryClientStats {
    requests_sent: usize = 0,
    cache_hits_fresh: usize = 0,
    cache_hits_304: usize = 0,
    network_fetches: usize = 0,
    retries_performed: usize = 0,
    total_backoff_ms: u64 = 0,
}

struct CachePaths {
    json_path: []u8,
    etag_path: []u8,
    meta_path: []u8,
}

struct ParsedWireReply {
    status: u16 = 0,
    headers: Headers = undefined,
    body: []const u8 = "",
    owns_body: bool = false,
    allocator: std.mem.Allocator = undefined,

    fn deinit(*_) {
        _.headers.deinit();
        if (_.owns_body && _.body.len > 0) {
            _.allocator.free(_.body);
        }
    }

    fn getHeader(name: []const u8): ([]const u8)? {
        return _.headers.get(name);
    }
}

/// Industrial NPM/Cap Registry HTTP Client with:
/// - Scoped registry + Bearer/Basic auth routing (`RegistryConfig`)
/// - `Accept: application/vnd.npm.install-v1+json` & `If-None-Match` HTTP/1.1 wire generation
/// - On-disk `<cache_dir>/manifests/<hash>.{json,etag,meta}` cache & 304 Not Modified reuse
/// - Exponential backoff + `Retry-After` handling (`RetryPolicy`)
/// - Pluggable Mock transport (`MockRegistryTransport`) + real POSIX/Zig HTTP/1.1 TCP client
pub struct RegistryHttpClient {
    allocator: std.mem.Allocator,
    config: *RegistryConfig,
    cache_dir: []const u8 = ".cap/cache",
    retry_policy: RetryPolicy = RetryPolicy{},
    default_ttl_secs: u64 = DEFAULT_MANIFEST_TTL_SECS,
    mock_transport: (*MockRegistryTransport)? = null,
    sleep_on_retry: bool = false,
    stats: RegistryClientStats = RegistryClientStats{},
    mem_cache: std.ArrayListUnmanaged(RegistryCacheEntry) = .empty,
    owned_strings: std.ArrayListUnmanaged([]u8) = .empty,

    pub static fn init(
        allocator: std.mem.Allocator,
        config: *RegistryConfig,
        cache_dir: []const u8,
    ): RegistryHttpClient {
        return RegistryHttpClient{
            allocator: allocator,
            config: config,
            cache_dir: std.mem.trimEnd(u8, cache_dir, "/"),
        };
    }

    pub fn deinit(*_) {
        _.mem_cache.deinit(_.allocator);
        for (s of _.owned_strings.items) _.allocator.free(s);
        _.owned_strings.deinit(_.allocator);
    }

    fn internStr(*_, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        const copy = try _.allocator.dupe(u8, s);
        errdefer _.allocator.free(copy);
        try _.owned_strings.append(_.allocator, copy);
        return copy;
    }

    pub fn setMockTransport(*_, transport: *MockRegistryTransport) {
        _.mock_transport = transport;
    }

    pub fn setRetryPolicy(*_, policy: RetryPolicy) {
        _.retry_policy = policy;
    }

    /// Computes deterministic 64-bit hex cache key for `(registry_url, pkg_name)`.
    pub fn manifestCacheKeyAlloc(pkg_name: []const u8): ([]u8)! {
        const scope = _.config.resolveForPackage(pkg_name);
        const h1 = Hash.wyhash(scope.registry_url, 0);
        const h2 = Hash.wyhash(pkg_name, h1);
        return std.fmt.allocPrint(_.allocator, "{x:0>16}", .{h2});
    }

    fn manifestsDirAlloc(): ([]u8)! {
        return std.fmt.allocPrint(_.allocator, "{s}/manifests", .{_.cache_dir});
    }

    fn cachePathsAlloc(pkg_name: []const u8): CachePaths! {
        const key = try _.manifestCacheKeyAlloc(pkg_name);
        defer _.allocator.free(key);
        const dir = try _.manifestsDirAlloc();
        defer _.allocator.free(dir);
        const p_json = try std.fmt.allocPrint(_.allocator, "{s}/{s}.json", .{ dir, key });
        errdefer _.allocator.free(p_json);
        const p_etag = try std.fmt.allocPrint(_.allocator, "{s}/{s}.etag", .{ dir, key });
        errdefer _.allocator.free(p_etag);
        const p_meta = try std.fmt.allocPrint(_.allocator, "{s}/{s}.meta", .{ dir, key });
        return CachePaths{ json_path: p_json, etag_path: p_etag, meta_path: p_meta };
    }

    fn readFileOpt(allocator: std.mem.Allocator, path: []const u8): ([]u8)? {
        const bytes = readFile(allocator, path) catch return null;
        return bytes;
    }

    /// Loads cached packument metadata & body from `<cache_dir>/manifests/<hash>.{json,etag,meta}`.
    pub fn loadDiskCache(*_, pkg_name: []const u8): RegistryCacheEntry? {
        const paths = _.cachePathsAlloc(pkg_name) catch return null;
        defer {
            _.allocator.free(paths.json_path);
            _.allocator.free(paths.etag_path);
            _.allocator.free(paths.meta_path);
        }
        if (!exists(paths.json_path)) return null;

        const body_bytes = readFile(_.allocator, paths.json_path) catch return null;
        const owned_body = _.internStr(body_bytes) catch {
            _.allocator.free(body_bytes);
            return null;
        };
        _.allocator.free(body_bytes);

        let etag_val: []const u8 = "";
        if (exists(paths.etag_path)) {
            if (eb := RegistryHttpClient.readFileOpt(_.allocator, paths.etag_path)) {
                etag_val = _.internStr(std.mem.trim(u8, eb, " \t\r\n")) catch "";
                _.allocator.free(eb);
            }
        }

        let last_mod: []const u8 = "";
        let fetched_at: i64 = 0;
        let ttl: u64 = _.default_ttl_secs;
        if (exists(paths.meta_path)) {
            if (mb := RegistryHttpClient.readFileOpt(_.allocator, paths.meta_path)) {
                defer _.allocator.free(mb);
                let lines = std.mem.splitScalar(u8, mb, '\n');
                while (raw_line := lines.next()) {
                    const line = std.mem.trim(u8, raw_line, " \t\r");
                    if (std.mem.startsWith(u8, line, "fetched_at=")) {
                        fetched_at = std.fmt.parseInt(i64, line[11..], 10) catch 0;
                    } else if (std.mem.startsWith(u8, line, "ttl_secs=")) {
                        ttl = std.fmt.parseInt(u64, line[9..], 10) catch _.default_ttl_secs;
                    } else if (std.mem.startsWith(u8, line, "last_modified=")) {
                        last_mod = _.internStr(line[14..]) catch "";
                    }
                }
            }
        }

        return RegistryCacheEntry{
            pkg_name: pkg_name,
            etag: etag_val,
            last_modified: last_mod,
            fetched_at: fetched_at,
            ttl_secs: ttl,
            body: owned_body,
        };
    }

    pub fn saveDiskCache(
        *_,
        pkg_name: []const u8,
        body: []const u8,
        etag: []const u8,
        last_modified: []const u8,
        fetched_at: i64,
        ttl_secs: u64,
    ): ! {
        const mdir = try _.manifestsDirAlloc();
        defer _.allocator.free(mdir);
        try mkdirAll(mdir, 0o755);

        const paths = try _.cachePathsAlloc(pkg_name);
        defer {
            _.allocator.free(paths.json_path);
            _.allocator.free(paths.etag_path);
            _.allocator.free(paths.meta_path);
        }

        try writeFile(paths.json_path, body, 0o644);
        try writeFile(paths.etag_path, etag, 0o644);

        const meta_txt = try std.fmt.allocPrint(
            _.allocator,
            "fetched_at={d}\nttl_secs={d}\nlast_modified={s}\n",
            .{ fetched_at, ttl_secs, last_modified },
        );
        defer _.allocator.free(meta_txt);
        try writeFile(paths.meta_path, meta_txt, 0o644);

        // Update in-memory cache entry
        const owned_body = try _.internStr(body);
        const owned_etag = try _.internStr(etag);
        const owned_lm = try _.internStr(last_modified);
        for (*e of _.mem_cache.items) {
            if (std.mem.eql(u8, e.pkg_name, pkg_name)) {
                e.body = owned_body;
                e.etag = owned_etag;
                e.last_modified = owned_lm;
                e.fetched_at = fetched_at;
                e.ttl_secs = ttl_secs;
                return;
            }
        }
        try _.mem_cache.append(_.allocator, RegistryCacheEntry{
            pkg_name: try _.internStr(pkg_name),
            etag: owned_etag,
            last_modified: owned_lm,
            fetched_at: fetched_at,
            ttl_secs: ttl_secs,
            body: owned_body,
        });
    }

    fn buildHttpGetWireAlloc(
        allocator: std.mem.Allocator,
        url: []const u8,
        accept: []const u8,
        if_none_match: ([]const u8)?,
        auth_header: ([]const u8)?,
    ): ([]u8)! {
        let remain = url;
        let is_ssl = false;
        if (std.mem.startsWith(u8, remain, "https://")) {
            is_ssl = true;
            remain = remain[8..];
        } else if (std.mem.startsWith(u8, remain, "http://")) {
            remain = remain[7..];
        }
        let host_port = remain;
        let path: []const u8 = "/";
        if (slash_idx := std.mem.indexOfScalar(u8, remain, '/')) {
            host_port = remain[0..slash_idx];
            path = remain[slash_idx..];
        }

        let hb = try HeaderBuilder.initAlloc(allocator, 1024);
        defer hb.deinit();
        if (!hb.writeRequestLine("GET", path)) return error.BufferTooSmall;
        if (host_port.len > 0 && !hb.append("Host", host_port)) return error.BufferTooSmall;
        if (!hb.append("User-Agent", "cap-pkg/1.0")) return error.BufferTooSmall;
        if (!hb.append("Accept", accept)) return error.BufferTooSmall;
        if (!hb.append("Accept-Encoding", "gzip")) return error.BufferTooSmall;
        if (!hb.append("Connection", "close")) return error.BufferTooSmall;
        if (etag := if_none_match) {
            if (etag.len > 0 && !hb.append("If-None-Match", etag)) return error.BufferTooSmall;
        }
        if (auth := auth_header) {
            if (auth.len > 0 && !hb.append("Authorization", auth)) return error.BufferTooSmall;
        }
        const finished = try hb.finish();
        return allocator.dupe(u8, finished);
    }

    fn parseHttpWire(allocator: std.mem.Allocator, wire: []const u8, auto_gunzip: bool): ParsedWireReply! {
        const head_end = std.mem.indexOf(u8, wire, "\r\n\r\n") orelse return error.HttpFailed;
        const head = wire[0..head_end];
        const first_crlf = std.mem.indexOf(u8, head, "\r\n") orelse head.len;
        const status_line = head[0..first_crlf];
        if (!std.mem.startsWith(u8, status_line, "HTTP/1.")) return error.HttpFailed;
        const sp1 = std.mem.indexOfScalar(u8, status_line, ' ') orelse return error.HttpFailed;
        const after_sp = std.mem.trimStart(u8, status_line[sp1 + 1 ..], " ");
        const sp2 = std.mem.indexOfScalar(u8, after_sp, ' ');
        const code_str = if (sp2 != null) after_sp[0..sp2.?] else after_sp;
        const status_code = std.fmt.parseInt(u16, code_str, 10) catch return error.HttpFailed;

        let headers = Headers.init(allocator);
        errdefer headers.deinit();
        if (first_crlf + 2 <= head.len) {
            let pos = first_crlf + 2;
            while (pos < head.len) {
                const next_crlf = std.mem.indexOfPos(u8, head, pos, "\r\n") orelse head.len;
                const line = head[pos..next_crlf];
                if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                    const hname = std.mem.trim(u8, line[0..colon], " \t");
                    const hval = std.mem.trim(u8, line[colon + 1 ..], " \t");
                    headers.append(hname, hval);
                }
                if (next_crlf >= head.len) break;
                pos = next_crlf + 2;
            }
        }

        let body: []const u8 = wire[head_end + 4 ..];
        let owns_body = false;
        if (!HTTPStatus.allowsBody(status_code)) {
            body = "";
        } else if (cl_s := headers.get("Content-Length")) {
            if (cl := std.fmt.parseInt(usize, std.mem.trim(u8, cl_s, " \t"), 10) catch null) {
                if (cl <= body.len) body = body[0..cl];
            }
        }

        if (auto_gunzip && body.len > 0) {
            if (ce := headers.get("Content-Encoding")) {
                if (std.ascii.eqlIgnoreCase(std.mem.trim(u8, ce, " \t"), "gzip")) {
                    if (plain := gunzip(body)) {
                        if (plain.len > 0) {
                            body = try allocator.dupe(u8, plain);
                            owns_body = true;
                        }
                    }
                }
            }
        }

        return ParsedWireReply{
            status: status_code,
            headers: headers,
            body: body,
            owns_body: owns_body,
            allocator: allocator,
        };
    }

    /// Constructs a raw HTTP/1.1 request for a package packument with `Accept`, `If-None-Match`, and scoped `Authorization`.
    pub fn buildPackumentRequestWireAlloc(
        pkg_name: []const u8,
        if_none_match: ([]const u8)?,
        extended_manifest: bool = false,
    ): ([]u8)! {
        const scope = _.config.resolveForPackage(pkg_name);
        const full_url = try scope.packageMetadataUrlAlloc(_.allocator, pkg_name);
        defer _.allocator.free(full_url);

        let auth_owned: ([]u8)? = try scope.authorizationHeaderAlloc(_.allocator);
        defer {
            if (a := auth_owned) _.allocator.free(a);
        }
        let auth_opt: ([]const u8)? = null;
        if (a := auth_owned) auth_opt = a;

        const accept = if (extended_manifest) NPM_FULL_ACCEPT_HEADER else NPM_INSTALL_ACCEPT_HEADER;
        return RegistryHttpClient.buildHttpGetWireAlloc(_.allocator, full_url, accept, if_none_match, auth_opt);
    }

    /// Constructs a raw HTTP/1.1 request for a tarball download (attaching auth only for matching host).
    pub fn buildTarballRequestWireAlloc(pkg_name: []const u8, tarball_url: []const u8): ([]u8)! {
        const scope = _.config.resolveForPackage(pkg_name);
        let req_host: []const u8 = "";
        let rem = tarball_url;
        if (std.mem.startsWith(u8, rem, "https://")) rem = rem[8..]
        else if (std.mem.startsWith(u8, rem, "http://")) rem = rem[7..];
        const slash_idx = std.mem.indexOfScalar(u8, rem, '/') ?? rem.len;
        const host_port = rem[0..slash_idx];
        if (c := std.mem.indexOfScalar(u8, host_port, ':')) {
            req_host = host_port[0..c];
        } else {
            req_host = host_port;
        }

        const host_path = scope.hostPath();
        const same_host = (req_host.len > 0 && std.mem.startsWith(u8, host_path, req_host));
        let auth_owned: ([]u8)? = null;
        if (same_host || scope.always_auth) {
            auth_owned = try scope.authorizationHeaderAlloc(_.allocator);
        }
        defer {
            if (a := auth_owned) _.allocator.free(a);
        }
        let auth_opt: ([]const u8)? = null;
        if (a := auth_owned) auth_opt = a;

        return RegistryHttpClient.buildHttpGetWireAlloc(_.allocator, tarball_url, "application/octet-stream, */*", null, auth_opt);
    }

    fn sendTcpHttp(url: []const u8, wire_req: []const u8): ([]u8)! {
        let host: []const u8 = "";
        let port: u16 = 80;
        let remain = url;
        if (std.mem.startsWith(u8, remain, "http://")) {
            remain = remain[7..];
            port = 80;
        } else if (std.mem.startsWith(u8, remain, "https://")) {
            // Plain TCP helper only handles http:// directly unless mock transport is installed
            return error.InvalidUrl;
        } else {
            return error.InvalidUrl;
        }

        const slash_idx = std.mem.indexOfScalar(u8, remain, '/') ?? remain.len;
        const host_port = remain[0..slash_idx];
        if (colon := std.mem.indexOfScalar(u8, host_port, ':')) {
            host = host_port[0..colon];
            port = std.fmt.parseInt(u16, host_port[colon + 1 ..], 10) catch 80;
        } else {
            host = host_port;
        }
        if (host.len == 0) return error.InvalidUrl;

        let out_bytes: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out_bytes.deinit(_.allocator);

        zig {
            const host_z = try self.allocator.allocSentinel(u8, host.len, 0);
            defer self.allocator.free(host_z);
            @memcpy(host_z[0..host.len], host);
            var port_buf: [16]u8 = undefined;
            const port_slice = std.fmt.bufPrint(&port_buf, "{d}\x00", .{port}) catch return error.InvalidUrl;
            const port_z: [*:0]const u8 = @ptrCast(port_slice.ptr);
            var hints: std.c.addrinfo = std.mem.zeroes(std.c.addrinfo);
            hints.family = std.c.AF.UNSPEC;
            hints.socktype = std.c.SOCK.STREAM;
            var res_ptr: ?*std.c.addrinfo = null;
            if (@intFromEnum(std.c.getaddrinfo(host_z.ptr, port_z, &hints, &res_ptr)) != 0 or res_ptr == null) {
                return error.HttpFailed;
            }
            defer std.c.freeaddrinfo(res_ptr.?);

            var sock_fd: i32 = -1;
            var cur = res_ptr;
            while (cur) |ai| : (cur = ai.next) {
                const s_rc = std.os.linux.socket(@intCast(ai.family), @intCast(ai.socktype), @intCast(ai.protocol));
                if (@as(isize, @bitCast(s_rc)) < 0) continue;
                const fd: i32 = @intCast(s_rc);
                const c_rc = std.os.linux.connect(fd, @ptrCast(ai.addr.?), ai.addrlen);
                if (@as(isize, @bitCast(c_rc)) == 0) {
                    sock_fd = fd;
                    break;
                }
                _ = std.os.linux.close(fd);
            }
            if (sock_fd < 0) return error.HttpFailed;
            defer _ = std.os.linux.close(sock_fd);

            var sent: usize = 0;
            while (sent < wire_req.len) {
                const w_rc = std.os.linux.write(sock_fd, wire_req[sent..].ptr, wire_req.len - sent);
                const w_signed: isize = @bitCast(w_rc);
                if (w_signed <= 0) return error.HttpFailed;
                sent += @intCast(w_signed);
            }

            var tmp: [4096]u8 = undefined;
            while (true) {
                const r_rc = std.os.linux.read(sock_fd, &tmp, tmp.len);
                const r_signed: isize = @bitCast(r_rc);
                if (r_signed < 0) return error.HttpFailed;
                if (r_signed == 0) break;
                try out_bytes.appendSlice(self.allocator, tmp[0..@intCast(r_signed)]);
            }
        }
        return out_bytes.toOwnedSlice(_.allocator);
    }

    fn dispatchWire(url: []const u8, wire_req: []const u8): ([]u8)! {
        if (mock := _.mock_transport) {
            return mock.exchange(_.allocator, url, wire_req);
        }
        return _.sendTcpHttp(url, wire_req);
    }

    /// Fetches and parses a package packument with:
    /// 1. In-memory & disk TTL freshness short-circuit
    /// 2. Conditional `If-None-Match` + `304 Not Modified` zero-traffic reuse
    /// 3. Exponential backoff + `Retry-After` retries on 429/500/502/503/504
    pub fn fetchPackument(
        *_,
        pkg_name: []const u8,
        now_secs_opt: i64? = null,
        force_revalidate: bool = false,
        extended_manifest: bool = false,
    ): PackumentFetchResult! {
        const now_secs: i64 = now_secs_opt ?? @divTrunc(realtimeMs(), 1000);

        // Check in-memory or disk cache first
        let cached: RegistryCacheEntry? = null;
        for (e of _.mem_cache.items) {
            if (std.mem.eql(u8, e.pkg_name, pkg_name)) {
                cached = e;
                break;
            }
        }
        if (cached == null) {
            cached = _.loadDiskCache(pkg_name);
        }

        if (!force_revalidate) {
            if (ce := cached) {
                if (ce.isFresh(now_secs)) {
                    _.stats.cache_hits_fresh += 1;
                    const json_dup = try _.allocator.dupe(u8, ce.body);
                    errdefer _.allocator.free(json_dup);
                    const owned_json = try _.internStr(json_dup);
                    const manifest = try NpmPackageManifest.parsePackument(_.allocator, owned_json);
                    return PackumentFetchResult{
                        manifest: manifest,
                        raw_json: json_dup,
                        etag: try _.allocator.dupe(u8, ce.etag),
                        status: 200,
                        from_cache: true,
                        not_modified_304: false,
                        attempts: 0,
                        total_backoff_ms: 0,
                    };
                }
            }
        }

        const scope = _.config.resolveForPackage(pkg_name);
        const url = try scope.packageMetadataUrlAlloc(_.allocator, pkg_name);
        defer _.allocator.free(url);

        let cond_etag: ([]const u8)? = null;
        if (ce := cached) {
            if (ce.etag.len > 0) cond_etag = ce.etag;
        }

        let attempt: u32 = 0;
        let total_backoff: u64 = 0;
        while (true) {
            attempt += 1;
            _.stats.requests_sent += 1;

            const wire_req = try _.buildPackumentRequestWireAlloc(pkg_name, cond_etag, extended_manifest);
            defer _.allocator.free(wire_req);

            const wire_resp = try _.dispatchWire(url, wire_req);
            defer _.allocator.free(wire_resp);

            let parsed = try RegistryHttpClient.parseHttpWire(_.allocator, wire_resp, true);
            defer parsed.deinit();

            if (parsed.status == 304) {
                if (ce := cached) {
                    if (ce.body.len > 0) {
                        _.stats.cache_hits_304 += 1;
                        let new_ttl = _.default_ttl_secs;
                        if (cc := parsed.getHeader("Cache-Control")) {
                            if (ma := RegistryCacheEntry.parseMaxAge(cc)) new_ttl = ma;
                        }
                        const new_etag = parsed.getHeader("ETag") ?? ce.etag;
                        const new_lm = parsed.getHeader("Last-Modified") ?? ce.last_modified;
                        try _.saveDiskCache(pkg_name, ce.body, new_etag, new_lm, now_secs, new_ttl);

                        const json_dup = try _.allocator.dupe(u8, ce.body);
                        errdefer _.allocator.free(json_dup);
                        const owned_json = try _.internStr(json_dup);
                        const manifest = try NpmPackageManifest.parsePackument(_.allocator, owned_json);
                        return PackumentFetchResult{
                            manifest: manifest,
                            raw_json: json_dup,
                            etag: try _.allocator.dupe(u8, new_etag),
                            status: 304,
                            from_cache: true,
                            not_modified_304: true,
                            attempts: attempt,
                            total_backoff_ms: total_backoff,
                        };
                    }
                }
                return error.CacheCorrupt;
            }

            if (RetryPolicy.isRetryableStatus(parsed.status) && attempt <= _.retry_policy.max_retries) {
                const retry_after = parsed.getHeader("Retry-After");
                const delay_ms = _.retry_policy.computeDelayMs(attempt - 1, retry_after);
                total_backoff += delay_ms;
                _.stats.retries_performed += 1;
                _.stats.total_backoff_ms += delay_ms;
                if (_.sleep_on_retry && delay_ms > 0) {
                    zig {
                        var ts = std.os.linux.timespec{
                            .sec = @intCast(delay_ms / 1000),
                            .nsec = @intCast((delay_ms % 1000) * 1000000),
                        };
                        _ = std.os.linux.nanosleep(&ts, null);
                    }
                }
                continue;
            }

            if (parsed.status == 401 || parsed.status == 403) return error.Unauthorized;
            if (parsed.status == 404) return error.NotFound;
            if (parsed.status < 200 || parsed.status >= 300) {
                if (RetryPolicy.isRetryableStatus(parsed.status)) return error.RetryExhausted;
                return error.HttpFailed;
            }

            _.stats.network_fetches += 1;
            const resp_etag = parsed.getHeader("ETag") ?? "";
            const resp_lm = parsed.getHeader("Last-Modified") ?? "";
            let resp_ttl = _.default_ttl_secs;
            if (cc := parsed.getHeader("Cache-Control")) {
                if (ma := RegistryCacheEntry.parseMaxAge(cc)) resp_ttl = ma;
            }

            try _.saveDiskCache(pkg_name, parsed.body, resp_etag, resp_lm, now_secs, resp_ttl);

            const json_dup = try _.allocator.dupe(u8, parsed.body);
            errdefer _.allocator.free(json_dup);
            const owned_json = try _.internStr(json_dup);
            const manifest = try NpmPackageManifest.parsePackument(_.allocator, owned_json);

            return PackumentFetchResult{
                manifest: manifest,
                raw_json: json_dup,
                etag: try _.allocator.dupe(u8, resp_etag),
                status: parsed.status,
                from_cache: false,
                not_modified_304: false,
                attempts: attempt,
                total_backoff_ms: total_backoff,
            };
        }
    }

    /// Downloads tarball bytes with auth routing & retry policy.
    pub fn fetchTarballBytes(*_, pkg_name: []const u8, tarball_url: []const u8): ([]u8)! {
        let attempt: u32 = 0;
        while (true) {
            attempt += 1;
            _.stats.requests_sent += 1;
            const wire_req = try _.buildTarballRequestWireAlloc(pkg_name, tarball_url);
            defer _.allocator.free(wire_req);

            const wire_resp = try _.dispatchWire(tarball_url, wire_req);
            defer _.allocator.free(wire_resp);

            let parsed = try RegistryHttpClient.parseHttpWire(_.allocator, wire_resp, false);
            defer parsed.deinit();

            if (RetryPolicy.isRetryableStatus(parsed.status) && attempt <= _.retry_policy.max_retries) {
                const delay_ms = _.retry_policy.computeDelayMs(attempt - 1, parsed.getHeader("Retry-After"));
                _.stats.retries_performed += 1;
                _.stats.total_backoff_ms += delay_ms;
                if (_.sleep_on_retry && delay_ms > 0) {
                    zig {
                        var ts = std.os.linux.timespec{
                            .sec = @intCast(delay_ms / 1000),
                            .nsec = @intCast((delay_ms % 1000) * 1000000),
                        };
                        _ = std.os.linux.nanosleep(&ts, null);
                    }
                }
                continue;
            }

            if (parsed.status == 401 || parsed.status == 403) return error.Unauthorized;
            if (parsed.status == 404) return error.NotFound;
            if (parsed.status < 200 || parsed.status >= 300) return error.HttpFailed;

            _.stats.network_fetches += 1;
            return _.allocator.dupe(u8, parsed.body);
        }
    }
}
