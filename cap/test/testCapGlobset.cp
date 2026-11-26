// cap/test/testCapGlobset.cp
// Comprehensive 6-test suite for @cap/globset (Rust globset v0.4 parity)

import { test, expect_eq } from "cap:test";
import {
    GLOBSET_MAX_PATTERNS,
    OVERRIDE_NONE,
    OVERRIDE_WHITELIST,
    OVERRIDE_IGNORE,
    GlobSetOptions,
    GlobSet,
    GlobOverride
} from "../../packages/@cap/globset/src/index.cp";

fn main() {
    test("1. @cap/globset — Multi-pattern GlobSet matches() returns all hit indices", () => {
        let gs = GlobSet.new();
        expect_eq(gs.add("src/**/*.cp"), true);     // 0
        expect_eq(gs.add("**/*.cp"), true);         // 1
        expect_eq(gs.add("src/core/*"), true);      // 2
        expect_eq(gs.add("tests/**/*.cp"), true);   // 3

        let hits: [8]usize = [0; 8];
        let n = gs.matches("src/core/main.cp", hits[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(hits[0], 0 as usize);
        expect_eq(hits[1], 1 as usize);
        expect_eq(hits[2], 2 as usize);

        expect_eq(gs.isMatch("src/core/main.cp"), true);
        expect_eq(gs.isMatch("docs/readme.md"), false);
    });

    test("2. @cap/globset — firstMatch() & matchCount() accuracy", () => {
        let gs = GlobSet.new();
        expect_eq(gs.add("*.md"), true);            // 0
        expect_eq(gs.add("src/**/*.rs"), true);     // 1
        expect_eq(gs.add("**/*.{rs,cp}"), true);    // 2
        expect_eq(gs.add("src/lib/*"), true);       // 3

        expect_eq(gs.firstMatch("src/lib/mod.rs"), 1 as i32);
        expect_eq(gs.matchCount("src/lib/mod.rs"), 3 as usize);
        expect_eq(gs.firstMatch("README.md"), 0 as i32);
        expect_eq(gs.matchCount("README.md"), 1 as usize);
        expect_eq(gs.firstMatch("build.zig"), -1 as i32);
        expect_eq(gs.matchCount("build.zig"), 0 as usize);
    });

    test("3. @cap/globset — Globstar **/*.cp, brace *.{rs,cp,zig} & character class combinations", () => {
        let gs = GlobSet.new();
        expect_eq(gs.add("packages/**/*.cp"), true);
        expect_eq(gs.add("*.{rs,cp,zig}"), true);
        expect_eq(gs.add("src/mod_[0-9].cp"), true);

        expect_eq(gs.isMatch("packages/@cap/bstr/src/index.cp"), true);
        expect_eq(gs.isMatch("build.zig"), true);
        expect_eq(gs.isMatch("main.rs"), true);
        expect_eq(gs.isMatch("main.py"), false);
        expect_eq(gs.isMatch("src/mod_7.cp"), true);
        expect_eq(gs.isMatch("src/mod_a.cp"), false);
    });

    test("4. @cap/globset — case_insensitive option & Windows backslash normalization", () => {
        let opts = GlobSetOptions.defaults();
        opts.case_insensitive = true;
        opts.backslash_escape = false;

        let gs = GlobSet.withOptions(opts);
        expect_eq(gs.add("src\\**\\*.CP"), true);
        expect_eq(gs.add("README.md"), true);

        expect_eq(gs.isMatch("SRC\\Core\\Index.cp"), true);
        expect_eq(gs.isMatch("src/core/index.CP"), true);
        expect_eq(gs.isMatch("readme.MD"), true);
        expect_eq(gs.firstMatch("SRC\\Core\\Index.cp"), 0 as i32);
        expect_eq(gs.firstMatch("readme.MD"), 1 as i32);
    });

    test("5. @cap/globset — GlobOverride positive whitelist & ! negative ignore precedence", () => {
        let ov = GlobOverride.new();
        expect_eq(ov.add("**/*.cp"), true);           // whitelist all .cp
        expect_eq(ov.add("!**/generated_*.cp"), true); // ignore generated_*.cp
        expect_eq(ov.add("src/generated_keep.cp"), true); // re-include specific file

        expect_eq(ov.matched("src/app.cp"), OVERRIDE_WHITELIST);
        expect_eq(ov.isAllowed("src/app.cp"), true);

        expect_eq(ov.matched("src/generated_foo.cp"), OVERRIDE_IGNORE);
        expect_eq(ov.isAllowed("src/generated_foo.cp"), false);

        expect_eq(ov.matched("src/generated_keep.cp"), OVERRIDE_WHITELIST);
        expect_eq(ov.isAllowed("src/generated_keep.cp"), true);

        expect_eq(ov.matched("notes.txt"), OVERRIDE_NONE);
        expect_eq(ov.isAllowed("notes.txt"), false); // whitelist present -> unmatched rejected

        // Ignore-only override allows unmatched paths
        let igOnly = GlobOverride.new();
        expect_eq(igOnly.add("!target/**"), true);
        expect_eq(igOnly.isAllowed("target/debug/app"), false);
        expect_eq(igOnly.isAllowed("src/main.cp"), true);
    });

    test("6. @cap/globset — Empty set, clear() & 32-pattern capacity boundary safety", () => {
        let gs = GlobSet.new();
        expect_eq(gs.isEmpty(), true);
        expect_eq(gs.len(), 0 as usize);
        expect_eq(gs.isMatch("anything.cp"), false);
        expect_eq(gs.firstMatch("anything.cp"), -1 as i32);
        expect_eq(gs.matchCount("anything.cp"), 0 as usize);

        // Empty pattern rejected
        expect_eq(gs.add(""), false);

        // Fill up to GLOBSET_MAX_PATTERNS (32)
        let i: usize = 0;
        while (i < GLOBSET_MAX_PATTERNS) {
            expect_eq(gs.add("*.cp"), true);
            i = i + 1;
        }
        expect_eq(gs.len(), 32 as usize);
        expect_eq(gs.isEmpty(), false);
        // 33rd pattern safely rejected
        expect_eq(gs.add("*.rs"), false);
        expect_eq(gs.matchCount("a.cp"), 32 as usize);

        // Truncated out_indices slice stops safely
        let small: [4]usize = [0; 4];
        expect_eq(gs.matches("a.cp", small[0..4]), 4 as usize);

        gs.clear();
        expect_eq(gs.isEmpty(), true);
        expect_eq(gs.len(), 0 as usize);
    });
}
