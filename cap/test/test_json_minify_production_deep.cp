import {
    minifyJson,
    minifyJsonc,
} from "cap:json_minify";
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

test("minifyJson preserves strings and strips non-string whitespace", () => {
    let buf: [256]u8 = [0; 256];
    const src = "{\n  \"msg\": \"hello   // not a comment\",\n  \"esc\": \"a\\\"b\",\n  \"nums\": [ 1, 2, 3 ]\n}";
    expect(strEq(
        minifyJson(src, buf[0..]).?,
        "{\"msg\":\"hello   // not a comment\",\"esc\":\"a\\\"b\",\"nums\":[1,2,3]}",
    ));
    expect(minifyJson("{\"unterminated", buf[0..]) == null);
});

test("minifyJsonc strips // comments, /* */ block comments, and trailing commas", () => {
    let buf: [256]u8 = [0; 256];
    const jsonc =
        "{\n" ++
        "  // line comment\n" ++
        "  \"compilerOptions\": {\n" ++
        "    \"strict\": true, /* block comment */\n" ++
        "  },\n" ++
        "  \"include\": [\"src\",],\n" ++
        "}\n";
    expect(strEq(
        minifyJsonc(jsonc, buf[0..]).?,
        "{\"compilerOptions\":{\"strict\":true},\"include\":[\"src\"]}",
    ));

    // Unterminated block comment rejected
    expect(minifyJsonc("{\"a\":1 /* unclosed", buf[0..]) == null);
});
