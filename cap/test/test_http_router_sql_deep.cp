// Source: /data/dev/bun/src/http/ & /data/dev/bun/src/router/ & /data/dev/bun/src/sql/
// cap/test/test_http_router_sql_deep.cp
// Comprehensive Deep Regression Suite for cap:http, cap:router, and cap:sql

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Method,
    Headers,
    Encoding,
    FetchRedirect,
    AcceptEncodingItem,
    ContentEncodingNegotiator,
    Decompressor,
    StreamingDecompressor,
    ChunkTrailer,
    ChunkedDecoder,
    ChunkedEncoder,
    RedirectAction,
    RedirectPolicy,
    HttpConnectionPool,
    ProxyTunnel,
    HttpProxyTunnel,
    NoProxyMatcher,
} from "cap:http";
import {
    gzip,
    deflate,
    deflateRaw,
} from "../../lib/pack/gzip.cp";
import {
    TrailingSlashMode,
    RouterOptions,
    Router,
    FileSystemRouter,
    FsRouteKind,
} from "cap:router";
import {
    SqlDialect,
    SqlType,
    SqlValue,
    SqlStatement,
    PostgresWire,
    PostgresBackendTag,
    PostgresBackendFrame,
    PostgresAuthKind,
    PostgresAuthChallenge,
    PostgresParameterStatus,
    PostgresBackendKeyData,
    PostgresRowDescription,
    PostgresDataRow,
    PostgresCommandComplete,
    PostgresTransactionStatus,
    PostgresReadyForQuery,
    PostgresErrorResponse,
    ScramSha256Client,
    MySqlPacketHeader,
    MySqlLenEnc,
    MySqlLenEncStr,
    MySqlCapabilities,
    MySqlCommandType,
    MySqlHandshakeV10,
    MySqlOkPacket,
    MySqlErrPacket,
    MySqlEofPacket,
    MySqlCommandEncoder,
    SqlConnectionPool,
    SqlTransactionState,
    SqlIsolationLevel,
    SqlTransactionStateMachine,
} from "cap:sql";
import { Base64 } from "cap:encoding";

fn testHttpChunkedCodec(): !void {
    // 1. Streaming chunked decode with extensions, split feed boundaries, and trailers
    let decoder = ChunkedDecoder.init(true);
    let out: [256]u8 = undefined;
    let out_pos: usize = 0;

    // Split right across hex size, extension, data, and trailer
    const wire = "5;ext=1;foo=bar\r\nHello\r\n7\r\n World!\r\n0;last=yes\r\nX-Checksum: sha256=deadbeef\r\nX-Timing: 12ms\r\n\r\n";

    // Feed 1 byte at a time to stress state machine boundaries
    let i: usize = 0;
    while (i < wire.len) : (i += 1) {
        const res = try decoder.feed(wire[i .. i + 1], out[out_pos..]);
        out_pos += res.bytes_written;
        if (i + 1 < wire.len) {
            expect_eq(res.is_done, false);
        } else {
            expect_eq(res.is_done, true);
        }
    }
    expect_eq(decoder.isDone(), true);
    expect_eq(std.mem.eql(u8, out[0..out_pos], "Hello World!"), true);
    expect_eq(decoder.trailer_count, 2);
    expect_eq(std.mem.eql(u8, decoder.getTrailer("x-checksum").?, "sha256=deadbeef"), true);
    expect_eq(std.mem.eql(u8, decoder.getTrailer("X-Timing").?, "12ms"), true);

    // 2. Encoder + decodeAll roundtrip with trailers
    let enc_buf: [256]u8 = undefined;
    let enc_pos: usize = 0;
    enc_pos += try ChunkedEncoder.encodeChunk("CP-", enc_buf[enc_pos..]);
    enc_pos += try ChunkedEncoder.encodeChunk("HTTP/1.1", enc_buf[enc_pos..]);
    const trailers: [1]ChunkTrailer = [
        ChunkTrailer{ name: "X-Done", value: "true" },
    ];
    enc_pos += try ChunkedEncoder.encodeFinalWithTrailers(trailers[0..1], enc_buf[enc_pos..]);

    let dec_out: [128]u8 = undefined;
    const all_res = try ChunkedDecoder.decodeAll(enc_buf[0..enc_pos], &dec_out);
    expect_eq(all_res.is_done, true);
    expect_eq(std.mem.eql(u8, dec_out[0..all_res.bytes_written], "CP-HTTP/1.1"), true);
    expect_eq(all_res.trailer_count, 1);

    // 3. encodeBody splitting + incomplete/malformed rejection
    let body_wire: [256]u8 = undefined;
    const bw_len = try ChunkedEncoder.encodeBody("ABCDEFGHIJ", 4, &body_wire);
    const body_res = try ChunkedDecoder.decodeAll(body_wire[0..bw_len], &dec_out);
    expect_eq(std.mem.eql(u8, dec_out[0..body_res.bytes_written], "ABCDEFGHIJ"), true);

    let bad_caught = false;
    * = ChunkedDecoder.decodeAll("5\r\nHello\r\n", &dec_out) catch {
        bad_caught = true;
    };
    expect_eq(bad_caught, true);

    let hex_bad = false;
    * = ChunkedDecoder.decodeAll("ZZ\r\n", &dec_out) catch {
        hex_bad = true;
    };
    expect_eq(hex_bad, true);
}

