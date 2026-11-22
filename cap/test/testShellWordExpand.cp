// Deep production regression tests for cap:shell wordExpand.cp
// Covers: tilde expansion (`~`, `~/sub`, `~user` non-expansion), quoted empty string preservation
// vs unquoted empty var elision, unquoted vs quoted command substitution newline folding & splitting,
// single vs array output mode, unquoted glob suspension + POSIX zero-match fallback, and state error guards.

import { test, expect, expectEq } from "cap:test";
import {
    WordExpandMode,
    WordAtomKind,
    WordExpandStepKind,
    WordAtom,
    WordExpandOutput,
    stripTrailingNewlines,
    hasUnquotedGlobMeta,
    normalizeUnquotedSubshellStdout,
    expandLeadingTilde,
    ShellWordExpander,
} from "cap:shell";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("wordExpand: pure helper functions for tilde, trailing newlines, glob meta, and stdout norm", fn () {
    expect(strEq(stripTrailingNewlines("hello\n\n\n"), "hello"));
    expect(strEq(stripTrailingNewlines("no-nl"), "no-nl"));
    expect(strEq(stripTrailingNewlines("\n\n"), ""));

    expect(hasUnquotedGlobMeta("src/*.cp"));
    expect(hasUnquotedGlobMeta("file?.txt"));
    expect(hasUnquotedGlobMeta("a[0-9]"));
    expect(!hasUnquotedGlobMeta("plain_file.cp"));

    let buf: [128]u8 = [0; 128];
    const n1 = normalizeUnquotedSubshellStdout("  foo\nbar\nbaz \n\n", buf[0..]) catch unreachable;
    expect(strEq(buf[0..n1], "foo bar baz"));

    const t1 = expandLeadingTilde("/home/dev", "", buf[0..]) catch unreachable;
    expect(strEq(buf[0..t1], "/home/dev"));
    const t2 = expandLeadingTilde("/home/dev", "/project", buf[0..]) catch unreachable;
    expect(strEq(buf[0..t2], "/home/dev/project"));
    const t3 = expandLeadingTilde("/home/dev", "bob/work", buf[0..]) catch unreachable;
    expect(strEq(buf[0..t3], "~bob/work"));
});

test("wordExpand: quoted empty string emits 1 empty word while unquoted empty var emits 0 words", fn () {
    // 1. Quoted empty literal "" -> 1 word ("")
    let out1 = WordExpandOutput.init();
    let exp1 = ShellWordExpander.init(.Array, "/root");
    * = exp1.pushAtom(WordAtom{ kind: .QuotedLiteral, text: "" }) catch unreachable;
    const st1 = exp1.step(*out1) catch unreachable;
    expectEq(st1.kind, WordExpandStepKind.Done);
    expectEq(out1.wordCount(), @as(usize, 1));
    expect(strEq(out1.wordAt(0), ""));

    // 2. Unquoted unset/empty $MISSING -> 0 words
    let out2 = WordExpandOutput.init();
    let exp2 = ShellWordExpander.init(.Array, "/root");
    * = exp2.pushAtom(WordAtom{ kind: .Var, text: "MISSING" }) catch unreachable;
    const st2 = exp2.step(*out2) catch unreachable;
    expectEq(st2.kind, WordExpandStepKind.Done);
    expectEq(out2.wordCount(), @as(usize, 0));

    // 3. Quoted unset "$MISSING" -> 1 empty word
    let out3 = WordExpandOutput.init();
    let exp3 = ShellWordExpander.init(.Array, "/root");
    * = exp3.pushAtom(WordAtom{ kind: .QuotedVar, text: "MISSING" }) catch unreachable;
    const st3 = exp3.step(*out3) catch unreachable;
    expectEq(st3.kind, WordExpandStepKind.Done);
    expectEq(out3.wordCount(), @as(usize, 1));
    expect(strEq(out3.wordAt(0), ""));
});

test("wordExpand: unquoted var splits words in Array mode but preserves spaces in QuotedVar/Single mode", fn () {
    let out_arr = WordExpandOutput.init();
    let exp_arr = ShellWordExpander.init(.Array, "/root");
    * = exp_arr.setVar("FLAGS", "-O2   --strip\t--fast") catch unreachable;
    * = exp_arr.pushAtom(WordAtom{ kind: .Var, text: "FLAGS" }) catch unreachable;
    const st_arr = exp_arr.step(*out_arr) catch unreachable;
    expectEq(st_arr.kind, WordExpandStepKind.Done);
    expectEq(out_arr.wordCount(), @as(usize, 3));
    expect(strEq(out_arr.wordAt(0), "-O2"));
    expect(strEq(out_arr.wordAt(1), "--strip"));
    expect(strEq(out_arr.wordAt(2), "--fast"));

    let out_q = WordExpandOutput.init();
    let exp_q = ShellWordExpander.init(.Array, "/root");
    * = exp_q.setVar("MSG", "hello   world") catch unreachable;
    * = exp_q.pushAtom(WordAtom{ kind: .QuotedVar, text: "MSG" }) catch unreachable;
    const st_q = exp_q.step(*out_q) catch unreachable;
    expectEq(st_q.kind, WordExpandStepKind.Done);
    expectEq(out_q.wordCount(), @as(usize, 1));
    expect(strEq(out_q.wordAt(0), "hello   world"));
});

