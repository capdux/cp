// cap/test/testCapPemRfc7468.cp
// Comprehensive unit tests for @cap/pem-rfc7468

import { test, expect_eq } from "cap:test";
import {
    PEM_LINE_WIDTH,
    PEM_ERR_INVALID_LABEL,
    PEM_ERR_BUFFER_TOO_SMALL,
    PEM_ERR_MISSING_BEGIN,
    PEM_ERR_MISSING_END,
    PEM_ERR_LABEL_MISMATCH,
    PEM_ERR_INVALID_BASE64,
    PEM_ERR_INVALID_HEADER,
    isValidLabel,
    pemEncodedLen,
    encodePem,
    decodePem,
    decodeLabel,
    locatePem,
} from "../../packages/pem-rfc7468/src/index.cp";

fn pemEqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/pem-rfc7468 — Standard CERTIFICATE PEM encoding & 64-col wrapping", () => {
        // 60 bytes of DER -> 80 Base64 chars -> 2 lines (64 + 16)
        let der: [60]u8 = [0; 60];
        let i: usize = 0;
        while (i < 60) {
            der[i] = ((i * 7 + 3) & 0xFF) as u8;
            i = i + 1;
        }
        const expected_len = pemEncodedLen(11, 60, false);
        let out: [256]u8 = [0; 256];
        const written = encodePem("CERTIFICATE", der[0..60], false, out[0..256]);
        expect_eq(written, expected_len as i64);

        // Verify header starts with "-----BEGIN CERTIFICATE-----\n" (28 bytes)
        expect_eq(pemEqBytes(out[0..28], "-----BEGIN CERTIFICATE-----\n"), true);
        // First Base64 line must be exactly 64 chars followed by '\n' at index 28 + 64 = 92
        expect_eq(out[28 + PEM_LINE_WIDTH], '\n' as u8);
        // Second Base64 line is 16 chars followed by '\n' at index 93 + 16 = 109
        expect_eq(out[93 + 16], '\n' as u8);
        // Footer starts at index 110: "-----END CERTIFICATE-----\n" (26 bytes)
        expect_eq(pemEqBytes(out[110..136], "-----END CERTIFICATE-----\n"), true);
        expect_eq(written, 136 as i64);
    });

    test("2. @cap/pem-rfc7468 — RFC 7468 round-trip encode & decode", () => {
        let der: [73]u8 = [0; 73];
        let i: usize = 0;
        while (i < 73) {
            der[i] = ((i * 13 + 0x30) & 0xFF) as u8;
            i = i + 1;
        }
        let pem_buf: [512]u8 = [0; 512];
        const pem_len = encodePem("PUBLIC KEY", der[0..73], false, pem_buf[0..512]);
        expect_eq(pem_len > 0, true);

        let label_out: [64]u8 = [0; 64];
        let label_len: usize = 0;
        let der_out: [128]u8 = [0; 128];
        const dec_len = decodePem(
            pem_buf[0..(pem_len as usize)],
            label_out[0..64],
            &label_len,
            der_out[0..128],
        );
        expect_eq(dec_len, 73 as i64);
        expect_eq(label_len, 10 as usize);
        expect_eq(pemEqBytes(label_out[0..label_len], "PUBLIC KEY"), true);
        expect_eq(pemEqBytes(der_out[0..73], der[0..73]), true);
    });

    test("3. @cap/pem-rfc7468 — LF vs CRLF line endings", () => {
        const msg = "Hello RFC 7468!";
        const lf_len = pemEncodedLen(7, msg.len, false);
        const crlf_len = pemEncodedLen(7, msg.len, true);
        // 3 lines total (BEGIN + 1 body line + END) -> CRLF adds 3 bytes
        expect_eq(crlf_len, lf_len + 3);

        let lf_buf: [128]u8 = [0; 128];
        let crlf_buf: [128]u8 = [0; 128];
        const w_lf = encodePem("PRIVATE", msg, false, lf_buf[0..128]);
        const w_crlf = encodePem("PRIVATE", msg, true, crlf_buf[0..128]);
        expect_eq(w_lf, lf_len as i64);
        expect_eq(w_crlf, crlf_len as i64);

        // Verify CRLF bytes at end of BEGIN line: "-----BEGIN PRIVATE-----\r\n"
        expect_eq(crlf_buf[23], '\r' as u8);
        expect_eq(crlf_buf[24], '\n' as u8);

        // Both LF and CRLF decode back to identical payload
        let label_out: [32]u8 = [0; 32];
        let label_len: usize = 0;
        let der_out: [64]u8 = [0; 64];
        const d_crlf = decodePem(
            crlf_buf[0..(w_crlf as usize)],
            label_out[0..32],
            &label_len,
            der_out[0..64],
        );
        expect_eq(d_crlf, msg.len as i64);
        expect_eq(pemEqBytes(der_out[0..(d_crlf as usize)], msg), true);
    });

    test("4. @cap/pem-rfc7468 — decodeLabel fast extraction & isValidLabel", () => {
        expect_eq(isValidLabel("CERTIFICATE"), true);
        expect_eq(isValidLabel("PUBLIC KEY"), true);
        expect_eq(isValidLabel("X509 CRL"), true);
        expect_eq(isValidLabel("EC-PRIVATE KEY"), true);
        expect_eq(isValidLabel(""), false);
        expect_eq(isValidLabel(" LEADING"), false);
        expect_eq(isValidLabel("TRAILING "), false);
        expect_eq(isValidLabel("DOUBLE  SPACE"), false);
        expect_eq(isValidLabel("DOUBLE--DASH"), false);
        expect_eq(isValidLabel("BAD\nLABEL"), false);

        const sample = "Bag Attributes\n    friendlyName: demo\n-----BEGIN EC PRIVATE KEY-----\nAAAA\n-----END EC PRIVATE KEY-----\n";
        let lbl: [64]u8 = [0; 64];
        const n = decodeLabel(sample, lbl[0..64]);
        expect_eq(n, 14 as i64);
        expect_eq(pemEqBytes(lbl[0..14], "EC PRIVATE KEY"), true);
    });

    test("5. @cap/pem-rfc7468 — Reject mismatched labels, malformed headers/footers & bad Base64", () => {
        let lbl: [64]u8 = [0; 64];
        let lbl_len: usize = 0;
        let der: [64]u8 = [0; 64];

        // Mismatched BEGIN/END labels
        const mismatch = "-----BEGIN CERTIFICATE-----\nSGVsbG8=\n-----END PUBLIC KEY-----\n";
        expect_eq(decodePem(mismatch, lbl[0..64], &lbl_len, der[0..64]), PEM_ERR_LABEL_MISMATCH);

        // Missing BEGIN
        expect_eq(decodePem("just some text\n", lbl[0..64], &lbl_len, der[0..64]), PEM_ERR_MISSING_BEGIN);

        // Missing END
        const no_end = "-----BEGIN CERTIFICATE-----\nSGVsbG8=\n";
        expect_eq(decodePem(no_end, lbl[0..64], &lbl_len, der[0..64]), PEM_ERR_MISSING_END);

        // Malformed header (missing closing dashes)
        const bad_hdr = "-----BEGIN CERTIFICATE---\nSGVsbG8=\n-----END CERTIFICATE-----\n";
        expect_eq(decodePem(bad_hdr, lbl[0..64], &lbl_len, der[0..64]), PEM_ERR_INVALID_HEADER);

        // Invalid Base64 character
        const bad_b64 = "-----BEGIN TEST-----\nSGVs!G8=\n-----END TEST-----\n";
        expect_eq(decodePem(bad_b64, lbl[0..64], &lbl_len, der[0..64]), PEM_ERR_INVALID_BASE64);

        // Invalid label on encode & buffer too small
        let small: [4]u8 = [0; 4];
        expect_eq(encodePem("BAD  LABEL", "abc", false, small[0..4]), PEM_ERR_INVALID_LABEL);
        expect_eq(encodePem("TEST", "abc", false, small[0..4]), PEM_ERR_BUFFER_TOO_SMALL);
    });

    test("6. @cap/pem-rfc7468 — Leading commentary skipping, empty DER & exact encodedLen", () => {
        // Leading explanatory commentary before BEGIN block
        const with_comments = "Subject: CN=example.com\nIssuer: CN=CA\n\n-----BEGIN MESSAGE-----\nSGVsbG8s\nIFBFTSE=\n-----END MESSAGE-----\ntrailing notes\n";
        const loc = locatePem(with_comments);
        expect_eq(loc.isOk(), true);

        let lbl: [32]u8 = [0; 32];
        let lbl_len: usize = 0;
        let der: [64]u8 = [0; 64];
        const dec_len = decodePem(with_comments, lbl[0..32], &lbl_len, der[0..64]);
        expect_eq(dec_len, 11 as i64);
        expect_eq(pemEqBytes(lbl[0..lbl_len], "MESSAGE"), true);
        expect_eq(pemEqBytes(der[0..11], "Hello, PEM!"), true);

        // Empty DER payload encodedLen & round-trip
        const empty_len = pemEncodedLen(5, 0, false);
        // "-----BEGIN EMPTY-----\n" (22) + "-----END EMPTY-----\n" (20) = 42
        expect_eq(empty_len, 42 as usize);
        let empty_buf: [64]u8 = [0; 64];
        let empty_der: [1]u8 = [0; 1];
        const w_empty = encodePem("EMPTY", empty_der[0..0], false, empty_buf[0..64]);
        expect_eq(w_empty, 42 as i64);
        const d_empty = decodePem(empty_buf[0..42], lbl[0..32], &lbl_len, der[0..64]);
        expect_eq(d_empty, 0 as i64);
    });
}
