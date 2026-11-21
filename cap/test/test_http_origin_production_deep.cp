import {
    Origin,
    isSameOriginUrl,
} from "cap:http";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("Origin parse, default-port normalization, and sameOrigin", () => {
    const o1 = Origin.parse("HTTPS://Example.COM:443/api/v1?x=1").?;
    const o2 = Origin.parse("https://example.com/other").?;
    expect(o1.sameOrigin(o2));
    expect(o1.isPotentiallyTrustworthy());

    let buf: [128]u8 = [0; 128];
    expect(strEq(o1.format(buf[0..]).?, "https://example.com"));

    const o3 = Origin.parse("http://[::1]:8080/test").?;
    expect(o3.port.? == 8080);
    expect(o3.isPotentiallyTrustworthy());
    expect(strEq(o3.format(buf[0..]).?, "http://[::1]:8080"));

    expect(isSameOriginUrl("http://a.dev:80/1", "http://A.DEV/2"));
    expect(!isSameOriginUrl("http://a.dev", "https://a.dev"));
    expect(!isSameOriginUrl("http://a.dev:8080", "http://a.dev:8081"));
    expect(Origin.parse("null") == null);
    expect(Origin.parse("data:text/plain,hi") == null);
});

test("isPotentiallyTrustworthy covers localhost subdomains and rejects plain http remote", () => {
    expect(Origin.parse("http://app.localhost:3000").?.isPotentiallyTrustworthy());
    expect(Origin.parse("http://127.0.0.1:8080").?.isPotentiallyTrustworthy());
    expect(!Origin.parse("http://example.com").?.isPotentiallyTrustworthy());
});
