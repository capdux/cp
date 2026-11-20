// Source: cp/cap/test/test_aho_production_deep.cp
// Deep Production Verification Suite for cap:aho & cap.aho
// Covers:
//   1. Classic "he"/"she"/"his"/"hers" overlapping findAll & failure/dict link traversal
//   2. LeftmostLongest vs LeftmostFirst non-overlapping disambiguation ("ab" vs "abcd", "bc" vs "abcd")
//   3. Zero-alloc containsAny short-circuit, replaceAll batch replacement, and initCaseInsensitive
//   4. Error interception (EmptyPattern, NoPatterns, PatternNotCompiled, InvalidOption) & 500+ pattern large dictionary
//   5. `import cap from "cap"; cap.aho` dynamic mounting, AhoBuilder, and std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AhoError,
    MatchKind,
    AhoMatch,
    AhoBuilder,
    AhoCorasick,
} from "cap:aho";

test("cap:aho - classic he/she/his/hers overlapping findAll and failure links") {
    const alloc = std.testing.allocator;
    let ac = AhoCorasick.init(alloc);
    defer ac.deinit();

    const p0 = ac.addPattern("he") catch unreachable;   // 0
    const p1 = ac.addPattern("she") catch unreachable;  // 1
    const p2 = ac.addPattern("his") catch unreachable;  // 2
    const p3 = ac.addPattern("hers") catch unreachable; // 3

    expect_eq(p0, 0);
    expect_eq(p1, 1);
    expect_eq(p2, 2);
    expect_eq(p3, 3);
    expect_eq(ac.patternCount(), 4);

    ac.build() catch unreachable;
    expect_eq(ac.stateCount() > 4, true);

    const text = "ushers";
    const matches = ac.findAll(alloc, text) catch unreachable;
    defer alloc.free(matches);

    // Expected matches in "ushers":
    //   "she"  (pid=1, 1..4)
    //   "he"   (pid=0, 2..4) via dictionary suffix link from "she"
    //   "hers" (pid=3, 2..6)
    expect_eq(matches.len, 3);

    expect_eq(matches[0].pattern_id, 1);
    expect_eq(matches[0].start, 1);
    expect_eq(matches[0].end, 4);

    expect_eq(matches[1].pattern_id, 0);
    expect_eq(matches[1].start, 2);
    expect_eq(matches[1].end, 4);

    expect_eq(matches[2].pattern_id, 3);
    expect_eq(matches[2].start, 2);
    expect_eq(matches[2].end, 6);

    // Also verify empty text returns 0 matches cleanly
    const empty_matches = ac.findAll(alloc, "") catch unreachable;
    defer alloc.free(empty_matches);
    expect_eq(empty_matches.len, 0);
}

test("cap:aho - LeftmostLongest vs LeftmostFirst non-overlapping disambiguation") {
    const alloc = std.testing.allocator;

    // Case A: "ab" (id=0) vs "abcd" (id=1) on "abcd"
    let ac1 = AhoCorasick.init(alloc);
    defer ac1.deinit();
    const patterns1: [3][]const u8 = [ "ab", "abcd", "cd" ];
    ac1.addPatterns(&patterns1) catch unreachable;
    ac1.build() catch unreachable;

    // LeftmostFirst: "ab" wins at start 0, then "cd" matches at start 2
    const first_hits = ac1.findLeftmost(alloc, "abcd", MatchKind.LeftmostFirst) catch unreachable;
    defer alloc.free(first_hits);
    expect_eq(first_hits.len, 2);
    expect_eq(first_hits[0].pattern_id, 0);
    expect_eq(first_hits[0].start, 0);
    expect_eq(first_hits[0].end, 2);
    expect_eq(first_hits[1].pattern_id, 2);
    expect_eq(first_hits[1].start, 2);
    expect_eq(first_hits[1].end, 4);

    // LeftmostLongest: "abcd" wins at start 0
    const longest_hits = ac1.findLeftmost(alloc, "abcd", MatchKind.LeftmostLongest) catch unreachable;
    defer alloc.free(longest_hits);
    expect_eq(longest_hits.len, 1);
    expect_eq(longest_hits[0].pattern_id, 1);
    expect_eq(longest_hits[0].start, 0);
    expect_eq(longest_hits[0].end, 4);

    // Case B: "bc" (id=0) vs "abcd" (id=1) -> both modes pick "abcd" because start 0 < start 1
    let ac2 = AhoCorasick.init(alloc);
    defer ac2.deinit();
    const patterns2: [2][]const u8 = [ "bc", "abcd" ];
    ac2.addPatterns(&patterns2) catch unreachable;
    ac2.build() catch unreachable;

    const hits2 = ac2.findLeftmost(alloc, "abcd", MatchKind.LeftmostFirst) catch unreachable;
    defer alloc.free(hits2);
    expect_eq(hits2.len, 1);
    expect_eq(hits2[0].pattern_id, 1);
    expect_eq(hits2[0].start, 0);
    expect_eq(hits2[0].end, 4);
}

