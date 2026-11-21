import {
    ReleaseBump,
    ConventionalCommit,
} from "cap:conventional_commit";
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

test("ConventionalCommit parses type, scope, breaking !, footer, and bump advice", () => {
    const c1 = ConventionalCommit.parse("feat(stdlib): add http_retry_after").?;
    expect(c1.isFeature());
    expect(strEq(c1.scope.?, "stdlib"));
    expect(!c1.is_breaking);
    expect(strEq(c1.description, "add http_retry_after"));
    expect(c1.recommendedBump() == ReleaseBump.minor);

    const c2 = ConventionalCommit.parse("fix!: drop legacy syntax").?;
    expect(c2.isFix());
    expect(c2.scope == null);
    expect(c2.is_breaking);
    expect(c2.recommendedBump() == ReleaseBump.major);

    const c3 = ConventionalCommit.parse("refactor(parser): simplify\n\nBREAKING CHANGE: removed old flag").?;
    expect(c3.is_breaking);
    expect(c3.recommendedBump() == ReleaseBump.major);

    const c4 = ConventionalCommit.parse("docs: update README").?;
    expect(c4.recommendedBump() == ReleaseBump.none);

    const c5 = ConventionalCommit.parse("perf(http): zero-copy headers").?;
    expect(c5.recommendedBump() == ReleaseBump.patch);

    // Invalid formats
    expect(ConventionalCommit.parse("no colon here") == null);
    expect(ConventionalCommit.parse("feat:missing space") == null);
    expect(ConventionalCommit.parse("feat(): empty scope") == null);
    expect(ConventionalCommit.parse("feat:   ") == null);
});
