// Regression Test for `cap:web` Native TLS Config, PEM Validation, ALPN, ClientHello SNI & WebServer TLS Sniff
import { App, Context, Response } from "../../lib/web/app.cp";
import { WebServer, ServerConfig } from "../../lib/web/server.cp";
import {
    TlsAlpnProto,
    TlsConfig,
    TlsConfigError,
    validatePemCertBytes,
    validatePemKeyBytes,
    isTlsClientHello,
    extractSniFromClientHello,
    negotiateAlpn,
    validateTlsFiles,
} from "../../lib/web/tls.cp";

const O_WRONLY_CREAT_TRUNC: i32 = 577; // O_WRONLY (1) | O_CREAT (64) | O_TRUNC (512)

@c struct TlsTestSockAddrIn {
    sin_family: u16,
    sin_port: u16,
    sin_addr: u32,
    sin_zero: [8]u8,
}

extern fn open(pathname: [*]const u8, flags: i32, mode: u32): i32;
extern fn unlink(pathname: [*]const u8): i32;
extern fn connect(fd: i32, addr: *const TlsTestSockAddrIn, addrlen: u32): i32;

fn htonsTls(v: u16): u16 {
    return ((v & 0x00FF) << 8) | ((v >> 8) & 0x00FF);
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn writeTempFile(path: string, content: string): bool {
    let cpath: [256]u8 = [0; 256];
    if (path.len == 0 || path.len >= 256) return false;
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        cpath[i] = path[i];
    }
    cpath[path.len] = 0;
    const fd = open(cpath[0..].ptr, O_WRONLY_CREAT_TRUNC, 420); // 0644
    if (fd < 0) return false;
    const nw = write(fd, content.ptr, content.len);
    * = close(fd);
    return nw == (content.len as isize);
}

fn removeTempFile(path: string) {
    let cpath: [256]u8 = [0; 256];
    if (path.len == 0 || path.len >= 256) return;
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        cpath[i] = path[i];
    }
    cpath[path.len] = 0;
    * = unlink(cpath[0..].ptr);
}

fn buildSampleClientHello(host: string, out: []u8): usize {
    // Build a valid TLS 1.2/1.3 ClientHello record with SNI extension (0x0000)
    const host_len = host.len;
    const sni_list_len = 3 + host_len;
    const sni_ext_body_len = 2 + sni_list_len;
    const exts_total_len = 4 + sni_ext_body_len;
    // ClientHello body: version(2) + random(32) + sid_len(1) + cs_len(2)+cs(2) + comp_len(1)+comp(1) + exts_len(2) + exts
    const ch_body_len = 2 + 32 + 1 + 4 + 2 + 2 + exts_total_len;
    const record_len = 4 + ch_body_len;

    let p: usize = 0;
    // TLS Record Header: Handshake (0x16), TLS 1.0/1.2 record version (0x03 0x01), length
    out[p] = 0x16; p += 1;
    out[p] = 0x03; p += 1;
    out[p] = 0x01; p += 1;
    out[p] = ((record_len >> 8) & 0xFF) as u8; p += 1;
    out[p] = (record_len & 0xFF) as u8; p += 1;

    // Handshake Header: ClientHello (0x01), 24-bit length
    out[p] = 0x01; p += 1;
    out[p] = ((ch_body_len >> 16) & 0xFF) as u8; p += 1;
    out[p] = ((ch_body_len >> 8) & 0xFF) as u8; p += 1;
    out[p] = (ch_body_len & 0xFF) as u8; p += 1;

    // ClientHello legacy_version = TLS 1.2 (0x0303)
    out[p] = 0x03; p += 1;
    out[p] = 0x03; p += 1;

    // 32-byte random
    let r: usize = 0;
    while (r < 32) : (r += 1) {
        out[p] = (r as u8) + 1;
        p += 1;
    }

    // legacy_session_id len = 0
    out[p] = 0; p += 1;

    // cipher_suites: len = 2, TLS_AES_128_GCM_SHA256 (0x1301)
    out[p] = 0x00; p += 1;
    out[p] = 0x02; p += 1;
    out[p] = 0x13; p += 1;
    out[p] = 0x01; p += 1;

    // compression_methods: len = 1, null (0x00)
    out[p] = 0x01; p += 1;
    out[p] = 0x00; p += 1;

    // extensions_length
    out[p] = ((exts_total_len >> 8) & 0xFF) as u8; p += 1;
    out[p] = (exts_total_len & 0xFF) as u8; p += 1;

    // Extension: server_name (0x0000)
    out[p] = 0x00; p += 1;
    out[p] = 0x00; p += 1;
    out[p] = ((sni_ext_body_len >> 8) & 0xFF) as u8; p += 1;
    out[p] = (sni_ext_body_len & 0xFF) as u8; p += 1;

    // server_name_list_length
    out[p] = ((sni_list_len >> 8) & 0xFF) as u8; p += 1;
    out[p] = (sni_list_len & 0xFF) as u8; p += 1;

    // host_name (0x00) + length + bytes
    out[p] = 0x00; p += 1;
    out[p] = ((host_len >> 8) & 0xFF) as u8; p += 1;
    out[p] = (host_len & 0xFF) as u8; p += 1;
    let hi: usize = 0;
    while (hi < host_len) : (hi += 1) {
        out[p] = host[hi];
        p += 1;
    }
    return p;
}