test("cap:aho - containsAny zero-alloc short-circuit, replaceAll, and initCaseInsensitive") {
    const alloc = std.testing.allocator;
    let ac = AhoCorasick.initCaseInsensitive(alloc);
    defer ac.deinit();

    const banned: [3][]const u8 = [ "Foo", "FooBar", "SecretKey" ];
    ac.addPatterns(&banned) catch unreachable;
    ac.build() catch unreachable;

    expect_eq(ac.containsAny("clean log line"), false);
    expect_eq(ac.containsAny(""), false);
    expect_eq(ac.containsAny("leaked SECRETKEY here"), true);
    expect_eq(ac.containsAny("prefix FOOBAR suffix"), true);

    const sanitized = ac.replaceAll(alloc, "user=FOOBAR token=secretkey ok=foo!", "[REDACTED]") catch unreachable;
    defer alloc.free(sanitized);
    expect_eq(std.mem.eql(u8, sanitized, "user=[REDACTED] token=[REDACTED] ok=[REDACTED]!"), true);

    const unchanged = ac.replaceAll(alloc, "nothing to replace", "***") catch unreachable;
    defer alloc.free(unchanged);
    expect_eq(std.mem.eql(u8, unchanged, "nothing to replace"), true);
}

test("cap:aho - error interception and 550-pattern large dictionary linear scan") {
    const alloc = std.testing.allocator;
    let ac = AhoCorasick.init(alloc);
    defer ac.deinit();

    // 1. NoPatterns before any pattern is added
    const r_no_pat = ac.build();
    expect_eq(r_no_pat == AhoError.NoPatterns, true);

    // 2. EmptyPattern rejection
    const r_empty = ac.addPattern("");
    expect_eq(r_empty == AhoError.EmptyPattern, true);

    // 3. PatternNotCompiled before build()
    * = ac.addPattern("alpha") catch unreachable;
    const r_not_compiled = ac.findAll(alloc, "alpha");
    expect_eq(r_not_compiled == AhoError.PatternNotCompiled, true);
    expect_eq(ac.containsAny("alpha"), false);

    ac.build() catch unreachable;

    // 4. InvalidOption when passing AllOverlapping to findLeftmost
    const r_invalid_opt = ac.findLeftmost(alloc, "alpha", MatchKind.AllOverlapping);
    expect_eq(r_invalid_opt == AhoError.InvalidOption, true);

    // 5. Large dictionary with 550 patterns
    let big = AhoCorasick.init(alloc);
    defer big.deinit();

    let buf: [32]u8 = undefined;
    let idx: usize = 0;
    while (idx < 550) : (idx += 1) {
        const pat = std.fmt.bufPrint(&buf, "kw_token_{d:0>4}", .{idx}) catch unreachable;
        const pid = big.addPattern(pat) catch unreachable;
        expect_eq(pid, idx);
    }
    expect_eq(big.patternCount(), 550);
    big.build() catch unreachable;

    const hay = "prefix kw_token_0000 middle kw_token_0274 tail kw_token_0549 end";
    expect_eq(big.containsAny(hay), true);

    const hits = big.findAll(alloc, hay) catch unreachable;
    defer alloc.free(hits);
    expect_eq(hits.len, 3);
    expect_eq(hits[0].pattern_id, 0);
    expect_eq(hits[1].pattern_id, 274);
    expect_eq(hits[2].pattern_id, 549);
}

test("cap:aho - cap.aho dynamic namespace mounting and AhoBuilder zero-leak") {
    const alloc = std.testing.allocator;

    let builder = cap.aho.AhoBuilder.init(alloc);
    defer builder.deinit();

    * = builder.addPattern("agent") catch unreachable;
    * = builder.addPattern("agentic") catch unreachable;
    * = builder.addPattern("beam") catch unreachable;
    expect_eq(builder.patternCount(), 3);

    let ac = builder.build() catch unreachable;
    defer ac.deinit();

    const text = "agentic beam runtime";
    const hits = ac.findLeftmost(alloc, text, cap.aho.MatchKind.LeftmostLongest) catch unreachable;
    defer alloc.free(hits);

    expect_eq(hits.len, 2);
    expect_eq(hits[0].pattern_id, 1); // "agentic"
    expect_eq(hits[0].start, 0);
    expect_eq(hits[0].end, 7);
    expect_eq(hits[1].pattern_id, 2); // "beam"
    expect_eq(hits[1].start, 8);
    expect_eq(hits[1].end, 12);
}
