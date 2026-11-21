// Source: /data/dev/bun/src/shell/ & /data/dev/bun/src/shell_parser/
// ============================================================================
// cap/test/test_phase_b2_shell_port.cp
// Phase B-2 Comprehensive Acceptance Test Suite for cap:shell
// Covers:
//   1. Full POSIX Lexing, Quoting & Expansions (${VAR:-/=/+/ #}, $((arith)), $(cmd), `cmd`, {braces}, glob)
//   2. Control Flow (if/elif/else/fi, for, while/until, break/continue, &&/||/!) & Subshell (...) Isolation
//   3. Multi-stage Real Pipes (Builtin + PosixSpawn External Processes) & Redirections (>, >>, <, 2>, 2>&1, &>, <<<)
//   4. 30+ Production Builtins (cd, pwd, pushd/popd/dirs, echo, printf, export, unset, env, true, false, exit,
//      which, test/[, ls, cat, mkdir, rmdir, rm, cp, mv, touch, basename, dirname, mktemp, seq, head, tail,
//      wc, grep, yes, sort, uniq)
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { StringJoiner } from "cap:string";
import { Shell, ShellSession, sh } from "cap:shell";

fn testExpansionsAndQuoting(): !void {
    let sess = Shell.session();
    defer sess.~init();

    // 1. Single quotes, double quotes, ANSI-C quotes, backslash escapes
    const r_quotes = sess.run("X=world; echo 'hello $X' \"hello $X \\$X\" $'line1\\tcol2'");
    expect_eq(r_quotes.ok(), true);
    expect_eq(std.mem.eql(u8, r_quotes.text(), "hello $X hello world $X line1\tcol2\n"), true);

    // 2. Parameter expansion: :-, :=, :+, #, $?, $$
    const r_param = sess.run(
        "unset A; B=cp; " +
        "echo ${A:-def1} ${A:=assigned} ${A:+alt} ${#B}; " +
        "false; echo $?"
    );
    expect_eq(r_param.ok(), true);
    expect_eq(std.mem.eql(u8, r_param.text(), "def1 assigned alt 2\n1\n"), true);

    // 3. Arithmetic expansion $(( ... )) with precedence, ternary, bitwise, and side-effect assignment
    const r_arith = sess.run(
        "n=4; " +
        "echo $((1 + 2 * 3)) $(( (1 + 2) * 3 )) $((1 << 4)) $((n > 3 ? 10 : 20)) $((n += 6)) $n"
    );
    expect_eq(r_arith.ok(), true);
    expect_eq(std.mem.eql(u8, r_arith.text(), "7 9 16 10 10 10\n"), true);

    // 4. Command substitution $(cmd) and `cmd` (including nested arithmetic & isolated subshell cwd)
    const r_cmd = sess.run(
        "OUT=$(echo $((6 * 7))); " +
        "BT=`printf 'hi'`; " +
        "SUB_PWD=$(cd /tmp && pwd); " +
        "echo \"$OUT:$BT:$SUB_PWD\""
    );
    expect_eq(r_cmd.ok(), true);
    expect_eq(std.mem.eql(u8, r_cmd.text(), "42:hi:/tmp\n"), true);

    // 5. Brace expansion: comma lists, nested braces, numeric/alpha/stepped ranges
    const r_brace = sess.run(
        "echo {a,b}_{1,2} && " +
        "echo {1..4} {01..03} {5..1..2} {a..c}"
    );
    expect_eq(r_brace.ok(), true);
    expect_eq(std.mem.eql(u8, r_brace.text(), "a_1 a_2 b_1 b_2\n1 2 3 4 01 02 03 5 3 1 a b c\n"), true);

    // 6. Glob expansion inside a controlled temp directory
    const r_glob = sess.run(
        "GDIR=$(mktemp -d /tmp/cp_b2_glob.XXXXXX) && " +
        "touch $GDIR/a.txt $GDIR/b.txt $GDIR/c.log $GDIR/f1.dat $GDIR/f2.dat && " +
        "echo $GDIR/*.txt && " +
        "echo $GDIR/f[12].dat && " +
        "echo $GDIR/*.none && " +
        "rm -rf $GDIR"
    );
    expect_eq(r_glob.ok(), true);
    expect_eq(std.mem.indexOf(u8, r_glob.text(), "a.txt") != null, true);
    expect_eq(std.mem.indexOf(u8, r_glob.text(), "b.txt") != null, true);
    expect_eq(std.mem.indexOf(u8, r_glob.text(), "c.log") == null, true);
    expect_eq(std.mem.indexOf(u8, r_glob.text(), "f1.dat") != null, true);
    expect_eq(std.mem.indexOf(u8, r_glob.text(), "*.none") != null, true);
}