fn hRoot(ctx: *Context): Response {
    * = ctx;
    return Response.text(200, "ok");
}

fn main() {
    // 1. PEM Cert & Key header/footer validation
    const valid_cert = "-----BEGIN CERTIFICATE-----\nMIIBTESTCERTDATA\n-----END CERTIFICATE-----\n";
    const valid_pkcs8 = "-----BEGIN PRIVATE KEY-----\nMIIBVQIBADANBg==\n-----END PRIVATE KEY-----\n";
    const valid_rsa = "-----BEGIN RSA PRIVATE KEY-----\nMIIBOgIBAAJBAQ==\n-----END RSA PRIVATE KEY-----\n";
    const valid_ec = "-----BEGIN EC PRIVATE KEY-----\nMHcCAQEEIAAAAA==\n-----END EC PRIVATE KEY-----\n";
    const bad_order = "-----END CERTIFICATE-----\n-----BEGIN CERTIFICATE-----\n";
    const missing_end = "-----BEGIN CERTIFICATE-----\nMIIBTESTCERTDATA\n";

    if (!validatePemCertBytes(valid_cert)) @panic("valid cert rejected");
    if (validatePemCertBytes(bad_order)) @panic("reversed cert accepted");
    if (validatePemCertBytes(missing_end)) @panic("incomplete cert accepted");
    if (validatePemCertBytes("")) @panic("empty cert accepted");

    if (!validatePemKeyBytes(valid_pkcs8)) @panic("valid pkcs8 key rejected");
    if (!validatePemKeyBytes(valid_rsa)) @panic("valid rsa key rejected");
    if (!validatePemKeyBytes(valid_ec)) @panic("valid ec key rejected");
    if (validatePemKeyBytes(valid_cert)) @panic("cert mistakenly accepted as key");
    if (validatePemKeyBytes("")) @panic("empty key accepted");

    // 2. Real temp file PEM validation via validateTlsFiles
    const cert_file = "/tmp/cp_web_tls_test_cert.pem";
    const key_file = "/tmp/cp_web_tls_test_key.pem";
    const bad_file = "/tmp/cp_web_tls_test_bad.pem";
    if (!writeTempFile(cert_file, valid_cert)) @panic("failed to write temp cert");
    if (!writeTempFile(key_file, valid_pkcs8)) @panic("failed to write temp key");
    if (!writeTempFile(bad_file, "NOT A PEM FILE\n")) @panic("failed to write temp bad file");

    if (validateTlsFiles(TlsConfig{ enabled: true, cert_path: cert_file, key_path: key_file, min_version: 0x0303, alpn: TlsAlpnProto.Auto }) != TlsConfigError.Ok) {
        @panic("validateTlsFiles failed on valid TLS 1.2 config");
    }
    if (validateTlsFiles(TlsConfig{ enabled: true, cert_path: cert_file, key_path: key_file, min_version: 0x0304, alpn: TlsAlpnProto.Http2 }) != TlsConfigError.Ok) {
        @panic("validateTlsFiles failed on valid TLS 1.3 config");
    }
    if (validateTlsFiles(TlsConfig{ enabled: true, cert_path: "", key_path: key_file }) != TlsConfigError.MissingCertPath) {
        @panic("expected MissingCertPath");
    }
    if (validateTlsFiles(TlsConfig{ enabled: true, cert_path: cert_file, key_path: "" }) != TlsConfigError.MissingKeyPath) {
        @panic("expected MissingKeyPath");
    }
    if (validateTlsFiles(TlsConfig{ enabled: true, cert_path: "/tmp/nonexistent_cp_tls_cert.pem", key_path: key_file }) != TlsConfigError.FileNotFound) {
        @panic("expected FileNotFound");
    }
    if (validateTlsFiles(TlsConfig{ enabled: true, cert_path: bad_file, key_path: key_file }) != TlsConfigError.InvalidPemHeader) {
        @panic("expected InvalidPemHeader for bad cert");
    }
    if (validateTlsFiles(TlsConfig{ enabled: true, cert_path: cert_file, key_path: bad_file }) != TlsConfigError.InvalidPemHeader) {
        @panic("expected InvalidPemHeader for bad key");
    }
    if (validateTlsFiles(TlsConfig{ enabled: true, cert_path: cert_file, key_path: key_file, min_version: 0x0301 }) != TlsConfigError.UnsupportedVersion) {
        @panic("expected UnsupportedVersion for TLS 1.0");
    }

    // 3. TLS ClientHello detection & zero-alloc SNI extraction
    let ch_buf: [256]u8 = [0; 256];
    const ch_len = buildSampleClientHello("api.cp-lang.dev", ch_buf[0..]);
    const ch_slice = ch_buf[0..ch_len];

    if (!isTlsClientHello(ch_slice)) @panic("valid ClientHello not detected");
    if (isTlsClientHello("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")) @panic("HTTP GET misidentified as ClientHello");
    if (isTlsClientHello("short")) @panic("short buffer misidentified as ClientHello");

    let sni_out: [64]u8 = [0; 64];
    if (sni_len := extractSniFromClientHello(ch_slice, sni_out[0..])) {
        if (!strEq(sni_out[0..sni_len], "api.cp-lang.dev")) {
            @panic("extracted SNI mismatch");
        }
    } else {
        @panic("SNI extraction returned null");
    }

    // Truncated record & undersized output buffer bounds safety
    if (extractSniFromClientHello(ch_buf[0..20], sni_out[0..]) != null) {
        @panic("truncated ClientHello should return null");
    }
    let tiny_out: [4]u8 = [0; 4];
    if (extractSniFromClientHello(ch_slice, tiny_out[0..]) != null) {
        @panic("undersized out_buf should return null");
    }

    // 4. ALPN negotiation
    if (!strEq(negotiateAlpn("http/1.1, h2", TlsAlpnProto.Auto), "h2")) @panic("ALPN Auto should prefer h2");
    if (!strEq(negotiateAlpn("http/1.1", TlsAlpnProto.Auto), "http/1.1")) @panic("ALPN Auto should fallback to http/1.1");
    if (!strEq(negotiateAlpn("h2,http/1.1", TlsAlpnProto.Http11), "http/1.1")) @panic("ALPN Http11 mismatch");
    if (!strEq(negotiateAlpn("http/1.1", TlsAlpnProto.Http2), "")) @panic("ALPN Http2 should fail when absent");
    if (!strEq(negotiateAlpn("spdy/3", TlsAlpnProto.Auto), "")) @panic("ALPN unknown proto should return empty");

    // 5. WebServer.isTlsEnabled() & validateTlsConfig()
    let plain_srv = WebServer.init(ServerConfig{ port: 18931, workers: 1 });
    if (plain_srv.isTlsEnabled()) @panic("plaintext server should have isTlsEnabled() == false");
    if (plain_srv.validateTlsConfig()) @panic("plaintext server without paths should fail validateTlsConfig()");

    let tls_srv = WebServer.init(ServerConfig{
        port: 18932,
        workers: 1,
        backlog: 64,
        shutdown_grace_ms: 50,
        tls_cert_path: cert_file,
        tls_key_path: key_file,
        tls_alpn: "h2,http/1.1",
    });
    if (!tls_srv.isTlsEnabled()) @panic("TLS server should have isTlsEnabled() == true");
    if (!tls_srv.validateTlsConfig()) @panic("TLS server with valid temp PEM files should pass validateTlsConfig()");

    // 6. Live socket ClientHello sniff -> graceful TLS Fatal Alert response
    let app = App.init();
    * = app.get("/", hRoot);
    if (!tls_srv.start(*app)) @panic("failed to start tls_srv");

    const cfd = socket(2, 1, 0);
    let addr = TlsTestSockAddrIn{
        sin_family: 2,
        sin_port: htonsTls(18932),
        sin_addr: 0x0100007F,
        sin_zero: [0; 8],
    };
    if (cfd < 0 || connect(cfd, *addr, 16) < 0) {
        tls_srv.stop();
        @panic("failed to connect to tls_srv");
    }
    * = write(cfd, ch_slice.ptr, ch_slice.len);
    let resp_buf: [64]u8 = [0; 64];
    const nr = read(cfd, &resp_buf[0], 64);
    * = close(cfd);
    tls_srv.stop();

    removeTempFile(cert_file);
    removeTempFile(key_file);
    removeTempFile(bad_file);

    // Expect standard 7-byte TLS Fatal Alert (0x15, 0x03, 0x03, 0x00, 0x02, 0x02, 0x28)
    if (nr != 7 || resp_buf[0] != 0x15 || resp_buf[1] != 0x03 || resp_buf[2] != 0x03 || resp_buf[5] != 0x02) {
        @panic("expected 7-byte TLS Fatal Alert response on ClientHello sniff");
    }

    @print("PASS testWebTlsConfig (PEM + TempFile + ClientHello + ZeroAlloc SNI + ALPN + WebServer TLS 6/6)");
}
