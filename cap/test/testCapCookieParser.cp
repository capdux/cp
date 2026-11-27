import { test, expect_eq } from "cap:test";
import {
    isSignedCookie, isJsonCookie, stripJsonCookiePrefix,
    signCookieValue, signedCookie, signedCookieWithSecrets,
    lookupRawCookie, parseCookieHeader, parseCookieHeaderWithSecrets
} from "../../packages/cookie-parser/src/index.cp";

fn main() {
    test("1. @cap/cookie-parser — unsigned cookie header parsing & lookupRawCookie", () => {
        let hdr = "theme=dark; lang=\"zh-CN\"; session=abc123";
        let parsed = parseCookieHeader(hdr, "");
        expect_eq(parsed.cookie_count, 3);
        expect_eq(parsed.signed_count, 0);
        expect_eq(parsed.get("theme") ?? "", "dark");
        expect_eq(parsed.get("lang") ?? "", "zh-CN");
        expect_eq(lookupRawCookie(hdr, "session") ?? "", "abc123");
    });

    test("2. @cap/cookie-parser — signCookieValue & signedCookie roundtrip", () => {
        let buf: [128]u8 = [0; 128];
        let n = signCookieValue("user_42", "top-secret", buf[0..128]);
        let signed_str = buf[0..n] as string;
        expect_eq(isSignedCookie(signed_str), true);
        expect_eq(signedCookie(signed_str, "top-secret") ?? "", "user_42");
        expect_eq(signedCookie(signed_str, "wrong-secret") == null, true);
        expect_eq(signedCookie("plain_val", "top-secret") ?? "", "plain_val");
    });

    test("3. @cap/cookie-parser — parseCookieHeader splits unsigned vs verified signed cookies", () => {
        let buf: [128]u8 = [0; 128];
        let n = signCookieValue("admin", "k1", buf[0..128]);
        let s_tok = buf[0..n] as string;
        let hdr_buf: [256]u8 = [0; 256];
        let prefix = "mode=fast; uid=";
        let pos: usize = 0;
        let i: usize = 0;
        while (i < prefix.len) { hdr_buf[pos] = prefix[i]; pos = pos + 1; i = i + 1; }
        i = 0;
        while (i < s_tok.len) { hdr_buf[pos] = s_tok[i]; pos = pos + 1; i = i + 1; }
        let suffix = "; forged=s:admin.bad_sig";
        i = 0;
        while (i < suffix.len) { hdr_buf[pos] = suffix[i]; pos = pos + 1; i = i + 1; }

        let parsed = parseCookieHeader(hdr_buf[0..pos] as string, "k1");
        expect_eq(parsed.cookie_count, 1);
        expect_eq(parsed.signed_count, 1);
        expect_eq(parsed.get("mode") ?? "", "fast");
        expect_eq(parsed.getSigned("uid") ?? "", "admin");
        expect_eq(parsed.getSigned("forged") == null, true);
        expect_eq(parsed.totalCount(), 2);
    });

    test("4. @cap/cookie-parser — multi-secret key rotation via signedCookieWithSecrets", () => {
        let buf: [128]u8 = [0; 128];
        let n = signCookieValue("tok99", "old-key", buf[0..128]);
        let s_tok = buf[0..n] as string;
        let secrets: [4]string = ["new-key", "old-key", "", ""];
        expect_eq(signedCookieWithSecrets(s_tok, secrets, 2) ?? "", "tok99");
        let bad_secrets: [4]string = ["new-key", "other", "", ""];
        expect_eq(signedCookieWithSecrets(s_tok, bad_secrets, 2) == null, true);
    });

    test("5. @cap/cookie-parser — parseCookieHeaderWithSecrets verifies rotated header", () => {
        let buf: [128]u8 = [0; 128];
        let n = signCookieValue("sid777", "rot-2", buf[0..128]);
        let hdr_buf: [160]u8 = [0; 160];
        let pre = "sid=";
        let pos: usize = 0;
        let i: usize = 0;
        while (i < pre.len) { hdr_buf[pos] = pre[i]; pos = pos + 1; i = i + 1; }
        i = 0;
        while (i < n) { hdr_buf[pos] = buf[i]; pos = pos + 1; i = i + 1; }

        let secrets: [4]string = ["rot-1", "rot-2", "", ""];
        let parsed = parseCookieHeaderWithSecrets(hdr_buf[0..pos] as string, secrets, 2);
        expect_eq(parsed.signed_count, 1);
        expect_eq(parsed.getSigned("sid") ?? "", "sid777");
    });

    test("6. @cap/cookie-parser — JSON cookie prefix (j:) detection & extraction", () => {
        expect_eq(isJsonCookie("j:{\"role\":\"pro\"}"), true);
        expect_eq(isJsonCookie("plain"), false);
        expect_eq(stripJsonCookiePrefix("j:{\"a\":1}") ?? "", "{\"a\":1}");
        expect_eq(stripJsonCookiePrefix("plain") == null, true);
    });
}