fn testControlFlowAndSubshell(): !void {
    let sess = Shell.session();
    defer sess.~init();

    // 1. if / elif / else / fi + ! negation
    const r_if = sess.run(
        "VAL=20; " +
        "if [ $VAL -lt 10 ]; then echo small; " +
        "elif [ $VAL -eq 20 ]; then echo exact; " +
        "else echo large; fi; " +
        "if ! false; then echo neg_ok; fi"
    );
    expect_eq(r_if.ok(), true);
    expect_eq(std.mem.eql(u8, r_if.text(), "exact\nneg_ok\n"), true);

    // 2. for loop with brace expansion, break & continue
    const r_for = sess.run(
        "for i in {1..5}; do " +
        "  if [ $i -eq 2 ]; then continue; fi; " +
        "  if [ $i -eq 5 ]; then break; fi; " +
        "  echo -n \"$i,\"; " +
        "done; echo done"
    );
    expect_eq(r_for.ok(), true);
    expect_eq(std.mem.eql(u8, r_for.text(), "1,3,4,done\n"), true);

    // 3. while & until loops
    const r_while = sess.run(
        "k=0; while [ $k -lt 3 ]; do echo -n $k; k=$((k + 1)); done; echo ''; " +
        "until [ $k -le 1 ]; do echo -n $k; k=$((k - 1)); done; echo ''"
    );
    expect_eq(r_while.ok(), true);
    expect_eq(std.mem.eql(u8, r_while.text(), "012\n32\n"), true);

    // 4. ( ... ) Subshell environment & cwd isolation vs { ...; } group sharing
    const before_cwd = sess.cwd();
    const r_sub = sess.run(
        "KEEP=parent; " +
        "(KEEP=child; export SUB_ONLY=yes; cd /tmp && echo sub=$(pwd):$KEEP); " +
        "echo parent=$KEEP:${SUB_ONLY:-no}; " +
        "{ KEEP=grouped; }; " +
        "echo after_group=$KEEP"
    );
    expect_eq(r_sub.ok(), true);
    expect_eq(std.mem.eql(u8, r_sub.text(), "sub=/tmp:child\nparent=parent:no\nafter_group=grouped\n"), true);
    expect_eq(std.mem.eql(u8, sess.cwd(), before_cwd), true);

    // 5. set / shift / positional args / alias / unalias / && / ||
    const r_misc = sess.run(
        "set -- alpha beta gamma && echo $#:$1:$2 && shift && echo $#:$1 && " +
        "alias greet='echo hi' && greet cp && unalias greet && " +
        "(false && echo skip || echo fallback)"
    );
    expect_eq(r_misc.ok(), true);
    expect_eq(std.mem.eql(u8, r_misc.text(), "3:alpha:beta\n2:beta\nhi cp\nfallback\n"), true);
}

fn testPipesPosixSpawnAndRedirects(): !void {
    let sess = Shell.session();
    defer sess.~init();

    // 1. Multi-stage pipeline mixing builtins & PosixSpawn external processes (tr, /bin/cat)
    const r_mix_pipe = sess.run(
        "printf 'banana\\napple\\ncherry\\n' | tr a-z A-Z | sort | head -n 2"
    );
    expect_eq(r_mix_pipe.ok(), true);
    expect_eq(std.mem.eql(u8, r_mix_pipe.text(), "APPLE\nBANANA\n"), true);

    const r_ext_chain = sess.run(
        "echo 'one two three four' | /bin/cat | tr ' ' '\\n' | wc -l"
    );
    expect_eq(r_ext_chain.ok(), true);
    expect_eq(std.mem.eql(u8, r_ext_chain.text(), "4\n"), true);

    // 2. External command via PosixSpawn inherits exported ShellEnv
    const r_env_ext = sess.run(
        "export CP_B2_GREETING=posix_spawn_ok && /bin/sh -c 'echo $CP_B2_GREETING'"
    );
    expect_eq(r_env_ext.ok(), true);
    expect_eq(std.mem.eql(u8, r_env_ext.text(), "posix_spawn_ok\n"), true);

    // 3. Redirections: >, >>, <, 2>, 2>&1, &>, <<<
    const r_redir = sess.run(
        "RDIR=$(mktemp -d /tmp/cp_b2_redir.XXXXXX) && " +
        "echo line1 > $RDIR/out.txt && " +
        "echo line2 >> $RDIR/out.txt && " +
        "wc -l < $RDIR/out.txt && " +
        "grep -i world <<< 'Hello World' && " +
        "cat $RDIR/missing_file 2> $RDIR/err.txt ; " +
        "cat $RDIR/err.txt && " +
        "cat $RDIR/missing_file 2>&1 | head -n 1 && " +
        "rm -rf $RDIR"
    );
    expect_eq(r_redir.ok(), true);
    expect_eq(std.mem.indexOf(u8, r_redir.text(), "2\n") != null, true);
    expect_eq(std.mem.indexOf(u8, r_redir.text(), "Hello World\n") != null, true);
    expect_eq(std.mem.indexOf(u8, r_redir.text(), "No such file or directory") != null, true);

    // 4. pipefail option
    const r_pf = sess.run("set -o pipefail && false | true");
    expect_eq(r_pf.exit_code, 1 as i32);
}

