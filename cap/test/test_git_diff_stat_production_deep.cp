// Source: cp/cap/test/test_git_diff_stat_production_deep.cp (Production deep tests for cap:git_diff_stat)

import { test, expect_eq } from "cap:test";
import {
    FileChangeKind,
    FileDiffStat,
    DiffSummary,
    GitDiffIter,
    summarizeGitDiff,
} from "cap:git";

test("git_diff_stat: multi-file patch with modified, added, deleted, renamed and binary files") {
    const patch =
        "commit 1234567890abcdef\n" ++
        "Author: Cap Dev\n\n" ++
        "diff --git a/src/app.cp b/src/app.cp\n" ++
        "index 111..222 100644\n" ++
        "--- a/src/app.cp\n" ++
        "+++ b/src/app.cp\n" ++
        "@@ -1,3 +1,4 @@\n" ++
        " line1\n" ++
        "-old_line\n" ++
        "+new_line_1\n" ++
        "+new_line_2\n" ++
        "@@ -10,2 +11,1 @@\n" ++
        "-removed_tail\n" ++
        " keep\n" ++
        "diff --git a/src/new.cp b/src/new.cp\n" ++
        "new file mode 100644\n" ++
        "--- /dev/null\n" ++
        "+++ b/src/new.cp\n" ++
        "@@ -0,0 +1,2 @@\n" ++
        "+pub fn hi() {}\n" ++
        "+// end\n" ++
        "diff --git a/src/old.cp b/src/old.cp\n" ++
        "deleted file mode 100644\n" ++
        "--- a/src/old.cp\n" ++
        "+++ /dev/null\n" ++
        "@@ -1,1 +0,0 @@\n" ++
        "-bye\n" ++
        "diff --git a/old_name.cp b/new_name.cp\n" ++
        "similarity index 95%\n" ++
        "rename from old_name.cp\n" ++
        "rename to new_name.cp\n" ++
        "--- a/old_name.cp\n" ++
        "+++ b/new_name.cp\n" ++
        "@@ -1,1 +1,1 @@\n" ++
        "-foo\n" ++
        "+bar\n" ++
        "diff --git a/assets/logo.png b/assets/logo.png\n" ++
        "index 000..999 100644\n" ++
        "Binary files a/assets/logo.png and b/assets/logo.png differ\n";

    let it = GitDiffIter.init(patch);

    const f1 = it.next().?;
    expect_eq(std.mem.eql(u8, f1.old_path, "src/app.cp"), true);
    expect_eq(std.mem.eql(u8, f1.new_path, "src/app.cp"), true);
    expect_eq(f1.kind == .modified, true);
    expect_eq(f1.additions, 2);
    expect_eq(f1.deletions, 2);
    expect_eq(f1.hunks, 2);
    expect_eq(f1.is_binary, false);

    const f2 = it.next().?;
    expect_eq(f2.kind == .added, true);
    expect_eq(f2.additions, 2);
    expect_eq(f2.deletions, 0);
    expect_eq(f2.hunks, 1);

    const f3 = it.next().?;
    expect_eq(f3.kind == .deleted, true);
    expect_eq(f3.additions, 0);
    expect_eq(f3.deletions, 1);

    const f4 = it.next().?;
    expect_eq(f4.kind == .renamed, true);
    expect_eq(std.mem.eql(u8, f4.old_path, "old_name.cp"), true);
    expect_eq(std.mem.eql(u8, f4.new_path, "new_name.cp"), true);
    expect_eq(f4.additions, 1);
    expect_eq(f4.deletions, 1);

    const f5 = it.next().?;
    expect_eq(f5.kind == .binary, true);
    expect_eq(f5.is_binary, true);
    expect_eq(f5.additions, 0);
    expect_eq(f5.deletions, 0);
    expect_eq(std.mem.eql(u8, f5.kind.asStr(), "binary"), true);

    expect_eq(it.next() == null, true);

    const sum = summarizeGitDiff(patch);
    expect_eq(sum.files_changed, 5);
    expect_eq(sum.insertions, 5);
    expect_eq(sum.deletions, 4);
    expect_eq(sum.binary_files, 1);
}

test("git_diff_stat: hunk lines starting with +++ or --- are counted while file headers are excluded") {
    const patch =
        "diff --git a/doc.md b/doc.md\n" ++
        "--- a/doc.md\n" ++
        "+++ b/doc.md\n" ++
        "@@ -1,2 +1,2 @@\n" ++
        "----\n" ++
        "++++\n" ++
        "\\ No newline at end of file\n";

    let it = GitDiffIter.init(patch);
    const f = it.next().?;
    expect_eq(f.additions, 1);
    expect_eq(f.deletions, 1);
    expect_eq(f.hunks, 1);
}

test("git_diff_stat: GIT binary patch marker detection") {
    const patch =
        "diff --git a/bin.dat b/bin.dat\n" ++
        "GIT binary patch\n" ++
        "literal 4\n" ++
        "LcmZQz0000100001\n";

    const sum = summarizeGitDiff(patch);
    expect_eq(sum.files_changed, 1);
    expect_eq(sum.binary_files, 1);
    expect_eq(sum.insertions, 0);
    expect_eq(sum.deletions, 0);
}

test("git_diff_stat: pure rename with 100% similarity and zero hunks") {
    const patch =
        "diff --git a/a.cp b/b.cp\r\n" ++
        "similarity index 100%\r\n" ++
        "rename from a.cp\r\n" ++
        "rename to b.cp\r\n";

    let it = GitDiffIter.init(patch);
    const f = it.next().?;
    expect_eq(f.kind == .renamed, true);
    expect_eq(std.mem.eql(u8, f.old_path, "a.cp"), true);
    expect_eq(std.mem.eql(u8, f.new_path, "b.cp"), true);
    expect_eq(f.hunks, 0);
    expect_eq(f.additions, 0);
    expect_eq(f.deletions, 0);
}

test("git_diff_stat: empty diff and non-diff text return zero summary") {
    const s1 = summarizeGitDiff("");
    expect_eq(s1.files_changed, 0);
    expect_eq(s1.insertions, 0);
    expect_eq(s1.deletions, 0);
    expect_eq(s1.binary_files, 0);

    const s2 = summarizeGitDiff("just some commit message\nwithout any diff\n");
    expect_eq(s2.files_changed, 0);
}
