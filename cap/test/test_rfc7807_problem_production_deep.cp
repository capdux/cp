import {
    PROBLEM_JSON_CONTENT_TYPE,
    defaultTitleForStatus,
    ProblemDetails,
} from "cap:rfc7807_problem";
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

test("ProblemDetails fromStatus, custom, and JSON escaping", () => {
    expect(strEq(PROBLEM_JSON_CONTENT_TYPE, "application/problem+json"));
    expect(strEq(defaultTitleForStatus(429), "Too Many Requests"));

    let buf: [512]u8 = [0; 512];
    const p1 = ProblemDetails.fromStatus(404, "User \"42\" not found");
    expect(strEq(
        p1.toJson(buf[0..]).?,
        "{\"type\":\"about:blank\",\"title\":\"Not Found\",\"status\":404,\"detail\":\"User \\\"42\\\" not found\"}",
    ));

    const p2 = ProblemDetails.custom(
        "https://example.com/probs/out-of-credit",
        "You do not have enough credit.",
        403,
        "Balance is 30\nCost is 50",
        "/account/12345/msgs/abc",
    );
    expect(strEq(
        p2.toJson(buf[0..]).?,
        "{\"type\":\"https://example.com/probs/out-of-credit\",\"title\":\"You do not have enough credit.\",\"status\":403,\"detail\":\"Balance is 30\\nCost is 50\",\"instance\":\"/account/12345/msgs/abc\"}",
    ));

    let tiny: [16]u8 = [0; 16];
    expect(p1.toJson(tiny[0..]) == null);
});