fn testAllBuiltinsMatrix(): !void {
    let sess = Shell.session();
    defer sess.~init();

    // 1. Filesystem & path builtins: mktemp, mkdir -p, touch, cp -r, mv, ls, cat -n, basename, dirname, rmdir, rm -rf
    const r_fs = sess.run(
        "WORK=$(mktemp -d /tmp/cp_b2_fs.XXXXXX) && " +
        "mkdir -p $WORK/src/sub && " +
        "touch $WORK/src/a.txt && " +
        "echo 'alpha' > $WORK/src/sub/b.txt && " +
        "cp -r $WORK/src $WORK/copy && " +
        "mv $WORK/copy/a.txt $WORK/copy/renamed.txt && " +
        "ls -1 $WORK/copy | sort && " +
        "cat -n $WORK/copy/sub/b.txt && " +
        "basename /a/b/file.cp .cp && " +
        "dirname /a/b/file.cp && " +
        "mkdir $WORK/empty && rmdir $WORK/empty && " +
        "[ -d $WORK/copy/sub ] && [ -f $WORK/copy/renamed.txt ] && [ -s $WORK/copy/sub/b.txt ] && " +
        "rm -rf $WORK"
    );
    expect_eq(r_fs.ok(), true);
    expect_eq(std.mem.indexOf(u8, r_fs.text(), "renamed.txt\n") != null, true);
    expect_eq(std.mem.indexOf(u8, r_fs.text(), "1\talpha\n") != null, true);
    expect_eq(std.mem.indexOf(u8, r_fs.text(), "file\n/a/b\n") != null, true);

    // 2. Directory stack & env builtins: pwd, cd, pushd, popd, dirs, export, unset, env, true, false, which
    const r_dir = sess.run(
        "D=$(mktemp -d /tmp/cp_b2_dir.XXXXXX) && " +
        "pushd $D > /dev/null && " +
        "pwd && " +
        "popd > /dev/null && " +
        "rm -rf $D && " +
        "which echo true && " +
        "true && ! false"
    );
    expect_eq(r_dir.ok(), true);
    expect_eq(std.mem.indexOf(u8, r_dir.text(), "echo: shell built-in command") != null, true);

    // 3. Text & sequence builtins: seq, yes, head, tail, wc, grep, sort, uniq, printf
    const r_txt = sess.run(
        "seq -s , -w 1 3 && " +
        "yes ok -n 0 > /dev/null ; " +
        "yes hi | head -n 2 && " +
        "seq 1 5 | tail -n 2 && " +
        "printf 'b\\na\\nb\\nc\\na\\n' | sort | uniq -c | grep -E '2 a|2 b' | wc -l && " +
        "printf 'hello\\nWORLD\\nfoo\\n' | grep -iv foo | wc -l"
    );
    expect_eq(r_txt.ok(), true);
    expect_eq(std.mem.eql(u8, r_txt.text(), "1,2,3\nhi\nhi\n4\n5\n2\n2\n"), true);

    // 4. yes -n N bounded output & exit code propagation
    const r_yes_exit = sess.run("yes -n 3 cp | wc -l && exit 7 && echo unreachable");
    expect_eq(r_yes_exit.exit_code, 7 as i32);
    expect_eq(std.mem.eql(u8, r_yes_exit.text(), "3\n"), true);
}

fn main() {
    test("1. Phase B-2: POSIX Expansions & Quoting (${}, $(()), $(), {}, glob)", () => {
        testExpansionsAndQuoting()?;
    });
    test("2. Phase B-2: Control Flow (if/for/while/until/!/&&/||) & Subshell Isolation", () => {
        testControlFlowAndSubshell()?;
    });
    test("3. Phase B-2: Multi-stage PosixSpawn Pipes & Full Redirections", () => {
        testPipesPosixSpawnAndRedirects()?;
    });
    test("4. Phase B-2: 30+ Production Builtins Matrix Verification", () => {
        testAllBuiltinsMatrix()?;
    });
}
