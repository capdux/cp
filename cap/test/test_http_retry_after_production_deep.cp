import {
    RetryAfterKind,
    RetryAfter,
    formatRetryAfterSeconds,
} from "cap:http_retry_after";
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

test("RetryAfter parses delta-seconds and IMF-fixdate", () => {
    const r1 = RetryAfter.parse(" 120 ").?;
    expect(r1.kind == RetryAfterKind.delay_seconds);
    expect(r1.waitSeconds(1000) == 120);
    expect(r1.waitSecondsCapped(1000, 60) == 60);

    // Wed, 21 Oct 2015 07:28:00 GMT == 1445412480
    const r2 = RetryAfter.parse("Wed, 21 Oct 2015 07:28:00 GMT").?;
    expect(r2.kind == RetryAfterKind.http_date);
    expect(r2.target_unix_sec == 1445412480);
    expect(r2.waitSeconds(1445412400) == 80);
    expect(r2.waitSeconds(1445412500) == 0); // past date -> 0

    expect(RetryAfter.parse("-5") == null);
    expect(RetryAfter.parse("invalid date") == null);

    let buf: [32]u8 = [0; 32];
    expect(strEq(formatRetryAfterSeconds(0, buf[0..]).?, "0"));
    expect(strEq(formatRetryAfterSeconds(3600, buf[0..]).?, "3600"));
});