fn testHttpConnectionPool(): !void {
    let pool = HttpConnectionPool.init(2, 4, 5000, 3);

    // Register & release connections
    expect_eq(pool.addConnection(10, "api.cp.dev", 443, true, 1000), true);
    expect_eq(pool.addConnection(11, "api.cp.dev", 443, true, 1000), true);
    // Exceeding max_per_host (2) rejected
    expect_eq(pool.addConnection(12, "api.cp.dev", 443, true, 1000), false);
    expect_eq(pool.countForHost("API.CP.DEV", 443, true), 2);

    // Release 10 back to idle, acquire reuses it
    expect_eq(pool.releaseConnection(10, "api.cp.dev", 443, true, 1200, true), true);
    expect_eq(pool.idleCountForHost("api.cp.dev", 443, true), 1);

    const reused = pool.acquire("API.CP.DEV", 443, true, 2000);
    expect_eq(reused != null, true);
    expect_eq(reused.?, 10);

    // Health check failure evicts connection
    expect_eq(pool.healthCheck(11, false), false);
    expect_eq(pool.countForHost("api.cp.dev", 443, true), 1);

    // Release 10 (requests_served = 2), acquire again (requests_served = 3 == max_requests_per_conn), release -> evicted!
    expect_eq(pool.releaseConnection(10, "api.cp.dev", 443, true, 2500, true), true);
    expect_eq(pool.acquire("api.cp.dev", 443, true, 2600).?, 10);
    expect_eq(pool.releaseConnection(10, "api.cp.dev", 443, true, 2700, true), false);

    // Idle timeout eviction
    expect_eq(pool.releaseConnection(20, "cdn.cp.dev", 80, false, 1000, true), true);
    expect_eq(pool.releaseConnection(21, "cdn.cp.dev", 80, false, 4000, true), true);
    const evicted = pool.evictExpired(7000); // 7000 - 1000 = 6000 > 5000 -> 20 evicted, 21 stays
    expect_eq(evicted, 1);
    expect_eq(pool.acquire("cdn.cp.dev", 80, false, 7500).?, 21);

    // Max total capacity triggers oldest idle eviction
    expect_eq(pool.releaseConnection(21, "cdn.cp.dev", 80, false, 7600, true), true);
    expect_eq(pool.addConnection(30, "a.dev", 80, false, 8000), true);
    expect_eq(pool.addConnection(31, "b.dev", 80, false, 8000), true);
    expect_eq(pool.addConnection(32, "c.dev", 80, false, 8000), true);
    // Total is now 4 (21 idle + 30,31,32 in use). Adding 33 should evict idle 21!
    expect_eq(pool.addConnection(33, "d.dev", 80, false, 8100), true);
    expect_eq(pool.totalCount(), 4);

    const st = pool.stats();
    expect_eq(st.hits, 3);
    expect_eq(st.unhealthy_closed, 1);
}

fn testHttpRedirectPolicy(): !void {
    const allocator = std.heap.page_allocator;
    let url_buf: [512]u8 = undefined;

    // 1. 301/302 POST -> GET & drop body + body headers, same-origin keeps Auth
    let policy = RedirectPolicy.init(FetchRedirect.Follow, 5);
    const d302 = policy.evaluate(302, Method.POST, "https://api.cp.dev/v1/items", "/v2/items?ok=1", &url_buf);
    expect_eq(d302.action == RedirectAction.Follow, true);
    expect_eq(d302.next_method == Method.GET, true);
    expect_eq(d302.keep_body, false);
    expect_eq(d302.strip_body_headers, true);
    expect_eq(d302.strip_auth_headers, false);
    expect_eq(d302.is_same_origin, true);
    expect_eq(std.mem.eql(u8, d302.next_url, "https://api.cp.dev/v2/items?ok=1"), true);

    // 2. 307/308 POST -> preserves method & body; cross-origin strips Authorization/Cookie
    let hdrs = Headers.init(allocator);
    defer hdrs.deinit();
    hdrs.append("Authorization", "Bearer secret");
    hdrs.append("Cookie", "sid=123");
    hdrs.append("Content-Type", "application/json");
    hdrs.append("X-Trace", "keep-me");

    const d307 = policy.evaluate(307, Method.POST, "https://api.cp.dev/v2/items", "https://other.cp.dev/upload", &url_buf);
    expect_eq(d307.action == RedirectAction.Follow, true);
    expect_eq(d307.next_method == Method.POST, true);
    expect_eq(d307.keep_body, true);
    expect_eq(d307.strip_auth_headers, true);
    expect_eq(d307.strip_body_headers, false);
    RedirectPolicy.applyHeaderPolicy(&hdrs, d307);
    expect_eq(hdrs.has("Authorization"), false);
    expect_eq(hdrs.has("Cookie"), false);
    expect_eq(hdrs.has("Content-Type"), true);
    expect_eq(hdrs.has("X-Trace"), true);

    // 3. 303 PUT -> GET + strip body headers; 303 HEAD stays HEAD
    const d303_put = policy.evaluate(303, Method.PUT, "https://other.cp.dev/a/b/c", "../done", &url_buf);
    expect_eq(d303_put.next_method == Method.GET, true);
    expect_eq(d303_put.keep_body, false);
    expect_eq(d303_put.strip_body_headers, true);
    expect_eq(std.mem.eql(u8, d303_put.next_url, "https://other.cp.dev/a/done"), true);
    RedirectPolicy.applyHeaderPolicy(&hdrs, d303_put);
    expect_eq(hdrs.has("Content-Type"), false);

    // 4. Relative protocol `//cdn.cp.dev/file`
    const rel_proto = try RedirectPolicy.resolveLocation("https://api.cp.dev/x", "//cdn.cp.dev/file", &url_buf);
    expect_eq(std.mem.eql(u8, rel_proto, "https://cdn.cp.dev/file"), true);

    // 5. Loop detection (A -> B -> A)
    let loop_policy = RedirectPolicy.initDefault();
    const hop1 = loop_policy.evaluate(302, Method.GET, "https://a.dev/1", "https://a.dev/2", &url_buf);
    expect_eq(hop1.action == RedirectAction.Follow, true);
    const hop2 = loop_policy.evaluate(302, Method.GET, "https://a.dev/2", "https://a.dev/1", &url_buf);
    expect_eq(hop2.action == RedirectAction.Error, true);

    // 6. Max redirect limit & Manual/Error modes
    let cap_policy = RedirectPolicy.init(FetchRedirect.Follow, 1);
    expect_eq(cap_policy.evaluate(301, Method.GET, "https://x.dev/1", "/2", &url_buf).action == RedirectAction.Follow, true);
    expect_eq(cap_policy.evaluate(301, Method.GET, "https://x.dev/2", "/3", &url_buf).action == RedirectAction.Error, true);

    let man_policy = RedirectPolicy.init(FetchRedirect.Manual, 10);
    expect_eq(man_policy.evaluate(302, Method.GET, "https://x.dev/1", "/2", &url_buf).action == RedirectAction.Stop, true);
}

