import { test, expect_eq } from "cap:test";
import {
    HasArg,
    Occur,
    Options,
    Matches,
    GOPT_OK,
    GOPT_ERR_ARGUMENT_MISSING,
    GOPT_ERR_UNRECOGNIZED_OPTION,
    GOPT_ERR_OPTION_MISSING,
    GOPT_ERR_OPTION_DUPLICATED,
    GOPT_ERR_UNEXPECTED_ARGUMENT,
} from "../../packages/getopts/src/index.cp";

fn main() {
    test("1. @cap/getopts — clustered short flags (-abc) & optPresent/optCount", () => {
        let opts: Options = Options.new();
        opts.optflag("a", "all", "show all");
        opts.optflag("b", "brief", "brief mode");
        opts.optflag("c", "color", "color output");
        opts.optflag("d", "debug", "debug mode");

        const argv: [1]string = ["-abc"];
        const m: Matches = opts.parse(argv[0..1]);

        expect_eq(m.ok, true);
        expect_eq(m.err_kind, GOPT_OK);
        expect_eq(m.optPresent("a"), true);
        expect_eq(m.optPresent("all"), true);
        expect_eq(m.optPresent("b"), true);
        expect_eq(m.optPresent("c"), true);
        expect_eq(m.optPresent("d"), false);
        expect_eq(m.optCount("color"), 1 as usize);
    });

    test("2. @cap/getopts — long --opt=val, separate -o val & attached -oval", () => {
        let opts: Options = Options.new();
        opts.optopt("o", "output", "output file");
        opts.optopt("m", "mode", "run mode");

        const argv1: [3]string = ["--output=app.bin", "-m", "release"];
        const m1: Matches = opts.parse(argv1[0..3]);
        expect_eq(m1.ok, true);
        expect_eq(m1.optStr("o").?, "app.bin");
        expect_eq(m1.optStr("output").?, "app.bin");
        expect_eq(m1.optStr("mode").?, "release");

        const argv2: [1]string = ["-oout.txt"];
        const m2: Matches = opts.parse(argv2[0..1]);
        expect_eq(m2.ok, true);
        expect_eq(m2.optStr("o").?, "out.txt");
    });

    test("3. @cap/getopts — optmulti collects multiple values via optStrs", () => {
        let opts: Options = Options.new();
        opts.optmulti("I", "include", "include path");

        const argv: [4]string = ["-I", "/usr/inc", "--include=/opt/inc", "-I./local"];
        const m: Matches = opts.parse(argv[0..4]);

        expect_eq(m.ok, true);
        expect_eq(m.optCount("I"), 3 as usize);
        expect_eq(m.optStr("include").?, "/usr/inc");

        let vals: [4]string = [""; 4];
        const n: usize = m.optStrs("include", vals[0..4]);
        expect_eq(n, 3 as usize);
        expect_eq(vals[0], "/usr/inc");
        expect_eq(vals[1], "/opt/inc");
        expect_eq(vals[2], "./local");
    });

    test("4. @cap/getopts — double-dash (--) preserves flag-like tokens as free args", () => {
        let opts: Options = Options.new();
        opts.optflag("v", "verbose", "verbose");

        const argv: [5]string = ["-v", "input.cp", "--", "--not-a-flag", "-x"];
        const m: Matches = opts.parse(argv[0..5]);

        expect_eq(m.ok, true);
        expect_eq(m.optPresent("v"), true);
        expect_eq(m.freeCount(), 3 as usize);
        expect_eq(m.freeAt(0).?, "input.cp");
        expect_eq(m.freeAt(1).?, "--not-a-flag");
        expect_eq(m.freeAt(2).?, "-x");
        expect_eq(m.freeAt(3) == null, true);
    });

    test("5. @cap/getopts — reqopt validation fails when required option is missing", () => {
        let opts: Options = Options.new();
        opts.reqopt("c", "config", "config path");
        opts.optflag("q", "quiet", "quiet");

        const argv_missing: [1]string = ["-q"];
        const m_fail: Matches = opts.parse(argv_missing[0..1]);
        expect_eq(m_fail.ok, false);
        expect_eq(m_fail.err_kind, GOPT_ERR_OPTION_MISSING);

        const argv_ok: [2]string = ["--config=cap.json", "-q"];
        const m_ok: Matches = opts.parse(argv_ok[0..2]);
        expect_eq(m_ok.ok, true);
        expect_eq(m_ok.optStr("c").?, "cap.json");
    });

    test("6. @cap/getopts — unrecognized, missing-arg, duplicate & unexpected-arg errors", () => {
        let opts: Options = Options.new();
        opts.optflag("f", "flag", "a flag");
        opts.optopt("o", "out", "output");
        opts.opt("x", "extra", "maybe arg", HasArg.Maybe, Occur.Optional);

        // Unrecognized option
        const a1: [1]string = ["--unknown"];
        const m1: Matches = opts.parse(a1[0..1]);
        expect_eq(m1.ok, false);
        expect_eq(m1.err_kind, GOPT_ERR_UNRECOGNIZED_OPTION);

        // Missing required argument for -o
        const a2: [1]string = ["-o"];
        const m2: Matches = opts.parse(a2[0..1]);
        expect_eq(m2.ok, false);
        expect_eq(m2.err_kind, GOPT_ERR_ARGUMENT_MISSING);

        // Duplicate non-multi option
        const a3: [2]string = ["-f", "--flag"];
        const m3: Matches = opts.parse(a3[0..2]);
        expect_eq(m3.ok, false);
        expect_eq(m3.err_kind, GOPT_ERR_OPTION_DUPLICATED);

        // Unexpected inline argument on HasArg.No flag
        const a4: [1]string = ["--flag=1"];
        const m4: Matches = opts.parse(a4[0..1]);
        expect_eq(m4.ok, false);
        expect_eq(m4.err_kind, GOPT_ERR_UNEXPECTED_ARGUMENT);

        // HasArg.Maybe without value succeeds
        const a5: [1]string = ["--extra"];
        const m5: Matches = opts.parse(a5[0..1]);
        expect_eq(m5.ok, true);
        expect_eq(m5.optPresent("x"), true);
        expect_eq(m5.optStr("x") == null, true);
    });
}
