// Source: bun/src/glob/matcher.cp & bun/src/glob/GlobWalker.cp
// cap/test/test_glob_complete.cp
import { test, expect_eq } from "cap:test";
import { Glob, GlobMatcher, GlobWalker, GlobWalkerOptions } from "cap:glob";
const std = @import("std");

fn testBraceExpansion(): !void {
    // 1. Single brace expansion *.{cp,zig}
    expect_eq(Glob.match("*.{cp,zig}", "main.cp"), true);
    expect_eq(Glob.match("*.{cp,zig}", "test.zig"), true);
    expect_eq(Glob.match("*.{cp,zig}", "other.rs"), false);

    // 2. Directory prefix with braces {src,lib}/**/*.cp
    expect_eq(Glob.match("{src,lib}/**/*.cp", "lib/encoding/base64.cp"), true);
    expect_eq(Glob.match("{src,lib}/**/*.cp", "src/core/main.cp"), true);
    expect_eq(Glob.match("{src,lib}/**/*.cp", "cap/test/test_glob.cp"), false);

    // 3. Multi-option brace
    expect_eq(Glob.match("file.{txt,md,json,cp}", "file.json"), true);
    expect_eq(Glob.match("file.{txt,md,json,cp}", "file.cp"), true);
    expect_eq(Glob.match("file.{txt,md,json,cp}", "file.bin"), false);
}

fn testCharacterClassesAndEscaping(): !void {
    // Negative character class [!0-9]
    expect_eq(Glob.match("v[!0-9].txt", "va.txt"), true);
    expect_eq(Glob.match("v[!0-9].txt", "v1.txt"), false);

    // Negative character class [^a-z]
    expect_eq(Glob.match("log_[^a-z].log", "log_1.log"), true);
    expect_eq(Glob.match("log_[^a-z].log", "log_a.log"), false);

    // Escaped wildcard \*
    expect_eq(Glob.match("literal\\*.txt", "literal*.txt"), true);
    expect_eq(Glob.match("literal\\*.txt", "literal_abc.txt"), false);

    // Double negation
    expect_eq(Glob.match("!!*.cp", "main.cp"), true);
    expect_eq(Glob.match("!!*.cp", "main.zig"), false);
}

fn testGlobWalkerScanning(): !void {
    const allocator = std.heap.page_allocator;

    // Scan lib/encoding/*.cp
    let walker = GlobWalker.init("*.cp", "lib/encoding");
    const matches = try walker.walk(allocator);
    defer {
        for (item in matches) {
            allocator.free(item);
        }
        allocator.free(matches);
    }

    // Should find base64.cp, hex.cp, index.cp
    expect_eq(matches.len >= 3, true);

    let has_base64 = false;
    let has_hex = false;
    let has_index = false;

    for (file in matches) {
        if (std.mem.eql(u8, file, "base64.cp")) has_base64 = true;
        if (std.mem.eql(u8, file, "hex.cp")) has_hex = true;
        if (std.mem.eql(u8, file, "index.cp")) has_index = true;
    }

    expect_eq(has_base64, true);
    expect_eq(has_hex, true);
    expect_eq(has_index, true);
}

fn testGlobWalkerRecursive(): !void {
    const allocator = std.heap.page_allocator;

    // One-shot scan for *.cp in lib/glob
    const matches = try Glob.scan(allocator, "*.cp", "lib/glob");
    defer {
        for (item in matches) {
            allocator.free(item);
        }
        allocator.free(matches);
    }

    let has_matcher = false;
    let has_walker = false;
    let has_index = false;

    for (file in matches) {
        if (std.mem.eql(u8, file, "matcher.cp")) has_matcher = true;
        if (std.mem.eql(u8, file, "walker.cp")) has_walker = true;
        if (std.mem.eql(u8, file, "index.cp")) has_index = true;
    }

    expect_eq(has_matcher, true);
    expect_eq(has_walker, true);
    expect_eq(has_index, true);
}

fn main() {
    test("Glob brace expansion {a,b,c}", () => {
        try testBraceExpansion();
    });
    test("Glob character classes and escape syntax", () => {
        try testCharacterClassesAndEscaping();
    });
    test("GlobWalker directory file scanning", () => {
        try testGlobWalkerScanning();
    });
    test("GlobWalker recursive scanning with Glob.scan", () => {
        try testGlobWalkerRecursive();
    });
}