fn testHttpProxyAndEncoding(): !void {
    // 1. Proxy-Authorization: Basic encoding & CONNECT request formatting
    let tunnel = HttpProxyTunnel.init("proxy.corp.local", 8080, "api.cp.dev", 443);
    expect_eq(tunnel.setBasicAuth("alice", "s3cr3t"), true);
    expect_eq(std.mem.eql(u8, tunnel.auth_header, "Basic YWxpY2U6czNjcjN0"), true);

    let conn_buf: [512]u8 = undefined;
    const req = try tunnel.formatConnectRequest(&conn_buf);
    expect_eq(std.mem.indexOf(u8, req, "CONNECT api.cp.dev:443 HTTP/1.1\r\n") != null, true);
    expect_eq(std.mem.indexOf(u8, req, "Proxy-Authorization: Basic YWxpY2U6czNjcjN0\r\n") != null, true);

    const ok_resp = ProxyTunnel.parseConnectResponse("HTTP/1.1 200 Connection Established\r\n\r\n").?;
    expect_eq(ok_resp.established, true);
    expect_eq(ok_resp.requires_auth, false);

    const auth_resp = ProxyTunnel.parseConnectResponse("HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"corp\"\r\n\r\n").?;
    expect_eq(auth_resp.established, false);
    expect_eq(auth_resp.requires_auth, true);
    expect_eq(std.mem.eql(u8, auth_resp.proxy_authenticate.?, "Basic realm=\"corp\""), true);

    // 2. NO_PROXY rules: exact, suffix, port, IPv4 CIDR, wildcard
    const no_proxy = "localhost, .internal.corp, db.local:5432, 10.0.0.0/8, 192.168.1.0/24";
    expect_eq(NoProxyMatcher.matches("localhost", 80, no_proxy), true);
    expect_eq(NoProxyMatcher.matches("api.internal.corp", 443, no_proxy), true);
    expect_eq(NoProxyMatcher.matches("internal.corp", 443, no_proxy), true);
    expect_eq(NoProxyMatcher.matches("notinternal.corp", 443, no_proxy), false);
    expect_eq(NoProxyMatcher.matches("db.local", 5432, no_proxy), true);
    expect_eq(NoProxyMatcher.matches("db.local", 3306, no_proxy), false);
    expect_eq(NoProxyMatcher.matches("10.42.9.1", 80, no_proxy), true);
    expect_eq(NoProxyMatcher.matches("192.168.1.55", 80, no_proxy), true);
    expect_eq(NoProxyMatcher.matches("192.168.2.55", 80, no_proxy), false);
    expect_eq(NoProxyMatcher.matches("public.example.com", 443, no_proxy), false);
    expect_eq(NoProxyMatcher.matches("anything.dev", 443, "*"), true);

    // 3. Accept-Encoding q-value sorting & negotiation
    let items: [8]AcceptEncodingItem = undefined;
    const cnt = ContentEncodingNegotiator.parseAcceptEncoding("deflate;q=0.5, gzip;q=0.9, br;q=1.0, identity;q=0.1", &items);
    expect_eq(cnt, 4);
    expect_eq(items[0].encoding == Encoding.Brotli, true);
    expect_eq(items[1].encoding == Encoding.Gzip, true);
    expect_eq(items[2].encoding == Encoding.Deflate, true);
    expect_eq(items[3].encoding == Encoding.Identity, true);

    const srv_supported: [3]Encoding = [Encoding.Gzip, Encoding.Deflate, Encoding.Identity];
    expect_eq(
        ContentEncodingNegotiator.negotiate("br;q=1.0, deflate;q=0.8, gzip;q=0.9", srv_supported[0..3]) == Encoding.Gzip,
        true,
    );
    // Explicit q=0 rejection overrides wildcard *
    expect_eq(
        ContentEncodingNegotiator.negotiate("gzip;q=0, *;q=0.5", srv_supported[0..3]) == Encoding.Deflate,
        true,
    );

    // 4. Real Gzip & Deflate decompression bridged to cap:pack
    const allocator = std.heap.page_allocator;
    const plain = "CP HTTP Content-Encoding gzip/deflate streaming decompression payload repeated repeated repeated!";
    const gz_bytes = gzip(plain, 6);
    let gz_dec = Decompressor.init(Encoding.Gzip);
    const ungz = try gz_dec.decompressStrict(gz_bytes, allocator);
    defer allocator.free(ungz);
    expect_eq(std.mem.eql(u8, ungz, plain), true);

    const def_bytes = deflate(plain, 6);
    let def_dec = Decompressor.init(Encoding.Deflate);
    const undef = try def_dec.decompressStrict(def_bytes, allocator);
    defer allocator.free(undef);
    expect_eq(std.mem.eql(u8, undef, plain), true);

    const raw_def_bytes = deflateRaw(plain, 6);
    const unraw = try def_dec.decompressStrict(raw_def_bytes, allocator);
    defer allocator.free(unraw);
    expect_eq(std.mem.eql(u8, unraw, plain), true);

    // StreamingDecompressor incremental chunks
    let sd = StreamingDecompressor.init(Encoding.Gzip);
    defer sd.release();
    const mid = gz_bytes.len / 2;
    const part1 = try sd.feed(gz_bytes[0..mid], allocator);
    defer allocator.free(part1);
    const part2 = try sd.feed(gz_bytes[mid..], allocator);
    defer allocator.free(part2);
    const part3 = try sd.finish(allocator);
    defer allocator.free(part3);
    expect_eq(part1.len + part2.len + part3.len, plain.len);

    // Corrupt gzip rejected in strict mode
    let corrupt_err = false;
    * = gz_dec.decompressStrict("not-valid-gzip", allocator) catch {
        corrupt_err = true;
    };
    expect_eq(corrupt_err, true);
}