test("wordExpand: unquoted vs quoted command substitution step driving", fn () {
    // Unquoted $(cmd) in Array mode folds newlines and splits into 3 args
    let out = WordExpandOutput.init();
    let exp = ShellWordExpander.init(.Array, "/root");
    * = exp.pushAtom(WordAtom{ kind: .Literal, text: "pre-" }) catch unreachable;
    * = exp.pushAtom(WordAtom{ kind: .CmdSubst, cmd_id: 7 }) catch unreachable;
    * = exp.pushAtom(WordAtom{ kind: .Literal, text: "-post" }) catch unreachable;

    const s1 = exp.step(*out) catch unreachable;
    expectEq(s1.kind, WordExpandStepKind.NeedCmdSubst);
    expectEq(s1.cmd_id, @as(u32, 7));
    expect(!s1.quoted);

    * = exp.onCmdSubstDone("alpha\nbeta  gamma\n\n", *out) catch unreachable;
    const s2 = exp.step(*out) catch unreachable;
    expectEq(s2.kind, WordExpandStepKind.Done);
    expectEq(out.wordCount(), @as(usize, 3));
    expect(strEq(out.wordAt(0), "pre-alpha"));
    expect(strEq(out.wordAt(1), "beta"));
    expect(strEq(out.wordAt(2), "gamma-post"));

    // Quoted "$(cmd)" keeps internal newlines and strips only trailing newlines
    let out2 = WordExpandOutput.init();
    let exp2 = ShellWordExpander.init(.Array, "/root");
    * = exp2.pushAtom(WordAtom{ kind: .QuotedCmdSubst, cmd_id: 9 }) catch unreachable;
    const sq1 = exp2.step(*out2) catch unreachable;
    expectEq(sq1.kind, WordExpandStepKind.NeedCmdSubst);
    expect(sq1.quoted);
    * = exp2.onCmdSubstDone("line1\nline2\n\n", *out2) catch unreachable;
    const sq2 = exp2.step(*out2) catch unreachable;
    expectEq(sq2.kind, WordExpandStepKind.Done);
    expectEq(out2.wordCount(), @as(usize, 1));
    expect(strEq(out2.wordAt(0), "line1\nline2"));
});

test("wordExpand: unquoted glob triggers NeedGlobWalk with matches and POSIX zero-match fallback", fn () {
    // 1. Glob matches -> emits matched paths
    let out1 = WordExpandOutput.init();
    let exp1 = ShellWordExpander.init(.Array, "/home/u");
    * = exp1.pushAtom(WordAtom{ kind: .Tilde }) catch unreachable;
    * = exp1.pushAtom(WordAtom{ kind: .Literal, text: "/*.cp" }) catch unreachable;

    const g1 = exp1.step(*out1) catch unreachable;
    expectEq(g1.kind, WordExpandStepKind.NeedGlobWalk);
    expect(strEq(exp1.pendingGlobPattern(), "/home/u/*.cp"));

    const found: [2][]const u8 = ["/home/u/a.cp", "/home/u/b.cp"];
    * = exp1.onGlobDone(found[0..], *out1) catch unreachable;
    expect(exp1.isDone());
    expectEq(out1.wordCount(), @as(usize, 2));
    expect(strEq(out1.wordAt(0), "/home/u/a.cp"));
    expect(strEq(out1.wordAt(1), "/home/u/b.cp"));

    // 2. Zero matches -> POSIX fallback keeps literal pattern
    let out2 = WordExpandOutput.init();
    let exp2 = ShellWordExpander.init(.Array, "/home/u");
    * = exp2.pushAtom(WordAtom{ kind: .GlobPattern, text: "nomatch_*.xyz" }) catch unreachable;
    const g2 = exp2.step(*out2) catch unreachable;
    expectEq(g2.kind, WordExpandStepKind.NeedGlobWalk);
    const empty: [0][]const u8 = [];
    * = exp2.onGlobDone(empty[0..], *out2) catch unreachable;
    expectEq(out2.wordCount(), @as(usize, 1));
    expect(strEq(out2.wordAt(0), "nomatch_*.xyz"));
});

test("wordExpand: state machine guards against out-of-order callbacks and double step while suspended", fn () {
    let out = WordExpandOutput.init();
    let exp = ShellWordExpander.init(.Array, "/root");
    * = exp.pushAtom(WordAtom{ kind: .CmdSubst, cmd_id: 1 }) catch unreachable;

    // Calling onCmdSubstDone before step() must fail
    let err_not_waiting = false;
    * = exp.onCmdSubstDone("ok", *out) catch {
        err_not_waiting = true;
    };
    expect(err_not_waiting);

    const s1 = exp.step(*out) catch unreachable;
    expectEq(s1.kind, WordExpandStepKind.NeedCmdSubst);

    // Calling step() again while awaiting cmd subst must fail
    let err_still_waiting = false;
    * = exp.step(*out) catch {
        err_still_waiting = true;
    };
    expect(err_still_waiting);
});
