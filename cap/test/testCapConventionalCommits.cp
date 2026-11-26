// cap/test/testCapConventionalCommits.cp
// Comprehensive unit tests for @cap/conventional-commits

import { test, expect_eq } from "cap:test";
import {
    SemverBump,
    CommitFooter,
    ParsedCommit,
    parseCommit,
    isValidHeader,
    recommendedBump,
    highestBump,
    formatCommitHeader,
} from "../../packages/@cap/conventional-commits/src/index.cp";

fn main() {
    test("1. @cap/conventional-commits — Basic header, scope & exclamation breaking", () => {
        const c1 = parseCommit("feat: add zero-alloc parser");
        expect_eq(c1.valid, true);
        expect_eq(c1.commit_type, "feat");
        expect_eq(c1.scope, "");
        expect_eq(c1.description, "add zero-alloc parser");
        expect_eq(c1.is_breaking, false);
        expect_eq(recommendedBump(c1) == SemverBump.Minor, true);

        const c2 = parseCommit("fix(http/client): handle 429 retry-after");
        expect_eq(c2.valid, true);
        expect_eq(c2.commit_type, "fix");
        expect_eq(c2.scope, "http/client");
        expect_eq(c2.description, "handle 429 retry-after");
        expect_eq(recommendedBump(c2) == SemverBump.Patch, true);

        const c3 = parseCommit("refactor(core)!: drop deprecated v1 API");
        expect_eq(c3.valid, true);
        expect_eq(c3.commit_type, "refactor");
        expect_eq(c3.scope, "core");
        expect_eq(c3.is_breaking, true);
        expect_eq(recommendedBump(c3) == SemverBump.Major, true);
    });

    test("2. @cap/conventional-commits — Body + multiple footers (Token: val & Token #val)", () => {
        const msg = "feat(auth): support OAuth2 PKCE flow\n\n"
            ++ "Implements RFC 7636 code verifier and S256 challenge.\n"
            ++ "Backward compatible with existing sessions.\n\n"
            ++ "Reviewed-by: Alice\n"
            ++ "Refs #123\n"
            ++ "Signed-off-by: Bob <bob@capdux.org>";
        const c = parseCommit(msg);
        expect_eq(c.valid, true);
        expect_eq(c.is_breaking, false);
        expect_eq(
            c.body,
            "Implements RFC 7636 code verifier and S256 challenge.\nBackward compatible with existing sessions.",
        );
        expect_eq(c.footer_count, 3 as usize);
        expect_eq(c.getFooter("Reviewed-by") ?? "", "Alice");
        expect_eq(c.getFooter("refs") ?? "", "123");
        expect_eq(c.getFooter("Signed-off-by") ?? "", "Bob <bob@capdux.org>");
    });

    test("3. @cap/conventional-commits — BREAKING CHANGE & BREAKING-CHANGE footers", () => {
        const msg1 = "chore(config): update default timeout\n\nBREAKING CHANGE: default timeout lowered from 60s to 15s";
        const c1 = parseCommit(msg1);
        expect_eq(c1.valid, true);
        expect_eq(c1.body, "");
        expect_eq(c1.is_breaking, true);
        expect_eq(c1.breaking_note, "default timeout lowered from 60s to 15s");
        expect_eq(c1.footer_count, 1 as usize);
        expect_eq(c1.footerAt(0).?.is_breaking, true);
        expect_eq(recommendedBump(c1) == SemverBump.Major, true);

        const msg2 = "feat: switch wire format\n\nBREAKING-CHANGE: binary header v2";
        const c2 = parseCommit(msg2);
        expect_eq(c2.is_breaking, true);
        expect_eq(c2.breaking_note, "binary header v2");
        expect_eq(recommendedBump(c2) == SemverBump.Major, true);
    });

    test("4. @cap/conventional-commits — Semver bump derivation & highestBump aggregation", () => {
        const docs = recommendedBump(parseCommit("docs: update README"));
        const perf = recommendedBump(parseCommit("perf: optimize SIMD loop"));
        const feat = recommendedBump(parseCommit("feat: add streaming"));
        const brk = recommendedBump(parseCommit("fix!: change return type"));

        expect_eq(docs == SemverBump.None, true);
        expect_eq(perf == SemverBump.Patch, true);
        expect_eq(feat == SemverBump.Minor, true);
        expect_eq(brk == SemverBump.Major, true);

        expect_eq(highestBump(docs, perf) == SemverBump.Patch, true);
        expect_eq(highestBump(perf, feat) == SemverBump.Minor, true);
        expect_eq(highestBump(feat, brk) == SemverBump.Major, true);
        expect_eq(highestBump(brk, docs) == SemverBump.Major, true);
    });

    test("5. @cap/conventional-commits — formatCommitHeader round-trip", () => {
        let out: [128]u8 = undefined;
        const c1 = parseCommit("feat(cli)!: add interactive wizard");
        expect_eq(formatCommitHeader(c1, out[0..128]), "feat(cli)!: add interactive wizard");

        const c2 = parseCommit("fix: prevent null deref");
        expect_eq(formatCommitHeader(c2, out[0..128]), "fix: prevent null deref");
    });

    test("6. @cap/conventional-commits — Invalid headers & malformed messages rejected", () => {
        expect_eq(isValidHeader("feat: valid header"), true);
        expect_eq(isValidHeader("feat(scope): valid"), true);

        // Missing space after colon, empty description, empty scope, no colon, multiline
        expect_eq(isValidHeader("feat:missing space"), false);
        expect_eq(isValidHeader("feat:   "), false);
        expect_eq(isValidHeader("feat(): empty scope"), false);
        expect_eq(isValidHeader("feat(unclosed: desc"), false);
        expect_eq(isValidHeader("just a plain commit message"), false);
        expect_eq(isValidHeader("123: numeric type"), false);
        expect_eq(isValidHeader("feat: ok\nsecond line"), false);
        expect_eq(isValidHeader(""), false);

        const bad = parseCommit("not conventional");
        expect_eq(bad.valid, false);
        expect_eq(recommendedBump(bad) == SemverBump.None, true);
    });
}