fn testRouterRadixDeep(): !void {
    const allocator = std.heap.page_allocator;
    let r = Router(string).initWithOptions(allocator, RouterOptions{
        trailing_slash: TrailingSlashMode.Ignore,
        detect_conflicts: true,
    });

    // Strict 5-tier priority: Static > Hybrid > Param > Wildcard > Optional Catch-All
    try r.tryInsert("GET", "/files/latest.json", "static_latest");
    try r.tryInsert("GET", "/files/doc-:id.json", "hybrid_doc");
    try r.tryInsert("GET", "/files/:name.json", "hybrid_ext");
    try r.tryInsert("GET", "/files/:slug", "param_slug");
    try r.tryInsert("GET", "/files/**path", "double_star");
    try r.tryInsert("GET", "/opt/[[...rest]]", "opt_catch");

    // Backtracking test: static deep route fails on sibling, falls back to param
    try r.tryInsert("GET", "/users/admin/settings", "admin_settings");
    try r.tryInsert("GET", "/users/:id/profile", "user_profile");

    expect_eq(std.mem.eql(u8, r.lookup("GET", "/files/latest.json").?.handler, "static_latest"), true);

    const m_doc = r.lookup("GET", "/files/doc-42.json").?;
    expect_eq(std.mem.eql(u8, m_doc.handler, "hybrid_doc"), true);
    expect_eq(std.mem.eql(u8, m_doc.params.get("id").?, "42"), true);

    const m_ext = r.lookup("GET", "/files/schema.json").?;
    expect_eq(std.mem.eql(u8, m_ext.handler, "hybrid_ext"), true);
    expect_eq(std.mem.eql(u8, m_ext.params.get("name").?, "schema"), true);

    const m_par = r.lookup("GET", "/files/readme").?;
    expect_eq(std.mem.eql(u8, m_par.handler, "param_slug"), true);
    expect_eq(std.mem.eql(u8, m_par.params.get("slug").?, "readme"), true);

    const m_wc = r.lookup("GET", "/files/a/b/c.txt").?;
    expect_eq(std.mem.eql(u8, m_wc.handler, "double_star"), true);

    // Optional catch-all matches both 0 segments and N segments
    const m_opt0 = r.lookup("GET", "/opt").?;
    expect_eq(std.mem.eql(u8, m_opt0.handler, "opt_catch"), true);
    expect_eq(m_opt0.params.count(), 0);
    const m_opt2 = r.lookup("GET", "/opt/guide/v1").?;
    expect_eq(std.mem.eql(u8, m_opt2.handler, "opt_catch"), true);
    expect_eq(std.mem.eql(u8, m_opt2.params.get("rest").?, "guide/v1"), true);

    // Backtrack: /users/admin/profile -> matches :id="admin"
    const m_bt = r.lookup("GET", "/users/admin/profile").?;
    expect_eq(std.mem.eql(u8, m_bt.handler, "user_profile"), true);
    expect_eq(std.mem.eql(u8, m_bt.params.get("id").?, "admin"), true);

    // Conflict & duplicate detection
    let dup_err = false;
    r.tryInsert("GET", "/files/latest.json", "dup") catch {
        dup_err = true;
    };
    expect_eq(dup_err, true);

    let conflict_err = false;
    r.tryInsert("GET", "/files/:other", "conflict") catch {
        conflict_err = true;
    };
    expect_eq(conflict_err, true);

    let misplaced_err = false;
    r.tryInsert("GET", "/bad/*mid/end", "bad") catch {
        misplaced_err = true;
    };
    expect_eq(misplaced_err, true);

    // Strict trailing slash mode
    let strict_r = Router(string).initWithOptions(allocator, RouterOptions{
        trailing_slash: TrailingSlashMode.Strict,
        detect_conflicts: true,
    });
    strict_r.get("/items", "no_slash");
    strict_r.get("/items/", "with_slash");
    expect_eq(std.mem.eql(u8, strict_r.lookup("GET", "/items").?.handler, "no_slash"), true);
    expect_eq(std.mem.eql(u8, strict_r.lookup("GET", "/items/").?.handler, "with_slash"), true);
}

