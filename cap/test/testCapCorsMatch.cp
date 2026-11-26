import { test, expect_eq } from "cap:test";
import {
    isValidOrigin,
    matchOrigin,
    matchOriginList,
    isMethodAllowed,
    areHeadersAllowed,
    isSimpleMethod,
} from "../../packages/@cap/cors-match/src/index.cp";

fn main() {
    test("1. @cap/cors-match — origin syntax validation", () => {
        expect_eq(isValidOrigin("https://example.com"), true);
        expect_eq(isValidOrigin("http://localhost:3000"), true);
        expect_eq(isValidOrigin("https://example.com/path"), false);
        expect_eq(isValidOrigin("example.com"), false);
        expect_eq(isValidOrigin(""), false);
    });

    test("2. @cap/cors-match — exact & wildcard '*' with credentials guard", () => {
        expect_eq(matchOrigin("https://App.Example.com", "https://app.example.com", true), true);
        expect_eq(matchOrigin("https://any.dev", "*", false), true);
        // Fetch spec forbids '*' when Access-Control-Allow-Credentials: true
        expect_eq(matchOrigin("https://any.dev", "*", true), false);
        expect_eq(matchOrigin("http://app.example.com", "https://app.example.com", false), false);
    });

    test("3. @cap/cors-match — wildcard subdomain matching & spoof prevention", () => {
        expect_eq(matchOrigin("https://api.example.com", "https://*.example.com", true), true);
        expect_eq(matchOrigin("https://a.b.example.com", "https://*.example.com", true), true);
        // Prevent spoofing via prefix without dot boundary or bare apex
        expect_eq(matchOrigin("https://evil-example.com", "https://*.example.com", true), false);
        expect_eq(matchOrigin("https://example.com", "https://*.example.com", true), false);
        expect_eq(matchOrigin("http://api.example.com", "https://*.example.com", true), false);
    });

    test("4. @cap/cors-match — matchOriginList multi-pattern evaluation", () => {
        let allowed: [2]string = undefined;
        allowed[0] = "http://localhost:3000";
        allowed[1] = "https://*.capdux.org";

        expect_eq(matchOriginList("http://localhost:3000", allowed[0..2], true), true);
        expect_eq(matchOriginList("https://registry.capdux.org", allowed[0..2], true), true);
        expect_eq(matchOriginList("https://evil.org", allowed[0..2], true), false);
    });

    test("5. @cap/cors-match — preflight method & simple method checks", () => {
        expect_eq(isMethodAllowed("PUT", "GET, POST, PUT, DELETE"), true);
        expect_eq(isMethodAllowed("delete", "GET, POST, DELETE"), true);
        expect_eq(isMethodAllowed("PATCH", "GET, POST"), false);
        expect_eq(isMethodAllowed("PATCH", "*"), true);
        expect_eq(isSimpleMethod("GET"), true);
        expect_eq(isSimpleMethod("POST"), true);
        expect_eq(isSimpleMethod("DELETE"), false);
    });

    test("6. @cap/cors-match — preflight Access-Control-Request-Headers validation", () => {
        expect_eq(areHeadersAllowed("Content-Type, Authorization", "authorization, content-type, x-trace-id"), true);
        expect_eq(areHeadersAllowed("Content-Type, X-Evil", "authorization, content-type"), false);
        expect_eq(areHeadersAllowed("X-Custom-Header", "*"), true);
        expect_eq(areHeadersAllowed("", "content-type"), true);
    });
}
