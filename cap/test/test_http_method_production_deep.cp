import { test, expect } from "cap:test";
import { HttpMethod, MethodSet } from "cap:http_method";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("http_method: parse, roundtrip all 36 methods, and RFC 9110 predicates", () => {
    let idx: u8 = 0;
    while (idx < 36) : (idx += 1) {
        const m: HttpMethod = @enumFromInt(idx);
        const parsed = HttpMethod.parse(m.asStr());
        expect(parsed != null && parsed.? == m);
    }

    expect(HttpMethod.parse("m-search") == HttpMethod.M_SEARCH);
    expect(HttpMethod.parse("UNKNOWN") == null);

    expect(HttpMethod.GET.isSafe() && HttpMethod.GET.isIdempotent() && HttpMethod.GET.isCacheable());
    expect(!HttpMethod.GET.hasRequestBody() && HttpMethod.GET.hasResponseBody());
    expect(!HttpMethod.HEAD.hasResponseBody());
    expect(HttpMethod.POST.hasRequestBody() && !HttpMethod.POST.isIdempotent() && HttpMethod.POST.isCorsSafelisted());
    expect(HttpMethod.PUT.isIdempotent() && !HttpMethod.PUT.isSafe() && !HttpMethod.PUT.isCorsSafelisted());
    expect(HttpMethod.QUERY.isSafe() && HttpMethod.QUERY.isIdempotent() && HttpMethod.QUERY.hasRequestBody());
});

test("http_method: MethodSet bitmask and Allow header formatting", () => {
    let buf: [64]u8 = [0; 64];
    let set = MethodSet.empty();
    set.insert(HttpMethod.POST);
    set.insert(HttpMethod.GET);
    set.insert(HttpMethod.HEAD);
    set.insert(HttpMethod.DELETE);
    set.remove(HttpMethod.DELETE);

    expect(set.contains(HttpMethod.GET));
    expect(!set.contains(HttpMethod.DELETE));
    expect(strEq(set.formatAllowHeader(buf[0..]), "GET, HEAD, POST"));
});