fn testFileSystemRouter(): !void {
    const allocator = std.heap.page_allocator;
    let fsr = FileSystemRouter.init(allocator);

    // Add in reverse priority order to verify automatic priority sorting
    try fsr.addFile("docs/[[...optional]].cp");
    try fsr.addFile("files/[...catchall].cp");
    try fsr.addFile("blog/[slug].cp");
    try fsr.addFile("blog/featured.cp");
    try fsr.addFile("(marketing)/about.cp");
    try fsr.addFile("(auth)/login/index.cp");
    try fsr.addFile("index.cp");

    fsr.sortRoutes();
    expect_eq(fsr.count(), 7);

    // Index route
    const m_root = fsr.matchRoute("/").?;
    expect_eq(std.mem.eql(u8, m_root.file_path, "index.cp"), true);

    // Route groups `(marketing)` & `(auth)` stripped from URL
    const m_about = fsr.matchRoute("/about").?;
    expect_eq(std.mem.eql(u8, m_about.file_path, "(marketing)/about.cp"), true);
    expect_eq(m_about.kind == FsRouteKind.Static, true);

    const m_login = fsr.matchRoute("/login").?;
    expect_eq(std.mem.eql(u8, m_login.file_path, "(auth)/login/index.cp"), true);

    // Static priority over [slug]
    const m_feat = fsr.matchRoute("/blog/featured").?;
    expect_eq(std.mem.eql(u8, m_feat.file_path, "blog/featured.cp"), true);

    const m_post = fsr.matchRoute("/blog/hello-cp").?;
    expect_eq(std.mem.eql(u8, m_post.file_path, "blog/[slug].cp"), true);
    expect_eq(std.mem.eql(u8, m_post.params.get("slug").?, "hello-cp"), true);

    // Catch-all requires >= 1 segment
    expect_eq(fsr.matchRoute("/files") == null, true);
    const m_files = fsr.matchRoute("/files/2026/report.pdf").?;
    expect_eq(std.mem.eql(u8, m_files.params.get("catchall").?, "2026/report.pdf"), true);

    // Optional catch-all matches 0 or N segments
    const m_docs0 = fsr.matchRoute("/docs").?;
    expect_eq(std.mem.eql(u8, m_docs0.file_path, "docs/[[...optional]].cp"), true);
    expect_eq(m_docs0.params.count(), 0);
    const m_docs2 = fsr.matchRoute("/docs/sql/postgres").?;
    expect_eq(std.mem.eql(u8, m_docs2.params.get("optional").?, "sql/postgres"), true);

    // Duplicate across route groups detected
    let dup_group = false;
    fsr.addFile("(other)/about.cp") catch {
        dup_group = true;
    };
    expect_eq(dup_group, true);

    // Real disk scan test in temp dir using POSIX syscalls
    * = std.c.unlink("/tmp/cp_fs_router_deep_test/(api)/v1/users.cp");
    * = std.c.unlink("/tmp/cp_fs_router_deep_test/(api)/v1/[id].cp");
    * = std.c.rmdir("/tmp/cp_fs_router_deep_test/(api)/v1");
    * = std.c.rmdir("/tmp/cp_fs_router_deep_test/(api)");
    * = std.c.rmdir("/tmp/cp_fs_router_deep_test");

    expect_eq(std.c.mkdir("/tmp/cp_fs_router_deep_test", 0o755), 0);
    expect_eq(std.c.mkdir("/tmp/cp_fs_router_deep_test/(api)", 0o755), 0);
    expect_eq(std.c.mkdir("/tmp/cp_fs_router_deep_test/(api)/v1", 0o755), 0);

    const fd1 = std.c.open("/tmp/cp_fs_router_deep_test/(api)/v1/users.cp", .{ .ACCMODE = .WRONLY, .CREAT = true, .TRUNC = true }, 0o644 as u32);
    expect_eq(fd1 >= 0, true);
    * = std.c.close(fd1);
    const fd2 = std.c.open("/tmp/cp_fs_router_deep_test/(api)/v1/[id].cp", .{ .ACCMODE = .WRONLY, .CREAT = true, .TRUNC = true }, 0o644 as u32);
    expect_eq(fd2 >= 0, true);
    * = std.c.close(fd2);

    defer {
        * = std.c.unlink("/tmp/cp_fs_router_deep_test/(api)/v1/users.cp");
        * = std.c.unlink("/tmp/cp_fs_router_deep_test/(api)/v1/[id].cp");
        * = std.c.rmdir("/tmp/cp_fs_router_deep_test/(api)/v1");
        * = std.c.rmdir("/tmp/cp_fs_router_deep_test/(api)");
        * = std.c.rmdir("/tmp/cp_fs_router_deep_test");
    }

    let disk_fsr = FileSystemRouter.init(allocator);
    const scanned = try disk_fsr.scanDir("/tmp/cp_fs_router_deep_test");
    expect_eq(scanned, 2);
    expect_eq(std.mem.eql(u8, disk_fsr.matchRoute("/v1/users").?.file_path, "(api)/v1/users.cp"), true);
    expect_eq(std.mem.eql(u8, disk_fsr.matchRoute("/v1/99").?.params.get("id").?, "99"), true);
}

