import { test, expect_eq } from "cap:test";
import {
    ProxyEnv,
    shouldBypassProxy,
    getProxyForUrl,
} from "../../packages/proxy-from-env/src/index.cp";

fn main() {
    test("1. @cap/proxy-from-env — selects http_proxy vs https_proxy by scheme", () => {
        const env = ProxyEnv {
            http_proxy: "http://proxy-http:8080",
            https_proxy: "http://proxy-https:8443",
            all_proxy: "socks5://all:1080",
            no_proxy: "",
        };
        expect_eq(getProxyForUrl("http://example.com/a", env), "http://proxy-http:8080");
        expect_eq(getProxyForUrl("https://example.com/b", env), "http://proxy-https:8443");
        expect_eq(getProxyForUrl("ftp://files.example.com", env), "socks5://all:1080");
    });

    test("2. @cap/proxy-from-env — falls back to all_proxy when scheme proxy is empty", () => {
        const env = ProxyEnv {
            http_proxy: "",
            https_proxy: "",
            all_proxy: "http://fallback:3128",
            no_proxy: "",
        };
        expect_eq(getProxyForUrl("https://api.capdux.org", env), "http://fallback:3128");
    });

    test("3. @cap/proxy-from-env — NO_PROXY wildcard '*' bypasses all URLs", () => {
        const env = ProxyEnv {
            http_proxy: "http://proxy:8080",
            https_proxy: "http://proxy:8080",
            all_proxy: "",
            no_proxy: "*",
        };
        expect_eq(getProxyForUrl("http://example.com", env), "");
        expect_eq(getProxyForUrl("https://capdux.org", env), "");
    });

    test("4. @cap/proxy-from-env — NO_PROXY domain suffix & subdomain boundary matching", () => {
        expect_eq(shouldBypassProxy("api.internal.local", 443, ".internal.local"), true);
        expect_eq(shouldBypassProxy("internal.local", 443, ".internal.local"), true);
        expect_eq(shouldBypassProxy("evilinternal.local", 443, "internal.local"), false);
        expect_eq(shouldBypassProxy("sub.corp.io", 80, "*.corp.io"), true);
    });

    test("5. @cap/proxy-from-env — NO_PROXY port-specific rules", () => {
        const env = ProxyEnv {
            http_proxy: "http://proxy:8080",
            https_proxy: "",
            all_proxy: "",
            no_proxy: "localhost:3000,127.0.0.1",
        };
        expect_eq(getProxyForUrl("http://localhost:3000/api", env), "");
        expect_eq(getProxyForUrl("http://localhost:4000/api", env), "http://proxy:8080");
        expect_eq(getProxyForUrl("http://127.0.0.1:9000/", env), "");
    });

    test("6. @cap/proxy-from-env — handles invalid URLs and IPv6 hosts safely", () => {
        const env = ProxyEnv {
            http_proxy: "http://proxy:8080",
            https_proxy: "",
            all_proxy: "",
            no_proxy: "::1",
        };
        expect_eq(getProxyForUrl("not-a-url", env), "");
        expect_eq(getProxyForUrl("http://[::1]:8080/health", env), "");
    });
}
