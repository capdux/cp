import { test, expect } from "cap:test";
import { HttpDateTime, formatHttpDate, parseHttpDate } from "cap:http";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("http_date: RFC 9110 IMF-fixdate parse, format, and Unix epoch roundtrip", () => {
    let buf: [32]u8 = [0; 32];

    // Unix epoch 0 -> Thu, 01 Jan 1970 00:00:00 GMT
    expect(strEq(formatHttpDate(0, buf[0..]).?, "Thu, 01 Jan 1970 00:00:00 GMT"));
    expect(parseHttpDate("Thu, 01 Jan 1970 00:00:00 GMT").? == 0);

    // Classic RFC 9110 example: Sun, 06 Nov 1994 08:49:37 GMT == 784111777
    const ts = parseHttpDate("Sun, 06 Nov 1994 08:49:37 GMT").?;
    expect(ts == 784111777);
    expect(strEq(formatHttpDate(ts, buf[0..]).?, "Sun, 06 Nov 1994 08:49:37 GMT"));

    // Leap day 2024-02-29
    const leap = HttpDateTime.parseImf("Thu, 29 Feb 2024 12:34:56 GMT").?;
    expect(leap.year == 2024 && leap.month == 2 && leap.day == 29);
    const back = HttpDateTime.fromUnixSeconds(leap.toUnixSeconds());
    expect(back.year == 2024 && back.month == 2 && back.day == 29 && back.weekday == 4);

    // Non-leap Feb 29 rejected, invalid time rejected, non-GMT rejected
    expect(HttpDateTime.parseImf("Wed, 29 Feb 2023 00:00:00 GMT") == null);
    expect(HttpDateTime.parseImf("Sun, 06 Nov 1994 25:00:00 GMT") == null);
    expect(HttpDateTime.parseImf("Sun, 06 Nov 1994 08:49:37 UTC") == null);
});
