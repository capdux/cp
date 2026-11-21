import {
    HostKind,
    SocketAddr,
} from "cap:net";
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

test("SocketAddr parse, parseBind, classification, and format round-trip", () => {
    let buf: [64]u8 = [0; 64];

    const s1 = SocketAddr.parse("127.0.0.1:8080").?;
    expect(s1.kind == HostKind.ipv4 && strEq(s1.host, "127.0.0.1") && s1.port == 8080);
    expect(strEq(s1.format(buf[0..]).?, "127.0.0.1:8080"));

    const s2 = SocketAddr.parse("[::1]:443").?;
    expect(s2.kind == HostKind.ipv6 && strEq(s2.host, "::1") && s2.port == 443);
    expect(strEq(s2.format(buf[0..]).?, "[::1]:443"));

    const s3 = SocketAddr.parse("api.cp-lang.dev:9000").?;
    expect(s3.kind == HostKind.hostname && s3.port == 9000);

    // Bind mode allows ":3000" and ephemeral ":0"
    const b1 = SocketAddr.parseBind(":3000").?;
    expect(strEq(b1.host, "0.0.0.0") && b1.port == 3000);
    const b2 = SocketAddr.parseBind("[::]:0").?;
    expect(b2.kind == HostKind.ipv6 && b2.port == 0);

    // Strict parse rejects ":3000", port 0, leading-zero port, unbracketed IPv6
    expect(SocketAddr.parse(":3000") == null);
    expect(SocketAddr.parse("127.0.0.1:0") == null);
    expect(SocketAddr.parse("127.0.0.1:080") == null);
    expect(SocketAddr.parse("::1:8080") == null);
});