fn testSqlPostgresDeep(): !void {
    let wire: [512]u8 = undefined;

    // 1. Extended Query Protocol encoders: Parse, Bind, Describe, Execute, Sync, Flush, Terminate
    const oids: [2]u32 = [23 as u32, 25 as u32]; // int4, text
    const p_len = PostgresWire.encodeParse(&wire, "s1", "SELECT $1, $2", oids[0..2]);
    expect_eq(wire[0], 'P' as u8);
    expect_eq(PostgresWire.parseFrameLength(wire[0..p_len]).?, (p_len - 1) as u32);

    let stmt = SqlStatement.prepare("SELECT $1, $2", SqlDialect.Postgres);
    expect_eq(stmt.bindInt(42), true);
    expect_eq(stmt.bindNull(), true);
    const fmts: [1]u16 = [0 as u16];
    const b_len = PostgresWire.encodeBind(&wire, "p1", "s1", fmts[0..1], stmt.params[0..stmt.param_count], fmts[0..1]);
    expect_eq(wire[0], 'B' as u8);
    expect_eq(PostgresWire.parseFrameLength(wire[0..b_len]).?, (b_len - 1) as u32);

    const d_len = PostgresWire.encodeDescribe(&wire, 'S', "s1");
    expect_eq(wire[0], 'D' as u8);
    expect_eq(wire[5], 'S' as u8);
    expect_eq(d_len, 9);

    const e_len = PostgresWire.encodeExecute(&wire, "p1", 100);
    expect_eq(wire[0], 'E' as u8);
    expect_eq(e_len, 12);

    expect_eq(PostgresWire.encodeSync(&wire), 5);
    expect_eq(wire[0], 'S' as u8);
    expect_eq(PostgresWire.encodeFlush(&wire), 5);
    expect_eq(wire[0], 'H' as u8);
    expect_eq(PostgresWire.encodeTerminate(&wire), 5);
    expect_eq(wire[0], 'X' as u8);

    // MD5 PasswordMessage
    const salt: [4]u8 = [0x11 as u8, 0x22, 0x33, 0x44];
    const md5_len = PostgresWire.encodeMD5Password(&wire, "postgres", "secret", salt);
    expect_eq(wire[0], 'p' as u8);
    expect_eq(std.mem.startsWith(u8, wire[5..md5_len], "md5"), true);

    // 2. SCRAM-SHA-256 full RFC 7677 client + server verification
    let scram = ScramSha256Client.init();
    let cf_buf: [128]u8 = undefined;
    const cf = try scram.buildClientFirst("postgres", "cNonce123", &cf_buf);
    expect_eq(std.mem.eql(u8, cf, "n,,n=postgres,r=cNonce123"), true);

    // Encode SASLInitialResponse
    const sasl_init_len = PostgresWire.encodeSASLInitialResponse(&wire, "SCRAM-SHA-256", cf);
    expect_eq(wire[0], 'p' as u8);
    expect_eq(sasl_init_len > 20, true);

    // Simulate server-first-message with base64("saltsalt") = "c2FsdHNhbHQ="
    const server_first = "r=cNonce123sNonce456,s=c2FsdHNhbHQ=,i=4096";
    let cfinal_buf: [256]u8 = undefined;
    const cfinal = try scram.buildClientFinal("my_password", server_first, &cfinal_buf);
    expect_eq(std.mem.startsWith(u8, cfinal, "c=biws,r=cNonce123sNonce456,p="), true);

    // Compute ServerSignature independently via Base64(expected_server_sig) and verify
    let sig_b64: [64]u8 = undefined;
    const sig_b64_len = Base64.encode(&sig_b64, &scram.expected_server_sig);
    let sf_buf: [96]u8 = undefined;
    const server_final = try std.fmt.bufPrint(&sf_buf, "v={s}", .{sig_b64[0..sig_b64_len]});
    expect_eq(scram.verifyServerFinal(server_final), true);
    expect_eq(scram.verifyServerFinal("v=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="), false);

    // 3. Backend Frame Decoding: ParameterStatus, BackendKeyData, RowDescription, ReadyForQuery, ErrorResponse
    // ParameterStatus ('S'): server_version = 16.2
    const param_frame = "S\x00\x00\x00\x18server_version\x0016.2\x00";
    const pf = PostgresBackendFrame.decode(param_frame).?;
    expect_eq(pf.tag == PostgresBackendTag.ParameterStatus, true);
    const ps = PostgresParameterStatus.parse(pf.payload).?;
    expect_eq(std.mem.eql(u8, ps.name, "server_version"), true);
    expect_eq(std.mem.eql(u8, ps.value, "16.2"), true);

    // BackendKeyData ('K'): pid=0x01020304, key=0x05060708
    const bkd_frame = "K\x00\x00\x00\x0C\x01\x02\x03\x04\x05\x06\x07\x08";
    const kf = PostgresBackendFrame.decode(bkd_frame).?;
    const bkd = PostgresBackendKeyData.parse(kf.payload).?;
    expect_eq(bkd.process_id, 0x01020304 as u32);
    expect_eq(bkd.secret_key, 0x05060708 as u32);

    // RowDescription ('T'): 1 field "id", type_oid=23, format=0
    const row_desc_payload = "\x00\x01id\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\x17\x00\x04\xFF\xFF\xFF\xFF\x00\x00";
    const rd = PostgresRowDescription.parse(row_desc_payload).?;
    expect_eq(rd.field_count, 1);
    expect_eq(rd.findField("id").?.type_oid, 23 as u32);

    // ReadyForQuery ('Z'): 'T'
    const rfq = PostgresReadyForQuery.parse("T").?;
    expect_eq(rfq.status == PostgresTransactionStatus.InTransaction, true);

    // ErrorResponse ('E'): Severity=ERROR, Code=42P01, Message=relation missing
    const err_payload = "SERROR\x00C42P01\x00Mrelation \"users\" does not exist\x00HCheck table name\x00\x00";
    const er = PostgresErrorResponse.parse(err_payload);
    expect_eq(std.mem.eql(u8, er.severity, "ERROR"), true);
    expect_eq(std.mem.eql(u8, er.sql_state, "42P01"), true);
    expect_eq(std.mem.eql(u8, er.message, "relation \"users\" does not exist"), true);
    expect_eq(std.mem.eql(u8, er.hint, "Check table name"), true);
}

