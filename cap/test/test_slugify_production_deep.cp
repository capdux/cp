import {
    slugify,
    slugifyWithSep,
    slugifyBounded,
    isValidSlug,
} from "cap:slugify";
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

test("slugify, slugifyWithSep, slugifyBounded, and isValidSlug", () => {
    let buf: [128]u8 = [0; 128];
    expect(strEq(slugify("  Hello, World! Don't Stop (v2.0)  ", buf[0..]).?, "hello-world-dont-stop-v2-0"));
    expect(strEq(slugifyWithSep("CP Lang / Stdlib", '_', buf[0..]).?, "cp_lang_stdlib"));
    expect(strEq(slugifyBounded("the quick brown fox jumps", 16, buf[0..]).?, "the-quick-brown"));
    expect(strEq(slugify("---!!!---", buf[0..]).?, ""));

    expect(isValidSlug("hello-world-2"));
    expect(!isValidSlug(""));
    expect(!isValidSlug("-leading"));
    expect(!isValidSlug("trailing-"));
    expect(!isValidSlug("double--dash"));
    expect(!isValidSlug("UpperCase"));
});
