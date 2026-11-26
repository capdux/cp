import { test, expect_eq } from "cap:test";
import {
    ExecaOptions, ExecaResult, escapePosixArg, formatCommand,
    stripFinalNewline, resolveExecPath
} from "../../packages/@cap/execa/src/index.cp";

fn main() {
    test("1. @cap/execa — escapePosixArg leaves safe tokens unquoted and escapes spaces/quotes", () => {
        let buf: [128]u8 = [0; 128];
        let n1 = escapePosixArg("--flag=value", buf[0..128]);
        expect_eq(buf[0..n1] as string, "--flag=value");

        let n2 = escapePosixArg("hello world", buf[0..128]);
        expect_eq(buf[0..n2] as string, "'hello world'");

        let n3 = escapePosixArg("it's", buf[0..128]);
        expect_eq(buf[0..n3] as string, "'it'\\''s'");
    });

    test("2. @cap/execa — formatCommand builds escaped CLI string", () => {
        let args: [3]string = ["commit", "-m", "fix bug"];
        let buf: [256]u8 = [0; 256];
        let n = formatCommand("git", args[0..3], buf[0..256]);
        expect_eq(buf[0..n] as string, "git commit -m 'fix bug'");
    });

    test("3. @cap/execa — resolveExecPath delegates to @cap/npm-run-path", () => {
        let opts = ExecaOptions.defaults("/repo/pkg", "/usr/bin");
        let buf: [256]u8 = [0; 256];
        let n = resolveExecPath(opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "/repo/pkg/node_modules/.bin:/repo/node_modules/.bin:/node_modules/.bin:/usr/bin");
    });

    test("4. @cap/execa — resolveExecPath with prefer_local=false keeps PATH only", () => {
        let opts = ExecaOptions.defaults("/repo/pkg", "/usr/bin");
        opts.prefer_local = false;
        let buf: [256]u8 = [0; 256];
        let n = resolveExecPath(opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "/usr/bin");
    });

    test("5. @cap/execa — stripFinalNewline strips LF and CRLF once", () => {
        expect_eq(stripFinalNewline("hello\n"), "hello");
        expect_eq(stripFinalNewline("hello\r\n"), "hello");
        expect_eq(stripFinalNewline("hello\n\n"), "hello\n");
        expect_eq(stripFinalNewline("plain"), "plain");
    });

    test("6. @cap/execa — ExecaResult success, failure & timeout states", () => {
        let ok = ExecaResult.success("done");
        expect_eq(ok.isOk(), true);
        expect_eq(ok.stdout, "done");

        let fail = ExecaResult.failure(2, "bad arg");
        expect_eq(fail.isOk(), false);
        expect_eq(fail.failed, true);
        expect_eq(fail.exit_code, 2);

        let to = ExecaResult.timeout();
        expect_eq(to.isOk(), false);
        expect_eq(to.timed_out, true);
    });
}