fn testSqlMysqlPoolAndTx(): !void {
    // 1. MySqlLenEncStr & EOF_Packet
    let buf: [256]u8 = undefined;
    const s_len = MySqlLenEncStr.encode(&buf, "hello_mysql");
    const dec_str = MySqlLenEncStr.decode(buf[0..s_len]).?;
    expect_eq(dec_str.is_null, false);
    expect_eq(std.mem.eql(u8, dec_str.value, "hello_mysql"), true);

    const null_str = MySqlLenEncStr.decode("\xFB").?;
    expect_eq(null_str.is_null, true);

    const eof = MySqlEofPacket.parse("\xFE\x02\x00\x22\x00").?;
    expect_eq(eof.warnings, 2);
    expect_eq(eof.status_flags, 0x0022);

    // 2. MySqlHandshakeV10 parse & scramble copy
    const hs_payload =
        "\x0A8.0.36\x00" +
        "\x2A\x00\x00\x00" +
        "12345678" +
        "\x00" +
        "\x00\x82" +
        "\x21" +
        "\x02\x00" +
        "\x08\x00" +
        "\x15" +
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
        "abcdefghijkl\x00" +
        "mysql_native_password\x00";
    const hs = MySqlHandshakeV10.parse(hs_payload).?;
    expect_eq(hs.protocol_version, 10);
    expect_eq(std.mem.eql(u8, hs.server_version, "8.0.36"), true);
    expect_eq(hs.connection_id, 42 as u32);
    expect_eq(std.mem.eql(u8, hs.auth_plugin_name, "mysql_native_password"), true);
    let scramble: [20]u8 = undefined;
    expect_eq(hs.copyScramble(&scramble), 20);
    expect_eq(std.mem.eql(u8, scramble[0..20], "12345678abcdefghijkl"), true);

    // 3. MySqlCommandEncoder: COM_QUERY, COM_PING, COM_QUIT, HandshakeResponse41
    const q_len = MySqlCommandEncoder.encodeQuery(&buf, "SELECT 1", 0);
    expect_eq(MySqlPacketHeader.decode(buf[0..4]).?.payload_length, 9 as u32);
    expect_eq(buf[4], MySqlCommandType.COM_QUERY as u8);
    expect_eq(std.mem.eql(u8, buf[5..q_len], "SELECT 1"), true);

    expect_eq(MySqlCommandEncoder.encodePing(&buf, 0), 5);
    expect_eq(buf[4], MySqlCommandType.COM_PING as u8);
    expect_eq(MySqlCommandEncoder.encodeQuit(&buf, 0), 5);
    expect_eq(buf[4], MySqlCommandType.COM_QUIT as u8);

    const hr_len = MySqlCommandEncoder.encodeHandshakeResponse41(
        &buf,
        MySqlCapabilities.defaultClient().flags,
        16777216,
        33,
        "root",
        "auth20bytes",
        "app_db",
        "mysql_native_password",
        1,
    );
    const hr_hdr = MySqlPacketHeader.decode(buf[0..4]).?;
    expect_eq(hr_hdr.sequence_id, 1);
    expect_eq((hr_hdr.payload_length as usize) + 4, hr_len);

    // 4. SqlValue & SqlStatement bindFloat / bindBlob
    let st = SqlStatement.prepare("INSERT INTO t VALUES ($1, $2)", SqlDialect.Postgres);
    expect_eq(st.bindFloat(3.5), true);
    expect_eq(st.bindBlob("\x01\x02\x03"), true);
    const v0 = st.getParam(0).?.toValue();
    expect_eq(v0.typeTag() == SqlType.Float64, true);
    expect_eq(v0.asFloat().? > 3.4 && v0.asFloat().? < 3.6, true);
    expect_eq(std.mem.eql(u8, st.getParam(1).?.toValue().asBlob().?, "\x01\x02\x03"), true);

    // 5. SqlConnectionPool
    let pool = SqlConnectionPool.init(SqlDialect.Postgres, 4, 3000);
    expect_eq(pool.registerConnection(101, 1000, false), true);
    expect_eq(pool.registerConnection(102, 1000, false), true);
    const c_id = pool.acquire(2000).?;
    expect_eq(c_id, 102 as u64);
    expect_eq(pool.releaseConnection(c_id, 2500, true), true);
    // Evict stale idle connection (101 at 1000 -> 5000-1000=4000 > 3000; 102 at 2500 -> 5000-2500=2500 <= 3000)
    expect_eq(pool.evictIdle(5000), 1);
    expect_eq(pool.stats().idle, 1);
    expect_eq(pool.evictIdle(6000), 1);
    expect_eq(pool.stats().idle, 0);

    // 6. SqlTransactionStateMachine
    let tx = SqlTransactionStateMachine.init(SqlDialect.Postgres);
    let sql_buf: [128]u8 = undefined;
    const beg = try tx.begin(SqlIsolationLevel.Serializable, true, &sql_buf);
    expect_eq(std.mem.eql(u8, beg, "BEGIN ISOLATION LEVEL SERIALIZABLE READ ONLY"), true);
    expect_eq(tx.state == SqlTransactionState.InTransaction, true);

    const sp1 = try tx.savepoint("sp_one", &sql_buf);
    expect_eq(std.mem.eql(u8, sp1, "SAVEPOINT sp_one"), true);

    tx.markFailed();
    expect_eq(tx.state == SqlTransactionState.Failed, true);

    // Commit in Failed state must error
    let commit_err = false;
    * = tx.commit(&sql_buf) catch {
        commit_err = true;
    };
    expect_eq(commit_err, true);

    // Rollback to savepoint recovers InTransaction
    const rb_sp = try tx.rollbackTo("sp_one", &sql_buf);
    expect_eq(std.mem.eql(u8, rb_sp, "ROLLBACK TO SAVEPOINT sp_one"), true);
    expect_eq(tx.state == SqlTransactionState.InTransaction, true);

    expect_eq(std.mem.eql(u8, try tx.releaseSavepoint("sp_one", &sql_buf), "RELEASE SAVEPOINT sp_one"), true);
    expect_eq(std.mem.eql(u8, try tx.commit(&sql_buf), "COMMIT"), true);
    expect_eq(tx.state == SqlTransactionState.Idle, true);

    // Sync from Postgres ReadyForQuery
    tx.syncFromPostgresReady(PostgresTransactionStatus.Failed);
    expect_eq(tx.state == SqlTransactionState.Failed, true);
    expect_eq(std.mem.eql(u8, try tx.rollback(&sql_buf), "ROLLBACK"), true);
    expect_eq(tx.state == SqlTransactionState.Idle, true);
}

