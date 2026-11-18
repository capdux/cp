import { test, expect_eq } from "cap:test";
import { URL, URLSearchParams, QueryParam, Scanner, PercentEncoding } from "cap:url";
const std = @import("std");

fn main() {
    test("1. URL.parse: Comprehensive WHATWG protocol & component decomposition", () => {
        // 1.1 Full HTTPS URL with auth, port, path, query and fragment
        let u1 = URL.parse(
            "https://dev_user:secret123@api.service.io:8443/v2/items/query?category=ai&sort=asc#results",
            std.heap.page_allocator
        ) catch unreachable;
        defer u1.deinit();

        expect_eq(u1.protocol, "https:");
        expect_eq(u1.username, "dev_user");
        expect_eq(u1.password, "secret123");
        expect_eq(u1.host, "api.service.io:8443");
        expect_eq(u1.hostname, "api.service.io");
        expect_eq(u1.port, "8443");
        expect_eq(u1.pathname, "/v2/items/query");
        expect_eq(u1.path, "/v2/items/query");
        expect_eq(u1.search, "?category=ai&sort=asc");
        expect_eq(u1.hash, "#results");
        expect_eq(u1.origin, "https://api.service.io:8443");

        expect_eq(u1.isHTTPS(), true);
        expect_eq(u1.isHTTP(), false);
        expect_eq(u1.isFile(), false);
        expect_eq(u1.hasHTTPLikeProtocol(), true);
        expect_eq(u1.hasValidPort(), true);
        expect_eq(u1.getPort() != null, true);
        if (p := u1.getPort()) {
            expect_eq(p, 8443 as u16);
        }
        expect_eq(u1.getPortAuto(), 8443 as u16);
        expect_eq(u1.getDefaultPort(), 443 as u16);
        expect_eq(u1.displayProtocol(), "https");
        expect_eq(u1.displayHostname(), "api.service.io");

        // 1.2 Default port fallback
        let u1_def = URL.parse("http://example.com/index.html", null) catch unreachable;
        expect_eq(u1_def.port, "");
        expect_eq(u1_def.getPort() == null, true);
        expect_eq(u1_def.getPortAuto(), 80 as u16);
        expect_eq(u1_def.getDefaultPort(), 80 as u16);
        expect_eq(u1_def.displayProtocol(), "http");
    });

    test("2. URL: IPv6 bracketed hosts, localhost, and IP address detection", () => {
        // 2.1 IPv6 address with port
        let u_v6 = URL.parse("http://[::1]:9090/metrics", null) catch unreachable;
        expect_eq(u_v6.protocol, "http:");
        expect_eq(u_v6.host, "[::1]:9090");
        expect_eq(u_v6.hostname, "[::1]");
        expect_eq(u_v6.port, "9090");
        expect_eq(u_v6.pathname, "/metrics");
        expect_eq(u_v6.isIPAddress(), true);
        expect_eq(u_v6.isLocalhost(), true);

        // 2.2 IPv4 address
        let u_v4 = URL.parse("http://192.168.1.100:3000/", null) catch unreachable;
        expect_eq(u_v4.hostname, "192.168.1.100");
        expect_eq(u_v4.isIPAddress(), true);
        expect_eq(u_v4.isLocalhost(), false);

        // 2.3 Localhost domain
        let u_lh = URL.parse("http://localhost:8080/app", null) catch unreachable;
        expect_eq(u_lh.hostname, "localhost");
        expect_eq(u_lh.isLocalhost(), true);
        expect_eq(u_lh.isIPAddress(), false);
    });

    test("3. URL: File, S3, Blob, and Unix special protocols", () => {
        // 3.1 file:// protocol
        let u_file = URL.parse("file:///data/local/config.json", null) catch unreachable;
        expect_eq(u_file.isFile(), true);
        expect_eq(u_file.pathname, "/data/local/config.json");
        expect_eq(u_file.origin, "null");

        // 3.2 s3:// protocol & s3Path()
        let u_s3 = URL.parse("s3://backup-vault/prod/2026/db.tar.gz?versionId=v1#summary", null) catch unreachable;
        expect_eq(u_s3.isS3(), true);
        expect_eq(u_s3.host, "backup-vault");
        expect_eq(u_s3.pathname, "/prod/2026/db.tar.gz");
        expect_eq(u_s3.s3Path(), "backup-vault/prod/2026/db.tar.gz");

        // 3.3 blob: protocol
        let u_blob = URL.parse("blob:https://example.com/c4b8-uuid", null) catch unreachable;
        expect_eq(u_blob.isBlob(), true);

        // 3.4 unix: protocol
        let u_unix = URL.parse("unix:///var/run/docker.sock", null) catch unreachable;
        expect_eq(u_unix.isUnix(), true);
    });

    test("4. URL: Path normalization via joinNormalize", () => {
        let buf: [256]u8 = undefined;

        // prefix + dirname + basename + extname
        let res1 = URL.joinNormalize(buf[0..256], "api", "v1/users", "profile", ".json");
        expect_eq(res1, "/api/v1/users/profile.json");

        // edge case: leading slashes
        let res2 = URL.joinNormalize(buf[0..256], "/assets", "/images", "logo", ".png");
        expect_eq(res2, "/assets/images/logo.png");

        // empty parts fallback
        let res3 = URL.joinNormalize(buf[0..256], "", "", "", "");
        expect_eq(res3, "/");
    });

    test("5. URLSearchParams: Scanner tokenizer, CRUD operations & in-place sorting", () => {
        // 5.1 Scanner zero-allocation query string tokenization
        let scanner = Scanner.init("?user=bob&action=read&tag=ai%20ml&active");
        let item1 = scanner.next();
        expect_eq(item1 != null, true);
        if (it1 := item1) {
            expect_eq(it1.name, "user");
            expect_eq(it1.value, "bob");
            expect_eq(it1.name_needs_decoding, false);
            expect_eq(it1.value_needs_decoding, false);
        }

        let item2 = scanner.next();
        expect_eq(item2 != null, true);
        if (it2 := item2) {
            expect_eq(it2.name, "action");
            expect_eq(it2.value, "read");
        }

        let item3 = scanner.next();
        expect_eq(item3 != null, true);
        if (it3 := item3) {
            expect_eq(it3.name, "tag");
            expect_eq(it3.value, "ai%20ml");
            expect_eq(it3.value_needs_decoding, true);
        }

        let item4 = scanner.next();
        expect_eq(item4 != null, true);
        if (it4 := item4) {
            expect_eq(it4.name, "active");
            expect_eq(it4.value, "");
        }

        expect_eq(scanner.next() == null, true);

        // 5.2 URLSearchParams parsing and CRUD
        let sp = URLSearchParams.parse("?foo=1&bar=2&foo=3", std.heap.page_allocator);
        defer sp.deinit();

        expect_eq(sp.count(), 3 as usize);
        expect_eq(sp.has("foo"), true);
        expect_eq(sp.has("unknown"), false);

        // First matching value for get
        if (val := sp.get("foo")) {
            expect_eq(val, "1");
        }

        // getAll for multiple keys
        let target_buf: [4]string = undefined;
        let found = sp.getAll("foo", target_buf[0..4]);
        expect_eq(found, 2 as usize);
        expect_eq(target_buf[0], "1");
        expect_eq(target_buf[1], "3");

        // append, set, delete
        sp.append("baz", "4");
        expect_eq(sp.count(), 4 as usize);

        sp.set("foo", "updated");
        if (v := sp.get("foo")) {
            expect_eq(v, "updated");
        }

        sp.delete("bar");
        expect_eq(sp.has("bar"), false);

        // In-place lexicographical sort
        // Currently keys are "foo", "foo" (3), "baz" -> after sort: "baz", "foo", "foo"
        sp.sort();
        let out_buf: [128]u8 = undefined;
        let serialized = sp.toString(out_buf[0..128]);
        expect_eq(serialized, "baz=4&foo=updated&foo=3");
    });

    test("6. PercentEncoding: RFC 3986 component encoding & decoding", () => {
        // Unreserved chars: A-Z, a-z, 0-9, -, ., _, ~
        expect_eq(PercentEncoding.isUnreserved(65 as u8), true); // 'A'
        expect_eq(PercentEncoding.isUnreserved(45 as u8), true); // '-'
        expect_eq(PercentEncoding.isUnreserved(32 as u8), false); // ' '
        expect_eq(PercentEncoding.isUnreserved(63 as u8), false); // '?'

        // Needs encode
        expect_eq(PercentEncoding.needsEncodeComponent("simple-name_123"), false);
        expect_eq(PercentEncoding.needsEncodeComponent("hello world!"), true);

        // encodeComponentToBuf
        let enc_buf: [64]u8 = undefined;
        let enc_len = PercentEncoding.encodeComponentToBuf("hello world@cp#1", enc_buf[0..64]);
        expect_eq(enc_buf[0..enc_len], "hello%20world%40cp%231");

        // decodeToBuf
        let dec_buf: [64]u8 = undefined;
        let dec_len = PercentEncoding.decodeToBuf("hello%20world%40cp%231", true, dec_buf[0..64]);
        expect_eq(dec_buf[0..dec_len], "hello world@cp#1");

        // decodeToBuf with plus_as_space
        let plus_len = PercentEncoding.decodeToBuf("query+string+test", true, dec_buf[0..64]);
        expect_eq(dec_buf[0..plus_len], "query string test");

        // Dynamic heap allocation helpers
        let heap_enc = PercentEncoding.encodeComponent("deep/mind", std.heap.page_allocator) catch unreachable;
        defer std.heap.page_allocator.free(heap_enc);
        expect_eq(heap_enc, "deep%2Fmind");

        let heap_dec = PercentEncoding.decodeAlloc(heap_enc, false, std.heap.page_allocator) catch unreachable;
        defer std.heap.page_allocator.free(heap_dec);
        expect_eq(heap_dec, "deep/mind");
    });
}
