import { test, expect_eq } from "cap:test";
import {
    parseProxyUrl,
    buildConnectRequest,
} from "../../packages/@cap/https-proxy-agent/src/index.cp";

fn main() {
    test("1. @cap/https-proxy-agent — parses HTTP proxy URL with credentials and port", () => {
        const ep = parseProxyUrl("http://alice:secret@proxy.internal:8080/");
        expect_eq(ep.valid, true);
        expect_eq(ep.scheme, "http");
        expect_eq(ep.is_tls, false);
        expect_eq(ep.host, "proxy.internal");
        expect_eq(ep.port, 8080 as u16);
        expect_eq(ep.has_auth, true);
        expect_eq(ep.username, "alice");
        expect_eq(ep.password, "secret");
    });

    test("2. @cap/https-proxy-agent — parses HTTPS proxy with default port 443", () => {
        const ep = parseProxyUrl("https://secure.proxy.dev");
        expect_eq(ep.valid, true);
        expect_eq(ep.is_tls, true);
        expect_eq(ep.host, "secure.proxy.dev");
        expect_eq(ep.port, 443 as u16);
        expect_eq(ep.has_auth, false);
    });

    test("3. @cap/https-proxy-agent — parses bracketed IPv6 proxy host", () => {
        const ep = parseProxyUrl("http://[::1]:3128");
        expect_eq(ep.valid, true);
        expect_eq(ep.host, "::1");
        expect_eq(ep.port, 3128 as u16);
    });

    test("4. @cap/https-proxy-agent — builds RFC 9110 CONNECT request without auth", () => {
        const ep = parseProxyUrl("http://proxy.local:8080");
        let buf: [256]u8 = [0; 256];
        const n = buildConnectRequest("api.example.com", 443, ep, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "CONNECT api.example.com:443 HTTP/1.1\r\nHost: api.example.com:443\r\nConnection: keep-alive\r\n\r\n",
        );
    });

    test("5. @cap/https-proxy-agent — builds CONNECT request with Basic Proxy-Authorization", () => {
        // "user:pass" -> base64 "dXNlcjpwYXNz"
        const ep = parseProxyUrl("http://user:pass@proxy.local:8080");
        let buf: [256]u8 = [0; 256];
        const n = buildConnectRequest("example.com", 443, ep, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "CONNECT example.com:443 HTTP/1.1\r\nHost: example.com:443\r\nProxy-Authorization: Basic dXNlcjpwYXNz\r\nConnection: keep-alive\r\n\r\n",
        );
    });

    test("6. @cap/https-proxy-agent — rejects malformed proxy URLs and invalid ports", () => {
        expect_eq(parseProxyUrl("socks5://127.0.0.1:1080").valid, false);
        expect_eq(parseProxyUrl("http://").valid, false);
        expect_eq(parseProxyUrl("http://host:99999").valid, false);
        expect_eq(parseProxyUrl("http://[::1").valid, false);
    });
}