fn main() {
    test("1. cap:http — Chunked Transfer-Encoding streaming decoder & encoder with trailers", () => {
        try testHttpChunkedCodec();
    });
    test("2. cap:http — HttpConnectionPool keep-alive reuse, idle eviction, limits & health check", () => {
        try testHttpConnectionPool();
    });
    test("3. cap:http — RedirectPolicy 301/302/303/307/308, URL merge, loop & cross-origin strip", () => {
        try testHttpRedirectPolicy();
    });
    test("4. cap:http — ProxyTunnel Basic auth, NO_PROXY CIDR/suffix & Accept-Encoding + cap:pack", () => {
        try testHttpProxyAndEncoding();
    });
    test("5. cap:router — 5-tier Radix-Tree priority, hybrid segments, backtracking & conflict check", () => {
        try testRouterRadixDeep();
    });
    test("6. cap:router — FileSystemRouter [slug], [...all], [[...opt]], (group) & disk scan", () => {
        try testFileSystemRouter();
    });
    test("7. cap:sql — Postgres v3 Extended Query, SCRAM-SHA-256 & all Backend Frames", () => {
        try testSqlPostgresDeep();
    });
    test("8. cap:sql — MySQL HandshakeV10/Commands, SqlConnectionPool & SqlTransactionStateMachine", () => {
        try testSqlMysqlPoolAndTx();
    });
}
