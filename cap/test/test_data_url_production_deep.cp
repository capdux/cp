import { test, expect } from "cap:test";
import {
    DataUrl,
    encodeShortestDataUrl,
    encodeBase64DataUrl,
    encodePercentDataUrl,
} from "cap:data_url";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("data_url: parse and decode plain, percent-encoded, and base64 data URLs", () => {
    let buf: [128]u8 = [0; 128];

    const u1 = DataUrl.parse("data:,Hello%2C%20World!");
    expect(u1 != null && !u1.?.is_base64);
    expect(strEq(u1.?.effectiveMimeType(), "text/plain;charset=US-ASCII"));
    expect(strEq(u1.?.decodePayload(buf[0..]).?, "Hello, World!"));

    const u2 = DataUrl.parse("DATA:text/javascript;base64,Y29uc29sZS5sb2coMSk=");
    expect(u2 != null && u2.?.is_base64);
    expect(strEq(u2.?.mime_type, "text/javascript"));
    expect(strEq(u2.?.decodePayload(buf[0..]).?, "console.log(1)"));

    // Invalid URLs
    expect(DataUrl.parse("https://example.com") == null);
    expect(DataUrl.parse("data:no-comma") == null);
});

test("data_url: encodeShortestDataUrl picks optimal encoding", () => {
    let out: [256]u8 = [0; 256];
    let dec_buf: [128]u8 = [0; 128];

    // Plain ASCII text -> percent-escaped is shorter than base64
    const s1 = encodeShortestDataUrl("text/plain", "hello=world&a=1", out[0..]).?;
    expect(strEq(s1, "data:text/plain,hello=world&a=1"));

    // Heavy binary/control bytes -> base64 is shorter than %XX per byte
    const bin: [12]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
    const s2 = encodeShortestDataUrl("application/octet-stream", bin[0..], out[0..]).?;
    const parsed = DataUrl.parse(s2).?;
    expect(parsed.is_base64);
    const decoded = parsed.decodePayload(dec_buf[0..]).?;
    expect(decoded.len == 12 && decoded[0] == 0 && decoded[11] == 11);

    const b64 = encodeBase64DataUrl("text/plain", "hi", out[0..]).?;
    expect(strEq(b64, "data:text/plain;base64,aGk="));

    const pct = encodePercentDataUrl("text/html", "<b>a#b</b>", out[0..]).?;
    expect(strEq(pct, "data:text/html,%3Cb%3Ea%23b%3C/b%3E"));
});
