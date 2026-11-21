import {
    KeepAliveParams,
    shouldKeepAlive,
} from "cap:http_keep_alive";
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

test("KeepAliveParams parse and format round-trip", () => {
    const ka = KeepAliveParams.parse("Timeout=15, MAX=\"1000\", foo=bar");
    expect(ka.timeout_sec.? == 15);
    expect(ka.max_requests.? == 1000);

    let buf: [64]u8 = [0; 64];
    expect(strEq(ka.format(buf[0..]).?, "timeout=15, max=1000"));

    const empty = KeepAliveParams.parse("timeout=abc");
    expect(empty.timeout_sec == null && empty.max_requests == null);
    expect(strEq(empty.format(buf[0..]).?, ""));
});

test("shouldKeepAlive follows RFC 9112 §9.3 for HTTP/1.0 and HTTP/1.1", () => {
    // HTTP/1.1 defaults to persistent unless Connection: close
    expect(shouldKeepAlive(1, null));
    expect(shouldKeepAlive(1, "keep-alive"));
    expect(!shouldKeepAlive(1, "keep-alive, Close"));

    // HTTP/1.0 defaults to close unless Connection: keep-alive
    expect(!shouldKeepAlive(0, null));
    expect(shouldKeepAlive(0, "Keep-Alive"));
    expect(!shouldKeepAlive(0, "keep-alive, close"));
});
