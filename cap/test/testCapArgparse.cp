// cap/test/testCapArgparse.cp
// Comprehensive unit tests for @cap/argparse

import { test, expect_eq } from "cap:test";
import {
    ArgAction,
    ArgDef,
    ArgParser,
    ARGP_ERR_NONE,
    ARGP_ERR_UNKNOWN_FLAG,
    ARGP_ERR_MISSING_VALUE,
    ARGP_ERR_MISSING_REQUIRED,
} from "../../packages/@cap/argparse/src/index.cp";

fn main() {
    test("1. @cap/argparse — long options (--output=file and --output file) & defaults", () => {
        let p = ArgParser.init();
        expect_eq(p.addArgument(ArgDef{
            short_flag: 'o',
            long_name: "--output",
            dest: "output",
            action: .Store,
            required: false,
            default_val: "out.txt",
            is_positional: false,
        }), true);
        expect_eq(p.addArgument(ArgDef{
            short_flag: 'f',
            long_name: "--format",
            dest: "format",
            action: .Store,
            required: false,
            default_val: "json",
            is_positional: false,
        }), true);

        const argv1: [2]string = ["--output=bundle.js", "--format"];
        const res1 = p.parseArgs(argv1[0..2]);
        expect_eq(res1.ok, false);
        expect_eq(res1.err_code, ARGP_ERR_MISSING_VALUE);

        const argv2: [3]string = ["--output=bundle.js", "--format", "yaml"];
        const res2 = p.parseArgs(argv2[0..3]);
        expect_eq(res2.ok, true);
        expect_eq(res2.err_code, ARGP_ERR_NONE);
        if (o := res2.getString("output")) {
            expect_eq(std.mem.eql(u8, o, "bundle.js"), true);
        } else {
            expect_eq(true, false);
        }
        if (f := res2.getString("format")) {
            expect_eq(std.mem.eql(u8, f, "yaml"), true);
        } else {
            expect_eq(true, false);
        }

        // Default values when omitted
        let empty: [0]string = undefined;
        const res3 = p.parseArgs(empty[0..0]);
        expect_eq(res3.ok, true);
        if (o := res3.getString("output")) {
            expect_eq(std.mem.eql(u8, o, "out.txt"), true);
        } else {
            expect_eq(true, false);
        }
    });

    test("2. @cap/argparse — StoreTrue, StoreFalse, Count (-v, -vvv)", () => {
        let p = ArgParser.init();
        * = p.addArgument(ArgDef{
            short_flag: 'v',
            long_name: "--verbose",
            dest: "verbose",
            action: .Count,
            required: false,
            default_val: "",
            is_positional: false,
        });
        * = p.addArgument(ArgDef{
            short_flag: 'd',
            long_name: "--debug",
            dest: "debug",
            action: .StoreTrue,
            required: false,
            default_val: "",
            is_positional: false,
        });
        * = p.addArgument(ArgDef{
            short_flag: 'q',
            long_name: "--no-color",
            dest: "color",
            action: .StoreFalse,
            required: false,
            default_val: "",
            is_positional: false,
        });

        const argv1: [3]string = ["-vvv", "-v", "--debug"];
        const res1 = p.parseArgs(argv1[0..3]);
        expect_eq(res1.ok, true);
        expect_eq(res1.getCount("verbose"), 4 as u32);
        expect_eq(res1.getBool("debug"), true);
        expect_eq(res1.getBool("color"), true);

        const argv2: [1]string = ["--no-color"];
        const res2 = p.parseArgs(argv2[0..1]);
        expect_eq(res2.ok, true);
        expect_eq(res2.getBool("color"), false);
        expect_eq(res2.getBool("debug"), false);
        expect_eq(res2.getCount("verbose"), 0 as u32);
    });

    test("3. @cap/argparse — bundled short flags (-ab, -abvalue)", () => {
        let p = ArgParser.init();
        * = p.addArgument(ArgDef{
            short_flag: 'a',
            long_name: "--all",
            dest: "all",
            action: .StoreTrue,
            required: false,
            default_val: "",
            is_positional: false,
        });
        * = p.addArgument(ArgDef{
            short_flag: 'b',
            long_name: "--bare",
            dest: "bare",
            action: .StoreTrue,
            required: false,
            default_val: "",
            is_positional: false,
        });
        * = p.addArgument(ArgDef{
            short_flag: 'o',
            long_name: "--out",
            dest: "out",
            action: .Store,
            required: false,
            default_val: "",
            is_positional: false,
        });

        const argv: [2]string = ["-ab", "-oapp.bin"];
        const res = p.parseArgs(argv[0..2]);
        expect_eq(res.ok, true);
        expect_eq(res.getBool("all"), true);
        expect_eq(res.getBool("bare"), true);
        if (o := res.getString("out")) {
            expect_eq(std.mem.eql(u8, o, "app.bin"), true);
        } else {
            expect_eq(true, false);
        }
    });

    test("4. @cap/argparse — double-dash (--) stops option parsing & preserves positionals", () => {
        let p = ArgParser.init();
        * = p.addArgument(ArgDef{
            short_flag: 'v',
            long_name: "--verbose",
            dest: "verbose",
            action: .StoreTrue,
            required: false,
            default_val: "",
            is_positional: false,
        });

        const argv: [4]string = ["-v", "--", "-v", "--not-a-flag"];
        const res = p.parseArgs(argv[0..4]);
        expect_eq(res.ok, true);
        expect_eq(res.getBool("verbose"), true);
        expect_eq(res.positionalCount(), 2 as usize);
        if (p0 := res.positionalAt(0)) {
            expect_eq(std.mem.eql(u8, p0, "-v"), true);
        } else {
            expect_eq(true, false);
        }
        if (p1 := res.positionalAt(1)) {
            expect_eq(std.mem.eql(u8, p1, "--not-a-flag"), true);
        } else {
            expect_eq(true, false);
        }
        expect_eq(res.positionalAt(2) == null, true);
    });

    test("5. @cap/argparse — required argument validation & named positionals", () => {
        let p = ArgParser.init();
        * = p.addArgument(ArgDef{
            short_flag: 'c',
            long_name: "--config",
            dest: "config",
            action: .Store,
            required: true,
            default_val: "",
            is_positional: false,
        });
        * = p.addArgument(ArgDef{
            short_flag: 0,
            long_name: "entry",
            dest: "entry",
            action: .Store,
            required: true,
            default_val: "",
            is_positional: true,
        });

        const missing_cfg: [1]string = ["main.cp"];
        const r1 = p.parseArgs(missing_cfg[0..1]);
        expect_eq(r1.ok, false);
        expect_eq(r1.err_code, ARGP_ERR_MISSING_REQUIRED);

        const valid: [3]string = ["-c", "cap.json", "main.cp"];
        const r2 = p.parseArgs(valid[0..3]);
        expect_eq(r2.ok, true);
        if (cfg := r2.getString("config")) {
            expect_eq(std.mem.eql(u8, cfg, "cap.json"), true);
        } else {
            expect_eq(true, false);
        }
        if (ent := r2.getString("entry")) {
            expect_eq(std.mem.eql(u8, ent, "main.cp"), true);
        } else {
            expect_eq(true, false);
        }
    });

    test("6. @cap/argparse — unknown flag detection & single dash positional", () => {
        let p = ArgParser.init();
        * = p.addArgument(ArgDef{
            short_flag: 'h',
            long_name: "--help",
            dest: "help",
            action: .StoreTrue,
            required: false,
            default_val: "",
            is_positional: false,
        });

        const bad_long: [1]string = ["--unknown"];
        const r1 = p.parseArgs(bad_long[0..1]);
        expect_eq(r1.ok, false);
        expect_eq(r1.err_code, ARGP_ERR_UNKNOWN_FLAG);

        const bad_short: [1]string = ["-z"];
        const r2 = p.parseArgs(bad_short[0..1]);
        expect_eq(r2.ok, false);
        expect_eq(r2.err_code, ARGP_ERR_UNKNOWN_FLAG);

        // Single "-" is treated as a positional argument (stdin convention)
        const stdin_arg: [1]string = ["-"];
        const r3 = p.parseArgs(stdin_arg[0..1]);
        expect_eq(r3.ok, true);
        expect_eq(r3.positionalCount(), 1 as usize);
        if (s := r3.positionalAt(0)) {
            expect_eq(std.mem.eql(u8, s, "-"), true);
        } else {
            expect_eq(true, false);
        }
    });
}
