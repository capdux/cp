// cap/test/testCapCommander.cp
// Comprehensive unit tests for @cap/commander (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    FlagSpec,
    Command,
    CommandParseResult,
    parseOptionFlags,
} from "../../packages/@cap/commander/src/index.cp";

fn containsSubstr(haystack: string, needle: string): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j = j + 1;
        }
        if (ok) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn main() {
    test("1. @cap/commander — parseOptionFlags for required/boolean/negated/optional-bracket syntax", () => {
        const s1: FlagSpec = parseOptionFlags("-p, --port <number>");
        expect_eq(s1.short_flag, 112 as u8); // 'p'
        expect_eq(s1.long_name, "port");
        expect_eq(s1.arg_name, "number");
        expect_eq(s1.takes_arg, true);
        expect_eq(s1.arg_required, true);
        expect_eq(s1.arg_optional, false);
        expect_eq(s1.negate, false);

        const s2: FlagSpec = parseOptionFlags("-d, --debug");
        expect_eq(s2.short_flag, 100 as u8); // 'd'
        expect_eq(s2.long_name, "debug");
        expect_eq(s2.arg_name, "");
        expect_eq(s2.takes_arg, false);
        expect_eq(s2.arg_required, false);
        expect_eq(s2.negate, false);

        const s3: FlagSpec = parseOptionFlags("--no-color");
        expect_eq(s3.short_flag, 0 as u8);
        expect_eq(s3.long_name, "color");
        expect_eq(s3.takes_arg, false);
        expect_eq(s3.negate, true);

        const s4: FlagSpec = parseOptionFlags("[-f, --file [path]]");
        expect_eq(s4.short_flag, 102 as u8); // 'f'
        expect_eq(s4.long_name, "file");
        expect_eq(s4.arg_name, "path");
        expect_eq(s4.takes_arg, true);
        expect_eq(s4.arg_required, false);
        expect_eq(s4.arg_optional, true);
        expect_eq(s4.negate, false);
    });

    test("2. @cap/commander — option defaults, short/long aliases & --no-* negation", () => {
        let cmd = Command.new("server");
        expect_eq(cmd.option("-p, --port <number>", "Port number", "3000"), true);
        expect_eq(cmd.option("-H, --host <addr>", "Bind host", "127.0.0.1"), true);
        expect_eq(cmd.option("-d, --debug", "Verbose debug", ""), true);
        expect_eq(cmd.option("--no-color", "Disable ANSI color", ""), true);

        // Empty argv -> defaults applied, --no-color defaults color=true
        const emptyDummy: [1]string = ["unused"];
        const rDefault: CommandParseResult = cmd.parse(emptyDummy[0..0]);
        let buf: [64]u8 = undefined;
        expect_eq(rDefault.ok, true);
        expect_eq(rDefault.getInt("port", 0), 3000 as i64);
        const hLen = rDefault.getString("host", buf[0..64]);
        expect_eq(buf[0..(hLen as usize)], "127.0.0.1");
        expect_eq(rDefault.getBool("debug", false), false);
        expect_eq(rDefault.getBool("color", false), true);

        // Override via short flag -p, long flag --host, boolean -d, and --no-color
        const argv: [6]string = ["-p", "8080", "--host", "0.0.0.0", "-d", "--no-color"];
        const rOverride = cmd.parse(argv[0..6]);
        expect_eq(rOverride.ok, true);
        expect_eq(rOverride.getInt("port", 0), 8080 as i64);
        const hLen2 = rOverride.getString("host", buf[0..64]);
        expect_eq(buf[0..(hLen2 as usize)], "0.0.0.0");
        expect_eq(rOverride.getBool("debug", false), true);
        expect_eq(rOverride.getBool("color", true), false);
    });

    test("3. @cap/commander — requiredOption and <required> positional argument validation", () => {
        let cmd = Command.new("deploy");
        expect_eq(cmd.requiredOption("-t, --token <key>", "API token"), true);
        expect_eq(cmd.argument("<env>", "Target environment"), true);
        expect_eq(cmd.argument("[region]", "Optional region"), true);

        let errBuf: [192]u8 = undefined;

        // Missing required option -> error
        const argv1: [1]string = ["prod"];
        const r1 = cmd.parse(argv1[0..1]);
        expect_eq(r1.ok, false);
        const e1 = r1.errorMessage(errBuf[0..192]);
        expect_eq(containsSubstr(errBuf[0..e1], "required option"), true);
        expect_eq(containsSubstr(errBuf[0..e1], "-t, --token <key>"), true);

        // Required option provided, but missing <env> -> error
        const argv2: [2]string = ["-t", "sec-123"];
        const r2 = cmd.parse(argv2[0..2]);
        expect_eq(r2.ok, false);
        const e2 = r2.errorMessage(errBuf[0..192]);
        expect_eq(containsSubstr(errBuf[0..e2], "missing required argument"), true);
        expect_eq(containsSubstr(errBuf[0..e2], "env"), true);

        // Both provided (optional [region] omitted) -> ok
        const argv3: [3]string = ["--token", "sec-123", "staging"];
        const r3 = cmd.parse(argv3[0..3]);
        expect_eq(r3.ok, true);
        let valBuf: [64]u8 = undefined;
        const tLen = r3.getString("token", valBuf[0..64]);
        expect_eq(valBuf[0..(tLen as usize)], "sec-123");
        expect_eq(r3.argCount(), 1 as usize);
        const aLen = r3.argAt(0, valBuf[0..64]);
        expect_eq(valBuf[0..(aLen as usize)], "staging");
    });

    test("4. @cap/commander — subcommand routing and positional argument extraction", () => {
        let cmd = Command.new("pkg");
        expect_eq(cmd.option("-d, --debug", "Enable debug", ""), true);
        expect_eq(cmd.subcommand("install", "Install dependencies"), true);
        expect_eq(cmd.subcommand("build", "Build project"), true);

        const argv: [4]string = ["-d", "install", "lodash", "chalk"];
        const res = cmd.parse(argv[0..4]);
        let buf: [64]u8 = undefined;

        expect_eq(res.ok, true);
        expect_eq(res.getBool("debug", false), true);
        expect_eq(res.hasSubcommand(), true);
        const sLen = res.subcommandName(buf[0..64]);
        expect_eq(buf[0..sLen], "install");

        expect_eq(res.argCount(), 2 as usize);
        const p0 = res.argAt(0, buf[0..64]);
        expect_eq(buf[0..p0], "lodash");
        const p1 = res.argAt(1, buf[0..64]);
        expect_eq(buf[0..p1], "chalk");
    });

    test("5. @cap/commander — -V/--version and -h/--help flags bypass required checks", () => {
        let cmd = Command.new("mycli");
        cmd.version("2.5.0");
        cmd.requiredOption("-k, --key <secret>", "Required secret");
        cmd.argument("<input>", "Required input file");

        const v1: [1]string = ["-V"];
        const rv1 = cmd.parse(v1[0..1]);
        expect_eq(rv1.ok, true);
        expect_eq(rv1.show_version, true);
        expect_eq(rv1.show_help, false);

        const v2: [1]string = ["--version"];
        const rv2 = cmd.parse(v2[0..1]);
        expect_eq(rv2.ok, true);
        expect_eq(rv2.show_version, true);

        const h1: [1]string = ["-h"];
        const rh1 = cmd.parse(h1[0..1]);
        expect_eq(rh1.ok, true);
        expect_eq(rh1.show_help, true);
        expect_eq(rh1.show_version, false);

        const h2: [1]string = ["--help"];
        const rh2 = cmd.parse(h2[0..1]);
        expect_eq(rh2.ok, true);
        expect_eq(rh2.show_help, true);
    });

    test("6. @cap/commander — helpInformation renders Usage/Arguments/Options/Commands via @cap/cliui", () => {
        let cmd = Command.new("cap-cli");
        cmd.version("1.4.0");
        cmd.description("High-performance CP build & package tool");
        cmd.argument("<entry>", "Entry source file");
        cmd.argument("[out]", "Optional output binary");
        cmd.option("-p, --port <number>", "Dev server port", "3000");
        cmd.option("-d, --debug", "Enable verbose logs", "");
        cmd.subcommand("build", "Compile workspace");
        cmd.subcommand("test", "Run unit tests");

        let helpBuf: [4096]u8 = undefined;
        const n = cmd.helpInformation(80, helpBuf[0..4096]);
        const helpStr = helpBuf[0..n];

        expect_eq(n > 120, true);
        expect_eq(containsSubstr(helpStr, "Usage: cap-cli [options] <entry> [out] [command]"), true);
        expect_eq(containsSubstr(helpStr, "High-performance CP build & package tool"), true);
        expect_eq(containsSubstr(helpStr, "Arguments:"), true);
        expect_eq(containsSubstr(helpStr, "<entry>"), true);
        expect_eq(containsSubstr(helpStr, "Entry source file"), true);
        expect_eq(containsSubstr(helpStr, "Options:"), true);
        expect_eq(containsSubstr(helpStr, "-V, --version"), true);
        expect_eq(containsSubstr(helpStr, "-p, --port <number>"), true);
        expect_eq(containsSubstr(helpStr, "(default: \"3000\")"), true);
        expect_eq(containsSubstr(helpStr, "-h, --help"), true);
        expect_eq(containsSubstr(helpStr, "Commands:"), true);
        expect_eq(containsSubstr(helpStr, "build"), true);
        expect_eq(containsSubstr(helpStr, "Compile workspace"), true);
        expect_eq(containsSubstr(helpStr, "test"), true);
    });
}
